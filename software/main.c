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
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "gbbus.pio.h"
#include "pins.h"
#include "tetris.h"

#define ADDR_LIMIT 0x100

void data_bus_init(uint sm, uint offset);
irq_handler_t clk_irq_handler();
uint16_t addr_trace[ADDR_LIMIT] = {};
uint16_t addr_index = 0;
uint8_t data = 0x1;
uint sm = 0;

// Choose PIO instance (0 or 1)
PIO pio = pio0;

int main() {
    // Get first free state machine in PIO 0
    sm = pio_claim_unused_sm(pio, true);
    uint offset = pio_add_program(pio, &gbbus_program);
    // Init the PIO
    data_bus_init(sm, offset);
    pio_sm_set_enabled(pio, sm, true);
    while(true){

    }
}

void data_bus_init(uint sm, uint offset) {
    // Set the function of all the data pins to use PIO
    pio_sm_config c = gbbus_program_get_default_config(offset);  // Generate a generic sm config

    // Allow PIO to control data bus (as output)
    for(uint i = 0; i < NUM_D_PINS; i++){
        pio_gpio_init(pio, D0 + i);
    }
    // Allow PIO to control address bus (as input)
    for(uint i = 0; i < NUM_A_PINS; i++){
        pio_gpio_init(pio, A0 + i);
    }
    // Allow PIO to control clock (as input)
    pio_gpio_init(pio, CLK);
    // Allow PIO to control read (as input)
    pio_gpio_init(pio, RD);

    // Connect pin to OUT (control with 'out' instruction)
    // TODO: might want to make these sticky
    sm_config_set_out_pins(&c, D0, NUM_D_PINS);
    // Connect pin to IN (control with 'in' instruction)
    sm_config_set_in_pins(&c, A0);
    // Tell it to shift left, don't autopush, push threshold 16 bits
    sm_config_set_in_shift(&c, false, false, 16);
    // Set up RD as JMP pin (JMP based on their status)
    sm_config_set_jmp_pin(&c, RD);

    // Turn on autopull so we refill the fifo after we have pulled 8 bits out of it
    // sm_config_set_out_shift(&c, true /* shift right */, true /* enable autopull */, 8);

    // Set the pin direction to output (in PIO)
    pio_sm_set_consecutive_pindirs(pio, sm, D0, NUM_D_PINS, true);
    
    // Set the pin direction to input (in PIO)
    pio_sm_set_consecutive_pindirs(pio, sm, CLK, 1, false);
    pio_sm_set_consecutive_pindirs(pio, sm, RD, 1, false);
    pio_sm_set_consecutive_pindirs(pio, sm, A0, NUM_A_PINS, false);

    // Enable the PIO0 IRQ
    pio_set_irq0_source_enabled(pio, pis_interrupt0 , true); //setting IRQ0_INTE - interrupt enable register
    // Set up the interrupt handler for the data bus
    irq_set_exclusive_handler(PIO0_IRQ_0, clk_irq_handler);
    // Enable the irq
    irq_set_enabled(PIO0_IRQ_0, true); 
    // Load configuration and jump to start of the program
    pio_sm_init(pio, sm, offset, &c);
}

irq_handler_t clk_irq_handler(){
    // Read out the contents of the ROM at that location
    // Write it back to data bus
    uint32_t addr = pio0->rxf[sm];
    if ((addr & 0xFFFF) < 0x7FFF){
        pio0->txf[sm] = tetris_rom[addr];
        if((addr_index < ADDR_LIMIT) && (addr != 0)){
            addr_trace[addr_index] = addr;
            addr_index++;
        }
        if(addr_index >= ADDR_LIMIT){
            addr_trace[addr_index] = addr;
        }
    }
    // Clear the interrupt so the PIO resumes
    pio_interrupt_clear(pio, 0);

}

