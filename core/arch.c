// SPDX-License-Identifier: MIT License
// Copyright (c) 2023-2024 Shac Ron and The Sled Project

#include <string.h>

#include <core/arch.h>
#include <core/common.h>
#include <core/riscv/rv.h>
#include <core/types.h>
#include <sled/arch.h>
#include <sled/core.h>

typedef struct {
    u2 value;
    const char *name;
} reg_name_t;

static const char * arch_name_map[] = {
    [SL_ARCH_MIPS] = "mips",
    [SL_ARCH_ARM] = "arm",
    [SL_ARCH_RISCV] = "riscv",
};

static const reg_name_t reg_common[] = {
    { SL_CORE_REG_PC, "pc" },
    { SL_CORE_REG_SP, "sp" },
    { SL_CORE_REG_LR, "lr" },
};

const arch_ops_t sl_arch_ops[] = {
    [SL_ARCH_MIPS] = { },
    [SL_ARCH_ARM] = { },
    [SL_ARCH_RISCV] = {
        .reg_index = rv_reg_index,
        .reg_for_name = rv_reg_for_name,
        .name_for_reg = rv_name_for_reg,
     },
};

typedef struct {
    u1 arch;
    u1 subarch;
    const char *arch_name;
    const char *subarch_name;
    u4 int_reg_count;
} arch_info_t;

static const arch_info_t arch_info[] = {
    {
        .arch = SL_ARCH_RISCV,
        .subarch = SL_SUBARCH_RV32,
        .subarch_name = "rv32",
        .int_reg_count = 32,
    },
    {
        .arch = SL_ARCH_RISCV,
        .subarch = SL_SUBARCH_RV64,
        .subarch_name = "rv64",
        .int_reg_count = 32,
    },
};

const char * sl_arch_name(u1 arch) {
    if (arch > SL_ARCH_NUM) return NULL;
    return arch_name_map[arch];
}

u1 sl_arch_reg_index(u1 arch, u4 reg) {
    return sl_arch_ops[arch].reg_index(reg);
}

u4 sl_arch_get_reg_count(u1 arch, u1 subarch, int type) {
    if (type != SL_CORE_REG_TYPE_INT) return 0;

    for (int i = 0; i < countof(arch_info); i++) {
        const arch_info_t *a = &arch_info[i];
        if ((a->arch == arch) && (a->subarch == subarch)) {
            return a->int_reg_count;
        }
    }
    return 0;
}

u4 sl_arch_reg_for_name(u1 arch, const char *name) {
    for (int i = 0; i < countof(reg_common); i++) {
        const reg_name_t *r = &reg_common[i];
        if (!strcmp(name, r->name)) return r->value;
    }

    const arch_ops_t *ops = &sl_arch_ops[arch];
    if (ops->reg_for_name != NULL)
        return ops->reg_for_name(name);

    return SL_CORE_REG_INVALID;
}
