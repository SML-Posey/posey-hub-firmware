#include "platform.hpp"

#include <zephyr/logging/log.h>

#include "platform/io/CDCACMDriver.h"
#include "platform/io/NordicNUSDriver.h"

#define LOG_MODULE_NAME posey_platform
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

NordicNUSWriter writer;
NordicNUSReader reader;

bool init_platform() {
    LOG_INF("Initializing platform...");

    LOG_INF("Initializing CDC ACM...");
    const struct device* uart_dev = cdc_acm_get_device();
    if (uart_dev == NULL) {
        LOG_ERR("Unable to get CDC ACM device!");
        return false;
    }
    if (cdc_acm_init(uart_dev) != 0) {
        LOG_ERR("Unable to initialize CDC ACM!");
        return false;
    }

    LOG_INF("Initializing NUS...");
    if (init_nus() != 0) {
        LOG_ERR("Unable to initialize BLE and NUS!");
        return false;
    }

    LOG_INF("Platform initialization complete.");

    return true;
}
