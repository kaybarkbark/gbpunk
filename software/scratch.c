#include "scratch.h"
#include "gb.h"
#include "mappers/mbc3.h"
#include <stdio.h>

#define DATA 0x66
#define RAM_ADDR (SRAM_START_ADDR + 0xF)
#define NUM 8

void scratch_workspace(){
    uint8_t write_buf[8] = {0, 1, 2, 3, 4, 5, 6, 7};
    uint8_t buf[8] = {0, 1, 2, 3, 4, 5, 6, 7};
    uint8_t read_buf[8] = {0};
    uint8_t initial0, readback0, initial1, readback1, readback3;
    // mbc3_set_ram_access(1);
    // mbc3_set_ram_bank(1);
    // initial0 = readb(SRAM_START_ADDR + 0xF);
    // writeb(DATA, SRAM_START_ADDR + 0xF);
    // readback0 = readb(SRAM_START_ADDR + 0xF);
    // mbc3_set_ram_bank(2);
    // initial1 = readb(SRAM_START_ADDR + 0xF);
    // writeb(DATA, SRAM_START_ADDR + 0xF);
    // readback1 = readb(SRAM_START_ADDR + 0xF);
    // mbc3_set_ram_access(0);
    // mbc3_memset_ram(write_buf, SRAM_START_ADDR + 0xF, 8);
    // mbc3_memcpy_ram(read_buf, SRAM_START_ADDR + 0xF, 8);
    // mbc3_set_ram_access(1);
    // readback3 = readb(SRAM_START_ADDR + 0xF);
    // mbc3_set_ram_access(0);
    // Keep track of where we are in RAM
    uint32_t ram_cursor = RAM_ADDR;
    // Keep track of our current bank
    uint16_t current_bank = fs_get_ram_bank(ram_cursor);
    // Set up the bank for transfer
    //mbc3_set_ram_bank(current_bank);
    for(uint32_t buf_cursor = 0; buf_cursor < NUM; buf_cursor++){
        // Determine if we need to bankswitch or not
        uint16_t new_bank = fs_get_rom_bank(ram_cursor);
        if(new_bank != current_bank){
            // Switch banks if we cross a boundary
            current_bank = new_bank;
            //mbc3_set_ram_bank(current_bank);
            // If we bankswitch, we start over again at the beginning of the the bank
            ram_cursor = 0;
        }
        // Write whatever is in memory to the current spot in RAM
        // Enable RAM access
        mbc3_set_ram_access(1);
        volatile uint8_t write_to = buf[buf_cursor];
        writeb(write_to, ram_cursor);
        uint8_t read_back = readb(ram_cursor);
        printf("read back 0x%x", read_back);
        // Disable RAM access
        mbc3_set_ram_access(0);
        ram_cursor++;
    }

    printf("0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, ", initial0, readback0, initial1, readback1, readback3, read_buf[0], write_buf[0]);
}