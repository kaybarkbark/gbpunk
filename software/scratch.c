#include "scratch.h"
#include "gb.h"
#include "mappers/mbc2.h"
#include <stdio.h>

#define DATA 0x66
#define RAM_ADDR (SRAM_START_ADDR + 0xF)
#define NUM 8

void scratch_workspace(){
    uint8_t write_buf[8] = {0, 1, 2, 3, 4, 5, 6, 7};
    uint8_t buf[8] = {0, 1, 2, 3, 4, 5, 6, 7};
    uint8_t read_buf[8] = {0};
    uint8_t initial, random, first_read, second_read;
        uint8_t num_iterations = 64;
        while(num_iterations){
            uint8_t addr = 0;
            uint8_t data = 0xFF;
            // uint8_t old_data = readb(SRAM_START_ADDR + addr);
            uint8_t old_data = 0;
            mbc2_memcpy_ram(&old_data, addr, 1);
            // writeb(data, SRAM_START_ADDR + addr);
            mbc2_memset_ram(&data, addr, 1);
            // volatile uint8_t changed_data = readb(SRAM_START_ADDR + addr);
            uint8_t changed_data = 0;
            mbc2_memcpy_ram(&changed_data, addr, 1);
            if(changed_data != data){
                printf("Failed!");
            }
            // writeb(old_data, SRAM_START_ADDR + addr);
            num_iterations--;
        }
    printf("0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, ", initial, random, first_read, initial, second_read, read_buf[0], write_buf[0]);
}