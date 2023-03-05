#include "mbc5.h"
#include "gb.h"
#include "utils.h"
#include "stdio.h"
#include "stdlib.h"
#include "msc_disk.h"

void mbc5_set_rom_bank(uint16_t bank){
    // Set the full 16 bits of the SRAM bank
    writeb(bank & 0xFF, MBC5_LOW_ROM_BANK_ADDR);
    writeb(bank & 0xFF00 >> 8, MBC5_HIGH_ROM_BANK_ADDR);
}

void mbc5_set_ram_bank(uint16_t bank){
    // Remember to enable RAM access first!
    writeb(bank, MBC5_RAM_BANK_ADDR);
}

void mbc5_set_ram_access(uint8_t on_off){
    if(on_off){
        writeb(MBC5_ENABLE_RAM_ACCESS_DATA, MBC5_ENABLE_RAM_ACCESS_ADDR);
        return;
    }
    writeb(0x0, MBC5_ENABLE_RAM_ACCESS_ADDR);

}

void mbc5_memcpy_rom(uint8_t* dest, uint32_t rom_addr, uint32_t num){
    // Determine the current bank
    uint16_t current_bank = fs_get_rom_bank(rom_addr);
    uint32_t rom_cursor = 0;
    // Keep track of where we are in ROM
    rom_cursor = rom_addr % ROM_BANK_SIZE;
    // Set up the bank for transfer
    mbc5_set_rom_bank(current_bank);
    for(uint32_t buf_cursor = 0; buf_cursor < num; buf_cursor++){
        // Determine if we need to bankswitch or not
        if(rom_cursor >= ROM_BANK_SIZE){
            // Switch banks if we cross a boundary
            current_bank++;
            mbc5_set_rom_bank(current_bank);
            // If we bankswitch, we start over again at the beginning of the the bank
            rom_cursor = 0;
        }
        // Read everything out of banked ROM, even bank 0. Easier that way
        dest[buf_cursor] = readb(rom_cursor + ROM_BANKN_START_ADDR); 
        rom_cursor++;
    }
}
// Note: This gets memory relative to RAM, not the cart. So 0x0 means start of RAM
void mbc5_memcpy_ram(uint8_t* dest, uint32_t ram_addr, uint32_t num){
    // Enable RAM reads
    mbc5_set_ram_access(1);
    // Determine current bank
    uint16_t current_bank = fs_get_ram_bank(ram_addr);
    uint32_t ram_cursor = 0;
    // Keep track of where we are in RAM
    ram_cursor = ram_addr % SRAM_BANK_SIZE;
    // Set up the bank for transfer
    mbc5_set_ram_bank(current_bank);
    for(uint32_t buf_cursor = 0; buf_cursor < num; buf_cursor++){
        // Determine if we need to bankswitch or not
        if(ram_cursor >= SRAM_BANK_SIZE){
            // Switch banks if we cross a boundary
            current_bank++;
            mbc5_set_ram_bank(current_bank);
            // If we bankswitch, we start over again at the beginning of the the bank
            ram_cursor = 0;
        }
        dest[buf_cursor] = readb(ram_cursor + SRAM_START_ADDR); 
        ram_cursor++;
    }
    // Disable RAM reads
    mbc5_set_ram_access(0);
}

void mbc5_memset_ram(uint8_t* buf, uint32_t ram_addr, uint32_t num){
    // Determine current bank
    uint16_t current_bank = fs_get_ram_bank(ram_addr);
    uint32_t ram_cursor = 0;
    // Keep track of where we are in RAM
    ram_cursor = ram_addr % SRAM_BANK_SIZE;
    // Enable RAM writes
    mbc5_set_ram_access(1);
    // Set up the bank for transfer
    mbc5_set_ram_bank(current_bank);
    for(uint32_t buf_cursor = 0; buf_cursor < num; buf_cursor++){
        // Determine if we need to bankswitch or not
        if(ram_cursor >= SRAM_BANK_SIZE){
            // Switch banks if we cross a boundary
            current_bank++;
            mbc5_set_ram_bank(current_bank);
            // If we bankswitch, we start over again at the beginning of the the bank
            ram_cursor = 0;
        }
        writeb(buf[buf_cursor], ram_cursor + SRAM_START_ADDR);
        ram_cursor++;
    }
    // Disable RAM writes
    mbc5_set_ram_access(0);
}


