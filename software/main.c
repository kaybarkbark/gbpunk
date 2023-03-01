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
#include "bsp/board.h"
#include "tusb.h"
#include "pins.h"
#include "gb.h"
#include "cart.h"
#include "mbc5.h"
#include "gbcam.h"
#include "utils.h"
#include "unit_tests.h"
#include "msc_disk.h"

#define DO_UNIT_TEST 1

int main() {
    init_disk_mem();
    stdio_init_all();
    gpio_init(STATUS_LED);
    gpio_set_dir(STATUS_LED, GPIO_OUT);
    gpio_put(STATUS_LED, true);
    init_bus();
    uint16_t cart_check_result = 0xFF;
    while(cart_check_result){
        cart_check_result = cart_check(working_mem);
        sleep_ms(1000);
    }
    gpio_put(STATUS_LED, false);
    append_status_file("CART CHK: PASS\n\0");
    populate_cart_info();
    dump_cart_info();
    if(DO_UNIT_TEST){
        if(the_cart.mapper_type == MAPPER_MBC5){
            mbc5_unit_test();
        }
        if(the_cart.mapper_type == MAPPER_GBCAM){
            gbcam_unit_test();
        }
    }
    init_disk();
    tusb_init();
    while(1){
        tud_task();
    }
}
