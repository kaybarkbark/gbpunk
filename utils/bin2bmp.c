#include <stdio.h>
#include "stdint.h"
uint8_t bmpStart[0x76] = {0x42, 0x4D, 0x76, 0x1C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x76, 0x00, 0x00, 0x00, 
	0x28, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x70, 0x00, 0x00, 0x00, 0x01, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x1C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x10, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x80, 0x00, 0x80, 
	0x00, 0x00, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x80, 0x00, 0x00, 0x80, 0x80, 0x80, 0x00, 0xC0, 0xC0, 0xC0, 0x00, 
	0x00, 0x00, 0xFF, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0xFF, 
	0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00};

uint8_t savBuffer[3584];

// Converts .sav file to BMP files
int main () {
    FILE *fp = fopen("sram.bin", "r");
    if (fp != NULL) {
        size_t newLen = fread(savBuffer, sizeof(char), 3584, fp);
        if ( ferror( fp ) != 0 ) {
            fputs("Error reading file", stderr);
        } else {
            savBuffer[newLen++] = '\0'; /* Just to be safe. */
        }

        fclose(fp);
    }
	FILE *bmpFile = fopen("outs.bmp", "wb");
	fwrite(bmpStart, 1, 0x76, bmpFile); // Write start of BMP file
	
	int currentByte = 0xD0E;
	
	// 13 vertical blocks of 8 x 8 pixels
	for (uint8_t v = 0; v < 14; v++) {
		
		// 8 Lines
		for (uint8_t l = 0; l < 8; l++) {
			
			// One line
			for (uint8_t x = 0; x < 16; x++) {
				
				// 1st byte stores whether the pixel is white (0) or silver (1)
				// 2nd byte stores whether the pixel is white (0), grey (1, if the bit in the first byte is 0) and black (1, if the bit in the first byte is 1).
				uint8_t pixelsWhiteSilver = savBuffer[currentByte];
				uint8_t pixelsGreyBlack = savBuffer[currentByte+1];
				
				uint8_t eightPixels[4];
				uint8_t epCounter = 0;
				uint8_t tempByte = 0;
				
				for (int8_t p = 7; p >= 0; p--) {
					// 8 bit BMP colour depth, each nibble is 1 pixel
					if ((pixelsWhiteSilver & 1<<p) && (pixelsGreyBlack & 1<<p)) {
						tempByte |= 0x00; // Black
					}
					else if (pixelsWhiteSilver & 1<<p) {
						tempByte |= 0x08; // Silver
					}
					else if (pixelsGreyBlack & 1<<p) {
						tempByte |= 0x07; // Grey
					}
					else {
						tempByte |= 0x0F; // White
					}
					
					// For odd bits, shift the result left by 4 and save the result to our buffer on even bits
					if (p % 2 == 0) {
						eightPixels[epCounter] = tempByte;
						epCounter++;
						tempByte = 0; // Reset byte
					}
					else {
						tempByte <<= 4;
					}
				}
				
				fwrite(eightPixels, 1, 4, bmpFile);
				currentByte += 0x10;
			}
			
			currentByte -= 0x102;
		}
		
		currentByte -= 0xF0;
	}
	
	fclose(bmpFile);
}
