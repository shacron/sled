#include <stdatomic.h>
#include <stdio.h>

#include <sl/device.h>
#include <sled/error.h>

int intc_create(device_t **dev_out);
int rtc_create(device_t **dev_out);
int uart_create(device_t **dev_out);

static atomic_uint_least32_t device_id = 0;

int device_create(uint32_t type, const char *name, device_t **dev_out) {
    int err;
    device_t *d;

    switch (type) {
    case DEVICE_UART: err = uart_create(&d); break;
    case DEVICE_INTC: err = intc_create(&d); break;
    case DEVICE_RTC:  err = rtc_create(&d);  break;
    default:
        return SL_ERR_ARG;
    }
    if (err) return err;
    d->name = name;
    *dev_out = d;
    return 0;
}

void dev_init(device_t *d, uint32_t type) {
    d->type = type;
    d->id = atomic_fetch_add_explicit(&device_id, 1, memory_order_relaxed);
}
