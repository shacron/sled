// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2023 Shac Ron and The Sled Project

#include <string.h>

#include <core/riscv/csr.h>
#include <core/riscv/rv.h>

static const char * reg_name[] = {
    [RV_ZERO] = "zero",
    [RV_RA] = "ra",
    [RV_SP] = "sp",
    [RV_GP] = "gp",
    [RV_TP] = "tp",
    [RV_T0] = "t0",
    [RV_T1] = "t1",
    [RV_T2] = "t2" ,
    [RV_FP] = "fp" ,
    // [RV_S0] = "s0" ,  same as fp
    [RV_S1] = "s1" ,
    [RV_A0] = "a0" ,
    [RV_A1] = "a1" ,
    [RV_A2] = "a2" ,
    [RV_A3] = "a3" ,
    [RV_A4] = "a4" ,
    [RV_A5] = "a5" ,
    [RV_A6] = "a6" ,
    [RV_A7] = "a7" ,
    [RV_S2] = "s2" ,
    [RV_S3] = "s3" ,
    [RV_S4] = "s4" ,
    [RV_S5] = "s5" ,
    [RV_S6] = "s6" ,
    [RV_S7] = "s7" ,
    [RV_S8] = "s8" ,
    [RV_S9] = "s9" ,
    [RV_S10] = "s10",
    [RV_S11] = "s11",
    [RV_T3] = "t3",
    [RV_T4] = "t4",
    [RV_T5] = "t5",
    [RV_T6] = "t6",
};

typedef struct {
    uint8_t num;
    const char *name;
} csr_name_t;

static const csr_name_t csr_name_0[] = {
    { 0x01, "fflags" },
    { 0x02, "frm" },
    { 0x03, "fcsr" },
    { 0xFF, NULL },
};

static const csr_name_t csr_name_1[] = {
    { 0x00, "sstatus" },
    { 0x04, "sie" },
    { 0x05, "stvec" },
    { 0x06, "scounteren" },
    { 0x0A, "senvofg" },
    { 0x40, "sscratch" },
    { 0x41, "sepc" },
    { 0x42, "scause" },
    { 0x43, "stval" },
    { 0x44, "sip" },
    { 0x80, "satp" },
    { 0xFF, NULL },
};

static const csr_name_t csr_name_2[] = {
    { 0x00, "vsstatus" },
    { 0x04, "vsie" },
    { 0x05, "vstvec" },
    { 0x40, "vsscratch" },
    { 0x41, "vsepc" },
    { 0x42, "vscause" },
    { 0x43, "vstval" },
    { 0x44, "vsip" },
    { 0x80, "vsatp" },
    { 0xFF, NULL },
};

