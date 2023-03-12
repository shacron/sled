// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2023 Shac Ron and The Sled Project

#include <ctype.h>
#include <fcntl.h>
#include <getopt.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
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

#include "cons.h"

#define ISSUE_INTERRUPT 1
#define DEFAULT_STEP_COUNT (1 * 1000 * 1000)
#define DEFAULT_CONSOLE    0

#define BIN_FLAG_ELF        (1u << 0)
#define BIN_FLAG_INIT       (1u << 1)
#define BIN_FLAG_FREE_NAME  (1u << 2)

typedef struct bin_file {
    struct bin_file* next;
    uint32_t flags;
    char *file;
    uint64_t addr;
} bin_file_t;

typedef struct {
    sl_machine_t *m;
    pthread_t core0;
    int core_id;

    uint64_t steps;
    uint64_t entry;
    bin_file_t *bin_list;
    bool cons_on_start;
    bool cons_on_err;

    int uart_fd_in;
    int uart_fd_out;
    int uart_io;
    const char *uart_path;
} sm_t;

static const struct option longopts[] = {
    { "console",   no_argument,        NULL,   'c' },
    { "entry",     required_argument,  NULL,   'e' },
    { "help",      no_argument,        NULL,   'h' },
    { "kernel",    required_argument,  NULL,   'k' },
    { "monitor",   required_argument,  NULL,   'm' },
    { "raw",       required_argument,  NULL,   'r' },
    { "serial",    required_argument,  NULL,   1   },
    { "step",      required_argument,  NULL,   's' },
    { "verbose",   no_argument,        NULL,   'v' },
    { NULL,        0,                  NULL,   0 }
};

static const char *shortopts = "ce:hk:m:r:s:v";

static void usage(void) {
    puts(
    "usage: sled [options] <executable>\n"
    "\n"
    "options:\n"
    "  <executable>\n"
    "       An ELF binary to be loaded and run in the default core.\n"
    "\n"
    "  -c, --console\n"
    "       Enter console before execution starts\n"
    "\n"
    "  -e, --entry=<addr>\n"
    "       Set execution entry point to <addr>. Overrides any entry point in loaded binaries\n"
    "\n"
    "  -m, --monitor=<binary>\n"
    "       An ELF binary to be loaded into the default core, setting the entry point\n"
    "       to run when execution begins. This option replaces the <executable> option.\n"
    "\n"
    "  -k, --kernel=<binary>\n"
    "       An ELF binary to be loaded into the default core. The code is not executed.\n"
    "\n"
    "  -r, --raw=<binary>:<addr>\n"
    "       A raw binary to be loaded into memory at a given address\n"
    "\n"
    "  -s, --step=<num>\n"
    "       Number of instructions to execute before exiting. 0 for infinite.\n"
    "\n"
    "  --serial=<output>\n"
    "       Set serial input and output. Possible 'output' values are:\n"
    "         '-' direct io to stdio (default)\n"
    "         'null' discard serial output\n"
    "         'file' direct output to file 'serial.txt'\n"
    "         'port:num' direct io to TCP network port. Execution will wait until a client\n"
    "            connects to this port.\n"
    "\n"
    "  -h, --help\n"
    "       Print this help text and exit.\n"
    );
}

static int add_binary(sm_t *sm, uint32_t flags, char *file, uint64_t addr) {
    bin_file_t *b = malloc(sizeof(*b));
    if (b == NULL) return -1;
    b->flags = flags;
    b->file = file;
    b->addr = addr;
    b->next = sm->bin_list;
    sm->bin_list = b;
    return 0;
}

