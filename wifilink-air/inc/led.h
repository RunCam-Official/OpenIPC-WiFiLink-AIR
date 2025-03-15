#ifndef _LED_H_
#define _LED_H_

#include <unistd.h>
#include <stdbool.h>

#define BLUE_LED (6)
#define GREEN_LED (7)

typedef enum {
    LED_OFF,
    LED_ON,
    LED_BLINK,
    LED_BLINK_INFINITE,
    LED_BLINK_ALTERNATE
} LEDState;

typedef enum {
    LED_UNDEFINED,
    LED_BLUE,
    LED_GREEN,
    LED_ALL
} LEDcolor;

typedef struct {
    LEDState state;
    LEDcolor color;
    unsigned int blink_time_ms;
    unsigned int blink_count;
    unsigned int current_blink;
    bool running;
} LEDControl;

void wifilink_led_start_thread(void);

void wifilink_led_stop_thread(void);

void wifilink_init_led(void);

void init_led(LEDControl *control);

void set_led_state(LEDControl *control, LEDState state, LEDcolor color, unsigned int blink_time_ms, unsigned int blink_count);

void update_led(LEDControl *control);

void audio_state_led(void);

void records_state_led(void);

#endif //#define _LED_H_