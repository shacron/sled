#include <ctype.h>
#include <fcntl.h>
#include <getopt.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <sled/arch.h>
#include <sled/device.h>
#include <sled/elf.h>
#include <sled/error.h>
#include <sled/machine.h>

#include "platform.h"

// #define ISSUE_INTERRUPT 1

typedef struct {
    int uart_io;
    const char *uart_path;

} opts_t;

static const struct option longopts[] = {
    // { "arch",      required_argument,  NULL,   'a' },
    { "help",      no_argument,        NULL,   'h' },
    { "verbose",   no_argument,        NULL,   'v' },
    { "serial",    required_argument,  NULL,   's' },
    { NULL,        0,                  NULL,   0 }
};

static const char *shortopts = "hs:v";

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

static int parse_opts(int argc, char *argv[], opts_t *o) {
    int ch;

    while ((ch = getopt_long(argc, argv, shortopts, longopts, NULL)) != -1) {
        switch (ch) {
        case 'h':
            return -2;

        case 's':
            if (!strcmp(optarg, "-")) {
                o->uart_io = UART_IO_CONS;
                break;
            }
            if (!strcmp(optarg, "null")) {
                o->uart_io = UART_IO_NULL;
                break;
            }
            if (!strcmp(optarg, "file")) {
                o->uart_io = UART_IO_FILE;
                o->uart_path = "serial.txt";
                break;
            }
            if (!strncmp(optarg, "port:", 5)) {
                fprintf(stderr, "port option not implemented\n");
                return -1;
            }
            fprintf(stderr, "unrecognized serial option: %s\n", optarg);
            return -1;

        case 'v':
            // todo
            break;

        default:
            fprintf(stderr, "invalid argument\n"); // which argument?
            return -1;
        }
    }
    return optind;
}

int simple_machine(const char *file, opts_t *o) {
    machine_t *m;
    elf_object_t *eo = NULL;
    int err;
    uint64_t a0, a1;
    uint32_t step_count;
    int uart_fd_in = -1;
    int uart_fd_out = -1;

    if (o->uart_io == UART_IO_CONS) {
        uart_fd_in = STDIN_FILENO;
        uart_fd_out = STDOUT_FILENO;
    } else if (o->uart_io == UART_IO_FILE) {
        uart_fd_out = open(o->uart_path, (O_WRONLY | O_APPEND | O_CREAT), (S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH));
        if (uart_fd_out < 0) {
            perror(o->uart_path);
            return uart_fd_out;
        }
    } else if (o->uart_io == UART_IO_PORT) {
        // todo:
        fprintf(stderr, "not yet implemented\n");
        return -1;
    }

    // open elf

    if ((err = elf_open(file, &eo))) {
        printf("failed to open %s\n", file);
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
    sled_uart_set_channel(d, o->uart_io, uart_fd_in, uart_fd_out);

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

    // run

    core_t *c = machine_get_core(m, p.id);

#if ISSUE_INTERRUPT
    // todo: set trap handler?
    step_count = 20 * 1000; // should be enough to set up interrupts...
    err = core_step(c, step_count);
    if (err == SL_ERR_SYSCALL) goto handle_syscall;
    if (err != SL_OK) {
        printf("unexpected run status: %s\n", st_err(err));
        goto out_err_machine;
    }

    printf("sending interrupt\n");
    machine_set_interrupt(m, 0, true);
#endif

    step_count = 1000 * 1000;
    err = core_step(c, step_count);
    if (err == SL_OK) {
        printf("stopped after %u steps\n", step_count);
        goto out;
    }
    if (err != SL_ERR_SYSCALL) {
        printf("unexpected run status: %s\n", st_err(err));
        goto out_err_machine;
    }

#if ISSUE_INTERRUPT
handle_syscall:
#endif
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
    if (o->uart_io != UART_IO_CONS) {
        if (uart_fd_in >= 0) close(uart_fd_in);
        if (uart_fd_out >= 0) close(uart_fd_out);
    }
    return err;
}

int main(int argc, char *argv[]) {
    opts_t o = {
        .uart_io = UART_IO_CONS,
    };

    int ret = parse_opts(argc, argv, &o);
    if (ret < 0) {
        if (ret == -2) {
            usage();
            return 0;
        }
        return 1;
    }

    argc -= ret;
    argv += ret;

    if (argc < 1) {
        fprintf(stderr, "executable name required\n");
        usage();
        return 1;
    }

    if (simple_machine(argv[0], &o)) return 1;
    return 0;
}
