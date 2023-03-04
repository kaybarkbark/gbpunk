#include "status_led.h"
#include "pico/stdlib.h"
#include "pins.h"

#define ALARM_NUM 0
#define ALARM_IRQ TIMER_IRQ_0

static volatile bool led_status = 0;
static volatile uint32_t led_blink_speed = 0;

// Set the blinking speed of the LED via an IRQ. Disable the LED and set it solid if passed in no delay
void set_led_speed(uint32_t delay_us){
    // Enable if we pass in some delay
    led_blink_speed = delay_us;
    if(led_blink_speed){
        // Enable the alarm irq
        irq_set_enabled(ALARM_IRQ, true);
        // Enable interrupt in block and at processor

        // Alarm is only 32 bits so if trying to delay more
        // than that need to be careful and keep track of the upper
        // bits
        uint64_t target = timer_hw->timerawl + led_blink_speed;

        // Write the lower 32 bits of the target time to the alarm which
        // will arm it
        timer_hw->alarm[ALARM_NUM] = (uint32_t) target;
    }
    // Disable if no delay given
    else{
        // Disable the alarm irq
        irq_set_enabled(ALARM_IRQ, false);
        // Set the LED on and solid
        led_status = 0;
        gpio_put(STATUS_LED, led_status);
    }
}

void toggle_led_irq(){
    // Clear the irq
    hw_clear_bits(&timer_hw->intr, 1u << ALARM_NUM);
    // Toggle the LED
    led_status = !led_status;
    // Write the new status of the LED
    gpio_put(STATUS_LED, led_status);
    // Re-enable the timer
    set_led_speed(led_blink_speed);
}

// Initialize the LED and the IRQ that controls it. By default, the LED will be off
void init_led_irq(){
    gpio_init(STATUS_LED);
    gpio_set_dir(STATUS_LED, GPIO_OUT);
    // Micros are usually better at sourcing current rather than sinking it, so
    // the LED is toggled on by giving it a path to ground. This disables it
    gpio_put(STATUS_LED, 1);
    // Enable the interrupt for our alarm (the timer outputs 4 alarm irqs)
    hw_set_bits(&timer_hw->inte, 1u << ALARM_NUM);
    // Set irq handler for alarm irq
    irq_set_exclusive_handler(ALARM_IRQ, toggle_led_irq);
}