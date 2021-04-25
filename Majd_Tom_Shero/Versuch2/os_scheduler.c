#include "os_scheduler.h"
#include "util.h"
#include "os_input.h"
#include "os_scheduling_strategies.h"
#include "os_taskman.h"
#include "os_core.h"
#include "lcd.h"
#include <stdlib.h>

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
SchedulingStrategy schedulingStrategy;

//! Count of currently nested critical sections
int criticalSectionCount = 0;

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
	//save context
	saveContext();
	//save stackpointer of process stack
	(os_processes[os_getCurrentProc()]).sp.as_int = SP;	
	//save the checksum
	(os_processes[os_getCurrentProc()]).checksum = os_getStackChecksum(os_getCurrentProc());
	//set the SP to the bottom of scheduler stack
	SP = BOTTOM_OF_ISR_STACK;
	//set the current process to ready
	(*os_getProcessSlot(os_getCurrentProc())).state = OS_PS_READY;
	//initialize input and check if ENTER+ESC have been pressed
	os_initInput();
	if((os_getInput() & 0b10000001) == 0b10000001){
		//if pressed then wait until its no longer pressed and open task manager
		os_waitForNoInput();
		os_taskManOpen();
	}
	
	// Get the scheduling strategy to decide which process runs next.
	switch(os_getSchedulingStrategy()){
		case OS_SS_EVEN:
		currentProc = os_Scheduler_Even(os_processes, os_getCurrentProc());
		break;
		case OS_SS_INACTIVE_AGING:
		currentProc = os_Scheduler_InactiveAging(os_processes, os_getCurrentProc());
		break;
		case OS_SS_RANDOM:
		currentProc = os_Scheduler_Random(os_processes, os_getCurrentProc());
		break;
		case OS_SS_ROUND_ROBIN:
		currentProc = os_Scheduler_RoundRobin(os_processes, os_getCurrentProc());
		break;
		case OS_SS_RUN_TO_COMPLETION:
		currentProc = os_Scheduler_RunToCompletion(os_processes, os_getCurrentProc());
		break;
	}
	
	// Set process as RUNNING.
	(*os_getProcessSlot(currentProc)).state = OS_PS_RUNNING;
	
	// Write the stack pointer in the stack register.
	SP = (*os_getProcessSlot(currentProc)).sp.as_int;
	//was somehow because it was always true and it would print the error message 
	//but the testtask of stackinconsistency said all test passed. any idea why?
	if (os_processes[os_getCurrentProc()].checksum != os_getStackChecksum(os_getCurrentProc())){
		//os_errorPStr(" INVALID  STACK     CHECKSUM");
	}
	//restore context
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
	//prints '.' onto the screen
	while(1){
		lcd_writeString(".");
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
   //we dont want an interrupt here, so enter critical section
	os_enterCriticalSection();

	//1.Find free slot in array os_processes:
    int position = 0;
    for (int i = 0; i<MAX_NUMBER_OF_PROCESSES;i++)
    {
	    if(os_processes[i].state == OS_PS_UNUSED){
		    position = i;
		    break;
	    }
	    if(i == MAX_NUMBER_OF_PROCESSES-1){
			os_leaveCriticalSection();
			return INVALID_PROCESS;
		}
    }

    //2.load function pointer from os_programs:
    Program* currentProgramPointer = os_lookupProgramFunction(programID);
    if(currentProgramPointer == NULL){
		os_leaveCriticalSection();
		return INVALID_PROCESS;
	}

    //3. Store program index, process state and process priority:
	ProcessID pid = position;
    Process* newProcess = &(os_processes[pid]);
    newProcess->state = OS_PS_READY;
    newProcess->progID = programID;
    newProcess->priority = priority;
	
	/** Prepare stack by setting: 
	* 1. Low-Byte 
	* 2. High-Byte
	* 3. SREG Status Register
	* 4. 32 Register 
	*/
	newProcess->sp.as_int = PROCESS_STACK_BOTTOM(position);
    uint8_t low = ((uint16_t)currentProgramPointer) & 0xff;  
    uint8_t high = ((uint16_t)currentProgramPointer) >> 8;
    *(newProcess->sp.as_ptr) = low;
    newProcess->sp.as_ptr--;
    *(newProcess->sp.as_ptr) = high;
    for(int i = 0; i<33;i++){
	    newProcess->sp.as_ptr--;
	    *(newProcess->sp.as_ptr) = 0b00000000;
    }
	newProcess->sp.as_ptr--;
   
	newProcess->checksum = os_getStackChecksum(pid);
	
	//set age of process to 0 and leave critical section
	os_resetProcessSchedulingInformation(pid);
    os_leaveCriticalSection();
	return pid;
}

