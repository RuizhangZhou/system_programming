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
//#warning IMPLEMENT STH. HERE
Process os_processes[MAX_NUMBER_OF_PROCESSES];//MAX_NUMBER_OF_PROCESSES=8 defined in defines.h

//! Array of function pointers for every registered program
//#warning IMPLEMENT STH. HERE
//MAX_NUMBER_OF_PROGRAMS=16
Program* os_programs[MAX_NUMBER_OF_PROGRAMS];//Program *os_programs[MAX_NUMBER_OF_PROGRAMS]; //Is also okay? have tried: yes
//int* p[10];//can also write as int *p[10];
//priority of [] is higher than *, first form the p[10] array, the datatype of the elements in the array is int*(int *)
//Is there difference here "Program*" and "*Program"? have tried: "*Program" doesn't work. 
//The grammar of this datatype(pointer points to the Program): "Program*" or "Program *",they are same

//! Index of process that is currently executed (default: idle)
//#warning IMPLEMENT STH. HERE
ProcessID currentProc;

//----------------------------------------------------------------------------
// Private variables
//----------------------------------------------------------------------------

//! Currently active scheduling strategy
//#warning IMPLEMENT STH. HERE
SchedulingStrategy currentStrategy;

//! Count of currently nested critical sections
//#warning IMPLEMENT STH. HERE
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
    //#warning IMPLEMENT STH. HERE
    //Laufzeitkontext auf dem Prozessstack des aktuellen Prozesses sichern //step 2    
	saveContext();

    //Stackpointers des aktuellen Prozesses sichern //step 3
	os_processes[os_getCurrentProc()].sp.as_int = SP; 
	
	//Stackpointer auf den Scheduler-Stack setzen//step 4
	SP = BOTTOM_OF_ISR_STACK;
	
	//nach dem Setzen des Stackpointers auf den Scheduler-Stack die Prüfsumme des Prozessstacks des unterbrochenen Prozesses ermittelt und abspeichert.
    os_processes[os_getCurrentProc()].checksum = os_getStackChecksum(os_getCurrentProc());
    
	
    if(os_getInput() == 0b00001001) {
		os_waitForNoInput();
		os_taskManMain();
	}

    //Prozesszustand des aktuellen Prozesses auf OS_PS_READY setzen//step 5
	os_processes[os_getCurrentProc()].state = OS_PS_READY; 

    

    //Scheduling-Strategie fuer naechsten Prozess auswaehlen//step 6
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

    //Fortzusetzender Prozesszustand auf OS_PS_RUNNING setzen//step 7
	os_processes[os_getCurrentProc()].state = OS_PS_RUNNING;

    // Pr�fen, ob die Stack Checksumme immer noch passt
    if (os_processes[currentProc].checksum != os_getStackChecksum(currentProc)) {
	    os_error("Stack Inconsistency!");
    }

    //Stackpointer wiederherstellen//step 8
    SP = os_processes[os_getCurrentProc()].sp.as_int;
    
	//step 9 & 10
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
    //#warning IMPLEMENT STH. HERE
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
    if ((programID >= MAX_NUMBER_OF_PROGRAMS) || (programID < 0)) {
        return NULL;
    }

    return os_programs[programID];//here we can see that the programID are 0,1,2,...,15
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
 *  This function is multitasking safe. That means that programs can repost
 *  themselves, simulating TinyOS 2 scheduling (just kick off interrupts ;) ).
 *
 *  \param programID The program id of the program to start (index of os_programs).
 *  \param priority A priority ranging 0..255 for the new process:
 *                   - 0 means least favorable
 *                   - 255 means most favorable
 *                  Note that the priority may be ignored by certain scheduling
 *                  strategies.
 *  \return The index of the new process or INVALID_PROCESS as specified in
 *          defines.h on failure
 */
