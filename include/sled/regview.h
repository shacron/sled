// SPDX-License-Identifier: MIT License
// Copyright (c) 2023 Shac Ron and The Sled Project

#pragma once

#include <sled/types.h>

#ifdef __cplusplus
extern "C" {
#endif

// reg_view is a wrapper for peripheral address ranges

// This is intended for:
// 1. platforms with messy MMIO spaces with overlapping device register addresses.
// 2. devices whose register layouts change per instance or per platform.

// Once allocated, new mappings can be added to the reg_view. The mappings are
// a static list of 'view addresses', the addresses visible in the MMIO space,
// and 'device addresses', the addresses presented to the device's IO functions.
// Multiple mappings can be added as needed, though they should not conflict for the
// same view address.
// The mapped devices receive a uniform address range, allowing the same code to be
// used without needing be aware of the variable user-facing addresses.

int sl_reg_view_create(const char *name, sl_dev_config_t *cfg, sl_reg_view_t **rv_out);

sl_dev_t * sl_reg_view_get_dev(sl_reg_view_t *rv);

int sl_reg_view_add_mapping(sl_reg_view_t *rv, sl_dev_t *dev, u4 view_offset, const u4 *view_addr, const u4 *dev_addr, u4 addr_count);

void sl_reg_view_print_mappings(sl_dev_t *d, u8 base);

#ifdef __cplusplus
}
#endif
