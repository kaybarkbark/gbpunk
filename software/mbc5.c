#include "mbc5.h"
#include "gb.h"
#include "utils.h"
#include "stdio.h"
#include "stdlib.h"

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
    writeb(bank & 0xFF, MBC5_LOW_ROM_BANK_ADDR);
    writeb(bank & 0xFF00 >> 8, MBC5_HIGH_ROM_BANK_ADDR);
}

void mbc5_set_ram_bank(uint8_t bank){
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

uint8_t mbc5_unit_test(struct Cart *cart){
    uint8_t ret = 0;
    printf("SRAM Test\n");
    printf("---------\n");
    if(mbc5_unit_test_sram(cart)){
        printf("--- SRAM Test FAILED ---\n");
        ret = 1;
    }
    printf("ROM Bankswitch Test\n");
    printf("---------\n");
    if(mbc5_unit_test_rom_bank_switching(cart)){
        printf("--- ROM Bankswitch Test FAILED ---\n");
        ret = 1;
    }
    printf("SRAM Bankswitch Test\n");
    printf("---------\n");
    if(mbc5_unit_test_ram_bank_switching(cart)){
        printf("--- SRAM Bankswitch Test FAILED ---\n");
        ret = 1;
    }
    return ret;
}

uint8_t mbc5_unit_test_sram(struct Cart *cart){
    uint8_t ret = 0;
    mbc5_set_ram_access(1);
    printf("Unit testing SRAM in %s...\n", cart->title);
    printf("SRAM Banks: %i\n", cart->ram_banks);
    printf("SRAM Ending Address: 0x%x\n", cart->ram_end_address);
    for(uint8_t sram_bank = 0; sram_bank < cart->ram_banks; sram_bank ++){
        printf("Testing SRAM bank %i\n", sram_bank);
        // Test the bank
        mbc5_set_ram_bank(sram_bank);
        // Generate a test address
        uint16_t test_address = (rand() % (cart->ram_end_address - SRAM_START_ADDR)) + SRAM_START_ADDR; 
        printf("Testing address 0x%x\n", test_address);
        // Save for later restore
        uint8_t saved_data = readb(test_address);
        printf("0x%x: 0x%x\n", test_address, saved_data);
        // Generate new data and write it 
        uint8_t new_data = saved_data + (rand() % 100);
        printf("Writing 0x%x to 0x%x\n", new_data, test_address);
        writeb(new_data, test_address);
        // Make sure it got written back
        printf("Making sure data got written...\n");
        uint8_t changed_data = readb(test_address);
        printf("0x%x: 0x%x\n", test_address, changed_data);
        if(changed_data == new_data){
            printf("+++ SUCCESS: Data written successfully\n");
        }
        else if(changed_data == saved_data){
            printf("--- FAILURE: Data did not change\n");
            ret = 1;
        }
        else{
            printf("--- FAILURE: Got completely different value back, 0x%x\n", changed_data);
            ret = 1;
        }
        printf("Restoring cart to its original state...\n");
        // TODO: Ensure this actually got written back
        writeb(saved_data, test_address);
        printf("0x%x: 0x%x\n", test_address, readb(test_address));
    }
    mbc5_set_ram_access(0);
    return ret;
}

uint8_t mbc5_unit_test_rom_bank_switching(struct Cart *cart){
    printf("Unit testing ROM banks in %s...\n", cart->title);
    printf("ROM Banks: %i\n", cart->rom_banks);
    uint8_t successes = 0;
    if(cart->rom_banks <= 2){
        printf("WARNING: This cart does not have extra ROM banks! Skipping...\n");
        return 0;
    }
    uint8_t initial_slice[10] = {0};
    uint8_t next_slice[10] = {0};

    // Generate a start address for the buffer read, with at least 10 bytes of space tp read
    uint16_t start_address = (rand() % ROM_BANK_SIZE) + ROM_BANKN_START_ADDR;
    if(start_address >= (ROM_BANKN_END_ADDR - 10)){
        start_address -= 10;
    }
    printf("Testing memory 0x%x -> 0x%x\n", start_address, start_address + 10);
    printf("Taking first slice of ROM from bank 1...\n");
    mbc5_set_rom_bank(1);
    for(uint8_t i = 0; i < 10; i++){
        initial_slice [i] = readb(start_address + i);
    }
    for(uint16_t bank = 2; bank < cart->rom_banks; bank++){
        mbc5_set_rom_bank(bank);
        printf("===\nTaking next slice from ROM bank %i...\n", bank);
        for(uint8_t i = 0; i < 10; i++){
            next_slice [i] = readb(start_address + i);
        }
        printf("Bank %i Slice: ", bank - 1);
        for(uint8_t i = 0; i < 10; i++){
            printf("0x%02x ", initial_slice[i]);
        }
        printf("\nBank %i Slice: ", bank);
        for(uint8_t i = 0; i < 10; i++){
            printf("0x%02x ", next_slice[i]);
        }
        if(bufncmp(initial_slice, next_slice, 10)){
            printf("\nWARNING: Memory is the same, cart did not *appear* to bank switch ROM\n===\n");
        }
        else{
            printf("\nMemory differs, ROM bank must have switched\n===\n");
            successes++;
        }
        // Save next slice to the initial slice so we can use it in the next loop
        bufncpy(initial_slice, next_slice, 10);
    }
    if(successes){
        printf("+++ SUCCESS: Cart provably bank switched ROM %i times\n", successes);
        return 0;
    }
    else{
        printf("--- FAILURE: Cart never bank switched ROM\n", successes);
        return 1;
    }
}

uint8_t mbc5_unit_test_ram_bank_switching(struct Cart *cart){
    mbc5_set_ram_access(1);
    printf("Unit testing SRAM banks in %s...\n", cart->title);
    printf("RAM Banks: %i\n", cart->ram_banks);
    uint8_t successes = 0;
    if(cart->ram_banks < 2){
        printf("WARNING: This cart does not have enough SRAM to test bank switching! Skipping...\n");
        return 0;
    }
    uint8_t initial_slice[64] = {0};
    uint8_t next_slice[64] = {0};

    // Generate a start address for the buffer read, with at least 64 bytes of space tp read
    uint16_t start_address = (rand() % SRAM_BANK_SIZE) + SRAM_START_ADDR;
    if(start_address >= (SRAM_END_ADDR - 64)){
        start_address -= 64;
    }
    printf("Testing memory 0x%x -> 0x%x\n", start_address, start_address + 64);
    printf("Taking first slice of SRAM from bank 0...\n");
    mbc5_set_ram_bank(0);
    for(uint8_t i = 0; i < 64; i++){
        initial_slice [i] = readb(start_address + i);
    }
    for(uint16_t bank = 1; bank < cart->ram_banks; bank++){
        mbc5_set_ram_bank(bank);
        printf("===\nTaking next slice from SRAM bank %i...\n", bank);
        for(uint8_t i = 0; i < 64; i++){
            next_slice [i] = readb(start_address + i);
        }
        printf("Bank %i Slice: ", bank - 1);
        for(uint8_t i = 0; i < 64; i++){
            printf("0x%02x ", initial_slice[i]);
        }
        printf("\nBank %i Slice: ", bank);
        for(uint8_t i = 0; i < 64; i++){
            printf("0x%02x ", next_slice[i]);
        }
        if(bufncmp(initial_slice, next_slice, 64)){
            printf("\nWARNING: Memory is the same, cart did not *appear* to bank switch SRAM\n===\n");
        }
        else{
            printf("\nMemory differs, SRAM bank must have switched\n===\n");
            successes++;
        }
        // Save next slice to the initial slice so we can use it in the next loop
        bufncpy(initial_slice, next_slice, 64);
    }
    mbc5_set_ram_access(0);
    if(successes){
        printf("+++ SUCCESS: Cart provably bank switched SRAM %i times\n", successes);
        return 0;
    }
    else{
        printf("--- FAILURE: Cart never bank switched SRAM \n", successes);
        return 1;
    }
}