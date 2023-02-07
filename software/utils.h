#ifndef GBUTILS_H_
#define GBUTILS_H_

#include "stdio.h"
#include "stdint.h"

// Nicely hexdump some data to the terminal, like xxd
void hexdump(uint8_t *data, uint16_t len, uint16_t start_address);
// Convert a byte to an ascii printable char. Replace nonprintables with '.'
char byte_to_printable(uint8_t byte_in);
// Return 0 if the bufs of size len are the same, 1 otherwise
uint8_t bufncmp(uint8_t *b1, uint8_t *b2, uint16_t len);
// Copy one buf to anoether of size len
void bufncpy(uint8_t *dest, uint8_t *src, uint16_t len);
#endif