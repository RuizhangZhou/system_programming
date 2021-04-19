#include <avr/io.h>
#include "button.h"
#include "lcd.h"
#include "pointer.h"


int main(void){
	
	
    //LED
    //led_init();
    
    //Buttons
    initInput();
    
    //lcd_init();
    
    while(1)
    {
	    buttonTest();
	    //led_fun();
	    //convert();
	    //loop();
	    //shift();
	    //learningPointers();
	    //displayArticles();
    }
	
}

