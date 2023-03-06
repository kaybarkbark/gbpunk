#include "gbcam.h"
#include "gb.h"
#include "utils.h"
#include "stdio.h"
uint8_t bmp_header[0x76] = {
    0x42, 0x4D, 0x76, 0x1C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x76, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x80, 
    0x00, 0x00, 0x00, 0x70, 0x00, 0x00, 0x00, 0x01, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1C, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x80, 0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00, 
    0x80, 0x00, 0x80, 0x80, 0x00, 0x00, 0x80, 0x80, 0x80, 0x00, 0xC0, 0xC0, 0xC0, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 
    0xFF, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0xFF, 0x00, 0x00, 
    0xFF, 0xFF, 0xFF, 0x00
};

void gbcam_memcpy_rom(uint8_t* dest, uint32_t rom_addr, uint32_t num){
    // Determine the current bank
    uint16_t current_bank = fs_get_rom_bank(rom_addr);
    uint32_t rom_cursor = 0;
    // Keep track of where we are in ROM
    rom_cursor = rom_addr % ROM_BANK_SIZE;
    // Set up the bank for transfer
    gbcam_set_rom_bank(current_bank);
    for(uint32_t buf_cursor = 0; buf_cursor < num; buf_cursor++){
        // Determine if we need to bankswitch or not
        if(rom_cursor >= ROM_BANK_SIZE){
            // Switch banks if we cross a boundary
            current_bank++;
            gbcam_set_rom_bank(current_bank);
            // If we bankswitch, we start over again at the beginning of the the bank
            rom_cursor = 0;
        }
        // Read everything out of banked ROM, even bank 0. Easier that way
        dest[buf_cursor] = readb(rom_cursor + ROM_BANKN_START_ADDR); 
        rom_cursor++;
    }
}

void gbcam_memcpy_ram(uint8_t* dest, uint32_t ram_addr, uint32_t num){
    // Enable RAM reads
    gbcam_set_ram_access(1);
    // Determine current bank
    uint16_t current_bank = fs_get_ram_bank(ram_addr);
    uint32_t ram_cursor = 0;
    // Keep track of where we are in RAM
    ram_cursor = ram_addr % SRAM_BANK_SIZE;
    // Set up the bank for transfer
    gbcam_set_ram_bank(current_bank);
    for(uint32_t buf_cursor = 0; buf_cursor < num; buf_cursor++){
        // Determine if we need to bankswitch or not
        if(ram_cursor >= SRAM_BANK_SIZE){
            // Switch banks if we cross a boundary
            current_bank++;
            gbcam_set_ram_bank(current_bank);
            // If we bankswitch, we start over again at the beginning of the the bank
            ram_cursor = 0;
        }
        dest[buf_cursor] = readb(ram_cursor + SRAM_START_ADDR); 
        ram_cursor++;
    }
    // Disable RAM reads
    gbcam_set_ram_access(0);
}


void gbcam_memset_ram(uint8_t* buf, uint32_t ram_addr, uint32_t num){
    // Determine current bank
    uint16_t current_bank = fs_get_ram_bank(ram_addr);
    uint32_t ram_cursor = 0;
    // Keep track of where we are in RAM
    ram_cursor = ram_addr % SRAM_BANK_SIZE;
    // Enable RAM writes
    gbcam_set_ram_access(1);
    // Set up the bank for transfer
    gbcam_set_ram_bank(current_bank);
    for(uint32_t buf_cursor = 0; buf_cursor < num; buf_cursor++){
        // Determine if we need to bankswitch or not
        if(ram_cursor >= SRAM_BANK_SIZE){
            // Switch banks if we cross a boundary
            current_bank++;
            gbcam_set_ram_bank(current_bank);
            // If we bankswitch, we start over again at the beginning of the the bank
            ram_cursor = 0;
        }
        writeb(buf[buf_cursor], ram_cursor + SRAM_START_ADDR);
        ram_cursor++;
    }
    // Disable RAM writes
    gbcam_set_ram_access(0);
}

