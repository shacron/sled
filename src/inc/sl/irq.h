#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct irq_handler irq_handler_t;

struct irq_handler {
    int (*irq)(irq_handler_t *h, uint32_t num,  bool high);
};