ProcessID os_exec(ProgramID programID, Priority priority) {
    //#warning IMPLEMENT STH. HERE
    //we dont want an interrupt here, so enter critical section
	os_enterCriticalSection();

	int8_t freeIndex = -1;
	//Nach neuem Platz im Array suchen, bis eines gefunden wurde
	for(uint8_t i = 0; i < MAX_NUMBER_OF_PROCESSES; i++){
		if(os_processes[i].state == OS_PS_UNUSED){
			freeIndex = i;
			break;
		}
	}
	
	// If freeIndex -1 then no free place in  array found and we can return INVALID_PROCESS
	if(freeIndex == -1){
		os_leaveCriticalSection();
		return INVALID_PROCESS;	
	}
		
	//Funktionszeiger des Prozesses laden. Wenn keiner vorhanden, dann quit
	Program* currentProgramPointer = os_lookupProgramFunction(programID);
	if(currentProgramPointer == NULL){
        os_leaveCriticalSection();
		return INVALID_PROCESS;
	}

	//Prozess in den Prozess-Array eintragen
    ProcessID pid = freeIndex;
	Process* newProcessPtr = &os_processes[pid];
	newProcessPtr->state = OS_PS_READY;//here newProcess is a point, so has to use ->
	newProcessPtr->progID = programID;
	newProcessPtr->priority = priority;
	newProcessPtr->sp.as_int = PROCESS_STACK_BOTTOM(freeIndex);
	
	//the address of PC register(Programmzaehler)is 16 bits.
	uint8_t low_byte= ((uint16_t)currentProgramPointer) & 0xff;
    //uint8_t automatically abandon the higher 8 bits //can I just: uint8_t low_byte = (uint16_t)currentProgramPointer?
	uint8_t high_byte= ((uint16_t)currentProgramPointer) >> 8;
    // move the higher 8 bits to the lower position, and the uint8_t then takes the lower 8 bits
	
	*(newProcessPtr->sp.as_ptr)=low_byte;
	newProcessPtr->sp.as_ptr--;
	*(newProcessPtr->sp.as_ptr)=high_byte;
	newProcessPtr->sp.as_ptr--;//
	
	for(uint8_t i = 0; i < 33; i++){
		*(newProcessPtr->sp.as_ptr) = 0b00000000; //all set as 0b00000000
		newProcessPtr->sp.as_ptr--; // Dekrement stackpointer // alternative: set by PROCESS_STACK_BOTTOM(PID)
	}
	
    newProcessPtr->checksum = os_getStackChecksum(pid);
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
    //#warning IMPLEMENT STH. HERE
	//1. Setzen der Variable für den aktuellen Prozess auf 0 (Leerlaufprozess)
    currentProc = 0;
	//2. Den Zustand des Leerlaufprozesses auf OS_PS_RUNNING ändern
	os_processes[0].state = OS_PS_RUNNING;
	//3. Setzen des Stackpointers auf den Prozessstack des Leerlaufprozesses
	SP = os_processes[0].sp.as_int;
	//4. Sprung in den Leerlaufprozess mit restoreContext()
	restoreContext();
}

/*!
 *  In order for the Scheduler to work properly, it must have the chance to
 *  initialize its internal data-structures and register.
 */
void os_initScheduler(void) {
    //#warning IMPLEMENT STH. HERE
    ProcessID id ;
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
    //#warning IMPLEMENT STH. HERE
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
    uint8_t count = 0;
    for (ProcessID i = 0; i < MAX_NUMBER_OF_PROGRAMS; i++)
        if (*(os_getProgramSlot(i))) count++;
    // Note that this only works because programs cannot be unregistered.
    return count;
}

/*!
 *  Sets the current scheduling strategy.
 *
 *  \param strategy The strategy that will be used after the function finishes.
 */
void os_setSchedulingStrategy(SchedulingStrategy strategy) {
    //#warning IMPLEMENT STH. HERE
    currentStrategy = strategy;
}

/*!
 *  This is a getter for retrieving the current scheduling strategy.
 *
 *  \return The current scheduling strategy.
 */
SchedulingStrategy os_getSchedulingStrategy(void) {
    //#warning IMPLEMENT STH. HERE
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
    //#warning IMPLEMENT STH. HERE
    //1.save current interrupt state
	uint8_t localGIEB = SREG & 0b10000000;
	//2.disable interrupt,
	SREG &= 0b01111111;
	//3.increment critical section count
	criticalSectionCount++;
	//4.deactivate scheduler
    TIMSK2 &= 0b11111101;
	//5.apply old state
	SREG|=localGIEB;
}

/*!
 *  Leaves a critical code section by enabling the scheduler if needed.
 *  This function utilizes the nesting depth of critical sections
 *  stored by os_enterCriticalSection to check if the scheduler
 *  has to be reactivated.
 */
void os_leaveCriticalSection(void) {
    //#warning IMPLEMENT STH. HERE
	if(criticalSectionCount <= 0){
		os_error("leaveCritSec count error");
		return;
	}
	
    //save current interrupt state
	uint8_t localGIEB = SREG & 0b10000000;
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
	SREG|=localGIEB;
}

/*!
 *  Calculates the checksum of the stack for a certain process.
 *
 *  \param pid The ID of the process for which the stack's checksum has to be calculated.
 *  \return The checksum of the pid'th stack.
 */
StackChecksum os_getStackChecksum(ProcessID pid) {
    //#warning IMPLEMENT STH. HERE
    StackPointer stackptr =  os_processes[pid].sp;
	//create new stack pointer to iterate through stack
	StackPointer bottom;
	//set it to bottom of process stack
	bottom.as_int = PROCESS_STACK_BOTTOM(pid);

	StackChecksum checksum = *(bottom.as_ptr);
	//iterate through the stack and update checksum
	while( stackptr.as_int < bottom.as_int){
		bottom.as_int--;
		checksum ^= *(bottom.as_ptr);
	}
	return checksum;
}