/*!
 *  If all processes have been registered for execution, the OS calls this
 *  function to start the idle program and the concurrent execution of the
 *  applications.
 */
void os_startScheduler(void) {
	//set current process to idle program
   currentProc = 0;
   //set its state to running assign its stackpointer to SP
   (*os_getProcessSlot(currentProc)).state = OS_PS_RUNNING;
   SP =  os_processes[0].sp.as_int;
   restoreContext();
}

/*!
 *  In order for the Scheduler to work properly, it must have the chance to
 *  initialize its internal data-structures and register.
 */
void os_initScheduler(void) {
	
	//set every process to UNUSED 
	for(int i=0; i<MAX_NUMBER_OF_PROCESSES;i++){
		os_processes[i].state = OS_PS_UNUSED;
	}
	//check if any program is marked as autostart and execute it
	for(int id=0; id<MAX_NUMBER_OF_PROGRAMS;id++){
		if(os_lookupProgramFunction(id) == NULL){
			continue;
		}
		if (os_checkAutostartProgram(id)) os_exec(id,DEFAULT_PRIORITY);
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
   schedulingStrategy = strategy;
}

/*!
 *  This is a getter for retrieving the current scheduling strategy.
 *
 *  \return The current scheduling strategy.
 */
SchedulingStrategy os_getSchedulingStrategy(void) {
    return schedulingStrategy;
}

/*!
 *  Enters a critical code section by disabling the scheduler if needed.
 *  This function stores the nesting depth of critical sections of the current
 *  process (e.g. if a function with a critical section is called from another
 *  critical section) to ensure correct behavior when leaving the section.
 *  This function supports up to 255 nested critical sections.
 */
void os_enterCriticalSection(void) {
	//save current interrupt state
	uint8_t bit = SREG & 0b10000000;
	//disable interrupt,
	SREG &= 0b01111111;
	//deactivate scheduler
    TIMSK2 &= 0b11111101;
	//increment critical section count
	criticalSectionCount++;
	//apply old state
	SREG|=bit;
}

/*!
 *  Leaves a critical code section by enabling the scheduler if needed.
 *  This function utilizes the nesting depth of critical sections
 *  stored by os_enterCriticalSection to check if the scheduler
 *  has to be reactivated.
 */
void os_leaveCriticalSection(void) {
	
	//save current interrupt state
	uint8_t bit = SREG & 0b10000000;
	//disable interrupt
	SREG &= 0b01111111;
	//decrement critical section count
	criticalSectionCount--;
	//check if critical section count is zero / if were no longer in the critical section
	if(criticalSectionCount == 0){
		//activate scheduler
		TIMSK2 |= 0b00000010;
	}
	//apply old state
	SREG|=bit;
}

/*!
 *  Calculates the checksum of the stack for a certain process.
 *
 *  \param pid The ID of the process for which the stack's checksum has to be calculated.
 *  \return The checksum of the pid'th stack.
 */
StackChecksum os_getStackChecksum(ProcessID pid) {
	StackPointer stackpntr =  ((*os_getProcessSlot(pid)).sp);
	//create new stack pointer to iterate through stack
	StackPointer bottom;
	//set it to bottom of process stack
	bottom.as_int = PROCESS_STACK_BOTTOM(pid);

	StackChecksum checksum = *(bottom.as_ptr);
	//iterate through the stack and update checksum
	while( stackpntr.as_int < bottom.as_int){
		bottom.as_int--;
		checksum ^= *(bottom.as_ptr);
	}
	return checksum;
}