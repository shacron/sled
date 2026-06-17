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

// hack
// #include "../core/inc/core/sym.h"

struct sl_sym_entry {
    u8 addr;
    u8 size;
    u4 flags;
    char *name;
};

struct sl_sym_list {
    sl_sym_list_t *next;
    char *name;
    u8 num;
    sl_sym_entry_t *ent;
};




typedef struct {
    const char *name;
    size_t size;
    void *buf;
} bin_file_t;

typedef struct {
    sl_sym_list_t *sl;
    const sl_sym_entry_t *cur;
    bool changed;
} sl_sym_iterator_t;

void sl_symbol_iterator_init(sl_sym_iterator_t *it, sl_sym_list_t *sl) {
    it->sl = sl;
    it->cur = NULL;
    it->changed = false;

    for (u4 i = 0; i < it->sl->num; i++) {
        const sl_sym_entry_t *ent = &it->sl->ent[i];
        printf("%8llx %8llx (%llu) %s\n", ent->addr, ent->addr + ent->size, ent->size, ent->name);
    }
}

int sl_symbol_iterator_find_for_addr(sl_sym_iterator_t *it, u8 addr) {
    const sl_sym_entry_t *ent = it->cur;
    if (ent != NULL) {
        if ((ent->addr <= addr) && ((ent->addr + ent->size) > addr)) {
            it->changed = false;
            return 0;
        }
        it->cur = NULL;
    }

    for (u4 i = 0; i < it->sl->num; i++) {
        ent = &it->sl->ent[i];
        if ((ent->addr <= addr) && ((ent->addr + ent->size) > addr)) {
            it->cur = ent;
            it->changed = true;
            return 0;
        }
    }
    return SL_ERR_NOT_FOUND;
}

const char *sl_symbol_iterator_name(sl_sym_iterator_t *it) {
    const sl_sym_entry_t *ent = it->cur;
    if (ent == NULL)
        return "<unknown>";
    return ent->name;
}

static int dis_region(sl_core_t *c, u8 base, void *buf, usize length, bool is_long, sl_sym_list_t *sl) {
    sl_sym_iterator_t iter;
    sl_symbol_iterator_init(&iter, sl);

    for (usize i = 0; i < length; ) {
        char s[128];

        const u8 addr = base + i;
        int err = sl_symbol_iterator_find_for_addr(&iter, addr);
        if (iter.changed) {
            printf("<%s>:\n", sl_symbol_iterator_name(&iter));
        }

        u4 inst;
        u4 step;
        memcpy(&inst, buf + i, 4);
        if ((err = sl_core_disassemble(c, inst, s, 128, &step)))
            return err;

        if (is_long)
            printf("%#16" PRIx64 ": %s\n", addr, s);
        else
            printf("%#8x: %s\n", (u4)(addr), s);
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
        Elf64_Xword memsz, filesz;
        Elf64_Addr vaddr;
        Elf64_Off offset;
        Elf64_Word flags;
        if (is64) {
            Elf64_Phdr *ph = vph;
            type = ph->p_type;
            memsz = ph->p_memsz;
            vaddr = ph->p_vaddr;
            offset = ph->p_offset;
            filesz = ph->p_filesz;
            flags = ph->p_flags;
        } else {
            Elf32_Phdr *ph = vph;
            type = ph->p_type;
            memsz = ph->p_memsz;
            vaddr = ph->p_vaddr;
            offset = ph->p_offset;
            filesz = ph->p_filesz;
            flags = ph->p_flags;
        }

        // load PT_LOAD with X flags
        if (type != PT_LOAD) continue;
        if ((flags & PF_X) == 0) continue;
        if (memsz == 0) continue;
        void *p = sl_elf_pointer_for_offset(elf, offset);

        printf("section %u: vaddr=%#" PRIx64 ", filesz=%#" PRIx64 "\n", i, vaddr, filesz);

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
