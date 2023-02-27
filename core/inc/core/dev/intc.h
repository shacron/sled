// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2023 Shac Ron and The Sled Project

#pragma once

#include <core/device.h>
#include <core/irq.h>

int intc_create(sl_dev_t **dev_out);
// int intc_add_target(device_t *dev, irq_handler_t *target, uint32_t num);
// irq_handler_t * intc_get_irq_handler(device_t *dev);
