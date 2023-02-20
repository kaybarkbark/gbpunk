#include "utils.h"

void hexdump(uint8_t *data, uint16_t len, uint16_t start_address){
    for(uint16_t i = 0; i < len; i++){
        if(!(i%8)){
            printf("\n0x%04x: ", start_address);
            start_address += 8;
        }
        printf("0x%02x ", data[i]);
    }
    printf("\n");
}

char byte_to_printable(uint8_t byte_in){
    if(0x20 > byte_in > 0x7e){
        return '.';
    }
    return (char) byte_in;
}

uint8_t bufncmp(uint8_t *b1, uint8_t *b2, uint16_t len){
    uint16_t samechars = 0;
    for(uint8_t i = 0; i < len; i++){
        if(b1[i] == b2[i]){
            samechars++;
        }
    }
    if(samechars < len){
        return 0;
    }
    return 1;
}

void bufncpy(uint8_t *dest, uint8_t *src, uint16_t len){
    for(uint8_t i = 0; i < len; i++){
        dest[i] = src[i];
    }
}

void delay_wait(uint32_t cycles){
    // Each cycle is roughly 7 ns when running at default speed
    while(cycles){
        cycles--;
    }
}