static const csr_name_t csr_name_3[] = {
    { 0x00, "mstatus" },
    { 0x01, "misa" },
    { 0x02, "medeleg" },
    { 0x03, "mideleg" },
    { 0x04, "mie" },
    { 0x05, "mtvec" },
    { 0x06, "mcounteren" },
    { 0x0A, "menvefg" },
    { 0x10, "mstatush" },
    { 0x1A, "menvcfgh" },
    { 0x20, "mcountinhibit" },
    { 0x23, "mhpmevent3" },  { 0x24, "mhpmevent4" },  { 0x25, "mhpmevent5" },  { 0x26, "mhpmevent6" },
    { 0x27, "mhpmevent7" },  { 0x28, "mhpmevent8" },  { 0x29, "mhpmevent9" },  { 0x2a, "mhpmevent10" },
    { 0x2b, "mhpmevent11" }, { 0x2c, "mhpmevent12" }, { 0x2d, "mhpmevent13" }, { 0x2e, "mhpmevent14" }, 
    { 0x2f, "mhpmevent15" }, { 0x30, "mhpmevent16" }, { 0x31, "mhpmevent17" }, { 0x32, "mhpmevent18" }, 
    { 0x33, "mhpmevent19" }, { 0x34, "mhpmevent20" }, { 0x35, "mhpmevent21" }, { 0x36, "mhpmevent22" },
    { 0x37, "mhpmevent23" }, { 0x38, "mhpmevent24" }, { 0x39, "mhpmevent25" }, { 0x3a, "mhpmevent26" },
    { 0x3b, "mhpmevent27" }, { 0x3c, "mhpmevent28" }, { 0x3d, "mhpmevent29" }, { 0x3e, "mhpmevent30" },
    { 0x3f, "mhpmevent31" },
    { 0x40, "mscratch" },
    { 0x41, "mepc" },
    { 0x42, "mcause" },
    { 0x43, "mtval" },
    { 0x44, "mip" },
    { 0x4A, "mtinst" },
    { 0x4B, "mtval2" },
    { 0x57, "mseccfgh" },
    { 0xA0, "pmpcfg0" },  { 0xA1, "pmpcfg1" },  { 0xA2, "pmpcfg2" },  { 0xA3, "pmpcfg3" },
    { 0xA4, "pmpcfg4" },  { 0xA5, "pmpcfg5" },  { 0xA6, "pmpcfg6" },  { 0xA7, "pmpcfg7" },
    { 0xA8, "pmpcfg8" },  { 0xA9, "pmpcfg9" },  { 0xAA, "pmpcfg10" }, { 0xAB, "pmpcfg11" },
    { 0xAC, "pmpcfg12" }, { 0xAD, "pmpcfg13" }, { 0xAE, "pmpcfg14" }, { 0xAF, "pmpcfg15" },
    { 0xB0, "pmpaddr0" },  { 0xB1, "pmpaddr1" },  { 0xB2, "pmpaddr2" },  { 0xB3, "pmpaddr3" },
    { 0xB4, "pmpaddr4" },  { 0xB5, "pmpaddr5" },  { 0xB6, "pmpaddr6" },  { 0xB7, "pmpaddr7" },
    { 0xB8, "pmpaddr8" },  { 0xB9, "pmpaddr9" },  { 0xBA, "pmpaddr10" }, { 0xBB, "pmpaddr11" },
    { 0xBC, "pmpaddr12" }, { 0xBD, "pmpaddr13" }, { 0xBE, "pmpaddr14" }, { 0xBF, "pmpaddr15" },
    { 0xC0, "pmpaddr16" }, { 0xC1, "pmpaddr17" }, { 0xC2, "pmpaddr18" }, { 0xC3, "pmpaddr19" },
    { 0xC4, "pmpaddr20" }, { 0xC5, "pmpaddr21" }, { 0xC6, "pmpaddr22" }, { 0xC7, "pmpaddr23" },
    { 0xC8, "pmpaddr24" }, { 0xC9, "pmpaddr25" }, { 0xCA, "pmpaddr26" }, { 0xCB, "pmpaddr27" },
    { 0xCC, "pmpaddr28" }, { 0xCD, "pmpaddr29" }, { 0xCE, "pmpaddr30" }, { 0xCF, "pmpaddr31" },
    { 0xD0, "pmpaddr32" }, { 0xD1, "pmpaddr33" }, { 0xD2, "pmpaddr34" }, { 0xD3, "pmpaddr35" },
    { 0xD4, "pmpaddr36" }, { 0xD5, "pmpaddr37" }, { 0xD6, "pmpaddr38" }, { 0xD7, "pmpaddr39" },
    { 0xD8, "pmpaddr40" }, { 0xD9, "pmpaddr41" }, { 0xDA, "pmpaddr42" }, { 0xDB, "pmpaddr43" },
    { 0xDC, "pmpaddr44" }, { 0xDD, "pmpaddr45" }, { 0xDE, "pmpaddr46" }, { 0xDF, "pmpaddr47" },
    { 0xE0, "pmpaddr48" }, { 0xE1, "pmpaddr49" }, { 0xE2, "pmpaddr50" }, { 0xE3, "pmpaddr51" },
    { 0xE4, "pmpaddr52" }, { 0xE5, "pmpaddr53" }, { 0xE6, "pmpaddr54" }, { 0xE7, "pmpaddr55" },
    { 0xE8, "pmpaddr56" }, { 0xE9, "pmpaddr57" }, { 0xEA, "pmpaddr58" }, { 0xEB, "pmpaddr59" },
    { 0xEC, "pmpaddr60" }, { 0xED, "pmpaddr61" }, { 0xEE, "pmpaddr62" }, { 0xEF, "pmpaddr63" },
    { 0xFF, NULL },
};

static const csr_name_t csr_name_5[] = {
    { 0xA8, "scontext" },
    { 0xFF, NULL },
};

