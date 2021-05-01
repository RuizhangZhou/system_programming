#include "os_input.h"

#include <avr/io.h>
#include <stdint.h>

/*! \file

Everything that is necessary to get the input from the Buttons in a clean format.

*/

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
    uint8_t upperBytes = (pinc & 0b11000000) >> 4;
    return ((pinc & 0b00000011) | upperBytes);
}

/*!
 *  Initializes DDR and PORT for input
 */
void os_initInput() {
    //Set Data Direction Register of used pins of PortC to input
    DDRC &= (0b00111100);

    //Set Pullups
    PORTC |= 0b11000011;
}

/*!
 *  Endless loop as long as at least one button is pressed.
 */
void os_waitForNoInput() {
    while(os_getInput() != 0b00000000);
}

/*!
 *  Endless loop until at least one button is pressed.
 */
void os_waitForInput() {
    // Wait as long as no button is pushed and then just return
    // Note that the buttons are active low
    while(os_getInput() == 0b00000000){
	    // wait here
    }
}