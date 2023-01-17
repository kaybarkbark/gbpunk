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

#include "pico/stdlib.h"
#include "pins.h"

void writeb(uint8_t data, uint16_t addr);
uint8_t readb(uint16_t addr);
void set_dbus_direction(uint8_t dir);
void init_bus();
void reset_pin_states();
void reset_cart();
void pulse_clock();

uint8_t gamebuf[0x8000] = {};

int main() {
    stdio_init_all();
    init_bus();
    reset_cart();
    while(1){
        // writeb(0xAA, 0x55);
        // Dump out bottom 24K
        for(uint16_t i = 0; i < 0x8000; i++){
            gamebuf[i] = readb(i);
        }
        for(uint16_t i = 0; i < 0x8000; i++){
            printf("0x%x: 0x%x (%c)\n", i, gamebuf[i] ,gamebuf[i]);
        }
        printf("Done\n");
    }   
}

void reset_cart(){
    reset_pin_states();
    gpio_put(RST, 0);
    pulse_clock();
    gpio_put(RST, 1);
    pulse_clock();
}

void pulse_clock(){
    gpio_put(CLK, 0);
    sleep_us(1);
    gpio_put(CLK, 1);
    sleep_us(1);
    gpio_put(CLK, 0);
}


void init_bus(){
    // Init the address pins, always out
    gpio_init(A0);
    gpio_set_dir(A0, GPIO_OUT);

    gpio_init(A1);
    gpio_set_dir(A1, GPIO_OUT);

    gpio_init(A2);
    gpio_set_dir(A2, GPIO_OUT);

    gpio_init(A3);
    gpio_set_dir(A3, GPIO_OUT);

    gpio_init(A4);
    gpio_set_dir(A4, GPIO_OUT);

    gpio_init(A5);
    gpio_set_dir(A5, GPIO_OUT);

    gpio_init(A6);
    gpio_set_dir(A6, GPIO_OUT);

    gpio_init(A7);
    gpio_set_dir(A7, GPIO_OUT);

    gpio_init(A8);
    gpio_set_dir(A8, GPIO_OUT);
    
    gpio_init(A9);
    gpio_set_dir(A9, GPIO_OUT);

    gpio_init(A10);
    gpio_set_dir(A10, GPIO_OUT);

    gpio_init(A11);
    gpio_set_dir(A11, GPIO_OUT);

    gpio_init(A12);
    gpio_set_dir(A12, GPIO_OUT);

    gpio_init(A13);
    gpio_set_dir(A13, GPIO_OUT);

    gpio_init(A14);
    gpio_set_dir(A14, GPIO_OUT);

    gpio_init(A15);
    gpio_set_dir(A15, GPIO_OUT);

    // Init the control pins, always out
    gpio_init(RST);
    gpio_set_dir(RST, GPIO_OUT);

    gpio_init(CS);
    gpio_set_dir(CS, GPIO_OUT);

    gpio_init(RD);
    gpio_set_dir(RD, GPIO_OUT);

    gpio_init(WR);
    gpio_set_dir(WR, GPIO_OUT);

    gpio_init(CLK);
    gpio_set_dir(CLK, GPIO_OUT);

    // Init the data pins, bidirectional
    // Init them as IN though
    gpio_init(D0);
    gpio_init(D1);
    gpio_init(D2);
    gpio_init(D3);
    gpio_init(D4);
    gpio_init(D5);
    gpio_init(D6);
    gpio_init(D7);
    set_dbus_direction(GPIO_IN);

    // Init the state of all the pins
    reset_pin_states();
}

void reset_pin_states(){
    // Address pins
    gpio_put(A0, 0);
    gpio_put(A1, 0);
    gpio_put(A2, 0);
    gpio_put(A3, 0);
    gpio_put(A4, 0);
    gpio_put(A5, 0);
    gpio_put(A6, 0);
    gpio_put(A7, 0);
    gpio_put(A8, 0);
    gpio_put(A9, 0);
    gpio_put(A10, 0);
    gpio_put(A11, 0);
    gpio_put(A12, 0);
    gpio_put(A13, 0);
    gpio_put(A14, 0);
    gpio_put(A15, 0);

    set_dbus_direction(GPIO_IN);

    // Control pins
    gpio_put(RST, 1);
    gpio_put(CS, 1);
    gpio_put(RD, 1);
    gpio_put(WR, 1);
    gpio_put(CLK, 1);
}

