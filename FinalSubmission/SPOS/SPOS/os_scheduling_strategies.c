/*! \file
 *  \brief Scheduling library for the OS.
 *
 *  Contains the scheduling strategies.
 *
 *  \author   Fatih, Youssef
 *  \date     2020
 *  \version  1.0
 */
#include "os_scheduling_strategies.h"
#include "defines.h"

#include <stdlib.h>

SchedulingInformation schedulingInfo;


void pqueue_init(ProcessQueue* queue) {
	queue->size = MAX_NUMBER_OF_PROCESSES;
	queue->head = 0;
	queue->tail = 0;
	for (int i = 0; i < 8; ++i) {
		queue->data[i] = 0;
	}
}

void pqueue_reset(ProcessQueue* queue) {
	queue->head = 0;
	queue->tail = 0;
}

uint8_t pqueue_hasNext(ProcessQueue* queue) {
	return queue->head != queue->tail;
}

ProcessID pqueue_getFirst(ProcessQueue* queue) {
	return queue->data[queue->head];
}

void pqueue_dropFirst(ProcessQueue* queue) {
	queue->data[queue->head] = 0;
	queue->head = (queue->head + 1) % queue->size;
}

void pqueue_append(ProcessQueue* queue, ProcessID pid) {
	queue->data[queue->tail] = pid;
	queue->tail = (queue->tail + 1) % queue->size;
}

ProcessQueue* MLFQ_getQueue(uint8_t queueID) {
	return &schedulingInfo.qs[queueID];
}

void os_initSchedulingInformation() {
	for (uint8_t i = 0; i < 4; ++i) {
		pqueue_init(&schedulingInfo.qs[i]);
	}
	for (uint8_t i = 1; i < 8; i++) {
		if (os_getProcessSlot(i)->state != OS_PS_UNUSED) {
			uint8_t q = os_getProcessSlot(i)->priority >> 6;
			ProcessQueue *pq = &schedulingInfo.qs[3 - q];
			pqueue_append(pq, i);
			schedulingInfo.mlfq_slice[i] = 1 << (3 - q);
		}
	}
}

/*!
 *  Reset the scheduling information for a specific strategy
 *  This is only relevant for RoundRobin and InactiveAging
 *  and is done when the strategy is changed through os_setSchedulingStrategy
 *
 * \param strategy  The strategy to reset information for
 */
void os_resetSchedulingInformation(SchedulingStrategy strategy) {
    // This is a presence task
	switch(strategy){
		case OS_SS_EVEN:
			break;
		case OS_SS_RANDOM:
			break;
		case OS_SS_RUN_TO_COMPLETION:
			break;	
		case OS_SS_ROUND_ROBIN:
			schedulingInfo.timeSlice = os_getProcessSlot(os_getCurrentProc())->priority;
			break;
		case OS_SS_INACTIVE_AGING:
			for (uint8_t i = 0; i < MAX_NUMBER_OF_PROCESSES; i++) {
				schedulingInfo.age[i] = 0;	
			}
			break;
		case OS_SS_MULTI_LEVEL_FEEDBACK_QUEUE:
			os_initSchedulingInformation();
			break;
	}
}

void os_removeFromMlfq(ProcessID id) {

	ProcessQueue local_q;
	pqueue_init(&local_q);
	ProcessQueue *pq;

	for (int i = 0; i < 4; ++i) {
		pq = &schedulingInfo.qs[i];
		while (pqueue_hasNext(pq)) {
			uint8_t p = pqueue_getFirst(pq);
			pqueue_dropFirst(pq);
			if (p != id) {
				pqueue_append(&local_q, p);
			}
		}
		while (pqueue_hasNext(&local_q)) {
			uint8_t p = pqueue_getFirst(&local_q);
			pqueue_dropFirst(&local_q);
			pqueue_append(pq, p);
		}
	}
}

/*!
 *  Reset the scheduling information for a specific process slot
 *  This is necessary when a new process is started to clear out any
 *  leftover data from a process that previously occupied that slot
 *
 *  \param id  The process slot to erase state for
 */
