// SPDX-License-Identifier: MIT License
// Copyright (c) 2023 Shac Ron and The Sled Project

#pragma once

#define MPU_MAX_MAPPINGS            64

#define MPU_REG_DEV_TYPE            0x0  // RO
#define MPU_REG_DEV_VERSION         0x4  // RO
#define MPU_REG_CONFIG              0x8  // RW
#define MPU_REG_STATUS              0xc  // RO
#define MPU_REG_MAP_ENTS            0x10 // RO
#define MPU_REG_MAP_VA_BASE_LO(i)   (0x100 + (8 * i))   // RW
#define MPU_REG_MAP_VA_BASE_HI(i)   (0x104 + (8 * i))   // RW
#define MPU_REG_MAP_PA_BASE_LO(i)   (0x300 + (8 * i))   // RW
#define MPU_REG_MAP_PA_BASE_HI(i)   (0x304 + (8 * i))   // RW
#define MPU_REG_MAP_LEN(i)          (0x500 + (4 * i))   // RW

#define MPU_APERTURE_LENGTH         0x600


// MPU_REG_CONFIG

// enable mapping using the last applied mapping values
#define MPU_CONFIG_ENABLE           (1u << 0)

// Apply the current mapping registers for mapping.
#define MPU_CONFIG_APPLY            (1u << 1)

// Clear all mapping registers. This does not affect applied mappings.
// If CLEAR and APPLY are both set, current values in the map are applied and then
// the mapping registers are cleared.
#define MPU_CONFIG_CLEAR            (1u << 2)

// MPU_REG_STATUS

// Indicates whether mapping is enabled.
#define MPU_STATUS_ENABLED          MPU_CONFIG_ENABLE


// MPU_REG_MAP_ENTS
// This register holds the maximum number of entries for each MPU_REG_MAP* register.
// Currently equivalent to MPU_MAX_MAPPINGS


// MPU_REG_MAP_VA_BASE
// Virtual base address of region to be mapped

// MPU_REG_MAP_PA_BASE
// Corresponding physical base address to return

// MPU_REG_MAP_LEN
// Length in bytes of the mapped region.
