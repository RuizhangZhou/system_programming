#include "os_scheduler.h"
#include "util.h"
#include "os_input.h"
#include "os_scheduling_strategies.h"
#include "os_taskman.h"
#include "os_core.h"
#include "lcd.h"
#include "os_memory.h"
#include <avr/interrupt.h>
#include <avr/common.h>

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
ProcessID currentProc = 0;

//----------------------------------------------------------------------------
// Private variables
//----------------------------------------------------------------------------

//! Currently active scheduling strategy
SchedulingStrategy schedulingStrategy;

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
 * 计时器中断，实现了我们的调度器。运行中的进程的执行被暂停，
 * 上下文被保存到堆栈中。然后扫描外围的任何输入事件。
 * 如果一切正常，下一个执行的进程将以可交换的策略产生。
 * 最后，调度器恢复下一个进程的执行，并将处理器的控制权交给该进程。
 */
ISR(TIMER2_COMPA_vect) {

    //Laufzeitkontext auf dem Prozessstack des aktuellen Prozesses sichern //step 2    
	saveContext();

	//Stackpointers des aktuellen Prozesses sichern //step 3
	os_processes[currentProc].sp.as_int = SP;
	
	//nach dem Setzen des Stackpointers auf den Scheduler-Stack die Prüfsumme des Prozessstacks des unterbrochenen Prozesses ermittelt und abspeichert.
	os_processes[currentProc].checksum = os_getStackChecksum(currentProc);

	//Stackpointer auf den Scheduler-Stack setzen//step 4
	SP = BOTTOM_OF_ISR_STACK;

	
	if (os_processes[currentProc].state == OS_PS_RUNNING) {
		os_processes[currentProc].state = OS_PS_READY;
	} else if (os_processes[currentProc].state != OS_PS_UNUSED && os_processes[currentProc].state != OS_PS_BLOCKED) {
		os_error("ass err unexpectprog state :-(");
	}


   
   os_initInput();
   if((os_getInput() & 0b00001001) == 0b00001001){
	   os_waitForNoInput();
	   os_taskManOpen();
   }
   
   
    //Scheduling-Strategie fuer naechsten Prozess auswaehlen//step 6
	switch (os_getSchedulingStrategy()) {
		case OS_SS_EVEN:
			currentProc = os_Scheduler_Even(os_processes, os_getCurrentProc());
			break;	
		case OS_SS_INACTIVE_AGING:
			currentProc = os_Scheduler_InactiveAging(os_processes, currentProc);
			break;
		case OS_SS_RANDOM:
			currentProc = os_Scheduler_Random(os_processes, currentProc);
			break;
		case OS_SS_ROUND_ROBIN:
			currentProc = os_Scheduler_RoundRobin(os_processes, currentProc);
			break;
		case OS_SS_RUN_TO_COMPLETION:
			currentProc = os_Scheduler_RunToCompletion(os_processes, currentProc);
			break;
		case OS_SS_MULTI_LEVEL_FEEDBACK_QUEUE:
			currentProc = os_Scheduler_MLFQ(os_processes, currentProc);
			break;
	}
	
	// BLOCKED prozesse sollen mindestens einmal aussetzen. Das haben sie nach dem switch gemacht.
	for (uint8_t i = 0; i < MAX_NUMBER_OF_PROCESSES; ++i) {
		if (os_processes[i].state == OS_PS_BLOCKED) {
			os_processes[i].state = OS_PS_READY;
		}
	}
	
    //Fortzusetzender Prozesszustand auf OS_PS_RUNNING setzen//step 7
	os_processes[currentProc].state = OS_PS_RUNNING;
	
    // Pruefen, ob die Stack Checksumme immer noch passt
	if (os_processes[currentProc].checksum != os_getStackChecksum(currentProc)) {
		os_error(" INVALID  STACK     CHECKSUM");
	}
	
    //Stackpointer wiederherstellen//step 8
	SP = os_processes[currentProc].sp.as_int;
	
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
		return INVALID_PROCESS;	//Rage quit
	}

	Program* funktionszeiger = os_lookupProgramFunction(programID);
	if(funktionszeiger == NULL){
			os_leaveCriticalSection();
			return INVALID_PROCESS;	
	}

	//Prozess in den Prozess-Array eintragen
	Process* newProcess = &os_processes[freeIndex];
	newProcess->state = OS_PS_READY;//here newProcess is a point, so has to use ->
	newProcess->progID = programID;
	newProcess->priority = priority;
	newProcess->sp.as_int = PROCESS_STACK_BOTTOM(freeIndex);

	
	// funktionszeiger (Typ: void) -> uint16_t
	uint16_t ptrFktZeiger = (uint16_t) &os_dispatcher;
	
	//the address of PC register(Programmzaehler)is 16 bits.
	uint8_t bytesDesFunktionsregisters[2];
	bytesDesFunktionsregisters[0] = ptrFktZeiger >> 8;
	// move the higher 8 bits to the lower position, and the uint8_t then takes the lower 8 bits
	bytesDesFunktionsregisters[1] = ptrFktZeiger & 0x00FF;	// LOW  Bytes
	//uint8_t automatically abandon the higher 8 bits //can I just: uint8_t low_byte = (uint16_t)currentProgramPointer?


	*(newProcess->sp.as_ptr) = bytesDesFunktionsregisters[1];	// LOW  Bytes
	newProcess->sp.as_int--;
	*(newProcess->sp.as_ptr) = bytesDesFunktionsregisters[0];  // HIGH Bytes
	newProcess->sp.as_int--;
	
	//Da alle Register mit 0 inizialisiert werden sollen, k�nnen wir 33x 8-Bit mit 0en f�llen
	for(uint8_t i = 0; i < 33; i++){
		*(newProcess->sp.as_ptr) = 0b00000000;//all set as 0b00000000
		newProcess->sp.as_int--;// Dekrement stackpointer // alternative: set by PROCESS_STACK_BOTTOM(PID)
	}
	
	newProcess->checksum = os_getStackChecksum(freeIndex);
	//set age of process to 0 and leave critical section
	os_resetProcessSchedulingInformation(freeIndex);

	os_leaveCriticalSection();
	
	return freeIndex;
}

