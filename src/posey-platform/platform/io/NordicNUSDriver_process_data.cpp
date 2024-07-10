#include "NordicNUSDriver.h"

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/hci_vs.h>

#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/services/hrs.h>
#include <zephyr/bluetooth/uuid.h>

#include "CDCACMDriver.h"

#include "posey-platform/ZephyrPlatform.hpp"

#define LOG_MODULE_NAME posey_nus_process
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include "platform/sensors/BaseFlashBlock.hpp"

extern "C" void bt_nus_pc_received(
    struct bt_conn* conn, const uint8_t* data, uint16_t len) {
    // int err;
    char addr[BT_ADDR_LE_STR_LEN] = {0};

    LOG_INF("NordicNUSDriver::bt_nus_pc_received");
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, ARRAY_SIZE(addr));

    auto written = reader.write_from(data, len);
    LOG_INF("Received %d of %d bytes from PC %s.", written, len, addr);
}

static void read_handle_rssi(uint16_t handle, int8_t* rssi) {
    struct net_buf *buf, *rsp = NULL;
    struct bt_hci_cp_read_rssi* cp;
    struct bt_hci_rp_read_rssi* rp;

    int err;

    buf = bt_hci_cmd_create(BT_HCI_OP_READ_RSSI, sizeof(*cp));
    if (!buf) {
        printk("Unable to allocate command buffer\n");
        return;
    }

    cp = reinterpret_cast<struct bt_hci_cp_read_rssi*>(
        net_buf_add(buf, sizeof(*cp)));
    cp->handle = sys_cpu_to_le16(handle);

    err = bt_hci_cmd_send_sync(BT_HCI_OP_READ_RSSI, buf, &rsp);
    if (err) {
        uint8_t reason =
            rsp ? ((struct bt_hci_rp_read_rssi*)rsp->data)->status : 0;
        printk("Read RSSI err: %d reason 0x%02x\n", err, reason);
        return;
    }

    rp = reinterpret_cast<struct bt_hci_rp_read_rssi*>(rsp->data);
    *rssi = rp->rssi;

    net_buf_unref(rsp);
}

extern "C" int8_t read_conn_rssi(struct bt_conn* conn) {
    static uint16_t conn_handle;
    int8_t rssi = 0;
    bt_hci_get_conn_handle(conn, &conn_handle);
    read_handle_rssi(conn_handle, &rssi);
    return rssi;
}

int write_uart(const uint8_t* const data, uint16_t size) {
    static const struct device* cdc_acm_dev = cdc_acm_get_device();
    uint16_t written = cdc_acm_send(cdc_acm_dev, data, size);
    if (written != size)
        LOG_DBG("Dropped %d bytes", size - written);

    return 0;
}

extern "C" void process_data(
    struct bt_conn* conn,
    const uint8_t slot,
    const uint8_t* data,
    const uint16_t size) {
    static BaseFlashBlock header;
    static int8_t rssi = 0;

    if (conn != NULL)
        rssi = read_conn_rssi(conn);
    else
        rssi = 0;

    const bt_addr_le_t* addr = bt_conn_get_dst(conn);
    if (addr != NULL) {
        for (int i = 0; i < 6; i++)
            header.data.mac[i] = addr->a.val[i];
    } else {
        for (int i = 0; i < 6; i++)
            header.data.mac[i] = 0;
    }

    // Write block header.
    header.data.slot = slot;
    header.data.time_ms = Clock::get_msec<uint32_t>();
    header.data.rssi = rssi;
    header.data.block_bytes = size;
    header.data.serialize(header.buffer);
    write_uart(header.buffer.get_buffer(), header.buffer.used());

    // Write block data.
    write_uart(data, size);
}
