#pragma once

#include <inttypes.h>
#include <stdint.h>

#if USING_RV32
#define XLEN 32
u4 typedef uxlen_t;
i4 typedef sxlen_t;
u8 typedef ux2len_t;
i8 typedef sx2len_t;
result32_t typedef resultxlen_t;
#define XLEN_PREFIX(name) rv32_ ## name
#define PRIXLENx PRIx32
#else
#define XLEN 64
u8 typedef uxlen_t;
i8 typedef sxlen_t;
__uint128_t typedef ux2len_t;
__int128_t  typedef sx2len_t;
result64_t  typedef resultxlen_t;
#define XLEN_PREFIX(name) rv64_ ## name
#define PRIXLENx PRIx64
#endif
