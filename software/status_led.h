#ifndef GBPUNK_STATUS_LED_H
#define GBPUNK_STATUS_LED_H
#include <stdint.h>

#define LED_SPEED_ERR       200000
#define LED_SPEED_TESTING   50000
#define LED_SPEED_HEALTHY   0

void init_led_irq();
void set_led_speed(uint32_t speed);

#endif