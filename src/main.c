#include <stdbool.h>
#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "cdc_acm.h"

LOG_MODULE_REGISTER(posey_hub, LOG_LEVEL_INF);

#define DummyDataSize (1024)

inline uint32_t get_time_us() {
    return k_cyc_to_us_floor32(sys_clock_tick_get());
}

int main(void) {
    LOG_INF("Getting CDC ACM device...");
    const struct device* dev = cdc_acm_get_device();
    if (dev == NULL) {
        LOG_ERR("Could not get CDC ACM device!");
        return -1;
    }

    LOG_INF("Initializing CDC ACM...");
    if (cdc_acm_init(dev) < 0) {
        LOG_ERR("Could not initialize CDC ACM!");
        return -1;
    }

    // Initialize dummy data to be sent.
    LOG_INF("Initializing dummy data...");
    static uint8_t dummy_data[DummyDataSize];
    for (uint16_t i = 0; i < DummyDataSize; i++) {
        dummy_data[i] = '0' + (i % 10);
    }
    for (uint16_t i = DummyDataSize - 4; i < DummyDataSize; i++) {
        dummy_data[i] = '-';
    }

    // We want to send the dummy data at a rate of 100 KB/s.
    LOG_INF("Sending dummy data at 100 KB/s...");
    const uint32_t PacketDelay_us = 1e6 / (100 * 1024 / DummyDataSize);
    uint16_t iter = 0;
    while (true) {
        // Get start time in us.
        uint32_t t0 = get_time_us();

        // Conver iter to a string, and roll over at 10000.
        static char iter_str[5];
        snprintf(iter_str, 5, "%04d", iter);
        strncpy((char*)&dummy_data[DummyDataSize - 4], iter_str, 4);
        iter = (iter + 1) % 10000;

        cdc_acm_send(dev, dummy_data, DummyDataSize);

        // Delay.
        uint32_t t1 = get_time_us();
        uint32_t cycle_dt = t1 - t0;
        if (cycle_dt >= PacketDelay_us)
            continue;
        k_usleep(PacketDelay_us - cycle_dt);
    }

    LOG_ERR("Exited main loop!");
    return 0;
}