void os_resetProcessSchedulingInformation(ProcessID id) {
    // This is a presence task
	schedulingInfo.age[id] = 0;

	if (id == 0) {
		return;
	}

	os_removeFromMlfq(id);
	
	uint8_t q = os_getProcessSlot(id)->priority >> 6;
	ProcessQueue *pq = &schedulingInfo.qs[3 - q];
	pqueue_append(pq, id);
	schedulingInfo.mlfq_slice[id] = 1 << (3 - q);
}
/*!
 *  This function implements the even strategy. Every process gets the same
 *  amount of processing time and is rescheduled after each scheduler call
 *  if there are other processes running other than the idle process.
 *  The idle process is executed if no other process is ready for execution
 *
 *  \param processes An array holding the processes to choose the next process from.
 *  \param current The id of the current process.
 *  \return The next process to be executed determined on the basis of the even strategy.
 */
ProcessID os_Scheduler_Even(Process const processes[], ProcessID current) {
	uint8_t nextProc = (current + 1) % MAX_NUMBER_OF_PROCESSES;
	while (nextProc != current) {
		if (processes[nextProc].state == OS_PS_READY && nextProc != 0) {
			return nextProc;
		}
		nextProc = (nextProc + 1) % MAX_NUMBER_OF_PROCESSES;
	}
	
	if (processes[nextProc].state == OS_PS_READY) {
		return nextProc;
	} else {
		return 0;
	}
}

/*!
 *  This function implements the random strategy. The next process is chosen based on
 *  the result of a pseudo random number generator.
 *
 *  \param processes An array holding the processes to choose the next process from.
 *  \param current The id of the current process.
 *  \return The next process to be executed determined on the basis of the random strategy.
 */
ProcessID os_Scheduler_Random(Process const processes[], ProcessID current) {
	
	/* Problem: Wir kennen die Reihenfolge und den Zustand der Prozesse im Array processes nicht
	 * L�sung: Wir legen ein eigenes Array an, in welches wir, wenn der Zustand des Prozesses
	 *         RUNNING oder READY ist, den Index aus dem processes Array ablegen. Zus�tzlich speichern wir die Anzahl der Prozesse.
	 *         Am Ende k�nnen wir dann mithilfe von rand() und der Modulo-Operation zuf�llig einen Prozess zur�ckgeben
	 */
	
	uint8_t aktiveProzesse[MAX_NUMBER_OF_PROCESSES];
	uint8_t anzahlAktiveProzesse = 0;
	
	for (uint8_t i = 0; i < MAX_NUMBER_OF_PROCESSES; i++) {
		if (processes[i].state == OS_PS_READY && processes[i].progID != 0) {
			aktiveProzesse[anzahlAktiveProzesse] = i;
			anzahlAktiveProzesse++;
		}
	}
	
	if (anzahlAktiveProzesse == 0) {
		return 0;
	}
	
	return aktiveProzesse[rand() % anzahlAktiveProzesse];
}

/*!
 *  This function implements the round-robin strategy. In this strategy, process priorities
 *  are considered when choosing the next process. A process stays active as long its time slice
 *  does not reach zero. This time slice is initialized with the priority of each specific process
 *  and decremented each time this function is called. If the time slice reaches zero, the even
 *  strategy is used to determine the next process to run.
 *
 *  \param processes An array holding the processes to choose the next process from.
 *  \param current The id of the current process.
 *  \return The next process to be executed determined on the basis of the round robin strategy.
 */
ProcessID os_Scheduler_RoundRobin(Process const processes[], ProcessID current) {
    
	schedulingInfo.timeSlice--;
	
	if (schedulingInfo.timeSlice < 1 || !os_isRunnable(processes + current)) {
		ProcessID res = os_Scheduler_Even(processes, current);
		schedulingInfo.timeSlice = processes[res].priority;
		return res; 
	}
	
	return current;
}

/*!
 *  This function realizes the inactive-aging strategy. In this strategy a process specific integer ("the age") is used to determine
 *  which process will be chosen. At first, the age of every waiting process is increased by its priority. After that the oldest
 *  process is chosen. If the oldest process is not distinct, the one with the highest priority is chosen. If this is not distinct
 *  as well, the one with the lower ProcessID is chosen. Before actually returning the ProcessID, the age of the process who
 *  is to be returned is reset to its priority.
 *
 *  \param processes An array holding the processes to choose the next process from.
 *  \param current The id of the current process.
 *  \return The next process to be executed, determined based on the inactive-aging strategy.
 */