static const csr_name_t csr_name_6[] = {
    { 0x00, "hstatus" },
    { 0x02, "hedeleg" },
    { 0x03, "hideleg" },
    { 0x04, "hie" },
    { 0x05, "htimedelta" },
    { 0x06, "hcounteren" },
    { 0x07, "hgeie" },
    { 0x0A, "henvofg" },
    { 0x15, "htimedeltah" },
    { 0x1A, "henvofgh" },
    { 0x43, "htval" },
    { 0x44, "hip" },
    { 0x45, "hvip" },
    { 0x4A, "htinst" },
    { 0x80, "hgatp" },
    { 0xA8, "hcontext" },
    { 0xFF, NULL },
};

static const csr_name_t csr_name_7[] = {
    { 0x47, "mseccfg" },
    { 0xA0, "tselect" },
    { 0xA1, "tdatal" },
    { 0xA2, "tdata2" },
    { 0xA3, "tdata3" },
    { 0xA8, "mcontext" },
    { 0xB0, "desr" },
    { 0xB1, "dpc" },
    { 0xB2, "dscratch0" },
    { 0xB3, "dscratch1" },
    { 0xFF, NULL },
};

static const csr_name_t csr_name_b[] = {
    { 0x00, "mcycle" },
    { 0x02, "minstret" },
    { 0x03, "mhpmcounter3" },  { 0x04, "mhoncounter4" },  { 0x05, "mhoncounter5" },  { 0x06, "mhoncounter6" },
    { 0x07, "mhoncounter7" },  { 0x08, "mhoncounter8" },  { 0x09, "mhoncounter9" },  { 0x0A, "mhoncounter10" },
    { 0x0B, "mhoncounter11" }, { 0x0C, "mhoncounter12" }, { 0x0D, "mhoncounter13" }, { 0x0E, "mhoncounter14" },
    { 0x0F, "mhoncounter15" }, { 0x10, "mhoncounter16" }, { 0x11, "mhoncounter17" }, { 0x12, "mhoncounter18" },
    { 0x13, "mhoncounter19" }, { 0x14, "mhoncounter20" }, { 0x15, "mhoncounter21" }, { 0x16, "mhoncounter22" },
    { 0x17, "mhoncounter23" }, { 0x18, "mhoncounter24" }, { 0x19, "mhoncounter25" }, { 0x1A, "mhoncounter26" },
    { 0x1B, "mhoncounter27" }, { 0x1C, "mhoncounter28" }, { 0x1D, "mhoncounter29" }, { 0x1E, "mhoncounter30" },
    { 0x1F, "mhoncounter31" },
    { 0x80, "mcycleh" },
    { 0x82, "minstreth" },
    { 0x83, "mhoncounter3h" },  { 0x84, "mhpmcounter4h" },  { 0x85, "mhpmcounter5h" },  { 0x86, "mhpmcounter6h" },
    { 0x87, "mhpmcounter7h" },  { 0x88, "mhpmcounter8h" },  { 0x89, "mhpmcounter9h" },  { 0x8A, "mhpmcounter10h" },
    { 0x8B, "mhpmcounter11h" }, { 0x8C, "mhpmcounter12h" }, { 0x8D, "mhpmcounter13h" }, { 0x8E, "mhpmcounter14h" },
    { 0x8F, "mhpmcounter15h" }, { 0x90, "mhpmcounter16h" }, { 0x91, "mhpmcounter17h" }, { 0x92, "mhpmcounter18h" },
    { 0x93, "mhpmcounter19h" }, { 0x94, "mhpmcounter20h" }, { 0x95, "mhpmcounter21h" }, { 0x96, "mhpmcounter22h" },
    { 0x97, "mhpmcounter23h" }, { 0x98, "mhpmcounter24h" }, { 0x99, "mhpmcounter25h" }, { 0x9A, "mhpmcounter26h" },
    { 0x9B, "mhpmcounter27h" }, { 0x9C, "mhpmcounter28h" }, { 0x9D, "mhpmcounter29h" }, { 0x9E, "mhpmcounter30h" },
    { 0x9F, "mhpmcounter31h" },
    { 0xFF, NULL },
};

