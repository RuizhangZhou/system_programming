#include "lcd.h"
#include "util.h"
#include "os_core.h"
#include "os_scheduler.h"
#include "os_memory.h"
#include <avr/interrupt.h>

//--------------CONFIG AREA------------------

//define strategies to test

#define EVEN  (1)       // OS_SS_EVEN
#define MLFQ  (1)       // OS_SS_MULTI_LEVEL_FEEDBACK_QUEUE
#define RAND  (1)       // OS_SS_RANDOM
#define INAC  (1)       // OS_SS_INACTIVE_AGING
#define RR    (1)       // OS_SS_ROUND_ROBIN
#define RCOMP (1)       // OS_SS_RUN_TO_COMPLETION

//------------ADVANCED CONFIG----------------

// Iteration exponent for yielding_iterations before the test is marked completed
// i.e. test will be run till yielding_iterations >= Base ^ MAX_TEST_ITERATIONS
// (by default yielding_iterations >= 2^9)
// DO NOT SET IT TO LOW,AS MOST STRATEGY-ITERATIONS NEED TO CONVERGE FIRST 
// MAX_TEST_ITERATIONS = 9 -> 2^9=512 yields takes ~40s on the sample solution
#define MAX_TEST_ITERATIONS    (9)
// Will enable yield/scheduler speed adjusted testing.
// If disabled resulting figures make no sense, but show basic yielding capabilities.
// If enabled the non_yielding process will wait the measured time a yield takes in OS_SS_EVEN.
#define SPEED_ADJUSTED_TESTING (1)

// after defined amount of time the test will be considered failed
#define TIMEOUT_MINUTES (15)

//------------END OF CONFIG------------------


//              WARNING
// YOU SHOULD NOT NEED TO CHANGE ANYTHING BEYOND THIS POINT

//---------Defines/Const/Typedefs------------
#define DELAY (1000ul)
#define BASE 2
#define STRATEGYCOUNT (sizeof(results) / sizeof(results[0]))

static char PROGMEM const  evenStr[] = "EVEN";
static char PROGMEM const  mlfqStr[] = "MLFQ";
static char PROGMEM const  randStr[] = "RAND";
static char PROGMEM const  inacStr[] = "INACTIVE AGING";
static char PROGMEM const    rrStr[] = "ROUND ROBIN";
static char PROGMEM const  runcomp[] = "RUN TO COMP.";

typedef struct TestResults{
    uint8_t ratio;
    char const* const name;
    SchedulingStrategy strat;
    uint8_t active;
    uint8_t cutoff_bot;
    uint8_t cutoff_top;
} TestResults;

typedef unsigned long volatile Iterations;

//------------Runtime variables---------------
Iterations       non_yielding_iterations = 0;
Iterations       yielding_iterations     = 0;
ProcessID        yielder_id;
ProcessID        non_yielder_id;
ProcessID        printer_id;
ProcessID        watchdog_id;
volatile uint8_t current_strat           = 0;
volatile uint8_t sreg_programm_finished  = 0;
Time             yield_speed             = 0;

#if(SPEED_ADJUSTED_TESTING)
    // if real speed adjusted yielding tests are performed, cutoffs are tighter and according to logical boundaries,
    // as nearly all strategies will converge to 2 times more shedules for the non yielding process.
    TestResults results[6] = {
        {0,evenStr  ,OS_SS_EVEN                         ,EVEN   ,2  ,2},
        {0,mlfqStr  ,OS_SS_MULTI_LEVEL_FEEDBACK_QUEUE   ,MLFQ   ,2  ,255},
        {0,randStr  ,OS_SS_RANDOM                       ,RAND   ,2  ,255},
        {0,inacStr  ,OS_SS_INACTIVE_AGING               ,INAC   ,2  ,255},
        {0,rrStr    ,OS_SS_ROUND_ROBIN                  ,RR     ,2  ,255},
        {0,runcomp  ,OS_SS_RUN_TO_COMPLETION            ,RCOMP  ,1  ,1},
    };
