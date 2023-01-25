#include "mbc5.h"
#include "gb.h"
#include "stdio.h"

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

void mbc5_set_rom_bank(uint16_t bank){
    // Set the full 16 bits of the SRAM bank
    writeb(bank & 0xFF, LOW_ROM_BANK_ADDR);
    writeb(bank & 0xFF00 >> 8, HIGH_ROM_BANK_ADDR);
}
void mbc5_set_ram_bank(uint8_t bank){
    // Remember to enable RAM access first!
    writeb(bank, RAM_BANK_ADDR);
}

void mbc5_set_ram_access(uint8_t on_off){
    if(on_off){
        writeb(ENABLE_RAM_ACCESS_DATA, ENABLE_RAM_ACCESS_ADDR);
        return;
    }
    writeb(0x0, ENABLE_RAM_ACCESS_ADDR);

}