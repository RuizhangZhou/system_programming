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

//! Structure used to store specific scheduling informations such as a time slice
// This is a presence task
typedef struct {
    uint8_t timeSlice;//Zeitscheibe
    Age age[MAX_NUMBER_OF_PROCESSES];//age of every Process
    ProcessQueue levelQueues[4];//4 Klasses(00,01,10,11)
    uint8_t mlfqSlices[MAX_NUMBER_OF_PROCESSES];
}SchedulingInformation;

//Ringbuffer for process queueing.
typedef struct{
    ProcessID data[MAX_NUMBER_OF_PROCESSES];
    uint8_t size;
    ProcessID head;
    ProcessID tail;
}ProcessQueue;

//Initialises the scheduling information.
void os_initSchedulingInformation();

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

//MultiLevelFeedbackQueue strategy.
ProcessID os_Scheduler_MLFQ(Process const processes[], ProcessID current);

//Function that removes the given ProcessID from the ProcessQueues.
void MLFQ_removePID(ProcessID pid);

//Returns the corresponding ProcessQueue.
ProcessQueue *MLFQ_getQueue(uint8_t queueID);

//Returns the default number of timeslices for a specific ProcessQueue/priority class.
uint8_t MLFQ_getDefaultTimeslice(uint8_t queueID);

//Maps a process-priority to a priority class.
uint8_t MLFQ_MapToQueue(Priority prio);

//Initializes the given ProcessQueue with a predefined size. 
void pqueue_init(ProcessQueue *queue);

//Resets the given ProcessQueue. 
void pqueue_reset(ProcessQueue *queue);

//Checks whether there is next a ProcessID.
uint8_t pqueue_hasNext(ProcessQueue *queue);

//Returns the first ProcessID of the given ProcessQueue.
ProcessID pqueue_getFirst(ProcessQueue *queue);

//Drops the first ProcessID of the given ProcessQueue.
void pqueue_dropFirst(ProcessQueue *queue);

//Appends a ProcessID to the given ProcessQueue.
void pqueue_append(ProcessQueue *queue, ProcessID pid);



















#endif
