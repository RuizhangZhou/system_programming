#include "os_scheduler.h"
#include "util.h"
#include "os_input.h"
#include "os_scheduling_strategies.h"
#include "os_taskman.h"
#include "os_core.h"
#include "lcd.h"

#include <avr/interrupt.h>

//----------------------------------------------------------------------------
// Private Types
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Globals
//----------------------------------------------------------------------------

//! Array of states for every possible process
Process os_processes[MAX_NUMBER_OF_PROCESSES]; 

//! Array of function pointers for every registered program
Program* os_programs[MAX_NUMBER_OF_PROGRAMS]; 

//! Index of process that is currently executed (default: idle)
ProcessID currentProc;

//----------------------------------------------------------------------------
// Private variables
//----------------------------------------------------------------------------

//! Currently active scheduling strategy
SchedulingStrategy currentStrategy;

//! Count of currently nested critical sections
uint8_t criticalSectionCount = 0;

//! Used to auto-execute programs.
uint16_t os_autostart;

//----------------------------------------------------------------------------
// Private function declarations
//----------------------------------------------------------------------------

//! ISR for timer compare match (scheduler)
ISR(TIMER2_COMPA_vect) __attribute__((naked));

//----------------------------------------------------------------------------
// Function definitions
//----------------------------------------------------------------------------

/*!
 *  Timer interrupt that implements our scheduler. Execution of the running
 *  process is suspended and the context saved to the stack. Then the periphery
 *  is scanned for any input events. If everything is in order, the next process
 *  for execution is derived with an exchangeable strategy. Finally the
 *  scheduler restores the next process for execution and releases control over
 *  the processor to that process.
 */
ISR(TIMER2_COMPA_vect) {
   
	//Laufzeitkontext auf dem Prozessstack des aktuellen Prozesses sichern     
	saveContext();
	
	//Stackpointers des aktuellen Prozesses sichern
	os_processes[os_getCurrentProc()].sp.as_int = SP; 
	
	//Stackpointer auf den Scheduler-Stack setzen
	SP = BOTTOM_OF_ISR_STACK;
	
	if(os_getInput()==0b00001001) {
		os_waitForNoInput();
		os_taskManMain();
	}
	
	//Prozesszustand des aktuellen Prozesses auf OS_PS_READY setzen
	os_processes[os_getCurrentProc()].state = OS_PS_READY; 
	
	os_processes[os_getCurrentProc()].checksum = os_getStackChecksum(os_getCurrentProc());
		
	//Scheduling-Strategie fuer naechsten Prozess auswaehlen
	switch(os_getSchedulingStrategy()){
		case OS_SS_EVEN:
			currentProc = os_Scheduler_Even(os_processes,os_getCurrentProc());
			break;
		case OS_SS_RANDOM:
		 	currentProc = os_Scheduler_Random(os_processes,os_getCurrentProc());
			break;
		case OS_SS_RUN_TO_COMPLETION:
			currentProc = os_Scheduler_RunToCompletion(os_processes,os_getCurrentProc());
			break;
		case OS_SS_ROUND_ROBIN:
			currentProc = os_Scheduler_RoundRobin(os_processes,os_getCurrentProc());
			break;
		case OS_SS_INACTIVE_AGING:
			currentProc = os_Scheduler_InactiveAging(os_processes,os_getCurrentProc()) ;
			break;
	}
	//Fortzusetzender Prozesszustand auf OS_PS_RUNNING setzen
	os_processes[os_getCurrentProc()].state = OS_PS_RUNNING;
	
	StackChecksum newChecksum = os_getStackChecksum(os_getCurrentProc());
	if(os_processes[os_getCurrentProc()].checksum != newChecksum) {
		os_error("Stack Inconsistency!");
	}
	
	//Stackpointer wiederherstellen
    	SP = os_processes[os_getCurrentProc()].sp.as_int;
    
    	restoreContext();
}

/*!
 *  Used to register a function as program. On success the program is written to
 *  the first free slot within the os_programs array (if the program is not yet
 *  registered) and the index is returned. On failure, INVALID_PROGRAM is returned.
 *  Note, that this function is not used to register the idle program.
 *
 *  \param program The function you want to register.
 *  \return The index of the newly registered program.
 */
