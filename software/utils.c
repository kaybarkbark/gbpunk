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