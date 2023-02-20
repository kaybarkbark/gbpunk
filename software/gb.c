#include <stdio.h>
#include <string.h>
#include <math.h>
#include "pico/stdlib.h"
#include "gb.h"
#include "pins.h"

const uint8_t logo[] = {0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B, 0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D,
						0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E, 0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99,
						0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC, 0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E};
uint8_t working_mem[0x8000] = {0};

void get_cart_info(struct Cart *cart){
    // Get the cartridge type
    cart->cart_type = readb(CART_TYPE_ADDR);
    // Get the human readible name for the cart type
    memset(cart->cart_type_str, 0, 30);
    // FIXME for some reason this replaces the first char with 0. No idea why
    switch(cart->cart_type){
        case 0: strncpy(cart->cart_type_str, "ROM ONLY", 8); cart->mapper_type = MAPPER_ROM_ONLY; break;
        case 1: strncpy(cart->cart_type_str, "MBC1", 4); cart->mapper_type = MAPPER_MBC1; break;
        case 2: strncpy(cart->cart_type_str, "MBC1+RAM", 8); cart->mapper_type = MAPPER_MBC1; break;
        case 3: strncpy(cart->cart_type_str, "MBC1+RAM+BATTERY", 16); cart->mapper_type = MAPPER_MBC1; break;
        case 5: strncpy(cart->cart_type_str, "MBC2", 4); cart->mapper_type = MAPPER_MBC2; break;
        case 6: strncpy(cart->cart_type_str, "MBC2+BATTERY", 12); cart->mapper_type = MAPPER_MBC2; break;
        case 8: strncpy(cart->cart_type_str, "ROM+RAM", 7); cart->mapper_type = MAPPER_ROM_RAM; break;
        case 9: strncpy(cart->cart_type_str, "ROM ONLY", 8); cart->mapper_type = MAPPER_ROM_ONLY; break;
        case 11: strncpy(cart->cart_type_str, "MMM01", 5); cart->mapper_type = MAPPER_MMM01; break;
        case 12: strncpy(cart->cart_type_str, "MMM01+RAM", 9); cart->mapper_type = MAPPER_MMM01; break;
        case 13: strncpy(cart->cart_type_str, "MMM01+RAM+BATTERY", 17); cart->mapper_type = MAPPER_MMM01; break;
        case 15: strncpy(cart->cart_type_str, "MBC3+TIMER+BATTERY", 18); cart->mapper_type = MAPPER_MBC3; break;
        case 16: strncpy(cart->cart_type_str, "MBC3+TIMER+RAM+BATTERY", 22); cart->mapper_type = MAPPER_MBC3; break;
        case 17: strncpy(cart->cart_type_str, "MBC3", 4); cart->mapper_type = MAPPER_MBC3; break;
        case 18: strncpy(cart->cart_type_str, "MBC3+RAM", 8); cart->mapper_type = MAPPER_MBC3; break;
        case 19: strncpy(cart->cart_type_str, "MBC3+RAM+BATTERY", 16); cart->mapper_type = MAPPER_MBC3; break;
        case 21: strncpy(cart->cart_type_str, "MBC4", 4); cart->mapper_type = MAPPER_MBC4; break;
        case 22: strncpy(cart->cart_type_str, "MBC4+RAM", 8); cart->mapper_type = MAPPER_MBC4; break;
        case 23: strncpy(cart->cart_type_str, "MBC4+RAM+BATTERY", 16); cart->mapper_type = MAPPER_MBC4; break;
        case 25: strncpy(cart->cart_type_str, "MBC5", 4); cart->mapper_type = MAPPER_MBC5; break;
        case 26: strncpy(cart->cart_type_str, "MBC5+RAM", 8); cart->mapper_type = MAPPER_MBC5; break;
        case 27: strncpy(cart->cart_type_str, "MBC5+RAM+BATTERY", 16); cart->mapper_type = MAPPER_MBC5; break;
        case 28: strncpy(cart->cart_type_str, "MBC5+RUMBLE", 11); cart->mapper_type = MAPPER_MBC5; break;
        case 29: strncpy(cart->cart_type_str, "MBC5+RUMBLE+RAM", 15); cart->mapper_type = MAPPER_MBC5; break;
        case 30: strncpy(cart->cart_type_str, "MBC5+RUMBLE+RAM+BATTERY", 23); cart->mapper_type = MAPPER_MBC5; break;
        case 252: strncpy(cart->cart_type_str, "GB CAMERA", 9); cart->mapper_type = MAPPER_GBCAM; break;
        default: strncpy(cart->cart_type_str, "UNKNOWN MAPPER", 14); cart->mapper_type = MAPPER_UNKNOWN; break;
    }
    // Calculate ROM banks
    cart->rom_banks = 2 << readb(ROM_BANK_SHIFT_ADDR);
    cart->rom_size_bytes = ROM_BANK_SIZE * cart->rom_banks; // Even ROM Only will report two banks
    // RAM banks are random-ish, need lookup
    uint8_t ram_size = readb(RAM_BANK_COUNT_ADDR);
    // Handle MBC2 w/ battery backed RAM. Only 512 bytes
    if(cart->cart_type == 6){
        cart->ram_banks = 1;
        cart->ram_end_address = 0xA1FF;
        cart->ram_size_bytes = SRAM_START_ADDR - 0xA1FF + 1;
    }
    // Handle 2K, don't need the full RAM address space
    else if(ram_size == 2){
        cart->ram_banks = 1;
        cart->ram_end_address = 0xA7FF;
        cart->ram_size_bytes = SRAM_START_ADDR - 0xA7FF + 1;
    }
    // All others will use the full address space
    else if(ram_size == 3){
        cart->ram_banks = 4;
        cart->ram_end_address = SRAM_END_ADDR;
        cart->ram_size_bytes = cart->ram_banks * SRAM_BANK_SIZE;
    }
    else if(ram_size == 4){
        cart->ram_banks = 16;
        cart->ram_end_address = SRAM_END_ADDR;
        cart->ram_size_bytes = cart->ram_banks * SRAM_BANK_SIZE;
    }
    else if(ram_size == 5){
        cart->ram_banks = 8;
        cart->ram_end_address = SRAM_END_ADDR;
        cart->ram_size_bytes = cart->ram_banks * SRAM_BANK_SIZE;
    }
    // No RAM
    else{
        cart->ram_banks = 0;
        cart->ram_end_address = SRAM_START_ADDR;
        cart->ram_size_bytes = 0;
    }
    // Read back the title
    for(uint8_t i = 0; i < CART_TITLE_LEN; i++){
        char c = readb(CART_TITLE_ADDR + i);
        // If unprintable, we hit the end. Null terminate
        if((c < 0x20) || (c > 0x7e)){
            cart->title[i] = 0;
            break;
        }
        cart->title[i] = c;
    }
    cart->title[CART_TITLE_LEN] = 0; // Ensure null terminated
}

