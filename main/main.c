/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/rtc.h"
#include "pico/util/datetime.h"

int ECHO_PIN = 15;
int TRIG_PIN = 17;
volatile uint64_t time_end = 0;
volatile uint64_t time_start = 0;
volatile bool time_alarm = false;

void rpc_callback(uint gpio, uint32_t events) {
    if (events == 0x4) { // fall
        time_end = to_us_since_boot(get_absolute_time());
    } else if (events == 0x8) { // rise
        time_start = to_us_since_boot(get_absolute_time());
    }
}

int64_t alarm_callback(alarm_id_t id, void *user_data) {
    time_alarm = true;
    return 0;
}

void trigger_sinal() {
    gpio_put(TRIG_PIN, 1);
    sleep_us(10);
    gpio_put(TRIG_PIN, 0);
}

int main() {
    stdio_init_all();

    gpio_init(ECHO_PIN);
    gpio_set_dir(ECHO_PIN, GPIO_IN);
    gpio_set_irq_enabled_with_callback(ECHO_PIN,
                                    GPIO_IRQ_EDGE_RISE |
                                        GPIO_IRQ_EDGE_FALL,
                                    true,
                                    &rpc_callback);

    gpio_init(TRIG_PIN);
    gpio_set_dir(TRIG_PIN, GPIO_OUT);


    datetime_t time = {
        .year = 2024,
        .month = 3,
        .day = 19,
        .dotw = 2,
        .hour = 10,
        .min = 0,
        .sec = 0
    };

    rtc_init();
    rtc_set_datetime(&time);

    while (true) {
        time_end = 0;
        trigger_sinal();
        alarm_id_t alarm = add_alarm_in_ms(500, alarm_callback, NULL, false);

        while (time_end == 0) {
            if (time_alarm) {
                printf("Erro: Timeout! Nenhum sinal de eco recebido.\n");
                break;
            }
        }

        // Se o tempo de eco foi recebido corretamente
        if (time_end > time_start) {
            uint64_t dif = time_end - time_start;
            float distance = (dif * 0.0343f) / 2.0f;
            if (distance <= 400 && distance >= 2){
                printf("Distancia: %.2f cm\n", distance);
            } else{
                printf("Out of range! Distancia muito longa ou muito curta.\n");
            }
            sleep_ms(400);
        }

        // Cancela o alarme se o eco foi recebido antes do tempo limite
        cancel_alarm(alarm);
    }
}