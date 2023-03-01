#include "mbc2.h"
#include "gb.h"

// Best docs on this mapper https://gbdev.gg8.se/wiki/articles/Memory_Bank_Controllers#MBC2_.28max_256KByte_ROM_and_512x4_bits_RAM.29

// Public functions
void mbc2_memcpy_rom(uint8_t* dest, uint32_t rom_addr, uint32_t num){
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
        mbc2_set_rom_bank(current_bank);
    }    
    for(uint16_t buf_cursor = 0; buf_cursor < num; buf_cursor++){
        // Determine if we need to bankswitch or not
        if(rom_cursor >= ROM_BANK_SIZE){
            // Switch banks if we cross a boundary
            current_bank++;
            mbc2_set_rom_bank(current_bank);
            // If we bankswitch, we start over again at the beginning of the the bank
            rom_cursor = 0;
            // Bankswitching will always imply that we have left Bank 0
            bank_offset = ROM_BANKN_START_ADDR;
        }
        // Read everything out of banked ROM, even bank 0. Easier that way
        dest[buf_cursor] = readb(rom_cursor + bank_offset); 
        rom_cursor++;
    }
}

void mbc2_memcpy_ram(uint8_t* dest, uint32_t ram_addr, uint32_t num){
    // Enable RAM access
    mbc2_set_ram_access(1);
    // There is only one memory bank in MBC2
    // Keep track of where we are in RAM
    for(uint16_t buf_cursor = 0; buf_cursor < num; buf_cursor++){
        dest[buf_cursor] = readb(ram_addr + SRAM_START_ADDR); 
        ram_addr++;
    }
    mbc2_set_ram_access(0);
}


// Private
void mbc2_set_rom_bank(uint16_t bank){
    // Only 16 banks (4 bits) allowed here
    writeb(bank & 0xF, MBC2_ROM_BANK_ADDR);
}

void mbc2_set_ram_access(uint8_t on_off){
    // This is required for reads and writes
    if(on_off){
        writeb(MBC2_ENABLE_RAM_ACCESS_DATA, MBC2_ENABLE_RAM_ACCESS_ADDR);
    }
    else{
        writeb(MBC2_DISABLE_RAM_ACCESS_DATA, MBC2_ENABLE_RAM_ACCESS_ADDR);
    }
}
