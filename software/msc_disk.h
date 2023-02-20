#ifndef MSC_DISK_H
#define MSC_DISK_H

#include <stdint.h>
#include "gb.h"

void init_disk(struct Cart* cart);
void new_file(uint8_t* name, uint32_t size);
void set_disk_size(uint32_t size);

#endif