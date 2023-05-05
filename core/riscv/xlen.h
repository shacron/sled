#pragma once

#include <inttypes.h>
#include <stdint.h>

#if USING_RV32
#define XLEN 32
u32   typedef uxlen_t;
i32    typedef sxlen_t;
u64   typedef ux2len_t;
i64    typedef sx2len_t;
result32_t typedef resultxlen_t;
#define XLEN_PREFIX(name) rv32_ ## name
#define PRIXLENx PRIx32
#else
#define XLEN 64
u64    typedef uxlen_t;
i64     typedef sxlen_t;
__uint128_t typedef ux2len_t;
__int128_t  typedef sx2len_t;
result64_t  typedef resultxlen_t;
#define XLEN_PREFIX(name) rv64_ ## name
#define PRIXLENx PRIx64
#endif
