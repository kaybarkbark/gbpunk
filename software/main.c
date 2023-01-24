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

void hexdump(uint8_t *data, uint16_t len);

uint8_t gamebuf[0x8000] = {};

int main() {
    stdio_init_all();
    while(1){
        printf("Initializing the bus...\n");
        init_bus();
        // printf("Resetting the cart...\n");
        // reset_cart();
        sleep_ms(500);
        printf("Performing cart check...\n");
        uint16_t test = 0x104;
        printf("DEBUG location 0x%x: 0x%x\n", test, readb(test));
        printf("DEBUG location 0x%x: 0x%x\n", test, readb(test));
        printf("DEBUG location 0x%x: 0x%x\n", test, readb(test));

        test = 0x105;
        printf("DEBUG location 0x%x: 0x%x\n", test, readb(test));
        printf("DEBUG location 0x%x: 0x%x\n", test, readb(test));
        printf("DEBUG location 0x%x: 0x%x\n", test, readb(test));
        uint8_t logo_buf[LOGO_LEN] = {0};
        uint16_t cart_check_result = cart_check(logo_buf);
        printf("Contents of logo...\n");
        hexdump(logo_buf, LOGO_LEN);
        if(cart_check_result){
            printf("Cart check failed at address 0x%x! Take it out and blow on it.\n", cart_check_result);
        }
        else{
            printf("Cart check passed!\n");
            struct Cart cart;
            get_cart_info(&cart);
            dump_cart_info(&cart);
        }
        sleep_ms(2000);
    }

}

void hexdump(uint8_t *data, uint16_t len){
    for(uint16_t i = 0; i < len; i++){
        if(!(i%8)){
            printf("\n");
        }
        printf("0x%02x ", data[i]);
    }
    printf("\n");
}
