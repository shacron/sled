// MIT License

// Copyright (c) 2022 Shac Ron

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// SPDX-License-Identifier: MIT License

#include <ctype.h>
#include <fcntl.h>
#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#include <sled/arch.h>
#include <sled/elf.h>

/*
% ./tools/Darwin/bin/llvm-objdump --all-headers test/elf.o

test/elf.o:	file format elf32-littleriscv
architecture: riscv32
start address: 0x00000000

Program Header:

Dynamic Section:

Sections:
Idx Name              Size     VMA      Type
  0                   00000000 00000000 
  1 .strtab           0000005b 00000000 
  2 .text             00000008 00000000 TEXT
  3 .comment          00000016 00000000 
  4 .note.GNU-stack   00000000 00000000 
  5 .riscv.attributes 0000001c 00000000 
  6 .llvm_addrsig     00000000 00000000 
  7 .symtab           00000030 00000000 

SYMBOL TABLE:
00000000 l    df *ABS*	00000000 test.c
00000000 g     F .text	00000008 add
*/

#define GET_HEADER_FIELD(o, f) (o->is64 ? ((Elf64_Ehdr *)(o->image))->f : ((Elf32_Ehdr *)(o->image))->f)
#define GET_STRUCT_FIELD(o, p, t, f) (o->is64 ? ((Elf64_ ## t *)p)->f : ((Elf32_ ## t *)p)->f)

struct elf_object {
    int fd;
    void *image;
    size_t size;
    bool is64;

    uint32_t arch;
    uint32_t subarch;
    uint32_t arch_options;

    char *str;      // strings
    char *shstr;    // section header strings
    void *sym_sh;   // symbol section header
    void *text_sh;  // text section header
};


void *elf_pointer_for_offset(elf_object_t *obj, uint64_t offset) {
    return obj->image + offset;
}

static char * get_string(elf_object_t *obj, Elf64_Word offset) {
    return obj->str + offset;
}

static char * get_sh_string(elf_object_t *obj, Elf64_Word offset) {
    return obj->shstr + offset;
}

static void * get_section_header_base(elf_object_t *obj) {
    return obj->image + GET_HEADER_FIELD(obj, e_shoff);
}

bool elf_is_64bit(elf_object_t *obj) {
    return obj->is64;
}

uint64_t elf_get_entry(elf_object_t *obj) {
    return GET_HEADER_FIELD(obj, e_entry);
}

static int elf_riscv_attributes(elf_object_t *o, void *vsh) {
    int err;
    Elf64_Off offset;
    // Elf64_Xword size;
    if (o->is64) {
        Elf64_Shdr *sh = vsh;
        offset = sh->sh_offset;
        // size = sh->sh_size;
    } else {
        Elf32_Shdr *sh = vsh;
        offset = sh->sh_offset;
        // size = sh->sh_size;
    }

    char *at = strdup(o->image + offset + 19);

    printf("RISC string: %s\n", at);

    const char *sep = "_";
    char *lasts;
    char *s = strtok_r(at, sep, &lasts);
    if (s == NULL) {
        printf("invalid format string\n");
        err = -1;
        goto out;
    }
    if (o->is64) err = strncmp(s, "rv64i", 5);
    else err = strncmp(s, "rv32i", 5);
    if (err) {
        printf("unexpected attribute: %s\n", s);
        goto out;
    }

    err = -1;
    while ((s = strtok_r(NULL, sep, &lasts))) {
        const char c0 = s[0];
        if (c0 == '\0') continue;
        const char c1 = s[1];
        if (isdigit(c1)) {
            switch (c0) {
            case 'm':
                printf("M attribute version %c\n", c1);
                o->arch_options |= RISCV_EXT_M;
                break;

            case 'a':
                printf("A attribute version %c\n", c1);
                o->arch_options |= RISCV_EXT_A;
                break;

            case 'c':
                printf("C attribute version %c\n", c1);
                o->arch_options |= RISCV_EXT_C;
                break;

            // case 'f':
            // case 'd':
            //     printf("Faking support for attribute F/D, not actually supported\n");
            //     break;

            default:
                printf("unhandled attribute %c\n", c0);
                goto out;
            }
        } else {
            printf("unexpected attribute: %s\n", s);
            goto out;
        }
    }
    err = 0;

out:
    free(at);
    return err;
}

