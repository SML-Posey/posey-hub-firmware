#include "platform.hpp"
#include "platform/config.h"

#include "TaskHub.hpp"

#include "MessageAck.hpp"

#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/sys/reboot.h>

#include <zephyr/drivers/pwm.h>

#include "posey-platform/platform/io/NordicNUSDriver.h"

#define LOG_MODULE_NAME posey_task_hub
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

// LEDs.
static const struct pwm_dt_spec red_pwm_led =
    PWM_DT_SPEC_GET(DT_ALIAS(pwm_led0));
static const struct pwm_dt_spec green_pwm_led =
    PWM_DT_SPEC_GET(DT_ALIAS(pwm_led1));
static const struct pwm_dt_spec blue_pwm_led =
    PWM_DT_SPEC_GET(DT_ALIAS(pwm_led2));

bool TaskHub::setup() {
    LOG_INF("Setting up TaskHub...");

    LOG_INF("Adding message listener...");
    if (!ml.add_listener(cmd)) {
        LOG_ERR("Could not add message listener!");
        return false;
    }

    LOG_INF("Checking LEDs are ready...");
    if (!pwm_is_ready_dt(&red_pwm_led) || !pwm_is_ready_dt(&green_pwm_led) ||
        !pwm_is_ready_dt(&blue_pwm_led)) {
        LOG_ERR("Error: one or more LED PWM devices not ready\n");
        return 0;
    }
    set_rgb(0, 1, 0);

    LOG_INF("TaskHub setup complete.");

    return true;
}

void TaskHub::set_rgb(const float r, const float g, const float b) {
    int ret;

    uint32_t pulse_red = (uint32_t)(r * red_pwm_led.period);
    ret = pwm_set_pulse_dt(&red_pwm_led, pulse_red);
    if (ret != 0) {
        LOG_ERR("Error %d: red write failed\n", ret);
    }

    uint32_t pulse_green = (uint32_t)(g * green_pwm_led.period);
    ret = pwm_set_pulse_dt(&green_pwm_led, pulse_green);
    if (ret != 0) {
        LOG_ERR("Error %d: green write failed\n", ret);
    }

    uint32_t pulse_blue = (uint32_t)(b * blue_pwm_led.period);
    ret = pwm_set_pulse_dt(&blue_pwm_led, pulse_blue);
    if (ret != 0) {
        LOG_ERR("Error %d: blue write failed\n", ret);
    }
}

void TaskHub::loop() {
    static const uint32_t max_loop_time = 1e3 / 50;
    static uint32_t iter = 0;

    tm.message.t_start_ms = Clock::get_msec<uint32_t>();

    // Check for messages. Process everything available, we'll
    // let this task overrun.
    ml.poll(reader);
    while (true) {
        auto mid = ml.process_next();
        if (mid <= -1)
            break;
        process_message(mid);
    }

    // Send task TM at 1Hz.
    if (iter % 50 == 0) {
        tm.message.ble_throughput = num_connected_sensors();
        tm.serialize();
        process_data(NULL, 0xFA, tm.buffer.get_buffer(), tm.buffer.used());
    }

    // Indicate BT connection count every 10 seconds.
    static bool indicating_connections = false;
    static uint32_t next_indicate = 0;
    static bool next_indicate_on = false;
    static int num_connections = 0;
    if (iter % 500 == 0) {
        num_connections = num_connected_sensors();
        LOG_INF("Connected sensors: %d", num_connections);
        indicating_connections = true;
        next_indicate = iter;
        next_indicate_on = true;
    }
    if (indicating_connections && (iter == next_indicate)) {
        if (next_indicate_on) {
            if (num_connections == 0) {
                set_rgb(1, 0, 0);
                next_indicate = iter + 50;
            } else {
                set_rgb(0, 0, 1);
                next_indicate = iter + 20;
            }
            next_indicate_on = false;
        } else {
            set_rgb(0, 0, 0);
            next_indicate_on = true;
            next_indicate = iter + 30;
            --num_connections;
        }

        if (next_indicate_on && (num_connections < 0)) {
            indicating_connections = false;
            set_rgb(0, 1, 0);
        }
    }

    ++iter;

    // Update missed deadlines if needed.
    tm.message.t_end_ms = Clock::get_msec<uint32_t>();
    if (tm.message.t_end_ms - tm.message.t_start_ms > max_loop_time) {
        tm.message.missed_deadline++;
    }
}

void TaskHub::process_message(const uint16_t mid) {
    bool invalid_checksum = false;

    // Command message?
    if (mid == Command::message_id) {
        // Extract command.
        if (cmd.valid_checksum()) {
            auto& msg = cmd.deserialize();
            cmd.message.ack = MessageAck::OK;
            cmd.serialize();
            process_data(
                NULL, 0xFA, cmd.buffer.get_buffer(), cmd.buffer.used());
            Clock::delay_msec(500);

            LOG_INF("Received %s command", msg.command_str());

            // Handle command.
            switch (msg.command) {
                case Command::NoOp:
                    LOG_WRN("No operation!");
                    break;

                case Command::Reboot:
                    LOG_INF("Waiting 5s then rebooting.");
                    Clock::delay_msec(5000);
                    sys_reboot(SYS_REBOOT_COLD);
                    break;

                case Command::Configure:
                case Command::ConnectPeripheral:
                    LOG_WRN("Not implemented.");
                    break;
                default:
                    LOG_WRN("Unknown command");
                    break;
            }
        } else {
            invalid_checksum = true;
            cmd.message.ack = MessageAck::Resend;
            cmd.serialize();
            process_data(
                NULL, 0xFA, cmd.buffer.get_buffer(), cmd.buffer.used());
        }
    }

    // Invalid checksum?
    if (invalid_checksum) {
        tm.message.invalid_checksum++;
    }
}