static const csr_name_t csr_name_c[] = {
    { 0x00, "cycle" },
    { 0x01, "time" },
    { 0x02, "instret" },
    { 0x03, "homcounter3" },  { 0x04, "homcounter4" },  { 0x05, "homcounter5" },  { 0x06, "homcounter6" },
    { 0x07, "homcounter7" },  { 0x08, "homcounter8" },  { 0x09, "homcounter9" },  { 0x0A, "homcounter10" },
    { 0x0B, "homcounter11" }, { 0x0C, "homcounter12" }, { 0x0D, "homcounter13" }, { 0x0E, "homcounter14" },
    { 0x0F, "homcounter15" }, { 0x10, "homcounter16" }, { 0x11, "homcounter17" }, { 0x12, "homcounter18" },
    { 0x13, "homcounter19" }, { 0x14, "homcounter20" }, { 0x15, "homcounter21" }, { 0x16, "homcounter22" },
    { 0x17, "homcounter23" }, { 0x18, "homcounter24" }, { 0x19, "homcounter25" }, { 0x1A, "homcounter26" },
    { 0x1B, "homcounter27" }, { 0x1C, "homcounter28" }, { 0x1D, "homcounter29" }, { 0x1E, "homcounter30" },
    { 0x1F, "hpmcounter31" },
    { 0x80, "cycleh" },
    { 0x81, "timeh" },
    { 0x82, "instreth" },
    { 0x83, "hpmcounter3h" },  { 0x84, "homcounter4h" },  { 0x85, "homcounter5h" },  { 0x86, "homcounter6h" },
    { 0x87, "homcounter7h" },  { 0x88, "homcounter8h" },  { 0x89, "homcounter9h" },  { 0x8A, "homcounter10h" },
    { 0x8B, "homcounter11h" }, { 0x8C, "homcounter12h" }, { 0x8D, "homcounter13h" }, { 0x8E, "homcounter14h" },
    { 0x8F, "homcounter15h" }, { 0x90, "homcounter16h" }, { 0x91, "homcounter17h" }, { 0x92, "homcounter18h" },
    { 0x93, "homcounter19h" }, { 0x94, "homcounter20h" }, { 0x95, "homcounter21h" }, { 0x96, "homcounter22h" },
    { 0x97, "homcounter23h" }, { 0x98, "homcounter24h" }, { 0x99, "homcounter25h" }, { 0x9A, "homcounter26h" },
    { 0x9B, "homcounter27h" }, { 0x9C, "homcounter28h" }, { 0x9D, "homcounter29h" }, { 0x9E, "homcounter30h" },
    { 0x9F, "hpmcounter31h" },
    { 0xFF, NULL },
};

static const csr_name_t csr_name_e[] = {
    { 0x12, "hgeip" },
    { 0xFF, NULL },
};

static const csr_name_t csr_name_f[] = {
    { 0x11, "mvendorid" },
    { 0x12, "marchid" },
    { 0x13, "mimpid" },
    { 0x14, "mhartid" },
    { 0x15, "mconfigpti" },
    { 0xFF, NULL },
};

const char *rv_name_for_reg(uint32_t reg) {
    switch (reg) {
    case CORE_REG_PC: return "pc";
    case CORE_REG_SP: reg = RV_SP; break;
    case CORE_REG_LR: reg = RV_RA; break;
    default: break;
    }
    return reg_name[reg];
}

uint32_t rv_reg_for_name(const char *name) {
    if (!strcmp(name, "pc")) return CORE_REG_PC;
    if (name[0] == 'x') {
        uint8_t c = name[1];
        if (c < '0' || c > '9') return CORE_REG_INVALID;
        if (name[2] == '\0')
            return c - '0';
        uint8_t d = name[2];
        if (d < '0' || d > '9') return CORE_REG_INVALID;
        c = ((c - '0') * 10) + (d - '0');
        if (c > 31) return CORE_REG_INVALID;
        return c;
    }
    for (uint32_t i = 0; i < countof(reg_name); i++) {
        if (!strcmp(name, reg_name[i])) return i;
    }
    return CORE_REG_INVALID;
}


static const csr_name_t* csr_name_index[] = {
    [0x0] = csr_name_0,
    [0x1] = csr_name_1,
    [0x2] = csr_name_2,
    [0x3] = csr_name_3,
    [0x5] = csr_name_5,
    [0x6] = csr_name_6,
    [0x7] = csr_name_7,
    [0xb] = csr_name_b,
    [0xc] = csr_name_c,
    [0xe] = csr_name_e,
    [0xf] = csr_name_f,
};

const char *rv_name_for_sysreg(rv_core_t *c, uint16_t num) {
    const uint16_t lt = (num >> 8) & 0xf;
    const csr_name_t *list = csr_name_index[lt];
    if (list == NULL) return NULL;
    num &= 0xff;
    if (num == 0xff) return NULL;
    int i;
    for (i = 0; list[i].num < num; i++) ;
    if (list[i].num == num) return list[i].name;
    return NULL;
}
