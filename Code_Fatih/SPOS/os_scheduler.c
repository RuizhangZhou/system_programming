/*! \file
 *  \brief Scheduling-Modul
 *
 *  Enth�lt zentrale ISR
 *
 *  \author   Fatih
 *  \date     2020
 *  \version  2.0
 */
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
 */
ISR(TIMER2_COMPA_vect) {
	/* Sichern des Prozesstacks des alten Prozesses (in folgender Reihenfolge):
	 * 1. Program Counter 
	 * 2. Laufzeitkontext (32 Register + Statusregister)
	 *
	 * Zu 1: Passiert automatisch, wenn ISR ausgef�hrt wird
	 * Zu 2: Kann einfach erledigt werden, durch Aufruf von saveContext()
	*/

	// Register auf dem Stack speichern und Stackpointer im Array os_processes sichern
	saveContext();
	os_processes[currentProc].sp.as_int = SP;
	
	// Neue Checksumme des Stacks berechnen und sichern
	os_processes[currentProc].checksum = os_getStackChecksum(currentProc);

	// Stackpointer auf unteres Ende vom Scheduler Stack setzen
	SP = BOTTOM_OF_ISR_STACK;

	/*
	 * Prozesszustand des alten Prozesses �ndern auf READY, falls er lief.
	 * Das bedeutet insbesondere, dass ein Prozess, der UNUSED (terminiert)
	 * ist, nicht wieder auf READY gesetzt wird.
	 */
	if (os_processes[currentProc].state == OS_PS_RUNNING) {
		os_processes[currentProc].state = OS_PS_READY;
	} else if (os_processes[currentProc].state != OS_PS_UNUSED && os_processes[currentProc].state != OS_PS_BLOCKED) {
		os_error("ass err unexpectprog state :-(");
	}


   // Pr�fen, ob der Taskmanager aufgerufen werden soll
   // Der Taskmanager wird via ESC + Enter aufgerufen
   os_initInput();
   if((os_getInput() & 0b00001001) == 0b00001001){
	   os_waitForNoInput();
	   os_taskManOpen();
   }
   
   /* Wiederherstellung eines neuen Prozesses (in folgender Reihenfolge):
    * 1. Ausw�hlen des neuen Prozesses (abh�ngig von Scheduling Strategy)
	* 2. gesicherten Stackpointer ins SP-Register schreiben
	* 3. Laufzeitkontext (32 Register + Statusregister) wiederherstellen
	* 4. Program Counter neu setzen
	*
	* Zu 3&4: Beide Schritte k�nnen mit der Methode restoreContext() durchgef�hrt werden
   */
   
	// Mithilfe der ausgew�hlten Scheduling Strategy den neuen auszuf�hrenden Prozess bestimmen
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
	
	// Neuen Prozess auf RUNNING setzen
	os_processes[currentProc].state = OS_PS_RUNNING;
	
	// Pr�fen, ob die Stack Checksumme immer noch passt
	if (os_processes[currentProc].checksum != os_getStackChecksum(currentProc)) {
		os_error(" INVALID  STACK     CHECKSUM");
	}
	
	// Schreiben des Stackpointers in das SP-Register
	SP = os_processes[currentProc].sp.as_int;
	
	// Wiederherstellen des Laufzeitkontextes
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
	os_enterCriticalSection();
	
    int8_t freeIndex = -1;

	//Nach neuem Platz im Array suchen, bis eines gefunden wurde
	for(uint8_t i = 0; i < MAX_NUMBER_OF_PROCESSES; i++){
		if(os_processes[i].state == OS_PS_UNUSED){
			freeIndex = i;
			break;
		}
	}

	//Wenn kein Platz gefunden worden ist, dann ist freeIndex immer noch -1 und wir k�nnen hier rausquitten
	if(freeIndex == -1){
		os_leaveCriticalSection();
		return INVALID_PROCESS;	//Rage quit
	}

	//Funktionszeiger des Prozesses laden. Wenn keiner vorhanden, dann Quit
	Program* funktionszeiger = os_lookupProgramFunction(programID);
	if(funktionszeiger == NULL){
			os_leaveCriticalSection();
			return INVALID_PROCESS;	//Rage quit
	}

	//Prozess in den Prozess-Array eintragen
	Process* newProcess = &os_processes[freeIndex];
	newProcess->state = OS_PS_READY;
	newProcess->progID = programID;
	newProcess->priority = priority;
	newProcess->sp.as_int = PROCESS_STACK_BOTTOM(freeIndex);

	/* Stack vorbereiten
	 * Auf den leeren Stack m�ssen wir folgende Daten ablegen (in dieser Reihenfolge):
	 * 1. LOW-Bytes des Funktionszeigers
	 * 2. HIGH-Bytes des Funktionszeigers
	 * 3. Statusregister SREG
	 * 4. Die 32 Register 
	 * (Alle Register (inkl. Statusregister) sollen mit 0 inizialisiert werden
	 * (5. Wir m�ssen ausserdem die Checksumme des Stacks berechnen)
	*/
	
	// funktionszeiger (Typ: void) -> uint16_t
	uint16_t ptrFktZeiger = (uint16_t) &os_dispatcher;
	
	//Auftrennen des Funktionszeigers
	uint8_t bytesDesFunktionsregisters[2];
	bytesDesFunktionsregisters[0] = ptrFktZeiger >> 8;		// HIGH Bytes
	bytesDesFunktionsregisters[1] = ptrFktZeiger & 0x00FF;	// LOW  Bytes
	
	//Schreiben des Funktionszeigers
	*(newProcess->sp.as_ptr) = bytesDesFunktionsregisters[1];	// LOW  Bytes
	newProcess->sp.as_int--;
	*(newProcess->sp.as_ptr) = bytesDesFunktionsregisters[0];  // HIGH Bytes
	newProcess->sp.as_int--;
	
	//Da alle Register mit 0 inizialisiert werden sollen, k�nnen wir 33x 8-Bit mit 0en f�llen
	for(uint8_t i = 0; i < 33; i++){
		*(newProcess->sp.as_ptr) = 0b00000000;
		newProcess->sp.as_int--;
	}
	
	//Initialisierung des StackChecksumme
	newProcess->checksum = os_getStackChecksum(freeIndex);

	//alter des Processes auf 0 setzenr
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
	/* Startet den Scheduler, welcher als erstes den Leerlaufprozess starten soll.
	 * Daf�r folgende Schritte:
	 * 1. Setzen der currentProc Variable auf 0 (Leerlaufprozess hat immer die ID 0)
	 * 2. Den Zustand des Prozesses auf RUNNING setzen
	 * 3. SP-Register auf Stackpointer des Prozesses setzen
	 * 4. restoreContext um Laufzeitcontext und PC-Register wiederherzustellen
	*/
	
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
	// Initialisierung des Arrays os_processes
	// Zur Initialisierung muss jeder Prozess den Status OS_PS_UNUSED zugewiesen bekommen
    for(uint8_t i = 0; i < MAX_NUMBER_OF_PROCESSES; i++){
		os_processes[i].state = OS_PS_UNUSED;
	}
	
	//Durchlaufen aller Programme um Autostart-Programme zu starten
	for(uint8_t progID = 0; progID < MAX_NUMBER_OF_PROGRAMS; progID++){
		
		//Pr�fen ob Programmfunktion = NULL ist -> continue
		if(os_lookupProgramFunction(progID) == NULL){
			continue;
		}
		
		//Pr�fen ob Programm automatisch gestartet werden soll
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
    /* Folgende Schritte m�ssen durchgef�hrt werden:
	 * 1. Sichern des Global Interrupt Enable Bit aus SREG
	 * 2. Setzen des Global Interrupt Enable Bit auf 0
	 * 3. criticalSectionCount++
	 * 4. Deaktivieren des Schedulers durch schreiben von 0 in OCIE2A im Register TIMSK2
	 * 5. Wiederherstellen des MSB im SREG
	*/
	
	// Sichern des MSB aka. GIEB aus dem SREG
	uint8_t msbSregVorher = SREG & 0b10000000;
	
	// Global Interrupt Enable Bit (7. / MSB) im Statusregister (SREG) auf 0 setzen
	SREG &= 0b01111111;
	
	if (criticalSectionCount == 255) {
		os_error("critical section count overflow");
	}
	
	criticalSectionCount++;
	
	//Deaktivieren des Schedulers
	TIMSK2 &= 0b11111101;
	
	//7. Bit (MSB) im Statusregister (SREG) wiederherstellen
	SREG |= msbSregVorher;
}

/*!
 *  Leaves a critical code section by enabling the scheduler if needed.
 *  This function utilizes the nesting depth of critical sections
 *  stored by os_enterCriticalSection to check if the scheduler
 *  has to be reactivated.
 */
void os_leaveCriticalSection(void) {
	
	/* Folgende Schritte m�ssen durchgef�hrt werden:
	 * 0. Pr�fen, ob wir eine critSecCount < 0 erreichen w�rden
	 * 1. Sichern des MSB aus dem SREG in einer lokalen Variable
	 * 2. Setzen des MSB im SREG auf 0
	 * 3. criticalSectionCount--
	 * 4. Wenn criticalSectionCount == 0, dann Timer wieder aktivieren
	 * 5. Wiederherstellen des MSB im SREG
	*/
	
    if(criticalSectionCount <= 0){
		os_error("leaveCritSec count error");
		return;
	}
	
	// Sichern des MSB aus dem SREG
	uint8_t msbSregVorher = SREG & 0b10000000;
	
	//7. Bit (MSB) im Statusregister (SREG) auf 0 setzen
	SREG &= 0b01111111;
	
	criticalSectionCount--;
	
	// Wenn die criticalSectionCount nach Erniedrigung = 0 ist, dann haben wir alle CritSecs verlassen
	// und k�nnen den Timer wieder aktivieren
	if(criticalSectionCount == 0){
		TIMSK2 |= 0b00000010;
	}
	
	//7. Bit (MSB) im Statusregister (SREG) wiederherstellen
	SREG |= msbSregVorher;
}

/*!
 *  Calculates the checksum of the stack for a certain process.
 *
 *  \param pid The ID of the process for which the stack's checksum has to be calculated.
 *  \return The checksum of the pid'th stack.
 */
StackChecksum os_getStackChecksum(ProcessID pid) {
     /*  Es sollen alle Bits zwischen Anfang des Stacks und Stackpointer
	 *  zur Berechnung des Hashes mit XOR verkn�pft werden
	 */
	
	StackPointer untereGrenze;
	untereGrenze.as_int = PROCESS_STACK_BOTTOM(pid);
	StackPointer stackPointer = os_processes[pid].sp;
	
	StackChecksum result = *(untereGrenze.as_ptr);
	while (untereGrenze.as_int > stackPointer.as_int)
	{
		untereGrenze.as_int--;
		result ^= *(untereGrenze.as_ptr);
	}
	
	return result;
}

void os_dispatcher() {

	if (os_processes[currentProc].state != OS_PS_RUNNING) {
		os_error("proc not in     state running");
	}

	// Aus dem dispatcher heraus die Programmfunktion aufrufen.
	ProgramID progID = os_processes[currentProc].progID;
	os_programs[progID]();

	// Prozess killen, nachdem er fertig ist.
	os_kill(currentProc);

	// auf den scheduler warten.
	// passiert auch in os_kill :-/
	while (1) { }
}

bool os_kill(ProcessID pid) {
	os_enterCriticalSection();

	if (pid == 0) {
		os_leaveCriticalSection();
		return false;
	}

	/*
	 * Annahme: Es kann immer nur ein Prozess,
	 * genauer genommen der aktuell laufende,
	 * in der critical section sein.
	 *
	 * Falls der zu killende Prozesse drin ist, muss er sie verlassen.
	 * Falls ein anderer Prozess l�uft und einen anderen killt, kann der nicht
	 * in der critical section sein, deshalb m�ssen wir die nicht freigeben.
	 */
	while (pid == currentProc && criticalSectionCount > 1) {
		os_leaveCriticalSection();
	}

	// os_processes aufr�umen
	os_processes[pid].state = OS_PS_UNUSED;
	os_processes[pid].progID = 0;
	os_processes[pid].priority = 0;
	os_processes[pid].sp.as_int = 0;

	// timeslice auf 0 u.�.
	os_resetProcessSchedulingInformation(pid);
	os_removeFromMlfq(pid);

	for (uint8_t i = 0; i < os_getHeapListLength() ; i++) {
		os_freeProcessMemory(os_lookupHeap(i), pid);
	}

	if (pid != currentProc) {
		os_leaveCriticalSection();
		return true;
	}
	// Falls sich Prozess selbst beendet, muss anderes Programm ausgef�hrt
	// werden.
	os_leaveCriticalSection();
	// ruft scheduler explizit auf
	TIMER2_COMPA_vect();
	return true;
}

/*!
 *  Der aufrufende Prozess gibt die CPU ab, speichert seinen lokalen Zustand (critical sections, Global Interrupt Enable Bit), auf seinem Stack
 *  re-aktiviert den Scheduler und ruft ebendiesen auf.
 *
 *  Wenn das Programm dann irgendwann mal wieder reingescheduled wird, macht's an der Stelle weiter; stellt den lokalen Zustand wieder her und weiter return't.
 */
void os_yield() {

	os_enterCriticalSection();

	uint8_t currentCritSecLvl = criticalSectionCount;
	criticalSectionCount = 0;
	
	// Sichern des MSB aka. GIEB aus dem SREG
	uint8_t currentGieb = SREG & 0b10000000;
	
	// Global Interrupt Enable Bit (7. / MSB) im Statusregister (SREG) auf 0 setzen
	SREG &= 0b01111111;
	
	os_processes[currentProc].state = OS_PS_BLOCKED;
	
	// Stellen Sie vor dem manuellen Auf-ruf des Schedulers au�erdem sicher,
	// dass der Scheduler auch weiterhin automatisch durch Timerinterrupts aufgerufen wird.
	TIMSK2 |= 0b00000010;
	
	// ruft scheduler explizit auf
	TIMER2_COMPA_vect();
	
	// re-deaktivieren des Schedulers
	TIMSK2 &= 0b11111101;
	
	// vorherigen Zustand wiederherstellen
	criticalSectionCount = currentCritSecLvl;

	// stellt GIEB wieder her, l�sst rest von SREG so wie's is.
	SREG = currentGieb | (SREG & 0b0111111);
	
	os_leaveCriticalSection();
}