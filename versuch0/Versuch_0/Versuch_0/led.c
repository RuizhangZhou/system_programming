/*
 * led.c
 *
 * Created: 22.04.2021 23:40:11
 *  Author: ap039658
 */ 

#include "led.h"
#include "button.h"

#include <avr/io.h>

uint8_t result = 0;

//!Initialize LEDs
void led_init() {
	
	DDRD = 0b11111111;
	PORTD = 0b11111111;
	
}

//!Functions for LEDs if some button is pressed
void led_fun() {
	
	//Init
	PORTD = ~result;
	
	if(getInput() == 0b1) {
		waitForNoInput();
		result = result ^ 0b1;
	}
	
	if(getInput() == 0b10) {
		waitForNoInput();
		uint8_t last_pin = result & 0b10000000;
		result = result << 1;
		if(last_pin) {
			result |= 0b1;
		}
	}
	
	if(getInput() == 0b100) {
		waitForNoInput();
		result = ~result;
	}
	
}