void gbcam_rom_dump(uint8_t *buf, uint8_t start_bank, uint8_t end_bank){
    // Iterate over the range of banks we want to dump
    for(uint16_t bank_offset = start_bank; bank_offset <= end_bank; bank_offset++){
        // Set the active bank in the cart
        gbcam_set_rom_bank(bank_offset);
        // Iterate over the full address space of a ROM bank
        for(uint16_t byte_offset = 0; byte_offset < ROM_BANK_SIZE; byte_offset++){
            // Read back banked ROM sequentially into our buffer
            // Remember, you can set banked rom to bank 0, so we don't even touch bank 0 address space
            uint8_t d = readb(byte_offset + ROM_BANKN_START_ADDR);
            // printf("0x%x: 0x%x\n", ((bank_offset - start_bank) * ROM_BANK_SIZE) + byte_offset, d);
            // |-----------Calculate bank offset-----------|-Add byte offset-|      |---Read back from banked ROM-----|
            buf[((bank_offset - start_bank) * ROM_BANK_SIZE) + byte_offset] = readb(byte_offset + ROM_BANKN_START_ADDR);
        }
    }
}

void gbcam_ram_dump(uint8_t *buf, uint8_t start_bank, uint8_t end_bank){
    gbcam_set_ram_access(1);
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
    gbcam_set_ram_access(0);
}

void gbcam_set_rom_bank(uint16_t bank){
    writeb(bank & 0x3F, GBCAM_ROM_BANK_ADDR);
}
void gbcam_set_ram_bank(uint16_t bank){
    writeb(bank, GBCAM_RAM_BANK_ADDR);
}

void gbcam_set_ram_access(uint8_t on_off){
    if(on_off){
        writeb(GBCAM_ENABLE_RAM_WRITE_DATA, GBCAM_ENABLE_RAM_WRITE_ADDR);
        return;
    }
    writeb(0x0, GBCAM_ENABLE_RAM_WRITE_ADDR);

}

void gbcam_pull_photo(uint8_t *buf, uint8_t num){
    // Copy the bitmap header
    bufncpy(buf, bmp_header, 0x76);
    uint16_t buf_head = 0x76;
    // Two photos per bank
    gbcam_set_ram_bank(num / 2);
    // First photo at 0xAD0E in bank, second at 0xBD0E
    uint16_t sram_offset = ((num % 2) * 0x1000) + GBCAM_IMAGE_START_ADDR + SRAM_START_ADDR;
    // printf("Pulling photo %i from bank %i at starting address 0x%x\n", num, num/2, sram_offset);
    // 13 vertical blocks of 8 x 8 pixels
	for (uint8_t v = 0; v < 14; v++) {
		// 8 Lines
		for (uint8_t l = 0; l < 8; l++) {
			// One line
			for (uint8_t x = 0; x < 16; x++) {
				// 1st byte stores whether the pixel is white (0) or silver (1)
				// 2nd byte stores whether the pixel is white (0), grey (1, if the bit in the first byte is 0) and black (1, if the bit in the first byte is 1).
				uint8_t pixels_white_silver = readb(sram_offset);
				uint8_t pixels_grey_black = readb(sram_offset+1);
				
				uint8_t eight_pixels[4];
				uint8_t ep_counter = 0;
				uint8_t temp_byte = 0;
				
				for (int8_t p = 7; p >= 0; p--) {
					// 8 bit BMP colour depth, each nibble is 1 pixel
					if ((pixels_white_silver & 1<<p) && (pixels_grey_black & 1<<p)) {
						temp_byte |= 0x00; // Black
					}
					else if (pixels_white_silver & 1<<p) {
						temp_byte |= 0x08; // Silver
					}
					else if (pixels_grey_black & 1<<p) {
						temp_byte |= 0x07; // Grey
					}
					else {
						temp_byte |= 0x0F; // White
					}
					
					// For odd bits, shift the result left by 4 and save the result to our buffer on even bits
					if (p % 2 == 0) {
						eight_pixels[ep_counter] = temp_byte;
						ep_counter++;
						temp_byte = 0; // Reset byte
					}
					else {
						temp_byte <<= 4;
					}
				}
                for(uint8_t i = 0; i < 4; i++){
                    buf[buf_head] = eight_pixels[i];
                    buf_head++;
                }
				//fwrite(eight_pixels, 1, 4, bmpFile);
				sram_offset += 0x10;
			}
			sram_offset -= 0x102;
		}
		sram_offset -= 0xF0;
	}
}