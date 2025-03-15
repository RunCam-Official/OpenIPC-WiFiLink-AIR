#include <stdio.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/resource.h>
#include "led.h"
#include "gpio.h"
#include "configuration.h"
#include "storage.h"
#include "debug.h"

LEDControl control;
pthread_t wifilink_led_thread = 0;
pthread_attr_t led_thread_attr;

bool wifilink_led_thread_exit = true;

extern AirParam_s airParam;

void init_led(LEDControl *control) {
    control->state = LED_OFF;
    control->color = LED_UNDEFINED;
    control->blink_time_ms = 0;
    control->blink_count = 0;
    control->current_blink = 0;
    control->running = false;
}

void set_led_state(LEDControl *control, LEDState state, LEDcolor color, unsigned int blink_time_ms, unsigned int blink_count) {
    control->state = state;
    control->color = color;
    control->blink_time_ms = blink_time_ms;
    control->blink_count = blink_count;
    control->current_blink = 0;
    control->running = true;
}

void audio_state_led(void)
{
    log_debug("airParam.maj_audio_enable = %s", airParam.maj_audio_enable);
    if(strcmp(airParam.maj_audio_enable, "true") == 0)
        set_led_state(&control, LED_ON, LED_GREEN, 0, 0);
    else
        set_led_state(&control, LED_OFF, LED_GREEN, 0, 0);
}

void records_state_led(void)
{
    log_debug("airParam.maj_video_records = %s", airParam.maj_video_records);
    if(storage_insert_state() && strcmp(airParam.maj_video_records, "true") == 0)
        set_led_state(&control, LED_BLINK_INFINITE, LED_GREEN, 1000, 0);
    else
        audio_state_led();
}

void update_led(LEDControl *control) {
    static int alternate = 0;
    
    if (!control->running) {
        return;
    }

    switch (control->state) {
        case LED_OFF:
            log_debug("LED is OFF\n");
            if(control->color == LED_BLUE)
                wifilink_gpio_set_val(BLUE_LED, GPIO_LOW);
            else if(control->color == LED_GREEN)
                wifilink_gpio_set_val(GREEN_LED, GPIO_LOW);
            else
            {
                wifilink_gpio_set_val(BLUE_LED, GPIO_LOW);
                wifilink_gpio_set_val(GREEN_LED, GPIO_LOW);
            }

            control->running = false;
            break;
        case LED_ON:
            log_debug("LED is ON\n");
            if(control->color == LED_BLUE)
                wifilink_gpio_set_val(BLUE_LED, GPIO_HIGH);
            else if(control->color == LED_GREEN)
                wifilink_gpio_set_val(GREEN_LED, GPIO_HIGH);
            else
            {
                wifilink_gpio_set_val(BLUE_LED, GPIO_HIGH);
                wifilink_gpio_set_val(GREEN_LED, GPIO_HIGH);
            }

            control->running = false;
            break;
        case LED_BLINK:
            if (control->current_blink < control->blink_count*2) {

                if(control->color == LED_BLUE)
                    wifilink_gpio_set_val(BLUE_LED, !wifilink_gpio_get_val(BLUE_LED));
                else
                    wifilink_gpio_set_val(GREEN_LED, !wifilink_gpio_get_val(GREEN_LED));

                usleep(control->blink_time_ms * 1000);

                control->current_blink++;
            } else {
                control->running = false;
                log_debug("LED has finished blinking %u times\n", control->blink_count);
            }

            break;
        case LED_BLINK_INFINITE:
            if(control->color == LED_BLUE)
                wifilink_gpio_set_val(BLUE_LED, !wifilink_gpio_get_val(BLUE_LED));
            else
                wifilink_gpio_set_val(GREEN_LED, !wifilink_gpio_get_val(GREEN_LED));

            usleep(control->blink_time_ms * 1000);
            break;
        case LED_BLINK_ALTERNATE:

            alternate = !alternate;

            if(alternate)
            {
                wifilink_gpio_set_val(BLUE_LED, GPIO_HIGH);
                wifilink_gpio_set_val(GREEN_LED, GPIO_LOW);
            }
            else
            {
                wifilink_gpio_set_val(BLUE_LED, GPIO_LOW);
                wifilink_gpio_set_val(GREEN_LED, GPIO_HIGH);
            }

            usleep(control->blink_time_ms * 1000);
            break;
        default:
            log_debug("Invalid LED state!\n");
            break;
    }
}

void wifilink_init_led(void)
{
    wifilink_gpio_export(BLUE_LED);
    wifilink_gpio_set_direction(BLUE_LED, "out");
    wifilink_gpio_set_val(BLUE_LED, GPIO_LOW);

    wifilink_gpio_export(GREEN_LED);
    wifilink_gpio_set_direction(GREEN_LED, "out");
    wifilink_gpio_set_val(GREEN_LED, GPIO_LOW);
    
    init_led(&control);

    wifilink_led_start_thread();
}

void *wifilink_led_task(void *argv)
{
    while (wifilink_led_thread_exit == false)
    {
        if(control.running)
        {
            update_led(&control);
        }

        usleep(20 * 1000);
    }
    pthread_exit(NULL);
}

void wifilink_led_start_thread(void)
{
    int ret = 0;
    
    if (false != wifilink_led_thread)
    {
        log_err("thread has start!\n");
        return;
    }

    wifilink_led_thread_exit = false;

    if(pthread_attr_init(&led_thread_attr) != 0)
    {
        log_err("pthread_attr_init.");
        return;
    }

    if(pthread_attr_setstacksize(&led_thread_attr, 1024*16))
    {
        log_err("pthread_attr_setstacksize.");
        return;
    }

    ret = pthread_create(&wifilink_led_thread, &led_thread_attr, wifilink_led_task, NULL);

    if (0 != ret)
    {
        log_err("pthread create failed!");
    }

    return;
}

void wifilink_led_stop_thread(void)
{
    wifilink_led_thread_exit = true;
    pthread_join(wifilink_led_thread, NULL);
    pthread_attr_destroy(&led_thread_attr);
}
