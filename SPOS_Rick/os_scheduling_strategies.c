#include "os_scheduling_strategies.h"
#include "defines.h"
#include <stdlib.h>

SchedulingInformation schedulingInfo; // global Variable

void pqueue_init(ProcessQueue *q) {
  q->size = MAX_NUMBER_OF_PROCESSES;
  q->head = 0;
  q->tail = 0;
  for (int i = 0; i < 8; ++i) {
    q->data[i] = 0;
  }
}

// Checks whether there is next a ProcessID.
uint8_t pqueue_hasNext(ProcessQueue *queue) {
  return queue->head != queue->tail;
}

// Returns the first ProcessID of the given ProcessQueue.
ProcessID pqueue_getFirst(ProcessQueue *queue) {
  return queue->data[queue->head];
}

// Resets the given ProcessQueue.
void pqueue_reset(ProcessQueue *q) {
  q->head = 0;
  q->tail = 0;
}

// Drops the first ProcessID of the given ProcessQueue.
void pqueue_dropFirst(ProcessQueue *queue) {
  queue->data[queue->head] = 0;
  queue->head = (queue->head + 1) % queue->size;
}

// Appends a ProcessID to the given ProcessQueue.
void pqueue_append(ProcessQueue *queue, ProcessID pid) {
  queue->data[queue->tail] = pid;
  queue->tail = (queue->tail + 1) % queue->size;
}

// Returns the corresponding ProcessQueue.
ProcessQueue *MLFQ_getQueue(uint8_t queueID) {
  return &schedulingInfo.queues[queueID];
}

void os_initSchedulingInformation() {
  for (uint8_t i = 0; i < 4; ++i) {
    pqueue_init(&schedulingInfo.queues[i]);
  }
  for (uint8_t j = 1; j < 8; j++) {
    if (os_getProcessSlot(j)->state != OS_PS_UNUSED) {
      uint8_t q = os_getProcessSlot(j)->priority >> 6;
      ProcessQueue *p = &schedulingInfo.queues[3 - q];
      pqueue_append(p, j);
      schedulingInfo.mlfq_slice[j] = 1 << (3 - q);
    }
  }
}

void os_resetSchedulingInformation(SchedulingStrategy strategy) {
  // This is a presence task
  switch (strategy) {
  case OS_SS_RUN_TO_COMPLETION:
    break;
  case OS_SS_ROUND_ROBIN:
    schedulingInfo.timeSlice = os_getProcessSlot(os_getCurrentProc())->priority;
    break;
  case OS_SS_RANDOM:
    break;
  case OS_SS_EVEN:
    break;
  case OS_SS_INACTIVE_AGING:
    for (uint8_t j = 0; j < MAX_NUMBER_OF_PROCESSES; j++) {
      schedulingInfo.age[j] = 0;
    }
    break;
  case OS_SS_MULTI_LEVEL_FEEDBACK_QUEUE:
    os_initSchedulingInformation();
    break;
  }
}

void os_removeFromMlfq(ProcessID id) {

  ProcessQueue localQueue;
  pqueue_init(&localQueue);
  ProcessQueue *processQueue;

  for (int j = 0; j < 4; ++j) {
    processQueue = &schedulingInfo.queues[j];
    while (pqueue_hasNext(processQueue)) {
      uint8_t t = pqueue_getFirst(processQueue);
      pqueue_dropFirst(processQueue);
      if (t != id) {
        pqueue_append(&localQueue, t);
      }
    }
    while (pqueue_hasNext(&localQueue)) {
      uint8_t temp = pqueue_getFirst(&localQueue);
      pqueue_dropFirst(&localQueue);
      pqueue_append(processQueue, temp);
    }
  }
}

void os_resetProcessSchedulingInformation(ProcessID id) {
  // This is a presence task

  // schedulingInfo.age[id] = 0;
  if (os_getSchedulingStrategy() == OS_SS_INACTIVE_AGING) {
    schedulingInfo.age[id] = 0;
  }

  os_removeFromMlfq(id);
  // after execution of process add it to the back of its corresponding class
  uint8_t q = os_getProcessSlot(id)->priority >> 6;
  ProcessQueue *pq = &schedulingInfo.queues[3 - q];
  pqueue_append(pq, id);
  schedulingInfo.mlfq_slice[id] = 1 << (3 - q);
}

ProcessID os_Scheduler_Even(Process const processes[], ProcessID curr) {
  uint8_t next = (curr + 1) % MAX_NUMBER_OF_PROCESSES;
  while (next != curr) {
    if (processes[next].state == OS_PS_READY && next != 0) {
      return next;
    } else {
      next = (next + 1) % MAX_NUMBER_OF_PROCESSES;
    }
  }
  // here represents all other Processes(not include processes[0],as this
  // represents the Leerlaufprozess) in the processes are not ready
  if (processes[next].state == OS_PS_READY) {
    return next;
  } else { // all Processes are not ready, now run the Leerlaufprozess
    return 0;
  }
}