#else
    // if only basic yielding capabilities are tested, empirical fantasy values are used as cutoff 
    TestResults results[6] = {
        {0,evenStr  ,OS_SS_EVEN                         ,EVEN   ,4  ,255},
        {0,mlfqStr  ,OS_SS_MULTI_LEVEL_FEEDBACK_QUEUE   ,MLFQ   ,30 ,255},
        {0,randStr  ,OS_SS_RANDOM                       ,RAND   ,3  ,255},
        {0,inacStr  ,OS_SS_INACTIVE_AGING               ,INAC   ,4  ,255},
        {0,rrStr    ,OS_SS_ROUND_ROBIN                  ,RR     ,8  ,255},
        {0,runcomp  ,OS_SS_RUN_TO_COMPLETION            ,RCOMP  ,1  ,1},
    };
#endif

//------------Internal functions---------------

//! advances the current_strat counter over active set strategies 
//! returns next active strategy or NULL if no more available
TestResults *select_next_strat() {
    while (current_strat < STRATEGYCOUNT - 1) {
        current_strat++;
        if (results[current_strat].active) {
            return &results[current_strat];
        }
    }
    return NULL;
}

//! Calculates the log to display the yield/noYield quotient
unsigned logBase(unsigned long base, unsigned long value) {
    if (base <= 1) {
        return 0;
    }
    unsigned result = 0;
    for (; value; value /= base) {
        result++;
    }
    return result;
}

//! Rounds an unsigned integer division to the nearest integer result UP 
unsigned long round_closest_UP(unsigned long dividend, unsigned long divisor){
    return (dividend + (divisor - 1)) / divisor;
}

//! opens count many critical sections
void enterMultipleCriticalSections(uint8_t count) {
    for (uint8_t i = 0; i < count; i++) {
        os_enterCriticalSection();
    }
}

//! closes count many critical sections
void leaveMultipleCriticalSections(uint8_t count) {
    for (uint8_t i = 0; i < count; i++) {
        os_leaveCriticalSection();
    }
}

/*!
 * For each iteration some operations are executed in a critical section and counting the completed iterations.
 * If yield is true yield will be called after half of the operations.
 * Therefore testing if yield() correctly passes computing time on to the next process.
 */
void loop(bool yield, Iterations* iterations) {
    while (1) {
        if (yield) {
            enterMultipleCriticalSections(128);
            os_yield();
            leaveMultipleCriticalSections(128);
        } else {
            // This will produce a critical section overflow, if yield doesn't reset the count
            // This is needed to test, because the criticalSectionCount is not available globally
            enterMultipleCriticalSections(128);
            
#if(SPEED_ADJUSTED_TESTING)
            delayMs(yield_speed);
#endif
            leaveMultipleCriticalSections(128);
        }
        ++*iterations;
    }
}

//! displays the final result table, then returns 
void showResultTable(){
    lcd_clear();
    uint8_t failed = STRATEGYCOUNT+1;
    for(uint8_t strat_runner = 0;strat_runner < STRATEGYCOUNT;strat_runner++){
        //evaluate all provided Test results if cutoffs are in bounds
        uint8_t error_cutoff_bot = results[strat_runner].ratio < results[strat_runner].cutoff_bot;
        uint8_t error_cutoff_top = results[strat_runner].ratio > results[strat_runner].cutoff_top;
        uint8_t strat_active     = results[strat_runner].active;
         
        //if the strategy was requested to be tested and any cutoffs were violated this marks a failed strat
        if (strat_active && (error_cutoff_bot || error_cutoff_top)){
            failed = strat_runner;
            break;
        }
    }
     
    if(failed == STRATEGYCOUNT+1){
        //no strategy index was marked as failed, print all results
        lcd_writeProgString(PSTR("----SUCCESS-----"));
        for (uint8_t iter = 0 ; iter < STRATEGYCOUNT ;iter++){
            if(results[iter].active){
                lcd_writeDec(results[iter].ratio);
            }
            else {
                lcd_writeProgString(PSTR("?"));
            }
            if(iter != STRATEGYCOUNT-1) {
                lcd_writeProgString(PSTR("|"));
            }
        }
    } else {
        //a strategy did not pass the iteration test, print all results upon to the failed strategy
        lcd_writeProgString(PSTR("-----FAILED-----"));
        for (uint8_t iter = 0 ; iter < failed+1 ;iter++){
            if(results[iter].active){
                lcd_writeDec(results[iter].ratio);
            }
            else {
                lcd_writeProgString(PSTR("?"));
            }
            if(iter != failed){
                lcd_writeProgString(PSTR("|"));
            }
        }
        lcd_writeProgString(PSTR("<"));
    }
}

