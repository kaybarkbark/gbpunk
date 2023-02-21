#ifndef MSC_DISK_H
#define MSC_DISK_H

#include <stdint.h>
#include "gb.h"

void init_disk_mem();
void init_disk(struct Cart* cart);
void append_status_file(const uint8_t* buf);

#endif