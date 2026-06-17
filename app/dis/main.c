// SPDX-License-Identifier: MIT License
// Copyright (c) 2026 Shac Ron and The Sled Project

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <sled/core.h>
#include <sled/elf.h>
#include <sled/error.h>
#include <sled/sym.h>

typedef struct {
    const char *name;
    size_t size;
    void *buf;
} bin_file_t;

static int dis_region(sl_core_t *c, u8 base, void *buf, usize length, bool is_long, sl_sym_list_t *sl) {
    sl_sym_entry_t *cur = NULL;
    for (usize i = 0; i < length; ) {
        char s[128];

        const u8 addr = base + i;
        sl_sym_entry_t *ent;
        int err = sl_sym_entry_for_addr(sl, addr, cur, &ent);
        if (err == 0) {
            if (cur != ent) {
                printf("\n%08x <%s>:\n", (u4)addr, ent->name);
                cur = ent;
            }
        } else {
            // not found
            if (cur != NULL) {
                printf("\n%08x <unknown>:\n", (u4)addr);
                cur = NULL;
            }
        }

        u4 inst;
        u4 step;
        memcpy(&inst, buf + i, 4);
        if ((err = sl_core_disassemble(c, inst, s, 128, &step)))
            return err;

        if (is_long)
            printf("%16" PRIx64 ": %s\n", addr, s);
        else
            printf("%8x: %08x      %s\n", (u4)(addr), inst, s);
        i += step;
    }
    return 0;
}

static int dis(char *name) {
    sl_elf_obj_t *elf = NULL;
    sl_core_t *c = NULL;
    sl_sym_list_t *sl = NULL;
    int err;

    if ((err = sl_elf_open(name, &elf))) {
        fprintf(stderr, "failed to open ELF file %s: %s\n", name, st_err(err));
        return err;
    }

    if ((err = sl_elf_symbol_list_create(elf, &sl))) {
        fprintf(stderr, "failed to load symbols %s: %s\n", name, st_err(err));
        // should this be fatal?
        goto out_err;
    }

    sl_core_params_t params = {};
    params.arch = sl_elf_arch(elf);
    params.subarch = sl_elf_subarch(elf);
    params.id = 0;
    params.name = "cpu0";

    if ((err = sl_core_create(&params, &c))) {
        fprintf(stderr, "core create failed: %s\n", st_err(err));
        goto out_err;
    }

    // iterate elf sections, disassemble in order
    const bool is64 = sl_elf_is_64bit(elf);

    for (u4 i = 0; ; i++) {
        void *vph = sl_elf_get_program_header(elf, i);
        if (vph == NULL) break;

        Elf64_Word type;
        Elf64_Xword memsz /*, filesz */;
        Elf64_Addr vaddr;
        Elf64_Off offset;
        Elf64_Word flags;
        if (is64) {
            Elf64_Phdr *ph = vph;
            type = ph->p_type;
            memsz = ph->p_memsz;
            vaddr = ph->p_vaddr;
            offset = ph->p_offset;
            // filesz = ph->p_filesz;
            flags = ph->p_flags;
        } else {
            Elf32_Phdr *ph = vph;
            type = ph->p_type;
            memsz = ph->p_memsz;
            vaddr = ph->p_vaddr;
            offset = ph->p_offset;
            // filesz = ph->p_filesz;
            flags = ph->p_flags;
        }

        // load PT_LOAD with X flags
        if (type != PT_LOAD) continue;
        if ((flags & PF_X) == 0) continue;
        if (memsz == 0) continue;
        void *p = sl_elf_pointer_for_offset(elf, offset);
        // printf("section %u: vaddr=%#" PRIx64 ", filesz=%#" PRIx64 "\n", i, vaddr, filesz);

        if ((err = dis_region(c, vaddr, p, memsz, is64, sl))) {
            fprintf(stderr, "diassembly failed for %s\n", name);
            goto out_err;
        }
    }

    err = 0;

out_err:
    sl_core_destroy(c);
    sl_elf_symbol_list_destroy(sl);
    sl_elf_close(elf);
    return err;
}

int main(int argc, char *argv[]) {
    // todo: parse args

    int err = 0;

    for (int i = 1; i < argc; i++) {
        err = dis(argv[i]);
        if (err) {
            fprintf(stderr, "%s: failed\n", argv[i]);
            break;
        }
    }
    if (err) return 1;
    return 0;
}
