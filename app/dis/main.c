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

typedef struct {
    const char *name;
    size_t size;
    void *buf;
} bin_file_t;

static int dis_region(sl_core_t *c, u8 base, void *buf, usize length) {
    for (usize i = 0; i < length; ) {
        char s[128];

        u4 inst;
        u4 step;
        memcpy(&inst, buf + i, 4);
        int err = sl_core_disassemble(c, inst, s, 128, &step);
        if (err)
            return err;
        printf("%#16" PRIx64 ": %s\n", base + i, s);
        i += step;
    }
    return 0;
}

static int dis(char *name) {
    sl_elf_obj_t *elf = NULL;
    sl_core_t *c = NULL;
    // sl_sym_list_t *sl = NULL;
    int err;

    if ((err = sl_elf_open(name, &elf))) {
        fprintf(stderr, "failed to open ELF file %s: %s\n", name, st_err(err));
        return err;
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

        if ((err = dis_region(c, vaddr, p, memsz))) {
            fprintf(stderr, "diassembly failed for %s\n", name);
            goto out_err;
        }
    }

    // sl = calloc(1, sizeof(*sl));
    // if (sl == NULL) {
    //     printf("failed to allocate symbol list\n");
    //     goto out_err;
    // }
    // if ((err = elf_read_symbols(elf, sl))) {
    //     printf("failed to read symbols\n");
    //     goto out_err;
    // }

    // sl_core_add_symbols(c, sl);

    err = 0;

out_err:
    sl_core_destroy(c);
    // sym_free(sl);
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
