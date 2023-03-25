#include "mbc1.h"
#include "gb.h"

// Best docs on this mapper https://gbdev.gg8.se/wiki/articles/MBC1

// Public functions
void mbc1_memcpy_rom(uint8_t* dest, uint32_t rom_addr, uint32_t num){
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
        mbc1_set_rom_bank(current_bank);
    }    
    for(uint32_t buf_cursor = 0; buf_cursor < num; buf_cursor++){
        // Determine if we need to bankswitch or not
        if(rom_cursor >= ROM_BANK_SIZE){
            // Switch banks if we cross a boundary
            current_bank++;
            mbc1_set_rom_bank(current_bank);
            // If we bankswitch, we start over again at the beginning of the the bank
            rom_cursor = 0;
            // Bankswitching will always imply that we have left Bank 0
            bank_offset = ROM_BANKN_START_ADDR;
        }
        if(mbc1_check_invalid_bank(current_bank)){
            // This bank does not exist.
            // Looks like modern emulators just put 0's here
            // TODO: Make it back to the redirected one instead
            dest[buf_cursor] = 0x0; 
        }
        else{
            // Read back data from the appropriate bank
            dest[buf_cursor] = readb(rom_cursor + bank_offset); 
        }
        rom_cursor++;
    }
}

void mbc1_memcpy_ram(uint8_t* dest, uint32_t ram_addr, uint32_t num){
    // Enable RAM reads
    mbc1_set_ram_access(1);
    // Determine current bank
    uint16_t current_bank = fs_get_ram_bank(ram_addr);
    uint32_t ram_cursor = 0;
    // Keep track of where we are in RAM
    ram_cursor = ram_addr % SRAM_BANK_SIZE;
    // Set up the bank for transfer
    mbc1_set_ram_bank(current_bank);
    for(uint32_t buf_cursor = 0; buf_cursor < num; buf_cursor++){
        // Determine if we need to bankswitch or not
        if(ram_cursor >= SRAM_BANK_SIZE){
            // Switch banks if we cross a boundary
            current_bank++;
            mbc1_set_ram_bank(current_bank);
            // If we bankswitch, we start over again at the beginning of the the bank
            ram_cursor = 0;
        }
        dest[buf_cursor] = readb(ram_cursor + SRAM_START_ADDR); 
        ram_cursor++;
    }
    // Disable RAM reads
    mbc1_set_ram_access(0);
}

void mbc1_memset_ram(uint8_t* buf, uint32_t ram_addr, uint32_t num){
    // Determine current bank
    uint16_t current_bank = fs_get_ram_bank(ram_addr);
    uint32_t ram_cursor = 0;
    // Keep track of where we are in RAM
    ram_cursor = ram_addr % SRAM_BANK_SIZE;
    // Enable RAM writes
    mbc1_set_ram_access(1);
    // Set up the bank for transfer. This has the side effect of also selecting RAM mode for writing
    mbc1_set_ram_bank(current_bank);
    for(uint32_t buf_cursor = 0; buf_cursor < num; buf_cursor++){
        // Determine if we need to bankswitch or not
        if(ram_cursor >= SRAM_BANK_SIZE){
            // Switch banks if we cross a boundary
            current_bank++;
            mbc1_set_ram_bank(current_bank);
            // If we bankswitch, we start over again at the beginning of the the bank
            ram_cursor = 0;
        }
        writeb(buf[buf_cursor], ram_cursor + SRAM_START_ADDR);
        ram_cursor++;
    }
    // Disable RAM writes
    mbc1_set_ram_access(0);
}


// Private
void mbc1_set_rom_bank(uint16_t bank){
    // You cannot select bank 0x0, 0x20, 0x40, 0x60 with this
    // Those default to bank  0x0, 0x21, 0x41, 0x61
    // Bank 0x0 must be accessed via addresses 0x0 - 0x3FFF. 
    // Banks 0x20, 0x40, 0x60 do not exist
    
    // Set the ROM/RAM mode select bit for ROM access
    writeb(0x0, MBC1_ROM_RAM_SELECT);
    // Set the lower 5 bits of the ROM bank
    writeb(bank & 0x1F, MBC1_ROM_BANK_LOWER_BITS);
    // Set the upper 2 bits of the ROM bank
    writeb((bank & 0x60) >> 5, MBC1_RAM_ROM_SHARED_BANK);
}

void mbc1_set_ram_bank(uint16_t bank){
    // Set the ROM/RAM mode select bit for RAM access
    writeb(0x1, MBC1_ROM_RAM_SELECT);
    // Set the two bits for the RAM bank
    writeb(bank & 0x3, MBC1_RAM_ROM_SHARED_BANK);
}

void mbc1_set_ram_access(uint8_t on_off){
    // This is required for reads and writes
    if(on_off){
        writeb(MBC1_ENABLE_RAM_ACCESS_DATA, MBC1_ENABLE_RAM_ACCESS_ADDR);
    }
    else{
        writeb(0x0, MBC1_ENABLE_RAM_ACCESS_ADDR);
    }
}

uint8_t mbc1_check_invalid_bank(uint16_t bank){
    // In MBC1, some banks just don't exist due to how the mapper works
    // Why did they make it this way. I swear they could have done better
    // Did they design this with graph paper and pencils? (maybe)
    return (bank == 0x20) || (bank == 0x40) || (bank == 0x60);
}