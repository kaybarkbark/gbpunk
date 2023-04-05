#include "scratch.h"
#include "gb.h"
#include "mappers/huc1.h"
#include <stdio.h>

#define DATA 0x66
#define RAM_ADDR (SRAM_START_ADDR + 0xF)
#define NUM 8

void scratch_workspace(){
    uint8_t write_buf[8] = {0, 1, 2, 3, 4, 5, 6, 7};
    uint8_t buf[8] = {0, 1, 2, 3, 4, 5, 6, 7};
    uint8_t read_buf[8] = {0};
    uint8_t volatile initial, random, first_read, second_read;
    huc1_set_ram_access(1);
    writeb(0xDE, 0xAABB);
    initial = readb(0xAABB);
    printf("0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, ", initial, random, first_read, initial, second_read, read_buf[0], write_buf[0]);
    huc1_set_ram_access(0);
}