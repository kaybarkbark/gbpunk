/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// SWD for Pico:
// Blue to right, green to left. USB facing away from you
// SWD for cart
// Blue to green, green to purple

#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "tusb.h"
#include "gb.h"
#include "cart.h"
#include "mbc5.h"
#include "gbcam.h"
#include "utils.h"
#include "unit_tests.h"
#include "msc_disk.h"
#include "status_led.h"

#define DO_UNIT_TEST 1

int main() {
    init_led_irq();
    init_disk_mem();
    stdio_init_all();
    init_bus();
    uint16_t cart_check_result = 0;
    while(!cart_check_result){
        cart_check_result = cart_check(working_mem);
        sleep_ms(1000);
        set_led_speed(LED_SPEED_ERR);
    }
    set_led_speed(LED_SPEED_TESTING);
    populate_cart_info();
    dump_cart_info();
    if(DO_UNIT_TEST){
        unit_test_cart();
    }
    uint8_t buf[16] = {0};
    init_disk();
    tusb_init();
    set_led_speed(LED_SPEED_HEALTHY);
    while(1){
        tud_task();
    }
}
