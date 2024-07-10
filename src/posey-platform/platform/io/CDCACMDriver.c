#include "CDCACMDriver.h"

#include <stdio.h>
#include <string.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/ring_buffer.h>

#include <zephyr/logging/log.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/usbd.h>

LOG_MODULE_REGISTER(cdc_acm, LOG_LEVEL_INF);

#define RingBufferSize 1024
static uint8_t outgoing_data[RingBufferSize];
struct ring_buf outgoing_ringbuf;

static void interrupt_handler(const struct device* dev, void* user_data) {
    ARG_UNUSED(user_data);

    while (uart_irq_update(dev) && uart_irq_is_pending(dev)) {
        if (uart_irq_rx_ready(dev)) {
            static uint8_t buffer[1024];
            int recv_len = uart_fifo_read(dev, buffer, 1024);
            LOG_INF("Received %d bytes.", recv_len);
        }

        if (uart_irq_tx_ready(dev)) {
            static uint8_t buffer[RingBufferSize];
            int rb_len =
                ring_buf_get(&outgoing_ringbuf, buffer, sizeof(buffer));
            if (!rb_len) {
                uart_irq_tx_disable(dev);
                continue;
            }

            int send_len = uart_fifo_fill(dev, buffer, rb_len);
            if (send_len < rb_len) {
                LOG_ERR("Drop %d bytes", rb_len - send_len);
            }
        }
    }
}

const struct device* cdc_acm_get_device() {
    const struct device* dev = DEVICE_DT_GET_ONE(zephyr_cdc_acm_uart);
    if (!device_is_ready(dev)) {
        LOG_ERR("CDC ACM device not ready");
        return NULL;
    }
    return dev;
}

int cdc_acm_init(const struct device* dev) {
    int ret;

    ret = usb_enable(NULL);
    if (ret != 0) {
        LOG_ERR("Failed to enable USB");
        return ret;
    }

    ring_buf_init(&outgoing_ringbuf, sizeof(outgoing_data), outgoing_data);

    k_msleep(500);

    uart_irq_callback_set(dev, interrupt_handler);
    uart_irq_rx_enable(dev);

    return 0;
}

uint16_t cdc_acm_send(
    const struct device* dev, const uint8_t* data, uint16_t len) {
    uint16_t to_write = ring_buf_put(&outgoing_ringbuf, data, len);
    if (to_write) {
        uart_irq_tx_enable(dev);
    }
    return to_write;
}
