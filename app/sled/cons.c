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
    machine_t *machine;
    core_t *core;

    uint32_t len;
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
    if (argc == 0) {
        printf("usage:\n"
               "  reg <reg>\n"
               "    read a register\n"
               "  reg <reg> <value>\n"
               "    write value to register\n");
        return 0;
    }

    int arch = core_get_arch(c->core);
    char *rname = argv[0];

    uint32_t r = arch_reg_for_name(arch, rname);
    if (r == CORE_REG_INVALID) {
        printf("invalid register name %s for architecture %s\n", rname, arch_name(arch));
        return 0;
    }

    if (argc == 1) {
        // read reg
        uint64_t val = core_get_reg(c->core, r);
        printf("%0" PRIx64 "\n", val);
        return 0;
    }

    return 0;
}

static const cons_command_t command_list[] = {
    {
        .sname = 'r',
        .lname = "reg",
        .handler = reg_handler,
        .help = "reg stuff",
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

int console_enter(machine_t *m) {
    int err = 0;
    console_t c;
    c.machine = m;
    c.core = machine_get_core(m, 0);

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

