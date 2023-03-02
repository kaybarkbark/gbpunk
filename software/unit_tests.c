#include "unit_tests.h"
#include "gb.h"
#include "mbc5.h"
#include "rom_only.h"
#include "gbcam.h"
#include "msc_disk.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

uint8_t mbc5_unit_test_memcpy();
uint8_t mbc5_unit_test_sram();
uint8_t mbc5_unit_test_rom_bank_switching();
uint8_t mbc5_unit_test_ram_bank_switching();

// Unit tests should follow the following structure
// - ROM coherency. Read the same ROM bank over and over, make sure it never changes
// - SRAM coherency. Read the same SRAM bank over and over, make sure it never changes
// - ROM bankswitching. Read a whole bank of ROM, then the next, make sure it always switches
// - SRAM bankswitching. Read a whole bank of SRAM, then the next, make sure it always switches
// - SRAM writes. Save a byte from SRAM, write a new one, make sure it got written, write back old
// - Run all these, then if any of them failed, return a fail

uint8_t memory_coherency(
    void (*memcpy_func)(uint8_t*, uint32_t, uint32_t), 
    uint32_t start_addr, 
    uint32_t num){
    // Continually read the entirety of Bank 0 to make sure we always get the same thing
    // Establish two pointers to the memory we want to compare
    uint8_t* written_buf = working_mem;
    uint8_t* comparison_buf = working_mem + num;
    // The amount of times to run the test
    uint8_t count = 255;
    // Set up the initial buffer
    (*memcpy_func)(written_buf, start_addr, num);
    while(count){
        // Read new memory to test
        (*memcpy_func)(comparison_buf, start_addr, num);
        // Compare the two
        if(!bufncmp(written_buf, comparison_buf, num)){
            return 0;
        }
        count--;
    }
    return 1;
}

uint8_t rom_ram_coherency(
    void (*rom_memcpy_func)(uint8_t*, uint32_t, uint32_t), 
    void (*ram_memcpy_func)(uint8_t*, uint32_t, uint32_t),
    uint32_t sram_size
){
    uint8_t ret = 1;
    // First test ROM
    if(!memory_coherency(rom_memcpy_func, ROM_BANK0_START_ADDR, ROM_BANK_SIZE)){
        ret = 0;
        append_status_file("ROM COHERENCY: FAIL\n");
    }
    else{
        append_status_file("ROM COHERENCY: PASS\n");
    }
    if(ram_memcpy_func && sram_size){
        if(!memory_coherency(ram_memcpy_func, SRAM_START_ADDR, sram_size)){
            ret = 0;
            append_status_file("SRAM COHERENCY: FAIL\n");
        }
        else{
            append_status_file("SRAM COHERENCY: PASS\n");
        }
    }
    else{
        append_status_file("SRAM COHERENCY: SKIPPED\n");
    }
}

/* DEPRECATED, OLD UNIT TESTS*/

uint8_t mbc5_unit_test(){
    uint8_t ret = 0;
    append_status_file("MBC5 TESTS\n\0");
    append_status_file("memcpy_rom: \0");
    if(mbc5_unit_test_memcpy()){
        append_status_file("FAIL\n\0");
        ret = 1;
    }
    else{
        append_status_file("PASS\n\0");
    }
    append_status_file("SRAM: \0");
    if(mbc5_unit_test_sram()){
        append_status_file("FAIL\n\0");
        ret = 1;
    }
    else{
        append_status_file("PASS\n\0");
    }
    append_status_file("ROM BANKSW: \0");
    if(mbc5_unit_test_rom_bank_switching()){
        append_status_file("FAIL\n\0");
        ret = 1;
    }
    else{
        append_status_file("PASS\n\0");
    }
    append_status_file("RAM BANKSW: \0");
    if(mbc5_unit_test_ram_bank_switching()){
        append_status_file("FAIL\n\0");
        ret = 1;
    }
    else{
        append_status_file("PASS\n\0");
    }
    return ret;
}


uint8_t mbc5_unit_test_memcpy(){
    printf("Copying the nintendo logo out of ROM...");
    mbc5_memcpy_rom(working_mem, LOGO_START_ADDR, LOGO_LEN);
    hexdump(working_mem, LOGO_LEN, LOGO_START_ADDR);
    return 0;
}

