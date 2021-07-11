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
#include "defines.h"
#include "os_scheduler.h"

typedef struct ProcessQueue {
  ProcessID data[MAX_NUMBER_OF_PROCESSES];
  uint8_t size;
  uint8_t head;
  uint8_t tail;
} ProcessQueue;

typedef struct {
  uint8_t timeSlice;
  Age age[MAX_NUMBER_OF_PROCESSES];
  uint8_t mlfq_slice[8];
  ProcessQueue queues[4];
} SchedulingInformation;

void os_resetSchedulingInformation(SchedulingStrategy strategy);
ProcessID os_Scheduler_InactiveAging(Process const processes[],
                                     ProcessID current);

ProcessID os_Scheduler_RoundRobin(Process const processes[], ProcessID current);

void pqueue_init(ProcessQueue *queue);

void pqueue_reset(ProcessQueue *queue);

ProcessID os_Scheduler_RunToCompletion(Process const processes[],
                                       ProcessID current);
void pqueue_dropFirst(ProcessQueue *queue);
ProcessQueue *MLFQ_getQueue(uint8_t queueID);
void pqueue_append(ProcessQueue *queue, ProcessID pid);

ProcessID os_Scheduler_Even(Process const processes[], ProcessID current);
ProcessID os_Scheduler_Random(Process const processes[], ProcessID current);
uint8_t pqueue_hasNext(ProcessQueue *queue);

ProcessID pqueue_getFirst(ProcessQueue *queue);

void os_removeFromMlfq(ProcessID id);

ProcessID os_Scheduler_MLFQ(Process const processes[], ProcessID current);
void os_resetProcessSchedulingInformation(ProcessID id);

#endif
