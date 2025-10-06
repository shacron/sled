// SPDX-License-Identifier: MIT License
// Copyright (c) 2023 Shac Ron and The Sled Project

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sled/arch.h>
#include <sled/error.h>

#include "cons.h"

#define countof(p) (sizeof(p) / sizeof(p[0]))

#define MAX_LINE 4096
#define MAX_ARGS 20

typedef struct {
    sl_machine_t *machine;
    sl_core_t *core;

    u4 len;
    char *line;
    bool done;
} console_t;

typedef struct {
    char sname;
    const char *lname;
    int (*handler)(console_t *, char *, int, char **);
    const char *help;
} cons_command_t;

static int help_handler(console_t *c, char *cmd, int argc, char **argv);

static int quit_handler(console_t *c, char *cmd, int argc, char **argv) {
    c->done = true;
    return 0;
}

static int reg_handler(console_t *c, char *cmd, int argc, char **argv) {
    int arch = sl_core_get_arch(c->core);
    u8 val;

    if (argc == 0) {
        val = sl_core_get_reg(c->core, SL_CORE_REG_PC);
        printf("pc : %" PRIx64 "\n", val);
        u4 count = sl_core_get_reg_count(c->core, SL_CORE_REG_TYPE_INT);
        for (u4 a = 0; a < count; a += 4) {
            u8 r[4];
            for (int b = 0; b < 4; b++) {
                if ((a + b) > count) break;
                r[b] = sl_core_get_reg(c->core, a + b);
            }
            printf("r%2u: %16" PRIx64 " %16" PRIx64 " %16" PRIx64 " %16" PRIx64 "\n", a, r[0], r[1], r[2], r[3]);
        }
        return 0;
    }

    char *rname = argv[0];

    u4 r = sl_arch_reg_for_name(arch, rname);
    if (r == SL_CORE_REG_INVALID) {
        printf("invalid register name %s for architecture %s\n", rname, sl_arch_name(arch));
        return 0;
    }

    if (argc == 1) {
        // read reg
        val = sl_core_get_reg(c->core, r);
        printf("%#" PRIx64 "\n", val);
        return 0;
    }

    val = strtoull(argv[1], NULL, 0);
    sl_core_set_reg(c->core, r, val);
    printf("%s = %#" PRIx64 "\n", rname, val);
    return 0;
}

#define LINE_LEN    100

static int mem_handler(console_t *c, char *cmd, int argc, char **argv) {
    int err = 0;

    if (argc < 2) {
        printf("usage:\n"
               "  mem r<size> <addr> <num>\n"
               "    read memory at address\n"
               "  mem w<size> <addr> <value> <num>\n"
               "    write value of size wsize to memory\n");
        return 0;
    }

    u4 size = 0;
    u4 num = 20;
    // u8 value = 0;
    const u4 line_len = LINE_LEN;
    char line[LINE_LEN];

    const char *op = argv[0];
    if ((op[0] == 'r') || (op[0] == 'w')) {
        switch (op[1]) {
        case '1': size = 1; break;
        case '2': size = 2; break;
        case '4': size = 4; break;
        case '8':
        case '\0': size = 8; break;
        default:
            printf("invalid mem size. Should be 1, 2, 4, or 8.\n");
            return 0;
        }
    } else {
        printf("invalid mem op %s\n", op);
        return 0;
    }

    u8 addr = strtoull(argv[1], NULL, 0);

    if (argc == 2) goto do_op;

    // const char *sval = NULL;
    const char *snum = argv[2];
    // if (op[0] == 'w') {
    //     sval = argv[2];
    //     if (argc >= 4) snum = argv[3];
    //     else snum = NULL;
    // }
    // if (sval != NULL) value = strtoull(sval, NULL, 0);
    if (snum != NULL) num = strtoull(snum, NULL, 0);

do_op:
    if (op[0] == 'r') {
        for (u4 i = 0; i < num; ) {
            line[0] = '\0';
            int cur = snprintf(line, line_len, "%"PRIx64":", addr);
            u4 j;
            for (j = i; j < num; j++) {
                u8 d;
                if ((err = sl_core_mem_read(c->core, addr, size, 1, &d))) {
                    printf("failed to read memory at %#"PRIx64": %s\n", addr, st_err(err));
                    return 0;
                }
                addr += size;
                switch (size) {
                case 1:
                    cur += snprintf(line + cur, line_len - cur, " %02x", (u1)d);
                    break;

                case 2:
                    cur += snprintf(line + cur, line_len - cur, " %04x", (u2)d);
                    break;

                case 4:
                    cur += snprintf(line + cur, line_len - cur, " %08x", (u4)d);
                    break;

                default:
                    cur += snprintf(line + cur, line_len - cur, " %016"PRIx64, d);
                    break;
                }
                if ((line_len - cur) < ((2 * size) + 2)) break;
            }
            puts(line);
            i = j;
        }
    } else {
        printf("memory writing not yet implemented\n");
    }
    return 0;
}