void writeb(uint8_t data, uint16_t addr){
    // Set the clock high
    gpio_put(CLK, 1);
    // Sleep 140 ns
    sleep_us(1);
    // Set read high
    gpio_put(RD, 1);
    // Set the address
    gpio_put(A0, addr & (0x1 << 0));
    gpio_put(A1, addr & (0x1 << 1));
    gpio_put(A2, addr & (0x1 << 2));
    gpio_put(A3, addr & (0x1 << 3));
    gpio_put(A4, addr & (0x1 << 4));
    gpio_put(A5, addr & (0x1 << 5));
    gpio_put(A6, addr & (0x1 << 6));
    gpio_put(A7, addr & (0x1 << 7));
    gpio_put(A8, addr & (0x1 << 8));
    gpio_put(A9, addr & (0x1 << 9));
    gpio_put(A10, addr & (0x1 << 10));
    gpio_put(A11, addr & (0x1 << 11));
    gpio_put(A12, addr & (0x1 << 12));
    gpio_put(A13, addr & (0x1 << 13));
    gpio_put(A14, addr & (0x1 << 14));
    gpio_put(A15, addr & (0x1 << 15));
    // Sleep 240 ns
    sleep_us(1);
    // Set CS low
    gpio_put(CS, 0);
    // Sleep for 480 ns
    sleep_us(1);
    // Set WR low
    gpio_put(WR, 0);
    // Drive the data bus
    set_dbus_direction(GPIO_OUT);
    gpio_put(D0, data & (0x1 << 0));
    gpio_put(D1, data & (0x1 << 1));
    gpio_put(D2, data & (0x1 << 2));
    gpio_put(D3, data & (0x1 << 3));
    gpio_put(D4, data & (0x1 << 4));
    gpio_put(D5, data & (0x1 << 5));
    gpio_put(D6, data & (0x1 << 6));
    gpio_put(D7, data & (0x1 << 7));
    // Set clock low
    gpio_put(CLK, 0);
    // Wait 840 ns
    sleep_us(1);
    // Reset all control signals
    gpio_put(CLK, 1);
    sleep_us(1);
    reset_pin_states();
}
uint8_t readb(uint16_t addr){
    // Set direction accordingly
    set_dbus_direction(GPIO_IN);
    // Set the clock high
    gpio_put(CLK, 1);
    // Sleep ??? ns
    //sleep_us(1);
    // Set read low
    gpio_put(RD, 0);
    // Ensure write is high
    gpio_put(WR, 1);
    // Sleep ??? ns
    //sleep_us(1);
    // Put address on bus
    gpio_put(A0, addr & (0x1 << 0));
    gpio_put(A1, addr & (0x1 << 1));
    gpio_put(A2, addr & (0x1 << 2));
    gpio_put(A3, addr & (0x1 << 3));
    gpio_put(A4, addr & (0x1 << 4));
    gpio_put(A5, addr & (0x1 << 5));
    gpio_put(A6, addr & (0x1 << 6));
    gpio_put(A7, addr & (0x1 << 7));
    gpio_put(A8, addr & (0x1 << 8));
    gpio_put(A9, addr & (0x1 << 9));
    gpio_put(A10, addr & (0x1 << 10));
    gpio_put(A11, addr & (0x1 << 11));
    gpio_put(A12, addr & (0x1 << 12));
    gpio_put(A13, addr & (0x1 << 13));
    gpio_put(A14, addr & (0x1 << 14));
    gpio_put(A15, addr & (0x1 << 15));
    // Sleep ??? ms
    //sleep_us(1);
    // Set CS low
    gpio_put(CS, 0);
    // Read back the data
    sleep_us(1);
    uint8_t data =
        (gpio_get(D0) << 0) |
        (gpio_get(D1) << 1) |
        (gpio_get(D2) << 2) |
        (gpio_get(D3) << 3) |
        (gpio_get(D4) << 4) |
        (gpio_get(D5) << 5) |
        (gpio_get(D6) << 6) |
        (gpio_get(D7) << 7);
    // Set CLK low
    gpio_put(CLK, 0);
    // Sleep ??? ms
    sleep_us(1);
    gpio_put(CS, 1);
    return data;
}

void set_dbus_direction(uint8_t dir){
    gpio_set_dir(D0, dir);
    gpio_set_dir(D1, dir);
    gpio_set_dir(D2, dir);
    gpio_set_dir(D3, dir);
    gpio_set_dir(D4, dir);
    gpio_set_dir(D5, dir);
    gpio_set_dir(D6, dir);
    gpio_set_dir(D7, dir);
}

