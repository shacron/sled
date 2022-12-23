// SPDX-License-Identifier: MIT License
// Copyright (c) 2022 Shac Ron and The Sled Project

#include <ctype.h>
#include <fcntl.h>
#include <getopt.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <plat/platform.h>
#include <sled/arch.h>
#include <sled/device.h>
#include <sled/elf.h>
#include <sled/error.h>
#include <sled/machine.h>

#define ISSUE_INTERRUPT 1

typedef struct {
    machine_t *m;
    pthread_t core0;
    int core_id;

    const char *monitor_file;
    const char *kernel_file;

    int uart_fd_in;
    int uart_fd_out;
    int uart_io;
    const char *uart_path;
} sm_t;

static const struct option longopts[] = {
    // { "arch",      required_argument,  NULL,   'a' },
    { "help",      no_argument,        NULL,   'h' },
    { "kernel",    required_argument,  NULL,   'k' },
    { "monitor",   required_argument,  NULL,   'm' },
    { "serial",    required_argument,  NULL,   's' },
    { "verbose",   no_argument,        NULL,   'v' },
    { NULL,        0,                  NULL,   0 }
};

static const char *shortopts = "hm:s:v";

static void usage(void) {
    printf("sled: [options] <executable>\n");

    printf("options:\n");
    const struct option *opt;
    for (opt = &longopts[0]; opt->name != NULL; opt++) {
        if ((opt->flag == NULL) && isalnum(opt->val)) printf("  -%c, ", (char)opt->val);
        else printf("  ");
        printf("--%s\n", opt->name);
    }
    printf(
    "\n"
    "--serial=<output>\n"
    "    Set serial input and output. Possible values are:\n"
    "    '-' direct io to stdio (default)\n"
    "    'null' discard serial output\n"
    "    'file' direct output to file 'serial.txt'\n"
    "    'port:num' direct io to TCP network port. Execution will wait until a client\n"
    "       connects to this port\n"
    "\n"
    );

}

static int parse_opts(int argc, char *argv[], sm_t *sm) {
    int ch;

    while ((ch = getopt_long(argc, argv, shortopts, longopts, NULL)) != -1) {
        switch (ch) {
        case 'h':return -2;
        case 'k': sm->kernel_file = optarg;     break;
        case 'm': sm->monitor_file = optarg;    break;
        case 'v': break;                // todo
        case 's':
            if (!strcmp(optarg, "-")) {
                sm->uart_io = UART_IO_CONS;
                break;
            }
            if (!strcmp(optarg, "null")) {
                sm->uart_io = UART_IO_NULL;
                break;
            }
            if (!strcmp(optarg, "file")) {
                sm->uart_io = UART_IO_FILE;
                sm->uart_path = "serial.txt";
                break;
            }
            if (!strncmp(optarg, "port:", 5)) {
                fprintf(stderr, "port option not implemented\n");
                return -1;
            }
            fprintf(stderr, "unrecognized serial option: %s\n", optarg);
            return -1;

        default:
            fprintf(stderr, "invalid argument\n"); // which argument?
            return -1;
        }
    }
    return optind;
}

void *core_runner(void *arg) {
    sm_t *sm = arg;
    core_t *c = machine_get_core(sm->m, sm->core_id);
    const uint32_t step_count = 1000 * 1000;
    int err = core_step(c, step_count);
    return (void *)(uintptr_t)err;
}

int start_thread_for_core(sm_t *sm) {
    int err = pthread_create(&sm->core0, NULL, core_runner, sm);
    if (err != 0) {
        perror("pthread_create");
        return -1;
    }
    return 0;
}