static int step_handler(console_t *c, char *cmd, int argc, char **argv) {
    u4 step = 1;
    if (argc > 0) {
        step = strtoul(argv[0], NULL, 0);
    }
    int err = sl_core_step(c->core, step);
    u8 pc = sl_core_get_reg(c->core, SL_CORE_REG_PC);
    if (err) {
        printf("instruction failed at pc=%#" PRIx64 ": %s\n", pc, st_err(err));
    } else {
        // todo: pretty-print instruction disassembly when decoder is done
        printf("pc = %#" PRIx64 "\n", pc);
    }
    return 0;
}

static const cons_command_t command_list[] = {
    {
        .sname = 's',
        .lname = "step",
        .handler = step_handler,
        .help = "step one or more instructions",
    },
    {
        .sname = 'r',
        .lname = "reg",
        .handler = reg_handler,
        .help = "reg read/write",
    },
    {
        .sname = 'm',
        .lname = "mem",
        .handler = mem_handler,
        .help = "mem read/write",
    },

    {
        .sname = '?',
        .lname = "help",
        .handler = help_handler,
        .help = "print out the help screen",
    },
    {
        .sname = 'q',
        .lname = "quit",
        .handler = quit_handler,
        .help = "exit console",
    },
};

static int help_handler(console_t *c, char *cmd, int argc, char **argv) {
    for (size_t i = 0; i < countof(command_list); i ++) {
        const cons_command_t *cc = &command_list[i];
        printf("%c %s\n  %s\n\n", cc->sname, cc->lname, cc->help);
    }
    return 0;
}

static int parse_command(console_t *c) {
    const char *sep = " \t";
    char *word, *br;

    char *args[MAX_ARGS];
    int err = 0, argc = 0;

    char *cmd = strtok_r(c->line, sep, &br);
    if (cmd == NULL) return 0;

    for (word = strtok_r(NULL, sep, &br); word; word = strtok_r(NULL, sep, &br)) {
        args[argc] = word;
        argc++;
        if (argc == MAX_ARGS) break;
    }

    bool handled = false;
    for (size_t i = 0; i < countof(command_list); i ++) {
        const cons_command_t *cc = &command_list[i];
        if ((cmd[0] == cc->sname && cmd[1] == '\0') || !strcmp(cmd, cc->lname)) {
            err = cc->handler(c, cmd, argc, args);
            if (err) return err;
            handled = true;
            break;
        }
    }

    if (!handled) {
        printf("unknown command: %s\n", cmd);
    }
    return 0;
}

static int read_line(console_t *c) {
    int pos = 0;
    for ( ; pos < MAX_LINE - 1; pos++) {
        int x = getchar();
        if (x == EOF) {
            c->len = 0;
            c->done = true;
            printf("\n");
            return 0;
        }
        if (x < 0) {
            perror("getchar");
            return -1;
        }
        if (x == '\n') break;
        c->line[pos] = x;
    }
    c->line[pos] = '\0';
    c->len = pos;
    return 0;
}

int console_enter(sl_machine_t *m) {
    int err = 0;
    console_t c;
    c.machine = m;
    c.core = sl_machine_get_core(m, 0);

    c.line = malloc(MAX_LINE);
    if (c.line == NULL) {
        perror("malloc");
        return SL_ERR_MEM;
    }

    for (c.done = false; !c.done; ) {
        printf("sled> ");
        if ((err = read_line(&c))) break;
        if (c.len == 0) continue;
        if ((err = parse_command(&c))) break;
    }
    free(c.line);
    return err;
}