//! performs the iteration Test on the current set scheduling strategy counter by (current_strat)
void performTest(){	
    //apply current strategy
    os_setSchedulingStrategy(results[current_strat].strat);
                
    //if current strategy is OS_SS_RUN_TO_COMPLETION perform different test to not get stuck
    if(os_getSchedulingStrategy() == OS_SS_RUN_TO_COMPLETION){
        //starting yield flag process
        os_exec(7, DEFAULT_PRIORITY);
        //force yield to flag process
        os_yield();
    } 
    else {
        //start testing by starting all needed processes
        yielder_id = os_exec(4, DEFAULT_PRIORITY);
        non_yielder_id = os_exec(3, DEFAULT_PRIORITY);
        printer_id = os_exec(5, DEFAULT_PRIORITY);
    }
}

//! removes testing processes + calculates yielding ratios, then resets runtime variables
void cleanupAfterTest(){
    //killing all test programs for clean next test
    os_kill(non_yielder_id);
    os_kill(yielder_id);
    os_kill(printer_id);
                
    //calculate yieldingratio
    results[current_strat].ratio = round_closest_UP(non_yielding_iterations,yielding_iterations);
                
    //resetting iteration counts for next strat
    non_yielding_iterations = 0;
    yielding_iterations = 0;
}

//! benchmarks os_yield return speed for the current strategy by spawning a dummy process to yield to.
//! and averaging the time it takes to return after the yield command.
//! Returns time in microseconds (1000000us = 1s)
unsigned long benchmarkYield(uint16_t count){
    uint8_t dummy = os_exec(8,DEFAULT_PRIORITY);
    Time time1 = os_systemTime_precise();
    for (long it=0;it<count;it++){
        os_yield();
    }
    Time time2 = os_systemTime_precise();	
    os_kill(dummy);
    
    return (time2 - time1) * 1000ul /count;
}

//------------Programs---------------

//! Main test program.
PROGRAM(1, AUTOSTART) { 
    lcd_writeProgString(PSTR("1:CHECK SREG    "));
    delayMs(DELAY);
    // Set ibit of SREG to 0 to check if it is properly restored
    SREG &= ~(1 << 7);
    os_exec(2, DEFAULT_PRIORITY);
    while (!sreg_programm_finished) {
        os_yield();
    }
    // Check if SREG has been restored
    if ((SREG & (1 << 7)) != 0) {
        os_error("SREG not restored");
        while (1);
    }
    lcd_writeProgString(PSTR("OK"));
    delayMs(DELAY);
    
#if(SPEED_ADJUSTED_TESTING)
    lcd_clear();
    lcd_writeProgString(PSTR("2:CHECK SPEED   "));
    delayMs(DELAY);
    os_setSchedulingStrategy(OS_SS_EVEN);
    yield_speed = benchmarkYield(200);
    lcd_writeDec(yield_speed);
    lcd_writeProgString(PSTR(" us"));
    // actually store a ms time value for delayMS calls later
    // should be reworked later to delayUS if becomes available 
    yield_speed = yield_speed/1000; 
    delayMs(DELAY);
#endif
    
    lcd_clear();
    lcd_writeProgString(PSTR("3:CHECK QUOTIENT"));
    delayMs(DELAY);
    //perform first test manually and start the watchdogs
    if (!results[current_strat].active){
        select_next_strat();
    }
    performTest();
    watchdog_id = os_exec(6, DEFAULT_PRIORITY);
    
    //iteration tests started now, this program can terminate
}

//! Not yielding process
PROGRAM(3, DONTSTART) {
    loop(false, &non_yielding_iterations);
}