int elf_open(const char *filename, elf_object_t **obj_out) {
    elf_object_t *obj = calloc(1, sizeof(elf_object_t));

    obj->fd = open(filename, O_RDONLY);
    if (obj->fd == -1) {
        perror("open");
        goto out_err;
    }

    struct stat stat;
    if (fstat(obj->fd, &stat)) {
        perror("fstat");
        goto out_err;
    }

    if (stat.st_size == 0) {
        printf("zero sized file\n");
        goto out_err;
    }
    obj->size = stat.st_size;
    obj->image = mmap(NULL, obj->size, PROT_READ, MAP_FILE | MAP_SHARED, obj->fd, 0);
    if (obj->image == MAP_FAILED) {
        perror("mmap");
        obj->image = NULL;
        goto out_err;
    }

    Elf32_Ehdr *h = obj->image;
    if ((h->e_ident[EI_MAG0] != ELFMAG0) || (h->e_ident[EI_MAG1] != ELFMAG1) ||
        (h->e_ident[EI_MAG2] != ELFMAG2) || (h->e_ident[EI_MAG3] != ELFMAG3)) {
        printf("invalid magic value\n");
        goto out_err;
    }

    switch (h->e_ident[EI_CLASS]) {
    case ELFCLASS32: obj->is64 = false;  break;
    case ELFCLASS64: obj->is64 = true;   break;
    default:
        printf("unknown elf class\n");
        goto out_err;
    }

    h = NULL;

    switch (GET_HEADER_FIELD(obj, e_machine)) {
    case EM_ARM:
        obj->arch = ARCH_ARM;
        obj->subarch = SUBARCH_ARM;
        break;

    case EM_AARCH64:
        obj->arch = ARCH_ARM;
        obj->subarch = SUBARCH_ARM64;
        break;

    case EM_RISCV:
        obj->arch = ARCH_RISCV;
        if (obj->is64) obj->subarch = SUBARCH_RV64;
        else           obj->subarch = SUBARCH_RV32;
        break;

    default:
        printf("unknown object architecture\n");
        goto out_err;
    }

    const Elf64_Half shnum = GET_HEADER_FIELD(obj, e_shnum);
    if (shnum == 0) {
        printf("no sections in elf\n");
        goto out_err;
    }

    void *sh_base = get_section_header_base(obj);

    // section header string table:
    const Elf64_Half shstrndx = GET_HEADER_FIELD(obj, e_shstrndx);
    const Elf64_Half shentsize = GET_HEADER_FIELD(obj, e_shentsize);

    void *sh = sh_base + (shstrndx * shentsize);
    obj->shstr = obj->image + GET_STRUCT_FIELD(obj, sh, Shdr, sh_offset);

    for (Elf64_Half i = 0; i < shnum; i++) {
        sh = sh_base + (i * shentsize);
        const char *name = get_sh_string(obj, GET_STRUCT_FIELD(obj, sh, Shdr, sh_name));
        const Elf64_Word type = GET_STRUCT_FIELD(obj, sh, Shdr, sh_type);
        // printf("section %u: %s (%x)\n", i, name, type);
        switch (type) {
        case SHT_SYMTAB:
            if (!strcmp(name, ".symtab")) obj->sym_sh = sh;
            break;
        case SHT_STRTAB:
            if (!strcmp(name, ".strtab")) obj->str = obj->image + GET_STRUCT_FIELD(obj, sh, Shdr, sh_offset);
            break;
        case SHT_PROGBITS:
            if (!strcmp(name, ".text")) obj->text_sh = sh;
            break;
        case SHT_RISCV_ATTRIBUTES:
            if (!strcmp(name, ".riscv.attributes")) {
                if ((elf_riscv_attributes(obj, sh))) goto out_err;
            }
            break;

        default:
            break;
        }
    }

    if (obj->text_sh == NULL) {
        printf("no text section found\n");
        goto out_err;
    }
    *obj_out = obj;
    return 0;

out_err:
    elf_close(obj);
    return -1;
}

