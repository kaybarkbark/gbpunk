#ifndef GBUTILS_H_
#define GBUTILS_H_

#include "stdio.h"
#include "stdint.h"

void hexdump(uint8_t *data, uint16_t len, uint16_t start_address);
char byte_to_printable(uint8_t byte_in);

#endif