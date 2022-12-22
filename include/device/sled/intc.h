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