ProcessID os_Scheduler_Random(Process const processes[], ProcessID current) {
  // count the currently ready process excluding the idle process
  uint8_t activPocess[MAX_NUMBER_OF_PROCESSES];
  uint8_t numAktivProcess = 0;
  // iterate the process array until we get the next process
  for (uint8_t j = 0; j < MAX_NUMBER_OF_PROCESSES; j++) {
    // since were using a random number between 0 and count we can just check if
    // the process[i] is ready and not idle process and if its true check if the
    // new count is 0.
    if (processes[j].state == OS_PS_READY && processes[j].progID != 0) {
      // if it is 0 then return the id
      // else decrement the count
      activPocess[numAktivProcess] = j;
      numAktivProcess++;
    }
  }

  if (numAktivProcess == 0) {
    return 0;
  }
  // if there is no process ready then return the idle process id

  return activPocess[rand() % numAktivProcess]; // get a random number
}

ProcessID os_Scheduler_RoundRobin(Process const processes[],
                                  ProcessID current) {
  // This is a presence task
  schedulingInfo.timeSlice--;

  if (schedulingInfo.timeSlice < 1 || !os_isRunnable(processes + current)) {
    ProcessID pid = os_Scheduler_Even(processes, current);
    // if no Process isRunnable, os_Schedular_Even will still return 0 back as
    // the Leeflaufprozess
    schedulingInfo.timeSlice = processes[pid].priority;
    return pid;
  }

  return current;
}

ProcessID os_Scheduler_InactiveAging(Process const processes[],
                                     ProcessID current) {
  // This is a presence task
  for (uint8_t j = 1; j < MAX_NUMBER_OF_PROCESSES; j++) {
    if (j != current && processes[j].state == OS_PS_READY) {
      schedulingInfo.age[j] += processes[j].priority;
    }
  }

  // processes[0] is Leerlaufprozess, availabe prozess is processes[1] to
  // processes[MAX_NUMBER_OF_PROCESSES-1]?
  uint8_t oldestProc = 1;
  for (uint8_t j = 2; j < MAX_NUMBER_OF_PROCESSES; j++) {

    if (processes[j].state != OS_PS_READY) {
      continue;
    }

    if (schedulingInfo.age[j] > schedulingInfo.age[oldestProc]) {
      oldestProc = j;
    }

    if (schedulingInfo.age[j] == schedulingInfo.age[oldestProc]) {
      if (processes[j].priority == processes[oldestProc].priority) {
        oldestProc = j < oldestProc ? j : oldestProc;
      } else {
        oldestProc = processes[j].priority > processes[oldestProc].priority
                         ? j
                         : oldestProc;
      }
    }
  }

  if (processes[oldestProc].state != OS_PS_READY) {
    return 0;
  }

  schedulingInfo.age[oldestProc] = processes[oldestProc].priority;

  return oldestProc;
}

ProcessID os_Scheduler_RunToCompletion(Process const processes[],
                                       ProcessID current) {
  // This is a presence task
  if ((processes[current].state == OS_PS_READY) && current != 0) {
    return current;
  } else {
    return os_Scheduler_Even(processes, current);
  }
}

ProcessID os_Scheduler_MLFQ(Process const processes[], ProcessID current) {

  for (uint8_t j = 0; j < 4; ++j) {
    ProcessQueue *curr_q = &schedulingInfo.queues[j];
    if (pqueue_hasNext(curr_q)) {
      ProcessID nextPid = pqueue_getFirst(curr_q);

      if (processes[nextPid].state == OS_PS_UNUSED) {
        pqueue_dropFirst(curr_q);
        if (pqueue_hasNext(curr_q)) {
          nextPid = pqueue_getFirst(curr_q);
        } else {
          continue;
        }
      }

      if (processes[nextPid].state == OS_PS_BLOCKED) {
        pqueue_dropFirst(curr_q);
        pqueue_append(curr_q, nextPid);
        ProcessID nextNextPid = pqueue_getFirst(curr_q);
        if (nextNextPid == nextPid) {
          continue;
        } else {
          nextPid = nextNextPid;
        }
      }

      if (--schedulingInfo.mlfq_slice[nextPid] == 0) {
        pqueue_dropFirst(curr_q);
        uint8_t x = j != 3 ? 1 : 0;
        pqueue_append(curr_q + x, nextPid);
        schedulingInfo.mlfq_slice[nextPid] = 1 << (j + x);
      }
      return nextPid;
    }
  }
  return 0;
}
