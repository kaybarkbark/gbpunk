#include "gb.h"
#include "cart.h"
#include "mappers/no_mapper.h"
#include "mappers/mbc1.h"
#include "mappers/mbc2.h"
#include "mappers/mbc3.h"
#include "mappers/mbc5.h"
#include "mappers/gbcam.h"
#include "mappers/huc1.h"

#include <string.h>
#include <stdio.h>

const uint8_t logo[] = {0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B, 0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D,
						0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E, 0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99,
						0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC, 0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E};

struct Cart the_cart = {0};


void populate_cart_info(){
    // Get the cartridge type
    the_cart.cart_type = readb(CART_TYPE_ADDR);
    // Get the human readible name for the cart type
    memset(the_cart.cart_type_str, 0, 30);
    // FIXME for some reason this replaces the first char with 0. No idea why
    switch(the_cart.cart_type){
        case 0: strncpy(the_cart.cart_type_str, "ROM ONLY", 8); the_cart.mapper_type = MAPPER_ROM_ONLY; break;
        case 1: strncpy(the_cart.cart_type_str, "MBC1", 4); the_cart.mapper_type = MAPPER_MBC1; break;
        case 2: strncpy(the_cart.cart_type_str, "MBC1+RAM", 8); the_cart.mapper_type = MAPPER_MBC1; break;
        case 3: strncpy(the_cart.cart_type_str, "MBC1+RAM+BATTERY", 16); the_cart.mapper_type = MAPPER_MBC1; break;
        case 5: strncpy(the_cart.cart_type_str, "MBC2", 4); the_cart.mapper_type = MAPPER_MBC2; break;
        case 6: strncpy(the_cart.cart_type_str, "MBC2+BATTERY", 12); the_cart.mapper_type = MAPPER_MBC2; break;
        case 8: strncpy(the_cart.cart_type_str, "ROM+RAM", 7); the_cart.mapper_type = MAPPER_ROM_RAM; break;
        case 9: strncpy(the_cart.cart_type_str, "ROM ONLY", 8); the_cart.mapper_type = MAPPER_ROM_ONLY; break;
        case 11: strncpy(the_cart.cart_type_str, "MMM01", 5); the_cart.mapper_type = MAPPER_MMM01; break;
        case 12: strncpy(the_cart.cart_type_str, "MMM01+RAM", 9); the_cart.mapper_type = MAPPER_MMM01; break;
        case 13: strncpy(the_cart.cart_type_str, "MMM01+RAM+BATTERY", 17); the_cart.mapper_type = MAPPER_MMM01; break;
        case 15: strncpy(the_cart.cart_type_str, "MBC3+TIMER+BATTERY", 18); the_cart.mapper_type = MAPPER_MBC3; break;
        case 16: strncpy(the_cart.cart_type_str, "MBC3+TIMER+RAM+BATTERY", 22); the_cart.mapper_type = MAPPER_MBC3; break;
        case 17: strncpy(the_cart.cart_type_str, "MBC3", 4); the_cart.mapper_type = MAPPER_MBC3; break;
        case 18: strncpy(the_cart.cart_type_str, "MBC3+RAM", 8); the_cart.mapper_type = MAPPER_MBC3; break;
        case 19: strncpy(the_cart.cart_type_str, "MBC3+RAM+BATTERY", 16); the_cart.mapper_type = MAPPER_MBC3; break;
        case 21: strncpy(the_cart.cart_type_str, "MBC4", 4); the_cart.mapper_type = MAPPER_MBC4; break;
        case 22: strncpy(the_cart.cart_type_str, "MBC4+RAM", 8); the_cart.mapper_type = MAPPER_MBC4; break;
        case 23: strncpy(the_cart.cart_type_str, "MBC4+RAM+BATTERY", 16); the_cart.mapper_type = MAPPER_MBC4; break;
        case 25: strncpy(the_cart.cart_type_str, "MBC5", 4); the_cart.mapper_type = MAPPER_MBC5; break;
        case 26: strncpy(the_cart.cart_type_str, "MBC5+RAM", 8); the_cart.mapper_type = MAPPER_MBC5; break;
        case 27: strncpy(the_cart.cart_type_str, "MBC5+RAM+BATTERY", 16); the_cart.mapper_type = MAPPER_MBC5; break;
        case 28: strncpy(the_cart.cart_type_str, "MBC5+RUMBLE", 11); the_cart.mapper_type = MAPPER_MBC5; break;
        case 29: strncpy(the_cart.cart_type_str, "MBC5+RUMBLE+RAM", 15); the_cart.mapper_type = MAPPER_MBC5; break;
        case 30: strncpy(the_cart.cart_type_str, "MBC5+RUMBLE+RAM+BATTERY", 23); the_cart.mapper_type = MAPPER_MBC5; break;
        case 252: strncpy(the_cart.cart_type_str, "GB CAMERA", 9); the_cart.mapper_type = MAPPER_GBCAM; break;
        case 254: strncpy(the_cart.cart_type_str, "HuC3+RAM+BATTERY", 16); the_cart.mapper_type = MAPPER_HUC1; break;
        case 255: strncpy(the_cart.cart_type_str, "HuC1+RAM+BATTERY", 16); the_cart.mapper_type = MAPPER_HUC3; break;
        default: the_cart.mapper_type = MAPPER_UNKNOWN; break;
    }
    if(the_cart.mapper_type == MAPPER_ROM_ONLY){
        the_cart.rom_memcpy_func = &no_mapper_memcpy_rom;
        // No actual mapper so none of these apply
        the_cart.ram_memcpy_func = NULL;
        the_cart.ram_memset_func = NULL;
        the_cart.ram_enable_func = NULL;
        the_cart.rom_banksw_func = NULL;
        the_cart.ram_banksw_func = NULL;
    }
    if(the_cart.mapper_type == MAPPER_ROM_RAM){
        the_cart.rom_memcpy_func = &no_mapper_memcpy_rom;
        the_cart.ram_memcpy_func = &no_mapper_memcpy_ram;
        the_cart.ram_memset_func = &no_mapper_memset_ram;
        // No banks to worry about
        the_cart.ram_enable_func = NULL;
        the_cart.rom_banksw_func = NULL;
        the_cart.ram_banksw_func = NULL;
    }
    else if(the_cart.mapper_type == MAPPER_MBC1){
        the_cart.rom_memcpy_func = &mbc1_memcpy_rom;
        the_cart.ram_memcpy_func = &mbc1_memcpy_ram;
        the_cart.ram_memset_func = &mbc1_memset_ram;
        the_cart.ram_enable_func = &mbc1_set_ram_access;
        the_cart.rom_banksw_func = &mbc1_set_rom_bank;
        the_cart.ram_banksw_func = &mbc1_set_ram_bank;
    }
    else if(the_cart.mapper_type == MAPPER_MBC2){
        the_cart.rom_memcpy_func = &mbc2_memcpy_rom;
        the_cart.ram_memcpy_func = &mbc2_memcpy_ram;
        the_cart.ram_memset_func = &mbc2_memset_ram;
        the_cart.ram_enable_func = &mbc2_set_ram_access;
        the_cart.rom_banksw_func = &mbc2_set_rom_bank;
        the_cart.ram_banksw_func = NULL;  // MBC2 has no RAM banks, RAM is in the mapper
    }
    else if(the_cart.mapper_type == MAPPER_MBC3){
        the_cart.rom_memcpy_func = &mbc3_memcpy_rom;
        the_cart.ram_memcpy_func = &mbc3_memcpy_ram;
        the_cart.ram_memset_func = &mbc3_memset_ram;
        the_cart.ram_enable_func = &mbc3_set_ram_access;
        the_cart.rom_banksw_func = &mbc3_set_rom_bank;
        the_cart.ram_banksw_func = &mbc3_set_ram_bank;
    }
    else if(the_cart.mapper_type == MAPPER_MBC5){
        the_cart.rom_memcpy_func = &mbc5_memcpy_rom;
        the_cart.ram_memcpy_func = &mbc5_memcpy_ram;
        the_cart.ram_memset_func = &mbc5_memset_ram;
        the_cart.ram_enable_func = &mbc5_set_ram_access;
        the_cart.rom_banksw_func = &mbc5_set_rom_bank;
        the_cart.ram_banksw_func = &mbc5_set_ram_bank;
    }
    else if(the_cart.mapper_type == MAPPER_GBCAM){
        the_cart.rom_memcpy_func = &gbcam_memcpy_rom;
        the_cart.ram_memcpy_func = &gbcam_memcpy_ram;
        the_cart.ram_memset_func = &gbcam_memset_ram;
        the_cart.ram_enable_func = &gbcam_set_ram_access;
        the_cart.rom_banksw_func = &gbcam_set_rom_bank;
        the_cart.ram_banksw_func = &gbcam_set_ram_bank;
    }
    else if(the_cart.mapper_type == MAPPER_HUC1){
        the_cart.rom_memcpy_func = &huc1_memcpy_rom;
        the_cart.ram_memcpy_func = &huc1_memcpy_ram;
        the_cart.ram_memset_func = &huc1_memset_ram;
        the_cart.ram_enable_func = &huc1_set_ram_access;
        the_cart.rom_banksw_func = &huc1_set_rom_bank;
        the_cart.ram_banksw_func = &huc1_set_ram_bank;
    }
    else if(the_cart.mapper_type == MAPPER_HUC3){
        // HuC3 and HuC1 are backwards compatible for this
        the_cart.rom_memcpy_func = &huc1_memcpy_rom;
        the_cart.ram_memcpy_func = &huc1_memcpy_ram;
        the_cart.ram_memset_func = &huc1_memset_ram;
        the_cart.ram_enable_func = &huc1_set_ram_access;
        the_cart.rom_banksw_func = &huc1_set_rom_bank;
        the_cart.ram_banksw_func = &huc1_set_ram_bank;
    }
    else if(the_cart.mapper_type == MAPPER_UNKNOWN){
        the_cart.rom_memcpy_func = NULL;
        the_cart.ram_memcpy_func = NULL;
        the_cart.ram_memset_func = NULL;
        the_cart.ram_enable_func = NULL;
        the_cart.rom_banksw_func = NULL;
        the_cart.ram_banksw_func = NULL;
        snprintf(the_cart.cart_type_str, 19, "UNKNOWN MAPPER 0x%2x", the_cart.cart_type); 
    }
    // Calculate ROM banks
    the_cart.rom_banks = 2 << readb(ROM_BANK_SHIFT_ADDR);
    the_cart.rom_size_bytes = ROM_BANK_SIZE * the_cart.rom_banks; // Even ROM Only will report two banks
    // RAM banks are random-ish, need lookup
    uint8_t ram_size = readb(RAM_BANK_COUNT_ADDR);
    // Handle MBC2 w/ battery backed RAM. Only 256 bytes, split among 512 4 bit memory locations
    // The RAM is in the mapper, so this should always exist
    if(the_cart.mapper_type == MAPPER_MBC2){
        the_cart.ram_banks = 1;
        the_cart.ram_end_address = 0xA0FF;
        the_cart.ram_size_bytes =  (1 + 0xA0FF) - (SRAM_START_ADDR);
    }
    // Handle 2K, don't need the full RAM address space
    else if(ram_size == 2){
        the_cart.ram_banks = 1;
        the_cart.ram_end_address = 0xA7FF;
        the_cart.ram_size_bytes =  (1 + 0xA7FF) - (SRAM_START_ADDR + 1);
    }
    // All others will use the full address space
    else if(ram_size == 3){
        the_cart.ram_banks = 4;
        the_cart.ram_end_address = SRAM_END_ADDR;
        the_cart.ram_size_bytes = the_cart.ram_banks * SRAM_BANK_SIZE;
    }
    else if(ram_size == 4){
        the_cart.ram_banks = 16;
        the_cart.ram_end_address = SRAM_END_ADDR;
        the_cart.ram_size_bytes = the_cart.ram_banks * SRAM_BANK_SIZE;
    }
    else if(ram_size == 5){
        the_cart.ram_banks = 8;
        the_cart.ram_end_address = SRAM_END_ADDR;
        the_cart.ram_size_bytes = the_cart.ram_banks * SRAM_BANK_SIZE;
    }
    // No RAM
    else{
        the_cart.ram_banks = 0;
        the_cart.ram_end_address = SRAM_START_ADDR;
        the_cart.ram_size_bytes = 0;
    }
    // Read back the title
    for(uint8_t i = 0; i < CART_TITLE_LEN; i++){
        char c = readb(CART_TITLE_ADDR + i);
        // If unprintable, we hit the end. Null terminate
        if((c < 0x20) || (c > 0x7e)){
            the_cart.title[i] = 0;
            break;
        }
        the_cart.title[i] = c;
    }
    the_cart.title[CART_TITLE_LEN] = 0; // Ensure null terminated

    // Special case Pokemon Crystal JP because I am pedantic
    // Pokemon Crystal JP's mapper MBC30 is functionally the same but can
    // access more SRAM. It's a different mapper and I want to make sure 
    // that shows up right in status.txt

    // Detect JP Crystal by looking for the game name and checking the amount of SRAM
    if(strncmp(the_cart.title, "PM_CRYSTAL", 10) && (the_cart.ram_banks == 8)){
        strncpy(the_cart.cart_type_str, "MBC30+TIMER+RAM+BATTERY", 23);
    }

}

void dump_cart_info(){
    printf("Cart Type: %s (0x%x)\n", the_cart.cart_type_str, the_cart.cart_type);
    printf("Num ROM Banks: %i\n", the_cart.rom_banks);
    printf("Num RAM Banks: %i\n", the_cart.ram_banks);
    printf("RAM Ending Address: 0x%x\n", the_cart.ram_end_address);
    printf("Cart Title: %s\n", the_cart.title);
}

uint16_t cart_check(uint8_t *logo_buf){
    readbuf(LOGO_START_ADDR, logo_buf, LOGO_LEN);
    for(uint8_t i = 0; i < LOGO_LEN; i++){
        if(logo_buf[i] != logo[i]){
            return 0;
        }
    }
    return 1;
}