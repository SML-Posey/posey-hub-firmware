#pragma once

#include <stdint.h>
#include <zephyr/bluetooth/conn.h>

#ifdef __cplusplus
extern "C" {
#endif

int init_nus();
int num_connected_sensors();
void close_connections();
void enable_scanning();
void disable_scanning();

int8_t read_conn_rssi(struct bt_conn* conn);

void disable_sensors();
void enable_sensors();

void process_data(
    struct bt_conn* conn,
    const uint8_t slot,
    const uint8_t* data,
    const uint16_t size);

void bt_nus_pc_received(
    struct bt_conn* conn, const uint8_t* data, uint16_t len);

struct bt_conn* get_pc_connection();

#ifdef __cplusplus
}
#endif
