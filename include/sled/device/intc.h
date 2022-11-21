#pragma once

#define INTC_REG_DEV_TYPE       0x0  // RO
#define INTC_REG_DEV_VERSION    0x4  // RO

// interrupts that are currently asserted (including masked)
// Writing to a bit clears that asserted bit, a 0 has no effect.
#define INTC_REG_ASSERTED       0x8  // RW

// interrupts that are masked. Masked interrupt will be asserted
// but will not trigger an IRQ
// Default value: 0xffffffff
#define INTC_REG_MASK           0xc  // RW

#define INTC_APERTURE_LENGTH    0x10
