/*
 * GccApplication1.c
 *
 * Created: 17.01.2014 11:35:33
 *  Author: rz523331
 */ 


#include <avr/io.h>
#include "button.h"
#include "led.h"
#include "datatypes.h"
#include "structures.h"
#include "lcd.h"
#include "pointer.h"

int main(void)
{
	
	//LED
	led_init();
	
	//Buttons
	initInput();
	 
	lcd_init();
	
	//shift();
	//convert();
	lcd_writeDec(133);
	
	while(1){
		//buttonTest();
		//led_fun();
		//convert();
		//loop();
		//shift();	
		//learningPointers();
		//displayArticles();
	}
		
}