/* DEPRECATED, OLD STUFF */
void mbc5_rom_dump(uint8_t *buf, uint16_t start_bank, uint16_t end_bank){
    // Iterate over the range of banks we want to dump
    for(uint16_t bank_offset = start_bank; bank_offset <= end_bank; bank_offset++){
        // Set the active bank in the cart
        mbc5_set_rom_bank(bank_offset);
        // Iterate over the full address space of a ROM bank
        for(uint16_t byte_offset = 0; byte_offset < ROM_BANK_SIZE; byte_offset++){
            // Read back banked ROM sequentially into our buffer
            // Remember, you can set banked rom to bank 0, so we don't even touch bank 0 address space
            // |-----------Calculate bank offset-----------|-Add byte offset-|      |---Read back from banked ROM-----|
            uint8_t d = readb(byte_offset + ROM_BANKN_START_ADDR);
            // printf("0x%x: 0x%x\n", ((bank_offset - start_bank) * ROM_BANK_SIZE) + byte_offset, d);
            buf[((bank_offset - start_bank) * ROM_BANK_SIZE) + byte_offset] = readb(byte_offset + ROM_BANKN_START_ADDR);
        }
    }
}

void simple_memset_ram(uint8_t* the_buf, uint32_t ram_addr, uint32_t num){
    mbc5_set_ram_access(1);
    mbc5_set_ram_bank(0);
    for(uint32_t i = 0; i < 16; i++){
        writeb(the_buf[i], SRAM_START_ADDR + i);
    }
    mbc5_set_ram_access(0); 
}

void mbc5_ram_test(uint8_t* buf, uint32_t size){
    uint32_t start_num = 0xa;
    uint8_t test_buf[16] = {0};
    mbc5_set_ram_access(1);
    mbc5_set_ram_bank(0);
    for(uint8_t i = 0; i < 16; i++){
        test_buf[i] = i + start_num + i;
    }
    simple_memset_ram(test_buf, 0, 16);
    // for(uint32_t i = 0; i < 16; i++){
    //     writeb(test_buf[i], SRAM_START_ADDR + i);
    // }
    // mbc5_memset_ram(test_buf, 0, 16);
    // for(uint8_t i = 0; i < 16; i++){
    //     buf[i] = readb(SRAM_START_ADDR + i);
    // }
    mbc5_memcpy_ram(buf, 0, 16);
    printf("Test");
    mbc5_set_ram_access(0);
    // // Enable SRAM access
    // mbc5_set_ram_access(1);
    // mbc5_set_ram_bank(0);
    // uint8_t rand_offset = rand();
    // // Iterate over the full address space of a RAM bank
    // for(uint16_t byte_offset = 0; byte_offset < size; byte_offset++){
    //     // Read back banked RAM sequentially into our buffer
    //     // |-----------Calculate bank offset------------|-Add byte offset-|      |---Read back from banked ROM-----|
    //     writeb((byte_offset + rand_offset) & 0xFF, byte_offset + SRAM_START_ADDR);
    //     buf[byte_offset] = readb(byte_offset + SRAM_START_ADDR);
    // }
    // // Disable SRAM access
    // mbc5_set_ram_access(0);
}

void mbc5_ram_dump(uint8_t *buf, uint8_t start_bank, uint8_t end_bank){
    // Enable SRAM access
    mbc5_set_ram_access(1);
    // Iterate over the range of banks we want to dump
    for(uint16_t bank_offset = start_bank; bank_offset <= end_bank; bank_offset++){
        // Set the active bank in the cart
        mbc5_set_ram_bank(bank_offset);
        // Iterate over the full address space of a RAM bank
        for(uint16_t byte_offset = 0; byte_offset < SRAM_BANK_SIZE; byte_offset++){
            // Read back banked RAM sequentially into our buffer
            // |-----------Calculate bank offset------------|-Add byte offset-|      |---Read back from banked ROM-----|
            buf[((bank_offset - start_bank) * SRAM_BANK_SIZE) + byte_offset] = readb(byte_offset + SRAM_START_ADDR);
        }
    }
    // Disable SRAM access
    mbc5_set_ram_access(0);
}

void mbc5_halfbank_ram_dump(uint8_t *buf){
    // Enable SRAM access
    mbc5_set_ram_access(1);
    // Some games only have half an SRAM bank, set to bank 0
    mbc5_set_ram_bank(0);
    // Just read half a bank
    for(uint16_t byte_offset = 0; byte_offset < SRAM_HALF_BANK_SIZE; byte_offset++){
        buf[byte_offset] = readb(byte_offset + SRAM_START_ADDR);
    }
    // Disable SRAM access
    mbc5_set_ram_access(0);
}