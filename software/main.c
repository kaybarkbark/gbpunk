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
#include "pins.h"
#include "gb.h"
#include "mbc5.h"
#include "gbcam.h"
#include "utils.h"

#define DO_UNIT_TEST 1

uint8_t gamebuf[0x8000] = {};

int main() {
    stdio_init_all();
    while(1){
        sleep_ms(1000);
        printf("Initializing the bus...\n");
        init_bus();
        // printf("Resetting the cart...\n");
        // reset_cart();
        sleep_ms(500);
        printf("Performing cart check...\n");
        uint8_t logo_buf[LOGO_LEN] = {0};
        uint16_t cart_check_result = cart_check(logo_buf);
        printf("Contents of logo...\n");
        hexdump(logo_buf, LOGO_LEN, 0x0);
        if(cart_check_result){
            printf("Cart check failed at address 0x%x! Take it out and blow on it.\n", cart_check_result);
        }
        else{
            printf("Cart check passed!\n");
            struct Cart cart;
            get_cart_info(&cart);
            dump_cart_info(&cart);
            sleep_ms(1000);
            if(DO_UNIT_TEST){
                printf("Performing cart unit tests...\n");
                if(cart.mapper_type == MAPPER_MBC5){
                    mbc5_unit_test(&cart);
                }
                if(cart.mapper_type == MAPPER_GBCAM){
                    gbcam_unit_test();
                }
            }
        }
        sleep_ms(1000);
    }

}
