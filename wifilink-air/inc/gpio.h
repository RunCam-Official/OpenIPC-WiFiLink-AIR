#ifndef _GPIO_H_
#define _GPIO_H_

#include <stdio.h>
#include <unistd.h>
#include <string.h>

//==============================================================================
//
//                              MACRO DEFINES
//
//==============================================================================

#define GPIO_DIR_OUT "out"
#define GPIO_DIR_IN "in"

#define GPIO_HIGH (1)
#define GPIO_LOW (0)

//==============================================================================
//
//                              FUNCTIONS
//
//==============================================================================
int wifilink_gpio_export(int gpioNum);
int wifilink_gpio_unexport(int gpioNum);
int wifilink_gpio_set_direction(int gpioNum, const char* direct);
int wifilink_gpio_get_val(int gpioNum);
int wifilink_gpio_set_val(int gpioNum, int value);

#endif //#define _GPIO_H_