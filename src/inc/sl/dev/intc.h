#pragma once

#include <sl/device.h>
#include <sl/irq.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct intc intc_t;

int intc_create(device_t **dev_out);
int intc_add_target(device_t *dev, irq_handler_t *target, uint32_t num);
irq_handler_t * intc_get_irq_handler(device_t *dev);

#ifdef __cplusplus
}
#endif
