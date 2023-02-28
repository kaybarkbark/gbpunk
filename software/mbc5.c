#include "mbc5.h"
#include "gb.h"
#include "utils.h"
#include "stdio.h"
#include "stdlib.h"
#include "msc_disk.h"

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

uint16_t mbc5_memcpy_rom(uint8_t* dest, uint32_t rom_addr, uint32_t num){
    // Determine the current bank
    uint16_t current_bank = fs_get_rom_bank(rom_addr);
    uint32_t rom_cursor = 0;
    // Keep track of where we are in ROM
    rom_cursor = rom_addr % ROM_BANK_SIZE;
    // Set up the bank for transfer
    mbc5_set_rom_bank(current_bank);
    for(uint16_t buf_cursor = 0; buf_cursor < num; buf_cursor++){
        // Determine if we need to bankswitch or not
        if(rom_cursor >= ROM_BANK_SIZE){
            // Switch banks if we cross a boundary
            current_bank++;
            mbc5_set_rom_bank(current_bank);
            // If we bankswitch, we start over again at the beginning of the the bank
            rom_cursor = 0;
        }
        // Old debug
        // uint8_t d = readb(rom_cursor + ROM_BANKN_START_ADDR);
        // if(d != 0){
        //     printf("0x%x ", d);
        // }
        // dest[buf_cursor] = d; 
        // Read everything out of banked ROM, even bank 0. Easier that way
        dest[buf_cursor] = readb(rom_cursor + ROM_BANKN_START_ADDR); 
        rom_cursor++;
    }
    // printf("\n");
}
// Note: This gets memory relative to RAM, not the cart. So 0x0 means start of RAM
uint16_t mbc5_memcpy_ram(uint8_t* dest, uint32_t ram_addr, uint32_t num){
    // Keep track of where we are in RAM
    uint32_t ram_cursor = ram_addr;
    // Keep track of our current bank
    uint16_t current_bank = fs_get_ram_bank(ram_cursor);
    // Set up the bank for transfer
    mbc5_set_ram_bank(current_bank);
    for(uint16_t buf_cursor = 0; buf_cursor < num; buf_cursor++){
        // Determine if we need to bankswitch or not
        uint16_t new_bank = fs_get_rom_bank(ram_cursor);
        if(new_bank != current_bank){
            // Switch banks if we cross a boundary
            current_bank = new_bank;
            mbc5_set_ram_bank(current_bank);
            // If we bankswitch, we start over again at the beginning of the the bank
            ram_cursor = 0;
        }
        // Read everything out of banked ROM, even bank 0. Easier that way
        dest[buf_cursor] = readb(ram_cursor + SRAM_START_ADDR); 
        ram_cursor++;
    }
}

uint8_t mbc5_unit_test(struct Cart *cart){
    uint8_t ret = 0;
    append_status_file("MBC5 TESTS\n\0");
    append_status_file("memcpy_rom: \0");
    if(mbc5_unit_test_memcpy(cart)){
        append_status_file("FAIL\n\0");
        ret = 1;
    }
    else{
        append_status_file("PASS\n\0");
    }
    append_status_file("SRAM: \0");
    if(mbc5_unit_test_sram(cart)){
        append_status_file("FAIL\n\0");
        ret = 1;
    }
    else{
        append_status_file("PASS\n\0");
    }
    append_status_file("ROM BANKSW: \0");
    if(mbc5_unit_test_rom_bank_switching(cart)){
        append_status_file("FAIL\n\0");
        ret = 1;
    }
    else{
        append_status_file("PASS\n\0");
    }
    append_status_file("RAM BANKSW: \0");
    if(mbc5_unit_test_ram_bank_switching(cart)){
        append_status_file("FAIL\n\0");
        ret = 1;
    }
    else{
        append_status_file("PASS\n\0");
    }
    return ret;
}