void dump_cart_info(struct Cart *cart){
    printf("Cart Type: %s (0x%x)\n", cart->cart_type_str, cart->cart_type);
    printf("Num ROM Banks: %i\n", cart->rom_banks);
    printf("Num RAM Banks: %i\n", cart->ram_banks);
    printf("RAM Ending Address: 0x%x\n", cart->ram_end_address);
    printf("Cart Title: %s\n", cart->title);
}

uint16_t cart_check(uint8_t *logo_buf){
    readbuf(LOGO_START_ADDR, logo_buf, LOGO_LEN);
    for(uint8_t i = 0; i < LOGO_LEN; i++){
        if(logo_buf[i] != logo[i]){
            return i + LOGO_START_ADDR;
        }
    }
    return 0;
}


void reset_cart(){
    reset_pin_states();
    gpio_put(RST, 0);
    pulse_clock();
    gpio_put(RST, 1);
    pulse_clock();
}

void pulse_clock(){
    gpio_put(CLK, 0);
    sleep_us(1);
    gpio_put(CLK, 1);
    sleep_us(1);
    gpio_put(CLK, 0);
}


void init_bus(){
    // Init the address pins, always out
    gpio_init(A0);
    gpio_set_dir(A0, GPIO_OUT);

    gpio_init(A1);
    gpio_set_dir(A1, GPIO_OUT);

    gpio_init(A2);
    gpio_set_dir(A2, GPIO_OUT);

    gpio_init(A3);
    gpio_set_dir(A3, GPIO_OUT);

    gpio_init(A4);
    gpio_set_dir(A4, GPIO_OUT);

    gpio_init(A5);
    gpio_set_dir(A5, GPIO_OUT);

    gpio_init(A6);
    gpio_set_dir(A6, GPIO_OUT);

    gpio_init(A7);
    gpio_set_dir(A7, GPIO_OUT);

    gpio_init(A8);
    gpio_set_dir(A8, GPIO_OUT);
    
    gpio_init(A9);
    gpio_set_dir(A9, GPIO_OUT);

    gpio_init(A10);
    gpio_set_dir(A10, GPIO_OUT);

    gpio_init(A11);
    gpio_set_dir(A11, GPIO_OUT);

    gpio_init(A12);
    gpio_set_dir(A12, GPIO_OUT);

    gpio_init(A13);
    gpio_set_dir(A13, GPIO_OUT);

    gpio_init(A14);
    gpio_set_dir(A14, GPIO_OUT);

    gpio_init(A15);
    gpio_set_dir(A15, GPIO_OUT);

    // Init the control pins, always out
    gpio_init(RST);
    gpio_set_dir(RST, GPIO_OUT);

    gpio_init(CS);
    gpio_set_dir(CS, GPIO_OUT);

    gpio_init(RD);
    gpio_set_dir(RD, GPIO_OUT);

    gpio_init(WR);
    gpio_set_dir(WR, GPIO_OUT);

    gpio_init(CLK);
    gpio_set_dir(CLK, GPIO_OUT);

    // Init the data pins, bidirectional
    // Init them as IN though
    gpio_init(D0);
    gpio_init(D1);
    gpio_init(D2);
    gpio_init(D3);
    gpio_init(D4);
    gpio_init(D5);
    gpio_init(D6);
    gpio_init(D7);
    set_dbus_direction(GPIO_IN);

    // Init the state of all the pins
    reset_pin_states();
}