uint8_t mbc5_unit_test_sram(){
    uint8_t ret = 0;
    mbc5_set_ram_access(1);
    printf("Unit testing SRAM in %s...\n", the_cart.title);
    printf("SRAM Banks: %i\n", the_cart.ram_banks);
    printf("SRAM Ending Address: 0x%x\n", the_cart.ram_end_address);
    for(uint8_t sram_bank = 0; sram_bank < the_cart.ram_banks; sram_bank ++){
        printf("Testing SRAM bank %i\n", sram_bank);
        // Test the bank
        mbc5_set_ram_bank(sram_bank);
        // Generate a test address
        uint16_t test_address = (rand() % (the_cart.ram_end_address - SRAM_START_ADDR)) + SRAM_START_ADDR; 
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

uint8_t mbc5_unit_test_rom_bank_switching(){
    const uint16_t slice_size = 10;
    printf("Unit testing ROM banks in %s...\n", the_cart.title);
    printf("ROM Banks: %i\n", the_cart.rom_banks);
    uint8_t successes = 0;
    if(the_cart.rom_banks <= 2){
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
    for(uint16_t bank = 2; bank < the_cart.rom_banks; bank++){
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

uint8_t mbc5_unit_test_ram_bank_switching(){
    const uint16_t slice_size = 64;
    mbc5_set_ram_access(1);
    printf("Unit testing SRAM banks in %s...\n", the_cart.title);
    printf("RAM Banks: %i\n", the_cart.ram_banks);
    uint8_t successes = 0;
    if(the_cart.ram_banks < 2){
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
    for(uint16_t bank = 1; bank < the_cart.ram_banks; bank++){
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


uint8_t gbcam_unit_test_sram(){
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
    printf("Reading the rest, using bank switching\n");
    for(uint8_t bank = 1; bank <= 0xF; bank++){
        gbcam_set_ram_bank(bank);
        for(uint16_t offset = 0; offset < SRAM_BANK_SIZE; offset++){
            buf[offset] = readb(SRAM_START_ADDR + offset);
        }
        printf("Bank %i\n", bank);
        hexdump(buf, SRAM_BANK_SIZE, SRAM_START_ADDR);
    }
    gbcam_set_ram_writable(0);
}

uint8_t gbcam_unit_test_rom_bank_switching(){

    printf("Unit testing ROM banks in GAMEBOYCAMERA...\n");
    uint8_t successes = 0;
    uint8_t initial_slice[10] = {0};
    uint8_t next_slice[10] = {0};

    // Generate a start address for the buffer read, with at least 10 bytes of space tp read
    uint16_t start_address = (rand() % ROM_BANK_SIZE) + ROM_BANKN_START_ADDR;
    if(start_address >= (ROM_BANKN_END_ADDR - 10)){
        start_address -= 10;
    }
    printf("Testing memory 0x%x -> 0x%x\n", start_address, start_address + 10);
    printf("Taking first slice of ROM from bank 1...\n");
    gbcam_set_rom_bank(1);
    for(uint8_t i = 0; i < 10; i++){
        initial_slice [i] = readb(start_address + i);
    }
    for(uint16_t bank = 2; bank < GBCAM_NUM_ROM_BANKS; bank++){
        gbcam_set_rom_bank(bank);
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
            printf("\nWARNING: Memory is the same, cart did not *appear* to bank switch\n===\n");
        }
        else{
            printf("\nMemory differs, bank must have switched\n===\n");
            successes++;
        }
        // Save next slice to the initial slice so we can use it in the next loop
        bufncpy(initial_slice, next_slice, 10);
    }
    if(successes){
        printf("+++ SUCCESS: Cart provably bank switched %i times\n", successes);
        return 0;
    }
    else{
        printf("--- FAILURE: Cart never bank switched \n", successes);
        return 1;
    }
}

uint8_t gbcam_unit_test_sram_bank_switching(){
    printf("Unit testing SRAM banks in GAMEBOYCAMERA...\n");
    printf("RAM Banks: %i\n", GBCAM_NUM_SRAM_BANKS);
    uint8_t successes = 0;
    uint8_t initial_slice[64] = {0};
    uint8_t next_slice[64] = {0};

    // Generate a start address for the buffer read, with at least 64 bytes of space tp read
    uint16_t start_address = (rand() % SRAM_BANK_SIZE) + SRAM_START_ADDR;
    if(start_address >= (SRAM_END_ADDR - 64)){
        start_address -= 64;
    }
    printf("Testing memory 0x%x -> 0x%x\n", start_address, start_address + 64);
    printf("Taking first slice of SRAM from bank 0...\n");
    gbcam_set_ram_bank(0);
    for(uint8_t i = 0; i < 64; i++){
        initial_slice [i] = readb(start_address + i);
    }
    for(uint16_t bank = 1; bank < GBCAM_NUM_SRAM_BANKS; bank++){
        gbcam_set_ram_bank(bank);
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
    if(successes){
        printf("+++ SUCCESS: Cart provably bank switched SRAM %i times\n", successes);
        return 0;
    }
    else{
        printf("--- FAILURE: Cart never bank switched SRAM \n", successes);
        return 1;
    }
}

uint8_t gbcam_unit_test(){
    uint8_t ret = 0;
    /*
    printf("SRAM Test\n");
    printf("---------\n");
    if(gbcam_unit_test_sram()){
        printf("--- SRAM Test FAILED ---");
        ret = 1;
    }
    */
    printf("ROM Bankswitch Test\n");
    printf("---------\n");
    if(gbcam_unit_test_rom_bank_switching()){
        printf("--- Bankswitch Test FAILED ---");
        ret = 1;
    }
    printf("RAM Bankswitch Test\n");
    printf("---------\n");
    if(gbcam_unit_test_sram_bank_switching()){
        printf("--- Bankswitch Test FAILED ---");
        ret = 1;
    }
    gbcam_unit_test_sram();
    return ret;
}