//! Yielding program
PROGRAM(4, DONTSTART) {
    loop(true, &yielding_iterations);
}

//! Watchdog, performing strategy changes
PROGRAM(6, DONTSTART) {
    while(1){
        if(logBase(BASE, yielding_iterations) >= MAX_TEST_ITERATIONS || os_getSchedulingStrategy() == OS_SS_RUN_TO_COMPLETION){
            os_enterCriticalSection();
            
            cleanupAfterTest();
            
            //select the next strategy, if there is none left, show results and halt execution
            if(select_next_strat() == NULL){
                showResultTable();
                while(1){};
            }
            
            performTest();
            
            os_leaveCriticalSection();
        }
        
        if(os_systemTime_coarse() >= TIME_M_TO_MS(TIMEOUT_MINUTES)){
            os_enterCriticalSection();
            lcd_clear();
            lcd_writeProgString(PSTR("TIMEOUT REACHED"));
            while(1){};
        }
    }
}

//! Prints the quotient between the iterations with and without yield().
PROGRAM(5, DONTSTART) {
    for (;;) {
        //print header + log entries
        os_enterCriticalSection();
        lcd_clear();
        lcd_writeProgString(results[current_strat].name);
        lcd_line2();
        lcd_writeDec(BASE);
        lcd_writeChar('^');
        lcd_writeDec(logBase(BASE, non_yielding_iterations));
        lcd_writeChar('/');
        lcd_writeDec(BASE);
        lcd_writeChar('^');
        lcd_writeDec(logBase(BASE, yielding_iterations));
        lcd_goto(2, 10);
        lcd_writeChar('=');
        os_leaveCriticalSection();

        Iterations i1, i2;
        do {
            //print live ratio
            os_enterCriticalSection();
            
            i1 = non_yielding_iterations;
            i2 = yielding_iterations;
            //prepare writing ratio + erasing former ratio
            lcd_goto(2, 12);
            lcd_writeProgString(PSTR("     "));
            lcd_goto(2, 12);
            
            if (non_yielding_iterations >= yielding_iterations) {
                lcd_writeDec(round_closest_UP(non_yielding_iterations,yielding_iterations));
            } else {
                lcd_writeChar('1');
                lcd_writeChar('/');
                lcd_writeDec(round_closest_UP(yielding_iterations,non_yielding_iterations));
            }
            os_leaveCriticalSection();
            //busy waiting for ratio change, do not use yield to not disturb other test processes
            while (i1 == non_yielding_iterations && i2 == yielding_iterations);
        } while (logBase(BASE, i1) == logBase(BASE, non_yielding_iterations) && logBase(BASE, i2) == logBase(BASE, yielding_iterations));
    }
}

//---------Utility-programs------------

//! Sets GIEB to check if it's correctly cleared again after yield of program 1
PROGRAM(2, DONTSTART) {
    // Re-enable GIEB
    SREG |= (1 << 7);
    // notify main test process
    sreg_programm_finished = 1;
    
    // terminate helper program
}

//! Special yielding test for OS_SS_RUN_TO_COMPLETION
PROGRAM(7, DONTSTART) {
    //set dedicated iteration counts in scheduler safe environment
    os_enterCriticalSection();
    
    non_yielding_iterations = 1;
    yielding_iterations = 1;
    
    //print test info
    lcd_clear();
    lcd_writeProgString(PSTR("SPECIAL TEST FOR"));
    lcd_writeProgString(PSTR("OS_SS_RUN_TO_CMP"));
    
    os_leaveCriticalSection();
    
    //intentionally wait in non thread safe environment to provoke errors in OS_SS_RUN_TO_COMPLETION
    delayMs(1000);
    
    //Let program terminate and watchdog evaluate non_yielding_iterations/yielding_iterations = 1.
    //Every other ration means another process, either yielder or non-yielder have run in the meantime,
    //violating the OS_SS_RUN_TO_COMPLETION strategy
}

//! Dummy program to be used to calculate yielding speeds
PROGRAM(8, DONTSTART) {
    while(1){};
}