ProgramID os_registerProgram(Program* program) {
    ProgramID slot = 0;

    // Find first free place to put our program
    while (os_programs[slot] &&
           os_programs[slot] != program && slot < MAX_NUMBER_OF_PROGRAMS) {
        slot++;
    }

    if (slot >= MAX_NUMBER_OF_PROGRAMS) {
        return INVALID_PROGRAM;
    }

    os_programs[slot] = program;
    return slot;
}

/*!
 *  Used to check whether a certain program ID is to be automatically executed at
 *  system start.
 *
 *  \param programID The program to be checked.
 *  \return True if the program with the specified ID is to be auto started.
 */
bool os_checkAutostartProgram(ProgramID programID) {
    return !!(os_autostart & (1 << programID));
}

/*!
 *  This is the idle program. The idle process owns all the memory
 *  and processor time no other process wants to have.
 */
PROGRAM(0, AUTOSTART) {
    while(1){
		lcd_writeChar('.');
		delayMs(DEFAULT_OUTPUT_DELAY);
	}
}

/*!
 * Lookup the main function of a program with id "programID".
 *
 * \param programID The id of the program to be looked up.
 * \return The pointer to the according function, or NULL if programID is invalid.
 */
Program* os_lookupProgramFunction(ProgramID programID) {
    // Return NULL if the index is out of range
    if (programID >= MAX_NUMBER_OF_PROGRAMS) {
        return NULL;
    }

    return os_programs[programID];
}

/*!
 * Lookup the id of a program.
 *
 * \param program The function of the program you want to look up.
 * \return The id to the according slot, or INVALID_PROGRAM if program is invalid.
 */
ProgramID os_lookupProgramID(Program* program) {
    ProgramID i;

    // Search program array for a match
    for (i = 0; i < MAX_NUMBER_OF_PROGRAMS; i++) {
        if (os_programs[i] == program) {
            return i;
        }
    }

    // If no match was found return INVALID_PROGRAM
    return INVALID_PROGRAM;
}

/*!
 *  This function is used to execute a program that has been introduced with
 *  os_registerProgram.
 *  A stack will be provided if the process limit has not yet been reached.
 *  In case of an error, an according message will be displayed on the LCD.
 *  This function is multitasking safe. That means that programs can repost
 *  themselves, simulating TinyOS 2 scheduling (just kick off interrupts ;) ).
 *
 *  \param programID The program id of the program to start (index of os_programs).
 *  \param priority A priority ranging 0..255 for the new process:
 *                   - 0 means least favorable
 *                   - 255 means most favorable
 *                  Note that the priority may be ignored by certain scheduling
 *                  strategies.
 *  \return The index of the new process (throws error on failure and returns
 *          INVALID_PROCESS as specified in defines.h).
 */
ProcessID os_exec(ProgramID programID, Priority priority) {
    os_enterCriticalSection();

    //1.Frei Plazt finden
    int i;
    for(i=0; i<MAX_NUMBER_OF_PROCESSES && os_processes[i].state!=OS_PS_UNUSED; i++){
    }
    if (i>=MAX_NUMBER_OF_PROCESSES){
	    os_leaveCriticalSection();
	    return INVALID_PROCESS;
    }

    //2.Funktionszeiger laden
    Program* zeiger = os_lookupProgramFunction(programID);
    if(zeiger == NULL){
	    os_leaveCriticalSection();
	    return INVALID_PROCESS;
    }

    //3.Programmindex,Prozesszustand und Prozessprioritaet speichern
    os_processes[i].state = OS_PS_READY;
    os_processes[i].progID = programID;
    os_processes[i].priority = priority;   
    os_resetProcessSchedulingInformation(i);
    
    //4. Prozessstack vorbereiten
    os_processes[i].sp.as_int = PROCESS_STACK_BOTTOM(i);
    *(os_processes[i].sp.as_ptr) = (uint8_t)((uint16_t)zeiger);
    *(os_processes[i].sp.as_ptr-1) = (uint8_t)(((uint16_t)zeiger)>>8);
    os_processes[i].sp.as_int -= 2;
	
    for (uint8_t j = 0; j < 33; j++){
        *os_processes[i].sp.as_ptr = 0;
        os_processes[i].sp.as_int--;
    }
    os_processes[i].checksum = os_getStackChecksum(i);
    os_leaveCriticalSection();
    return i;
}


/*!
 *  If all processes have been registered for execution, the OS calls this
 *  function to start the idle program and the concurrent execution of the
 *  applications.
 */
void os_startScheduler(void) {
    currentProc = 0;
	os_processes[0].state = OS_PS_RUNNING;
	SP = os_processes[0].sp.as_int;
	restoreContext();
}

