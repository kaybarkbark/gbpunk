#ifndef MSC_DISK_H
#define MSC_DISK_H

#include <stdint.h>
#include "gb.h"
#include "cart.h"

void init_disk_mem();
void init_disk();
void append_status_file(const uint8_t* buf);
void append_status_file_buf(uint8_t* buf);
#endif