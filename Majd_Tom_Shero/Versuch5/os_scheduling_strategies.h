/*! \file
 *  \brief Scheduling library for the OS.
 *
 *  Contains the scheduling strategies.
 *
 *  \author   Lehrstuhl Informatik 11 - RWTH Aachen
 *  \date     2013
 *  \version  2.0
 */

#ifndef _OS_SCHEDULING_STRATEGIES_H
#define _OS_SCHEDULING_STRATEGIES_H

#include "os_scheduler.h"
#include "defines.h"

//! 
typedef struct {
	ProcessID data[MAX_NUMBER_OF_PROCESSES];
	int size;
	ProcessID head;
	ProcessID tail;
} ProcessQueue;


//! Used to store specific scheduling informations such as a time slice
typedef struct {
	uint8_t timeSlice;	
	uint8_t quamtumManager[MAX_NUMBER_OF_PROCESSES];
	Age age[MAX_NUMBER_OF_PROCESSES];
	ProcessQueue levelQueue[4];  
} SchedulingInformation;

//!
SchedulingInformation schedulingInfo;

//! Used to reset the SchedulingInfo for one process
void os_resetProcessSchedulingInformation(ProcessID id);

//! Used to reset the SchedulingInfo for a strategy
void os_resetSchedulingInformation(SchedulingStrategy strategy);

//! Even strategy
ProcessID os_Scheduler_Even(Process const processes[], ProcessID current);

//! Random strategy
ProcessID os_Scheduler_Random(Process const processes[], ProcessID current);

//! RoundRobin strategy
ProcessID os_Scheduler_RoundRobin(Process const processes[], ProcessID current);

//! InactiveAging strategy
ProcessID os_Scheduler_InactiveAging(Process const processes[], ProcessID current);

//! RunToCompletion strategy
ProcessID os_Scheduler_RunToCompletion(Process const processes[], ProcessID current);

//! Multi level feedback queue strategy
ProcessID os_Scheduler_MLFQ(Process const processes[], ProcessID current);

//! 
void 	pqueue_init (ProcessQueue *queue);

//! 
void 	pqueue_reset (ProcessQueue *queue);

//! 
uint8_t 	pqueue_hasNext (ProcessQueue *queue);

//! 
ProcessID 	pqueue_getFirst (ProcessQueue *queue);

//! 
void 	pqueue_dropFirst (ProcessQueue *queue);

//! 
void 	pqueue_append (ProcessQueue *queue, ProcessID pid);

//! 
void	pqueue_removePID(ProcessQueue *queue, ProcessID pid);

//! 
void os_initSchedulingInformation();

//! 
ProcessQueue* MLFQ_getQueue(uint8_t queueID	);

//!
void MLFQ_removePID(ProcessID pid);

#endif
