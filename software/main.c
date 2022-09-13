/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "gbbus.pio.h"
#include "pins.h"

void data_bus_init(PIO pio, uint sm, uint offset, uint d0, uint clk);
irq_handler_t clk_irq_handler();
uint8_t data = 0;
uint sm = 0;
int main() {
    // Choose PIO instance (0 or 1)
    PIO pio = pio0;

    // Get first free state machine in PIO 0
    sm = pio_claim_unused_sm(pio, true);
    uint offset = pio_add_program(pio, &gbbus_program);
    // Set up the interrupt handler for the data bus
    irq_set_exclusive_handler(PIO0_IRQ_0, clk_irq_handler);
    // Init the PIO
    data_bus_init(pio, sm, offset, D0, CLK);
    pio0->txf[sm] = 0xa5;
    pio_sm_set_enabled(pio, sm, true);
}

irq_handler_t clk_irq_handler(){
    // Increment data by 1
    data++;
    // Write it back to the tx fifo
    pio0->txf[sm] = data;
    // Clear the interrupt so the PIO resumes
    irq_clear(PIO0_IRQ_0);
}

void data_bus_init(PIO pio, uint sm, uint offset, uint d0, uint clk) {
    // Set the function of all the data pins to use PIO
    pio_sm_config c = gbbus_program_get_default_config(offset);  // Generate a generic sm config

    // Allow PIO to control data bus (as output)
    for(uint i = 0; i < NUM_D_PINS; i++){
        pio_gpio_init(pio, d0 + i);
    }
    // Allow PIO to control clock (as input)
    pio_gpio_init(pio, clk);

    // Connect pin to SET pin (control with 'set' instruction)
    //sm_config_set_set_pins(&c, d0, 2);
    // Connect pin to OUT (control with 'out' instruction)
    sm_config_set_out_pins(&c, d0, NUM_D_PINS);
    // Turn on autopull so we refill the fifo after we have pulled 8 bits out of it
    sm_config_set_out_shift(&c, true /* shift right */, true /* enable autopull */, 8);

    // Set the pin direction to output (in PIO)
    pio_sm_set_consecutive_pindirs(pio, sm, d0, NUM_D_PINS, true);
    
    // Set the pin direction to input (in PIO)
    // CLK becomes input pin 0 (need to know that for PIO)
    pio_sm_set_consecutive_pindirs(pio, sm, clk, 1, false);

    // Enable the PIO0 IRQ
    irq_set_enabled(PIO0_IRQ_0, true);

    // Load configuration and jump to start of the program
    pio_sm_init(pio, sm, offset, &c);
}