/*
 * led.h
 *
 * Created: 22.04.2021 23:40:31
 *  Author: ap039658
 */ 


#ifndef LED_H_
#define LED_H_

#include <stdint.h>

//----------------------------------------------------------------------------
// Function headers
//----------------------------------------------------------------------------

//!Initialize LEDs
void led_init(void);

//!Functions for LEDs if some button is pressed
void led_fun(void);

#endif /* LED_H_ */