/*!
 *  If all processes have been registered for execution, the OS calls this
 *  function to start the idle program and the concurrent execution of the
 *  applications.
 */
void os_startScheduler(void) {
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
    for(uint8_t i = 0; i < MAX_NUMBER_OF_PROCESSES; i++){
		os_processes[i].state = OS_PS_UNUSED;
	}
	
	for(uint8_t progID = 0; progID < MAX_NUMBER_OF_PROGRAMS; progID++){
		
		if(os_lookupProgramFunction(progID) == NULL){
			continue;
		}
		
		if(os_checkAutostartProgram(progID)){
			os_exec(progID, DEFAULT_PRIORITY);
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
    schedulingStrategy = strategy;
	//Versuch 3
	os_resetSchedulingInformation(strategy);
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
    
    //1.save current interrupt state
	uint8_t localGIEB = SREG & 0b10000000;
	
	//2.disable interrupt,
	SREG &= 0b01111111;
	
	if (criticalSectionCount == 255) {
		os_error("critical section count overflow");
	}
	//3.increment critical section count
	criticalSectionCount++;
	
	//4.deactivate scheduler
	TIMSK2 &= 0b11111101;
	
	//5.apply old state
	SREG |= localGIEB;
}

/*!
 *  Leaves a critical code section by enabling the scheduler if needed.
 *  This function utilizes the nesting depth of critical sections
 *  stored by os_enterCriticalSection to check if the scheduler
 *  has to be reactivated.
 */
void os_leaveCriticalSection(void) {
	
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
	SREG |= localGIEB;
}

/*!
 *  Calculates the checksum of the stack for a certain process.
 *
 *  \param pid The ID of the process for which the stack's checksum has to be calculated.
 *  \return The checksum of the pid'th stack.
 */
StackChecksum os_getStackChecksum(ProcessID pid) {
	StackPointer underBound;

	//set it to bottom of process stack
	underBound.as_int = PROCESS_STACK_BOTTOM(pid);
	StackPointer stackPointer = os_processes[pid].sp;
	//create new stack pointer to iterate through stack

	
	StackChecksum result = *(underBound.as_ptr);
	//iterate through the stack and update checksum
	while (underBound.as_int > stackPointer.as_int)
	{
		underBound.as_int--;
		result ^= *(underBound.as_ptr);
	}
	
	return result;
}

void os_dispatcher() {//调度

	if (os_processes[currentProc].state != OS_PS_RUNNING) {
		os_error("proc not in     state running");
	}

	ProgramID progID = os_processes[currentProc].progID;
	os_programs[progID]();//run the newProc

	os_kill(currentProc);//kill the currentProc

	while (1) { }
}

bool os_kill(ProcessID pid) {
	os_enterCriticalSection();

	if (pid == 0) {
		os_leaveCriticalSection();
		return false;
	}

	while (pid == currentProc && criticalSectionCount > 1) {//only if (pid==currentProc) we endlessloop
		os_leaveCriticalSection();//release all the criticalSection
	}


	os_processes[pid].state = OS_PS_UNUSED;
	os_processes[pid].progID = 0;
	os_processes[pid].priority = 0;
	os_processes[pid].sp.as_int = 0;

	//we have tested the os_freeProcessMemory is still not so efficient,
	//but here in Versuch 3 we don't have to call this here
	//os_freeProcessMemory(intHeap,pid);
	
	os_resetProcessSchedulingInformation(pid);
	os_removeFromMlfq(pid);
	
	for (uint8_t i = 0; i < os_getHeapListLength() ; i++) {
		os_freeProcessMemory(os_lookupHeap(i), pid);
	}

	if (pid != currentProc) {
		os_leaveCriticalSection();
		return true;
	}
	
	os_leaveCriticalSection();

	TIMER2_COMPA_vect();
	return true;
}


void os_yield() {

	os_enterCriticalSection();

	uint8_t currentCritSecLvl = criticalSectionCount;
	criticalSectionCount = 0;
	
	
	uint8_t GIEB = SREG & 0b10000000;//GIEB=0bX00000000
	
	SREG &= 0b01111111;//SREG=0b0XXXXXXXX;
	
	os_processes[currentProc].state = OS_PS_BLOCKED;
	
	TIMSK2 |= 0b00000010;
	
	TIMER2_COMPA_vect();
	
	TIMSK2 &= 0b11111101;
	
	criticalSectionCount = currentCritSecLvl;

	SREG = GIEB | (SREG & 0b0111111);//SREG=0bYXXXXXXXX; GIEB=0bX00000000 right now
	
	os_leaveCriticalSection();
}