uint8_t mbc5_unit_test_memcpy(struct Cart *cart){
    printf("Copying the nintendo logo out of ROM...");
    mbc5_memcpy_rom(working_mem, LOGO_START_ADDR, LOGO_LEN);
    hexdump(working_mem, LOGO_LEN, LOGO_START_ADDR);
    return 0;
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
    const uint16_t slice_size = 10;
    printf("Unit testing ROM banks in %s...\n", cart->title);
    printf("ROM Banks: %i\n", cart->rom_banks);
    uint8_t successes = 0;
    if(cart->rom_banks <= 2){
        printf("WARNING: This cart does not have extra ROM banks! Skipping...\n");
        return 0;
    }
    // Generate a start address for the buffer read, with at least slice_size bytes of space to read
    uint16_t start_address = (rand() % ROM_BANK_SIZE) + ROM_BANKN_START_ADDR;
    if(start_address >= (ROM_BANKN_END_ADDR - slice_size)){
        start_address -= slice_size;
    }
    printf("Testing memory 0x%x -> 0x%x\n", start_address, start_address + slice_size);
    printf("Taking first slice of ROM from bank 1...\n");
    mbc5_set_rom_bank(1);
    for(uint8_t i = 0; i < slice_size; i++){
        working_mem[i] = readb(start_address + i);
    }
    for(uint16_t bank = 2; bank < cart->rom_banks; bank++){
        mbc5_set_rom_bank(bank);
        printf("===\nTaking next slice from ROM bank %i...\n", bank);
        for(uint8_t i = 0; i < slice_size; i++){
            working_mem[i + slice_size] = readb(start_address + i);
        }
        printf("Bank %i Slice: ", bank - 1);
        for(uint8_t i = 0; i < slice_size; i++){
            printf("0x%02x ", working_mem[i]);
        }
        printf("\nBank %i Slice: ", bank);
        for(uint8_t i = 0; i < slice_size; i++){
            printf("0x%02x ", working_mem[i + slice_size]);
        }
        if(bufncmp(working_mem, working_mem + slice_size, slice_size)){
            printf("\nWARNING: Memory is the same, cart did not *appear* to bank switch ROM\n===\n");
        }
        else{
            printf("\nMemory differs, ROM bank must have switched\n===\n");
            successes++;
        }
        // Save next slice to the initial slice so we can use it in the next loop
        bufncpy(working_mem, working_mem + slice_size, slice_size);
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
    const uint16_t slice_size = 64;
    mbc5_set_ram_access(1);
    printf("Unit testing SRAM banks in %s...\n", cart->title);
    printf("RAM Banks: %i\n", cart->ram_banks);
    uint8_t successes = 0;
    if(cart->ram_banks < 2){
        printf("WARNING: This cart does not have enough SRAM to test bank switching! Skipping...\n");
        return 0;
    }

    // Generate a start address for the buffer read, with at least slice_size bytes of space tp read
    uint16_t start_address = (rand() % SRAM_BANK_SIZE) + SRAM_START_ADDR;
    if(start_address >= (SRAM_END_ADDR - slice_size)){
        start_address -= slice_size;
    }
    printf("Testing memory 0x%x -> 0x%x\n", start_address, start_address + slice_size);
    printf("Taking first slice of SRAM from bank 0...\n");
    mbc5_set_ram_bank(0);
    for(uint8_t i = 0; i < slice_size; i++){
        working_mem[i] = readb(start_address + i);
    }
    for(uint16_t bank = 1; bank < cart->ram_banks; bank++){
        mbc5_set_ram_bank(bank);
        printf("===\nTaking next slice from SRAM bank %i...\n", bank);
        for(uint8_t i = 0; i < slice_size; i++){
            working_mem[i + slice_size] = readb(start_address + i);
        }
        printf("Bank %i Slice: ", bank - 1);
        for(uint8_t i = 0; i < slice_size; i++){
            printf("0x%02x ", working_mem[i]);
        }
        printf("\nBank %i Slice: ", bank);
        for(uint8_t i = 0; i < slice_size; i++){
            printf("0x%02x ", working_mem[i + slice_size]);
        }
        if(bufncmp(working_mem, working_mem + slice_size, slice_size)){
            printf("\nWARNING: Memory is the same, cart did not *appear* to bank switch SRAM\n===\n");
        }
        else{
            printf("\nMemory differs, SRAM bank must have switched\n===\n");
            successes++;
        }
        // Save next slice to the initial slice so we can use it in the next loop
        bufncpy(working_mem, working_mem + slice_size, slice_size);
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