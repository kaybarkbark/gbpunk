#include "gbcam.h"
#include "gb.h"
#include "utils.h"
#include "stdio.h"

void gbcam_rom_dump(uint8_t *buf, uint8_t start_bank, uint8_t end_bank){
    // Iterate over the range of banks we want to dump
    for(uint16_t bank_offset = start_bank; bank_offset <= end_bank; bank_offset++){
        // Set the active bank in the cart
        gbcam_set_rom_bank(bank_offset);
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

void gbcam_ram_dump(uint8_t *buf, uint8_t start_bank, uint8_t end_bank){
    gbcam_set_ram_writable(1);
    // Iterate over the range of banks we want to dump
    for(uint16_t bank_offset = start_bank; bank_offset <= end_bank; bank_offset++){
        // Set the active bank in the cart
        gbcam_set_ram_bank(bank_offset);
        // Iterate over the full address space of a RAM bank
        for(uint16_t byte_offset = 0; byte_offset < SRAM_BANK_SIZE; byte_offset++){
            // Read back banked RAM sequentially into our buffer
            // |-----------Calculate bank offset------------|-Add byte offset-|      |---Read back from banked ROM-----|
            buf[((bank_offset - start_bank) * SRAM_BANK_SIZE) + byte_offset] = readb(byte_offset + SRAM_START_ADDR);
        }
    }
    gbcam_set_ram_writable(0);
}

void gbcam_set_rom_bank(uint8_t bank){
    writeb(bank & 0x3F, GBCAM_ROM_BANK_ADDR);
}
void gbcam_set_ram_bank(uint8_t bank){
    writeb(bank, GBCAM_RAM_BANK_ADDR);
}

void gbcam_set_ram_writable(uint8_t on_off){
    if(on_off){
        writeb(GBCAM_ENABLE_RAM_WRITE_DATA, GBCAM_ENABLE_RAM_WRITE_ADDR);
        return;
    }
    writeb(0x0, GBCAM_ENABLE_RAM_WRITE_ADDR);

}

void gbcam_unit_test_sram(){
    printf("Reading each bank of SRAM...\n");
    printf("Enabling RAM...\n");
    gbcam_set_ram_writable(1);
    printf("Starting from bank 0, without any bank switching\n");
    uint8_t buf[SRAM_BANK_SIZE] = {0};
    for(uint16_t offset = 0; offset < SRAM_BANK_SIZE; offset++){
        buf[offset] = readb(SRAM_START_ADDR + offset);
    }
    hexdump(buf, SRAM_BANK_SIZE, SRAM_START_ADDR);
    printf("Reading from CAM registers...\n");
    gbcam_set_ram_bank(GBCAM_CAM_REG_BANK);
    for(uint16_t offset = 0; offset < GBCAM_CAM_REG_SIZE; offset++){
        buf[offset] = readb(SRAM_START_ADDR + offset);
    }
    hexdump(buf, GBCAM_CAM_REG_SIZE, SRAM_START_ADDR);
    /*
    printf("Reading the rest, using bank switching\n");
    for(uint8_t bank = 1; bank <= 0xF; bank++){
        gbcam_set_ram_bank(bank);
        for(uint16_t offset = 0; offset < SRAM_BANK_SIZE; offset++){
            buf[offset] = readb(SRAM_START_ADDR + offset);
        }
        printf("Bank %i\n", bank);
        hexdump(buf, SRAM_BANK_SIZE, SRAM_START_ADDR);
    }
    */
    gbcam_set_ram_writable(0);
}