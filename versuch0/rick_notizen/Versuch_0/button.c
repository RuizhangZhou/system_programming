#include "button.h"
#include "lcd.h"

#include <avr/io.h>
#include <stdint.h>
#include <util/delay.h>

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
 */
uint8_t getInput(void) {
    const uint8_t pressed = ~PINC & 0b11000011;//获取0,1,6,7位取反的值,其它位置置0
	//如果某个按钮按下去了(gedrueckt), PINC=0,~PINC=1,pressed=1
	//相反如果某个没按下去(ungedrueckt),PINC=1,~PINC=0,pressed=0
	return (pressed & 0b00000011) | (pressed >> 4);
	//(pressed & 0b00000011)这个是保留1,2位,其余置0;
	//(pressed >> 4)这个相当于把7,8位移到了1,2位,其余位都是0
	//两个| 就是相当于分别用0,1,2,3表示了按钮1,2,3,4
}

/*!
 *	Initializes DDR and PORT for input
 */
void initInput(){
	//Set Data Direction Register of used pins of PortC to input
	DDRC &= ~(0b11000011);//DDRC &= 0b00111100;是一样的吗?都是将0,1,6,7位置0?其它位置保持原来的值不变?

	//Set Pullups
	PORTC |= 0b11000011;
}

/*!
 *  Endless loop as long as at least one button is pressed.
 */
void waitForNoInput() {
	while((PINC & 0b11000011) != 0b11000011);
	//这里应该是等待所有button都释放了,即0,1,6,7位都为1
}


void waitForInput() {
	//Wait as long as no button is pushed and then just return
	//Note that the buttons are active low
	while((PINC & 0b11000011) == 0b11000011);
	//这里是等待某一个button按下,即某一位为0
}

/*!
 *	Tests all button functions
 */
void buttonTest() {
	//init everything
	initInput();
	lcd_init();
	lcd_clear();
	lcd_writeProgString(PSTR("Initializing\nInput"));
	_delay_ms(1000);
	lcd_clear();
	if ((DDRC & 0b11000011) != 0) 
	{
		lcd_writeProgString(PSTR("DDR wrong"));
		while (1);
	}
	else{
		lcd_writeProgString(PSTR("DDR Ok"));
		_delay_ms(1000);
	}
	
	lcd_clear();
	if ((PORTC & 0b11000011) != 0b11000011)
	{
		lcd_writeProgString(PSTR("Pullups wrong"));
		while(1);
	}
	else{
		lcd_writeProgString(PSTR("Pullups Ok"));
		_delay_ms(1000);
	}

	//test all 4 buttons
	for (uint8_t i = 0; i < 4; i++) {
		lcd_clear();
		lcd_writeProgString(PSTR("Press Button "));
		lcd_writeDec(i + 1);
		uint8_t pressed = 0;
		while (!pressed)
		{
			waitForInput();
			uint8_t buttonsPressed = getInput();
			if (buttonsPressed != 1 << i) //Not the right button pressed so show which ones were pressed and wait for release
			{
				lcd_line2();
				lcd_writeProgString(PSTR("Pressed: "));
				lcd_writeHexByte(buttonsPressed);
				waitForNoInput();
				lcd_line2();
				lcd_writeProgString(PSTR("                "));
			}else {	//right button pressed so wait for release and continue to next one
				pressed = 1;
				lcd_clear();
				lcd_writeProgString(PSTR("Release Button "));
				lcd_writeDec(i + 1);
				waitForNoInput();
			}
		}
	}
	lcd_clear();
	lcd_writeProgString(PSTR("Test passed"));
	while(1);
}