int simple_machine(sm_t *sm) {
    machine_t *m;
    elf_object_t *eo = NULL;
    int err;
    uint64_t a0, a1;
    sm->uart_fd_in = -1;
    sm->uart_fd_out = -1;

    if (sm->uart_io == UART_IO_CONS) {
        sm->uart_fd_in = STDIN_FILENO;
        sm->uart_fd_out = STDOUT_FILENO;
    } else if (sm->uart_io == UART_IO_FILE) {
        sm->uart_fd_out = open(sm->uart_path, (O_WRONLY | O_APPEND | O_CREAT), (S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH));
        if (sm->uart_fd_out < 0) {
            perror(sm->uart_path);
            return sm->uart_fd_out;
        }
    } else if (sm->uart_io == UART_IO_PORT) {
        // todo:
        fprintf(stderr, "not yet implemented\n");
        return -1;
    }

    // open elf

    // todo: handle no-monitor kernel-only
    if ((err = elf_open(sm->monitor_file, &eo))) {
        printf("failed to open %s\n", sm->monitor_file);
        goto out_err;
    }

    // configure core

    core_params_t p = {};
    p.arch = elf_arch(eo);
    p.subarch = elf_subarch(eo);
    p.id = 0;
    p.options = CORE_OPT_TRAP_SYSCALL;
    p.arch_options = elf_arch_options(eo);

    // create machine

    if ((err = machine_create(&m))) {
        fprintf(stderr, "machine_init failed: %s\n", st_err(err));
        goto out_err;
    }
    sm->m = m;

    if ((err = machine_add_mem(m, PLAT_MEM_BASE, PLAT_MEM_SIZE))) {
        fprintf(stderr, "machine_add_mem failed: %s\n", st_err(err));
        goto out_err_machine;
    }

    if ((err = machine_add_device(m, DEVICE_INTC, PLAT_INTC_BASE, "intc0"))) {
        fprintf(stderr, "add interrupt controller failed: %s\n", st_err(err));
        goto out_err_machine;
    }

    if ((err = machine_add_device(m, DEVICE_RTC, PLAT_RTC_BASE, "rtc"))) {
        fprintf(stderr, "add real time clock failed: %s\n", st_err(err));
        goto out_err_machine;
    }

    if ((err = machine_add_device(m, DEVICE_UART, PLAT_UART_BASE, "uart0"))) {
        fprintf(stderr, "add uart failed: %s\n", st_err(err));
        goto out_err_machine;
    }

    device_t *d = machine_get_device_for_name(m, "uart0");
    sled_uart_set_channel(d, sm->uart_io, sm->uart_fd_in, sm->uart_fd_out);

    // create core

    if ((err = machine_add_core(m, &p))) {
        printf("machine_add_core failed: %s\n", st_err(err));
        goto out_err_machine;
    }

    // load elf

    if ((err = machine_load_core(m, p.id, eo))) {
        fprintf(stderr, "machine_load_core failed: %s\n", st_err(err));
        goto out_err_machine;
    }
    elf_close(eo);
    eo = NULL;

    if (sm->kernel_file != NULL) {
        if ((err = elf_open(sm->kernel_file, &eo))) {
            printf("failed to open %s\n", sm->kernel_file);
            goto out_err_machine;
        }
        if ((err = machine_load_core(m, p.id, eo))) {
            fprintf(stderr, "machine_load_core failed: %s\n", st_err(err));
            goto out_err_machine;
        }
        elf_close(eo);
        eo = NULL;
    }

    // run
    sm->core_id = p.id;
    if ((err = start_thread_for_core(sm))) {
        fprintf(stderr, "start_thread_for_core failed\n");
        goto out_err_machine;
    }

#if ISSUE_INTERRUPT
    sleep(1);
    printf("sending interrupt\n");
    // pulse interrupt since there's no way for software to clear this
    machine_set_interrupt(m, 0, true);
    machine_set_interrupt(m, 0, false);
#endif

    void *retval;
    pthread_join(sm->core0, &retval);
    err = (int)(uintptr_t)retval;


    core_t *c = machine_get_core(m, sm->core_id);

    if (err == SL_OK) goto out;
    if (err != SL_ERR_SYSCALL) {
        printf("unexpected run status: %s\n", st_err(err));
        goto out_err_machine;
    }

    a0 = core_get_reg(c, CORE_REG_ARG0);
    if (a0 != 0x666) {
        printf("unexpected exit syscall %#" PRIx64 "\n", a0);
        err = SL_ERR;
        goto out_err_machine;
    }

    a1 = core_get_reg(c, CORE_REG_ARG1);
    if (a1 != 0) {
        printf("executable exit status: %" PRId64 "\n", a1);
        err = SL_ERR;
        goto out_err_machine;
    }

    // printf("success\n");
out:
    printf("%" PRIu64 " instructions dispatched\n", core_get_cycles(c));
    err = 0;

out_err_machine:
    machine_destroy(m);
out_err:
    elf_close(eo);
    if (sm->uart_io != UART_IO_CONS) {
        if (sm->uart_fd_in >= 0) close(sm->uart_fd_in);
        if (sm->uart_fd_out >= 0) close(sm->uart_fd_out);
    }
    return err;
}

int main(int argc, char *argv[]) {
    sm_t sm = {
        .uart_io = UART_IO_CONS,
    };

    int ret = parse_opts(argc, argv, &sm);
    if (ret < 0) {
        if (ret == -2) {
            usage();
            return 0;
        }
        return 1;
    }

    argc -= ret;
    argv += ret;

    if (sm.monitor_file == NULL) {
        if (argc > 0) {
            sm.monitor_file = argv[0];
        } else {
            fprintf(stderr, "monitor executable name required\n");
            usage();
            return 1;
        }
    }

    if (simple_machine(&sm)) return 1;
    return 0;
}
