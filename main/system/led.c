/*
 * Copyright (c) 2019-2024, Jacques Gagnon
 * SPDX-License-Identifier: Apache-2.0
 */

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <soc/efuse_reg.h>
#include "zephyr/atomic.h"
#include "system/gpio.h"
#include "driver/ledc.h"
#include "adapter/config.h"
#include "tools/util.h"
#include "led.h"

#ifdef CONFIG_BLUERETRO_SYSTEM_SEA_BOARD
#define ERR_LED_PIN 32
#else
#define ERR_LED_PIN 17
#endif
#define PICO_ERR_LED_PIN 20

#ifdef CONFIG_RETROSCALER_BLUERETRO_4LEDS_HW
/* GPIO 14 is also the BT monitor & coverage UART CTS pin, and GPIO 12 & 15
 * used by those UARTs are the HW1 port LEDs, so those debug features can't
 * be combined with this HW.
 */
#if defined(CONFIG_BLUERETRO_BT_H4_TRACE) || defined(CONFIG_BLUERETRO_COVERAGE)
#error "RETROSCALER_BLUERETRO_4LEDS_HW conflicts with BT_H4_TRACE & COVERAGE UART pins"
#endif

/* Per BT device slot LEDs of the RetroScaler NGC 4 LEDs HW */
static const uint8_t dev_led_pins[] = {
    14, 32, 33, 25
};
#endif

/* LED flags */
enum {
    ERR_LED_SET = 0,
};

static atomic_t led_flags = 0;
static TaskHandle_t err_led_task_hdl;
static uint8_t err_led_pin = ERR_LED_PIN;

static void err_led_task(void *param) {
    while (1) {
        if (!atomic_test_bit(&led_flags, ERR_LED_SET)) {
            ledc_set_fade_time_and_start(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, hw_config.led_pulse_duty_max,
                hw_config.led_pulse_fade_time_ms, LEDC_FADE_NO_WAIT);
            vTaskDelay(hw_config.led_pulse_fade_cycle_delay_ms / portTICK_PERIOD_MS);
            ledc_set_fade_time_and_start(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, hw_config.led_pulse_duty_min,
                hw_config.led_pulse_fade_time_ms, LEDC_FADE_NO_WAIT);
            vTaskDelay(hw_config.led_pulse_fade_cycle_delay_ms / portTICK_PERIOD_MS);
        }
        else {
            vTaskSuspend(err_led_task_hdl);
        }
    }
}

void err_led_init(uint32_t package) {
    ledc_timer_config_t ledc_timer = {
        .duty_resolution = LEDC_TIMER_13_BIT,
        .freq_hz = hw_config.led_pulse_hz,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ledc_channel_config_t ledc_channel = {
        .channel    = LEDC_CHANNEL_0,
        .duty       = hw_config.led_pulse_off_duty_cycle,
        .gpio_num   = ERR_LED_PIN,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .hpoint     = 0,
        .timer_sel  = LEDC_TIMER_0,
    };

    if (package == EFUSE_RD_CHIP_VER_PKG_ESP32PICOV302) {
        ledc_channel.gpio_num = PICO_ERR_LED_PIN;
        err_led_pin = PICO_ERR_LED_PIN;
    }

    ledc_timer_config(&ledc_timer);
    ledc_channel_config(&ledc_channel);
    ledc_fade_func_install(0);
    ledc_set_duty_and_update(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, hw_config.led_pulse_off_duty_cycle, 0);

    xTaskCreatePinnedToCore(&err_led_task, "err_led_task", 768, NULL, 5, &err_led_task_hdl, 0);
    err_led_clear();
}

void err_led_cfg_update(void) {
    ledc_set_freq(LEDC_HIGH_SPEED_MODE, LEDC_TIMER_0, hw_config.led_pulse_hz);
}

void err_led_set(void) {
    vTaskSuspend(err_led_task_hdl);
    ledc_set_duty_and_update(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, hw_config.led_pulse_on_duty_cycle, 0);
    atomic_set_bit(&led_flags, ERR_LED_SET);
}

void err_led_clear(void) {
    /* When error is set it stay on until power cycle */
    if (!atomic_test_bit(&led_flags, ERR_LED_SET)) {
        vTaskSuspend(err_led_task_hdl);
        ledc_set_duty_and_update(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, hw_config.led_pulse_off_duty_cycle, 0);
    }
}

void err_led_pulse(void) {
    vTaskResume(err_led_task_hdl);
}

uint32_t err_led_get_pin(void) {
    return err_led_pin;
}

#ifdef CONFIG_RETROSCALER_BLUERETRO_4LEDS_HW
void dev_led_init(void) {
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_ENABLE,
    };

    for (uint32_t i = 0; i < ARRAY_SIZE(dev_led_pins); i++) {
        io_conf.pin_bit_mask = 1ULL << dev_led_pins[i];
        gpio_config(&io_conf);
        gpio_set_level(dev_led_pins[i], 0);
    }
}

void dev_led_set(uint8_t num, uint8_t value) {
    if (num < ARRAY_SIZE(dev_led_pins)) {
        gpio_set_level(dev_led_pins[num], value);
    }
}
#endif /* CONFIG_RETROSCALER_BLUERETRO_4LEDS_HW */
