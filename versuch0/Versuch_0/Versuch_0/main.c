#include <avr/io.h>
#include "button.h"
#include "led.h"
#include "lcd.h"


int main(void)
{
	
	
    //LED
    led_init();
    
    //Buttons
    initInput();
    
    //lcd_init();
    
    while(1)
    {
	    //buttonTest();
	    led_fun();
	    //convert();
	    //loop();
	    //shift();
	    //learningPointers();
	    //displayArticles();
    }
	
}