static int parse_opts(int argc, char *argv[], sm_t *sm) {
    int ch;

    while ((ch = getopt_long(argc, argv, shortopts, longopts, NULL)) != -1) {
        switch (ch) {
        case 'c': sm->cons_on_start = true; break;
        case 'e': sm->entry = strtoull(optarg, NULL, 0); break;
        case 'h': return -2;
        case 'k': add_binary(sm, BIN_FLAG_ELF, optarg, 0);   break;
        case 'm': add_binary(sm, BIN_FLAG_ELF | BIN_FLAG_INIT, optarg, 0);   break;
        case 's': sm->steps = strtoull(optarg, NULL, 0); break;
        case 'v': break;                // todo

        case 'r':
        {
            char *s = strdup(optarg);
            char *a = strrchr(s, ':');
            if (a == NULL) {
                fprintf(stderr, "binary address required for raw entry\n");
                free(s);
                return -1;
            }
            *a = '\0';
            a++;
            uint64_t addr = strtoull(a, NULL, 0);
            if (addr == 0) {
                fprintf(stderr, "invalid binary address '%s'\n", a);
                free(s);
                return -1;
            }
            add_binary(sm, BIN_FLAG_FREE_NAME, s, addr);
            break;
        }

        case 1:
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
    int err = 0;
    sm_t *sm = arg;
    core_t *c = sl_machine_get_core(sm->m, sm->core_id);

    if (sm->cons_on_start) {
        err = console_enter(sm->m);
        goto out_err;
    }

    if (sm->steps == 0) {
        err = sl_core_dispatch_loop(c, true);
    } else {
        for (uint64_t s = sm->steps; s > 0; ) {
            uint64_t step;
            if (s > 0xffffffff) step = 0xffffffff;
            else step = s;
            if ((err = sl_core_step(c, step))) break;
            s -= step;
        }
    }
    if (err && sm->cons_on_err)
        console_enter(sm->m);

out_err:
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

static int load_binary(sl_machine_t *m, uint32_t core_id, bin_file_t *b) {
    int err = -1;
    void *buf = NULL;
    int fd = open(b->file, O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "failed to open %s\n", b->file);
        return -1;
    }
    struct stat stat;
    if ((err = fstat(fd, &stat))) {
        fprintf(stderr, "failed to open %s\n", b->file);
        goto out_err;
    }
    size_t size = stat.st_size;
    if (size == 0) {
        err = 0;
        goto out_err;
    }
    if ((buf = malloc(size)) == NULL) {
        perror("malloc");
        goto out_err;
    }
    ssize_t num = read(fd, buf, size);
    if (num < 0) {
        perror(b->file);
        goto out_err;
    }
    err = sl_machine_load_core_raw(m, core_id, b->addr, buf, size);

out_err:
    free(buf);
    if (fd >= 0) close(fd);
    return err;
};


int simple_machine(sm_t *sm) {
    sl_machine_t *m;
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

    // create machine

    if ((err = sl_machine_create(&m))) {
        fprintf(stderr, "sl_machine_init failed: %s\n", st_err(err));
        goto out_err;
    }
    sm->m = m;

    if ((err = sl_machine_add_mem(m, PLAT_MEM_BASE, PLAT_MEM_SIZE))) {
        fprintf(stderr, "sl_machine_add_mem failed: %s\n", st_err(err));
        goto out_err_machine;
    }

    if ((err = sl_machine_add_device(m, SL_DEV_INTC, PLAT_INTC_BASE, "intc0"))) {
        fprintf(stderr, "add interrupt controller failed: %s\n", st_err(err));
        goto out_err_machine;
    }

    if ((err = sl_machine_add_device(m, SL_DEV_RTC, PLAT_RTC_BASE, "rtc"))) {
        fprintf(stderr, "add real time clock failed: %s\n", st_err(err));
        goto out_err_machine;
    }

    if ((err = sl_machine_add_device(m, SL_DEV_UART, PLAT_UART_BASE, "uart0"))) {
        fprintf(stderr, "add uart failed: %s\n", st_err(err));
        goto out_err_machine;
    }

    sl_dev_t *d = sl_machine_get_device_for_name(m, "uart0");
    sled_uart_set_channel(d, sm->uart_io, sm->uart_fd_in, sm->uart_fd_out);

    // create core

    sl_core_params_t params = {};
    params.arch = PLAT_CORE_ARCH;
    params.subarch = PLAT_CORE_SUBARCH;
    params.id = 0;
    params.options = SL_CORE_OPT_TRAP_SYSCALL;
    params.arch_options = PLAT_ARCH_OPTIONS;

    if ((err = sl_machine_add_core(m, &params))) {
        printf("sl_machine_add_core failed: %s\n", st_err(err));
        goto out_err_machine;
    }

    bool configured = false;
    for (bin_file_t *b = sm->bin_list; b != NULL; b = b->next) {
        if (b->flags & BIN_FLAG_ELF) {
            sl_elf_obj_t *eo = NULL;
            if ((err = sl_elf_open(b->file, &eo))) {
                printf("failed to open %s\n", b->file);
                goto out_err_machine;
            }
            const bool config = (b->flags & BIN_FLAG_INIT) ? true : false;
            if (config && configured) printf("warning: cpu already configured\n");
            if ((err = sl_machine_load_core(m, params.id, eo, config))) {
                fprintf(stderr, "sl_machine_load_core failed: %s\n", st_err(err));
                goto out_err_machine;
            }
            configured = true;
            sl_elf_close(eo);
            eo = NULL;
        } else {
            if ((err = load_binary(m, params.id, b))) goto out_err_machine;
        }
    }

    core_t *c = sl_machine_get_core(m, sm->core_id);

    if (sm->entry != 0) sl_core_set_reg(c, SL_CORE_REG_PC, sm->entry);

    // run
    sm->core_id = params.id;
    if ((err = start_thread_for_core(sm))) {
        fprintf(stderr, "start_thread_for_core failed\n");
        goto out_err_machine;
    }

#if ISSUE_INTERRUPT
    sleep(1);
    printf("sending interrupt\n");
    // pulse interrupt since there's no way for software to clear this
    sl_machine_set_interrupt(m, 0, true);
    sl_machine_set_interrupt(m, 0, false);
#endif

    void *retval;
    pthread_join(sm->core0, &retval);
    err = (int)(uintptr_t)retval;

    if (err == SL_OK) goto out;
    if (err != SL_ERR_SYSCALL) {
        printf("unexpected run status: %s\n", st_err(err));
        goto out_err_machine;
    }

    a0 = sl_core_get_reg(c, SL_CORE_REG_ARG0);
    if (a0 != 0x666) {
        printf("unexpected exit syscall %#" PRIx64 "\n", a0);
        err = SL_ERR;
        goto out_err_machine;
    }

    a1 = sl_core_get_reg(c, SL_CORE_REG_ARG1);
    if (a1 != 0) {
        printf("executable exit status: %" PRId64 "\n", a1);
        err = SL_ERR;
        goto out_err_machine;
    }

    // printf("success\n");
out:
    printf("%" PRIu64 " instructions dispatched\n", sl_core_get_cycles(c));
    err = 0;

out_err_machine:
    sl_machine_destroy(m);
out_err:
    if (sm->uart_io != UART_IO_CONS) {
        if (sm->uart_fd_in >= 0) close(sm->uart_fd_in);
        if (sm->uart_fd_out >= 0) close(sm->uart_fd_out);
    }
    return err;
}

int main(int argc, char *argv[]) {
    bin_file_t *next = NULL;
    sm_t sm = {
        .uart_io = UART_IO_CONS,
        .steps = DEFAULT_STEP_COUNT,
        .cons_on_err = DEFAULT_CONSOLE,
    };

    int ret = parse_opts(argc, argv, &sm);
    if (ret < 0) {
        if (ret == -2) {
            usage();
            ret = 0;
        } else {
            ret = 1;
        }
        goto out_err;
    }

    argc -= ret;
    argv += ret;

    if (argc > 0) add_binary(&sm, BIN_FLAG_ELF | BIN_FLAG_INIT, argv[0], 0);

    ret = simple_machine(&sm);

out_err:
    for (bin_file_t *b = sm.bin_list; b != NULL; b = next) {
        next = b->next;
        if (b->flags & BIN_FLAG_FREE_NAME) free(b->file);
        free(b);
    }
    if (ret) return 1;
    return 0;
}