ProcessID os_Scheduler_InactiveAging(Process const processes[], ProcessID current) {
    
	// Das Alter aller inaktiven Prozesse wird um deren Priorit�t erh�ht
	for (uint8_t i = 1; i < MAX_NUMBER_OF_PROCESSES; i++) {
		if (i != current && processes[i].state == OS_PS_READY) {
			schedulingInfo.age[i] += processes[i].priority;
		}
	}
	
	
	// Der �lteste Prozess wird ausgew�hlt.
	uint8_t oldest_proc = 1;
	for (uint8_t i = 2; i < MAX_NUMBER_OF_PROCESSES; i++) {
		
		if (processes[i].state != OS_PS_READY) {
			continue;
		}
		
		if (schedulingInfo.age[i] > schedulingInfo.age[oldest_proc]) {
			oldest_proc = i;
		}
		
		if (schedulingInfo.age[i] == schedulingInfo.age[oldest_proc]) {
			if (processes[i].priority == processes[oldest_proc].priority) {
				// Existieren mehrere Prozesse mit gleichem Alter und gleicher Priorit�t,
				// wird der Prozess mit der kleinsten Prozess-ID ausgew�hlt
				oldest_proc = i < oldest_proc ? i : oldest_proc;
			} else {
				// Existieren mehrere Prozesse mit dem gleichen Alter, wird der h�chstpriorisierte ausgew�hlt.
				oldest_proc = processes[i].priority > processes[oldest_proc].priority ? i : oldest_proc;
			}
		}
	}
	
	// Nach dem ersten Prozesswechsel soll der Leerlaufprozess nur noch aufgerufen werden,
	// sofern kein anderer Prozess zur Verf�gung steht.
	if (processes[oldest_proc].state != OS_PS_READY) {
		return 0;
	}
	
	// Das Alter des ausgew�hlten Prozesses auf dessen Priorit�t zur�ckgesetzt.
	schedulingInfo.age[oldest_proc] = processes[oldest_proc].priority;	
	
	return oldest_proc;
}

/*!
 *  This function realizes the run-to-completion strategy.
 *  As long as the process that has run before is still ready, it is returned again.
 *  If  it is not ready, the even strategy is used to determine the process to be returned
 *
 *  \param processes An array holding the processes to choose the next process from.
 *  \param current The id of the current process.
 *  \return The next process to be executed, determined based on the run-to-completion strategy.
 */
ProcessID os_Scheduler_RunToCompletion(Process const processes[], ProcessID current) {
    // This is a presence task
	if ((processes[current].state == OS_PS_READY) && current != 0) {
		return current;
	} else {
		return os_Scheduler_Even(processes, current);
	}
}

ProcessID os_Scheduler_MLFQ(Process const processes[], ProcessID current) {

	for (uint8_t i = 0; i < 4; ++i) {
		ProcessQueue *q = &schedulingInfo.qs[i];
		if (pqueue_hasNext(q)) {
			ProcessID next = pqueue_getFirst(q);

			if (processes[next].state == OS_PS_UNUSED) {
				pqueue_dropFirst(q);
				if (pqueue_hasNext(q)) {
					next = pqueue_getFirst(q);
				} else {
					continue;
				}
			}

			if (processes[next].state == OS_PS_BLOCKED) {
				pqueue_dropFirst(q);
				pqueue_append(q, next);
				ProcessID nextnext = pqueue_getFirst(q);
				if (nextnext == next) {
					continue;
				} else {
					next = nextnext;
				}
			}

			if (--schedulingInfo.mlfq_slice[next] == 0) {
				pqueue_dropFirst(q);
				uint8_t a = i != 3 ? 1 : 0;
				pqueue_append(q + a, next);
				schedulingInfo.mlfq_slice[next] = 1 << (i + a);
			}
			return next;
		}
	}
	return 0;
}