void elf_close(elf_object_t *obj) {
    if (obj == NULL) return;
    if (obj->image != NULL) munmap(obj->image, obj->size);
    if (obj->fd >= 0) close(obj->fd);
    free(obj);
}

int elf_arch(elf_object_t *obj) {
    return obj->arch;
}

int elf_subarch(elf_object_t *obj) {
    return obj->subarch;
}

uint32_t elf_arch_options(elf_object_t *obj) {
    return obj->arch_options;
}

static void * find_sym_for_name(elf_object_t *obj, const char *name) {
    Elf64_Off offset;
    Elf64_Xword size, entsize;

    if (obj->is64) {
        Elf64_Shdr *sh = obj->sym_sh;
        offset = sh->sh_offset;
        size = sh->sh_size;
        entsize = sh->sh_entsize;
    } else {
        Elf32_Shdr *sh = obj->sym_sh;
        offset = sh->sh_offset;
        size = sh->sh_size;
        entsize = sh->sh_entsize;
    }
    void *symtab = obj->image + offset;
    Elf64_Xword num_symbols = size / entsize;
    for (Elf64_Xword i = 0; i < num_symbols; i++) {
        void *s = symtab + (i * entsize);
        Elf64_Word sym_name = GET_STRUCT_FIELD(obj, s, Sym, st_name);
        if (!strcmp(name, get_string(obj, sym_name))) return s;
    }
    return NULL;
}

ssize_t elf_symbol_length(elf_object_t *obj, const char *name) {
    void *s = find_sym_for_name(obj, name);
    if (s == NULL) return -1;
    return GET_STRUCT_FIELD(obj, s, Sym, st_size);
}

ssize_t elf_read_symbol(elf_object_t *obj, const char *name, void *buf, size_t buflen) {
    void *s = find_sym_for_name(obj, name);
    if (s == NULL) return -1;

    Elf64_Xword size;
    Elf64_Off offset;
    Elf64_Addr value;
    if (obj->is64) {
        Elf64_Sym *st = s;
        Elf64_Shdr *sh = obj->text_sh;
        size = st->st_size;
        value = st->st_value;
        offset = sh->sh_offset;
    } else {
        Elf32_Sym *st = s;
        Elf32_Shdr *sh = obj->text_sh;
        size = st->st_size;
        value = st->st_value;
        offset = sh->sh_offset;
    }
    if (buflen < size) return -1;
    memcpy(buf, obj->image + offset + value, size);
    return size;
}

void *elf_get_program_header(elf_object_t *obj, uint32_t index) {
    Elf64_Half phnum, phentsize;
    Elf64_Off phoff;
    if (obj->is64) {
        Elf64_Ehdr *h = obj->image;
        phnum = h->e_phnum;
        phentsize = h->e_phentsize;
        phoff = h->e_phoff;
    } else {
        Elf32_Ehdr *h = obj->image;
        phnum = h->e_phnum;
        phentsize = h->e_phentsize;
        phoff = h->e_phoff;
    }
    if (index >= phnum) return NULL;
    return obj->image + phoff + (index * phentsize);
}

int elf_symbols(elf_object_t *obj) {
    Elf64_Off offset;
    Elf64_Xword size, entsize;

    if (obj->is64) {
        Elf64_Shdr *sh = obj->sym_sh;
        offset = sh->sh_offset;
        size = sh->sh_size;
        entsize = sh->sh_entsize;
    } else {
        Elf32_Shdr *sh = obj->sym_sh;
        offset = sh->sh_offset;
        size = sh->sh_size;
        entsize = sh->sh_entsize;
    }

    void *symtab = obj->image + offset;
    Elf64_Xword num_symbols = size / entsize;
    for (Elf64_Xword i = 0; i < num_symbols; i++) {
        if (obj->is64) {
            Elf64_Sym *s = symtab + (i * entsize);
            printf("%016" PRIx64 " %016" PRIx64 " %s\n", s->st_value, s->st_size, get_string(obj, s->st_name));
        } else {
            Elf32_Sym *s = symtab + (i * entsize);
            printf("%08x %8u %s\n", s->st_value, s->st_size, get_string(obj, s->st_name));
        }
    }
    return 0;
}
