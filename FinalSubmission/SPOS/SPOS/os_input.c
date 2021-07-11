#include <avr/io.h>
#include <stdint.h>
#include "os_input.h"

/*!
 *  A simple "Getter"-Function for the Buttons on the evaluation board.\n
 *
 *  \returns The state of the button(s) in the lower bits of the return value.\n
 *  example: 1 Button:  -pushed:   00000001
 *                      -released: 00000000
 *           4 Buttons: 1,3,4 -pushed: 000001101
 *
 */
uint8_t os_getInput(void) {
	uint8_t pinc = ~PINC;
	//when button is pressed this bit is 0,so need to (~PINC) then & 0b11000011
	uint8_t upperBytes = (pinc & 0b11000000) >> 4;
	return ((pinc & 0b00000011) | upperBytes);
}

/*!
 *  A simple "Getter"-Function for the Buttons on the evaluation board.\n
 *
 *  \returns The state of the button(s) in the lower bits of the return value.\n
 *  example: 1 Button:  -pushed:   00000001
 *                      -released: 00000000
 *           4 Buttons: 1,3,4 -pushed: 000001101
 *
 */
void os_initInput() {
	// Button Pins auf "Eingang" stellen
	DDRC &= 0b00111100;

	// Pull-UP Widerst�nde aktivieren
	PORTC |= 0b11000011;
}


/*!
 *  Endless loop until at least one button is pressed.
 */
void os_waitForInput() {
	// Mindestens ein Knopf gedr�ckt <=> Nicht kein Knopf gedr�ckt
	while(os_getInput() == 0b00000000){
		//weiter warten
	}
}

/*!
 *  Endless loop as long as at least one button is pressed.
 */
void os_waitForNoInput() {
	// Mindestens ein Knopf gedr�ckt <=> Nicht kein Knopf gedr�ckt
	while(os_getInput() != 0b00000000){
		//weiter warten
	}
}