void reset_pin_states(){
    // Address pins
    gpio_put(A0, 0);
    gpio_put(A1, 0);
    gpio_put(A2, 0);
    gpio_put(A3, 0);
    gpio_put(A4, 0);
    gpio_put(A5, 0);
    gpio_put(A6, 0);
    gpio_put(A7, 0);
    gpio_put(A8, 0);
    gpio_put(A9, 0);
    gpio_put(A10, 0);
    gpio_put(A11, 0);
    gpio_put(A12, 0);
    gpio_put(A13, 0);
    gpio_put(A14, 0);
    gpio_put(A15, 0);

    set_dbus_direction(GPIO_IN);

    // Control pins
    gpio_put(RST, 1);
    gpio_put(CS, 1);
    gpio_put(RD, 1);
    gpio_put(WR, 1);
    gpio_put(CLK, 1);
}

void writeb(uint8_t data, uint16_t addr){
    // init_bus();  // Put this here cause I was an idiot with tusb
    // TODO: ensure clock always starts low, ends low. First thing should be posedge clock
    // Set the clock high
    gpio_put(CLK, 1);
    // Set CS high. I know it seems wierd for this to go here, it's high from the previous transaction.
    gpio_put(CS, 1);
    // 140 ns
    sleep_us(1);
    // Set read high
    gpio_put(RD, 1);
    // Set the address
    gpio_put(A0, addr & (0x1 << 0));
    gpio_put(A1, addr & (0x1 << 1));
    gpio_put(A2, addr & (0x1 << 2));
    gpio_put(A3, addr & (0x1 << 3));
    gpio_put(A4, addr & (0x1 << 4));
    gpio_put(A5, addr & (0x1 << 5));
    gpio_put(A6, addr & (0x1 << 6));
    gpio_put(A7, addr & (0x1 << 7));
    gpio_put(A8, addr & (0x1 << 8));
    gpio_put(A9, addr & (0x1 << 9));
    gpio_put(A10, addr & (0x1 << 10));
    gpio_put(A11, addr & (0x1 << 11));
    gpio_put(A12, addr & (0x1 << 12));
    gpio_put(A13, addr & (0x1 << 13));
    gpio_put(A14, addr & (0x1 << 14));
    gpio_put(A15, addr & (0x1 << 15));
    // 240 ns
    sleep_us(2);
    // Set CS low if we are doing a RAM access
    // Revert back to always set low if everything brakes
    if(addr >= SRAM_START_ADDR){
        gpio_put(CS, 0);
    }
    // 480 ns
    sleep_us(1);
    // Set clock low
    gpio_put(CLK, 0);
    // Set WR low
    gpio_put(WR, 0);
    // Drive the data bus
    set_dbus_direction(GPIO_OUT);
    gpio_put(D0, data & (0x1 << 0));
    gpio_put(D1, data & (0x1 << 1));
    gpio_put(D2, data & (0x1 << 2));
    gpio_put(D3, data & (0x1 << 3));
    gpio_put(D4, data & (0x1 << 4));
    gpio_put(D5, data & (0x1 << 5));
    gpio_put(D6, data & (0x1 << 6));
    gpio_put(D7, data & (0x1 << 7));
    // 840 ns
    sleep_us(2);
    // Set WR high
    gpio_put(WR, 1);
    // 960ns ns
    sleep_us(1);
    // Set CLK high
    gpio_put(CLK, 1);
    gpio_put(CS, 1); // added for gbcam, remove if mbc5 no longer works
    // Stop driving bus
    set_dbus_direction(GPIO_IN);
    // Maybe put cs = 1 down here, if other things break. 
    sleep_us(1);
}
uint8_t readb(uint16_t addr){
    // init_bus();  // Put this here cause I was an idiot with tusb
    // TODO: Ensure clock always starts low, ends low
    // Pulse clock, ensure we get a posedge
    //gpio_put(CLK, 0);
    //sleep_us(1);
    gpio_put(CLK, 1);
    // Set CS high. I know it seems wierd for this to go here, it's high from the previous transaction.
    gpio_put(CS, 1);
    // Sleep ??? ns
    sleep_us(1);
    // Set read low
    gpio_put(RD, 0);
    // Ensure write is high
    // gpio_put(WR, 1);
    // Set direction accordingly
    set_dbus_direction(GPIO_IN);
    // Sleep ??? ns
    sleep_us(1);
    // Put address on bus
    gpio_put(A0, addr & (0x1 << 0));
    gpio_put(A1, addr & (0x1 << 1));
    gpio_put(A2, addr & (0x1 << 2));
    gpio_put(A3, addr & (0x1 << 3));
    gpio_put(A4, addr & (0x1 << 4));
    gpio_put(A5, addr & (0x1 << 5));
    gpio_put(A6, addr & (0x1 << 6));
    gpio_put(A7, addr & (0x1 << 7));
    gpio_put(A8, addr & (0x1 << 8));
    gpio_put(A9, addr & (0x1 << 9));
    gpio_put(A10, addr & (0x1 << 10));
    gpio_put(A11, addr & (0x1 << 11));
    gpio_put(A12, addr & (0x1 << 12));
    gpio_put(A13, addr & (0x1 << 13));
    gpio_put(A14, addr & (0x1 << 14));
    gpio_put(A15, addr & (0x1 << 15));
    // Sleep ??? ms
    sleep_us(2);
    // Set CS low
    // TODO: Don't need this for ROM access I think
    gpio_put(CS, 0);
    // Sleep ??? ns
    // Set clock low
    gpio_put(CLK, 0);
    /// Sleep ??? ns, wait for data on the bus
    sleep_us(4);
    // Read back the data
    uint8_t data =
        (gpio_get(D0) << 0) |
        (gpio_get(D1) << 1) |
        (gpio_get(D2) << 2) |
        (gpio_get(D3) << 3) |
        (gpio_get(D4) << 4) |
        (gpio_get(D5) << 5) |
        (gpio_get(D6) << 6) |
        (gpio_get(D7) << 7);
    // Sleep ??? ms
    // Maybe put cs = 1 down here, if other things break. 
    sleep_us(1);
    return data;
}

void readbuf(uint16_t addr, uint8_t *buf, uint16_t len){
    for(uint16_t i = 0; i < len; i++){
        buf[i] = readb(addr + i);
    }
}

void set_dbus_direction(uint8_t dir){
    gpio_set_dir(D0, dir);
    gpio_set_dir(D1, dir);
    gpio_set_dir(D2, dir);
    gpio_set_dir(D3, dir);
    gpio_set_dir(D4, dir);
    gpio_set_dir(D5, dir);
    gpio_set_dir(D6, dir);
    gpio_set_dir(D7, dir);
}


uint16_t fs_get_rom_bank(uint16_t addr){
    return addr / ROM_BANK_SIZE; 
}

uint16_t fs_get_ram_bank(uint16_t addr){
    return addr / SRAM_BANK_SIZE; 
}