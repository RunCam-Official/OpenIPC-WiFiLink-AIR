#!/bin/sh

GPIO_PIN=7

delay_ms=500
BLINK_INTERVAL=$(awk -v ms="$delay_ms" 'BEGIN{print ms/1000}')

if [ ! -e "/sys/class/gpio/gpio$GPIO_PIN" ]; then
    echo "$GPIO_PIN" > /sys/class/gpio/export
fi

echo "out" > /sys/class/gpio/gpio$GPIO_PIN/direction

set_gpio_value() {
    value=$1
    if [ "$value" = "on" ]; then
        echo "1" > /sys/class/gpio/gpio$GPIO_PIN/value
    elif [ "$value" = "off" ]; then
        echo "0" > /sys/class/gpio/gpio$GPIO_PIN/value
    else
        echo "Invalid value. Use 'on' or 'off'."
        exit 1
    fi
}

blink_led() {
    while true; do
        set_gpio_value on
        sleep $BLINK_INTERVAL
        set_gpio_value off
        sleep $BLINK_INTERVAL
    done
}

blink_led
