#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <zephyr/device.h>

const struct device* cdc_acm_get_device();
int cdc_acm_init(const struct device* dev);
uint16_t cdc_acm_send(
    const struct device* dev, const uint8_t* data, uint16_t len);

#ifdef __cplusplus
}
#endif
