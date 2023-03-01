#include "mbc3.h"
#include "gb.h"

void mbc3_set_rom_bank(uint16_t bank){
    writeb(bank, MBC3_ROM_BANK_ADDR);
}

void mbc3_set_ram_bank(uint16_t bank){
    writeb(bank, MBC3_RAM_BANK_ADDR);
}

void mbc3_set_ram_access(uint8_t on_off){
    if(on_off){
        writeb(MBC3_ENABLE_RAM_ACCESS_DATA, MBC3_ENABLE_RAM_ACCESS_ADDR);
    }
    else{
        writeb(0x0, MBC3_ENABLE_RAM_ACCESS_ADDR);
    }
}

void mbc3_memcpy_rom(uint8_t* dest, uint32_t rom_addr, uint32_t num){
    // Annoyingly, Bank 0 cannot be mapped with this mapper, so we need to 
    // special case this. Assume we are not reading from Bank 0
    uint16_t bank_offset = ROM_BANKN_START_ADDR;
    // Determine the current bank
    uint16_t current_bank = fs_get_rom_bank(rom_addr);
    
    // Keep track of where we are in ROM
    uint32_t rom_cursor = 0;
    rom_cursor = rom_addr % ROM_BANK_SIZE;

    // Handle if we are in bank 0
    if(current_bank == 0){
        bank_offset = ROM_BANK0_START_ADDR;
    }
    // Otherwise, set the bank
    else{
        mbc3_set_rom_bank(current_bank);
    }    
    for(uint16_t buf_cursor = 0; buf_cursor < num; buf_cursor++){
        // Determine if we need to bankswitch or not
        if(rom_cursor >= ROM_BANK_SIZE){
            // Switch banks if we cross a boundary
            current_bank++;
            mbc3_set_rom_bank(current_bank);
            // If we bankswitch, we start over again at the beginning of the the bank
            rom_cursor = 0;
            // Bankswitching will always imply that we have left Bank 0
            bank_offset = ROM_BANKN_START_ADDR;
        }
        // Read back data from the appropriate bank
        dest[buf_cursor] = readb(rom_cursor + bank_offset); 
        rom_cursor++;
    }
}

void mbc3_memcpy_ram(uint8_t* dest, uint32_t ram_addr, uint32_t num){
    // Enable RAM access
    mbc3_set_ram_access(1);
    // Keep track of where we are in RAM
    uint32_t ram_cursor = ram_addr;
    // Keep track of our current bank
    uint16_t current_bank = fs_get_ram_bank(ram_cursor);
    // Set up the bank for transfer
    mbc3_set_ram_bank(current_bank);
    for(uint16_t buf_cursor = 0; buf_cursor < num; buf_cursor++){
        // Determine if we need to bankswitch or not
        uint16_t new_bank = fs_get_rom_bank(ram_cursor);
        if(new_bank != current_bank){
            // Switch banks if we cross a boundary
            current_bank = new_bank;
            mbc3_set_ram_bank(current_bank);
            // If we bankswitch, we start over again at the beginning of the the bank
            ram_cursor = 0;
        }
        // Read everything out of banked ROM, even bank 0. Easier that way
        dest[buf_cursor] = readb(ram_cursor + SRAM_START_ADDR); 
        ram_cursor++;
    }
    // Disable RAM access
    mbc3_set_ram_access(0);
}