/*!
 *  In order for the Scheduler to work properly, it must have the chance to
 *  initialize its internal data-structures and register.
 */
void os_initScheduler(void) {
    ProcessID id = 0;
    for(id=0; id<MAX_NUMBER_OF_PROCESSES; id++){
	os_processes[id].state = OS_PS_UNUSED;
    }
    for(uint8_t id=0; id<MAX_NUMBER_OF_PROGRAMS; id++){
	if(os_checkAutostartProgram(id)){
	    os_exec(id,DEFAULT_PRIORITY);
	}
    }
}

/*!
 *  A simple getter for the slot of a specific process.
 *
 *  \param pid The processID of the process to be handled
 *  \return A pointer to the memory of the process at position pid in the os_processes array.
 */
Process* os_getProcessSlot(ProcessID pid) {
    return os_processes + pid;
}

/*!
 *  A simple getter for the slot of a specific program.
 *
 *  \param programID The ProgramID of the process to be handled
 *  \return A pointer to the function pointer of the program at position programID in the os_programs array.
 */
Program** os_getProgramSlot(ProgramID programID) {
    return os_programs + programID;
}

/*!
 *  A simple getter to retrieve the currently active process.
 *
 *  \return The process id of the currently active process.
 */
ProcessID os_getCurrentProc(void) {
    return currentProc;
}

/*!
 *  This function return the the number of currently active process-slots.
 *
 *  \returns The number currently active (not unused) process-slots.
 */
uint8_t os_getNumberOfActiveProcs(void) {
    uint8_t num = 0;

    ProcessID i = 0;
    do {
        num += os_getProcessSlot(i)->state != OS_PS_UNUSED;
    } while (++i < MAX_NUMBER_OF_PROCESSES);

    return num;
}

/*!
 *  This function returns the number of currently registered programs.
 *
 *  \returns The amount of currently registered programs.
 */
uint8_t os_getNumberOfRegisteredPrograms(void) {
    uint8_t i;
    for (i = 0; i < MAX_NUMBER_OF_PROGRAMS && *(os_getProgramSlot(i)); i++);
    // Note that this only works because programs cannot be unregistered.
    return i;
}

/*!
 *  Sets the current scheduling strategy.
 *
 *  \param strategy The strategy that will be used after the function finishes.
 */
void os_setSchedulingStrategy(SchedulingStrategy strategy) {
    currentStrategy = strategy;
}

/*!
 *  This is a getter for retrieving the current scheduling strategy.
 *
 *  \return The current scheduling strategy.
 */
SchedulingStrategy os_getSchedulingStrategy(void) {
    return currentStrategy;
}

/*!
 *  Enters a critical code section by disabling the scheduler if needed.
 *  This function stores the nesting depth of critical sections of the current
 *  process (e.g. if a function with a critical section is called from another
 *  critical section) to ensure correct behavior when leaving the section.
 *  This function supports up to 255 nested critical sections.
 */
void os_enterCriticalSection(void) {
    uint8_t GIEB = SREG & 0b10000000;
    SREG &= 0b01111111;
    criticalSectionCount++;
    TIMSK2 &= 0b11111101;
    SREG |= GIEB;
}

/*!
 *  Leaves a critical code section by enabling the scheduler if needed.
 *  This function utilizes the nesting depth of critical sections
 *  stored by os_enterCriticalSection to check if the scheduler
 *  has to be reactivated.
 */
void os_leaveCriticalSection(void) {
    uint8_t GIEB = SREG & 0b10000000;
    SREG &= 0b01111111;
    criticalSectionCount--;
    if(criticalSectionCount==0) {
	    TIMSK2 |= 0b00000010;
    }
    else if(criticalSectionCount<0) {
	    os_error("Es existiert kein kritischer Bereiche.");
    }
    SREG |= GIEB;
}

/*!
 *  Calculates the checksum of the stack for a certain process.
 *
 *  \param pid The ID of the process for which the stack's checksum has to be calculated.
 *  \return The checksum of the pid'th stack.
 */
StackChecksum os_getStackChecksum(ProcessID pid) {
    uint8_t *a = (uint8_t*) (uint16_t) (PROCESS_STACK_BOTTOM(pid));
    StackChecksum checksum = *a;
    while((uint16_t)a != os_processes[pid].sp.as_int){
        checksum ^= *(a--);
    }
    return checksum;
	
}

	

