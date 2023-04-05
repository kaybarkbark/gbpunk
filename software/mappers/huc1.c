#include "huc1.h"

void huc1_memcpy_rom(uint8_t* dest, uint32_t rom_addr, uint32_t num){
    // Determine the current bank
    uint16_t current_bank = fs_get_rom_bank(rom_addr);
    uint32_t rom_cursor = 0;
    // Keep track of where we are in ROM
    rom_cursor = rom_addr % ROM_BANK_SIZE;
    // Set up the bank for transfer
    huc1_set_rom_bank(current_bank);
    for(uint32_t buf_cursor = 0; buf_cursor < num; buf_cursor++){
        // Determine if we need to bankswitch or not
        if(rom_cursor >= ROM_BANK_SIZE){
            // Switch banks if we cross a boundary
            current_bank++;
            huc1_set_rom_bank(current_bank);
            // If we bankswitch, we start over again at the beginning of the the bank
            rom_cursor = 0;
        }
        // Read everything out of banked ROM, even bank 0. Easier that way
        dest[buf_cursor] = readb(rom_cursor + ROM_BANKN_START_ADDR); 
        rom_cursor++;
    }
}

void huc1_memcpy_ram(uint8_t* dest, uint32_t ram_addr, uint32_t num){
    // Enable RAM reads
    huc1_set_ram_access(1);
    // Determine current bank
    uint16_t current_bank = fs_get_ram_bank(ram_addr);
    uint32_t ram_cursor = 0;
    // Keep track of where we are in RAM
    ram_cursor = ram_addr % SRAM_BANK_SIZE;
    // Set up the bank for transfer
    huc1_set_ram_bank(current_bank);
    for(uint32_t buf_cursor = 0; buf_cursor < num; buf_cursor++){
        // Determine if we need to bankswitch or not
        if(ram_cursor >= SRAM_BANK_SIZE){
            // Switch banks if we cross a boundary
            current_bank++;
            huc1_set_ram_bank(current_bank);
            // If we bankswitch, we start over again at the beginning of the the bank
            ram_cursor = 0;
        }
        dest[buf_cursor] = readb(ram_cursor + SRAM_START_ADDR); 
        ram_cursor++;
    }
    // Disable RAM reads
    huc1_set_ram_access(0);
}

void huc1_memset_ram(uint8_t* buf, uint32_t ram_addr, uint32_t num){
    // Determine current bank
    uint16_t current_bank = fs_get_ram_bank(ram_addr);
    uint32_t ram_cursor = 0;
    // Keep track of where we are in RAM
    ram_cursor = ram_addr % SRAM_BANK_SIZE;
    // Enable RAM writes
    huc1_set_ram_access(1);
    // Set up the bank for transfer
    huc1_set_ram_bank(current_bank);
    for(uint32_t buf_cursor = 0; buf_cursor < num; buf_cursor++){
        // Determine if we need to bankswitch or not
        if(ram_cursor >= SRAM_BANK_SIZE){
            // Switch banks if we cross a boundary
            current_bank++;
            huc1_set_ram_bank(current_bank);
            // If we bankswitch, we start over again at the beginning of the the bank
            ram_cursor = 0;
        }
        writeb(buf[buf_cursor], ram_cursor + SRAM_START_ADDR);
        ram_cursor++;
    }
    // Disable RAM writes
    huc1_set_ram_access(0);
}

void huc1_set_rom_bank(uint16_t bank){
    writeb(bank, HUC1_ROM_BANK_ADDR);
}

void huc1_set_ram_bank(uint16_t bank){
    writeb(bank, HUC1_RAM_BANK_ADDR);
}

void huc1_set_ram_access(uint8_t on_off){
    // RAM is enabled by default, and IR is disabled. Still, good idea to set this here
    if(on_off){
        writeb(HUC1_IR_SRAM_SELECT_SRAM, HUC1_IR_SRAM_SELECT_ADDR);
    }
    else{
        writeb(HUC1_IR_SRAM_SELECT_IR, HUC1_IR_SRAM_SELECT_ADDR);
    }
}