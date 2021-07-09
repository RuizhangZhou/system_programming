#include "os_scheduling_strategies.h"
#include "defines.h"
#include <stdlib.h>

SchedulingInformation schedulingInfo;//global Variable


/*!
 *  Reset the scheduling information for a specific strategy
 *  This is only relevant for RoundRobin and InactiveAging
 *  and is done when the strategy is changed through os_setSchedulingStrategy
 *
 * \param strategy  The strategy to reset information for
 */
void os_resetSchedulingInformation(SchedulingStrategy strategy) {
    // This is a presence task
	switch (strategy)
	{
		case OS_SS_ROUND_ROBIN:
			schedulingInfo.timeSlice=os_getProcessSlot(os_getCurrentProc())->priority;
			break;
		case OS_SS_INACTIVE_AGING:
			for(uint8_t i=0; i<MAX_NUMBER_OF_PROCESSES;i++){
				schedulingInfo.age[i]=0;
			}
			break;
		case OS_SS_MULTI_LEVEL_FEEDBACK_QUEUE:
			os_initSchedulingInformation();
			break; 
		default :
			break;
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
	schedulingInfo.age[id]=0;

	if(os_getSchedulingStrategy()==OS_SS_MULTI_LEVEL_FEEDBACK_QUEUE){
		MLFQ_removePID(id);
		// after execution of process add it to the back of its corresponding class 
		uint8_t klass=MLFQ_MapToQueue(os_getProcessSlot(id)->priority);
		pqueue_append(MLFQ_getQueue(klass), id);
		schedulingInfo.mlfqSlices[id]=MLFQ_getDefaultTimeslice(klass);
	}
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
    //#warning IMPLEMENT STH. HERE
    uint8_t nextProc = (current + 1) % MAX_NUMBER_OF_PROCESSES;
	while (nextProc != current) {
		if (processes[nextProc].state == OS_PS_READY && nextProc != 0) {
			return nextProc;
		}
		nextProc = (nextProc + 1) % MAX_NUMBER_OF_PROCESSES;
	}
	//here represents all other Processes(not include processes[0],as this represents the Leerlaufprozess) in the processes are not ready
	if (processes[nextProc].state == OS_PS_READY) {
		return nextProc;
	} else {//all Processes are not ready, now run the Leerlaufprozess
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
    //#warning IMPLEMENT STH. HERE
    //count the currently ready process excluding the idle process
	uint8_t count = 0;
	for (uint8_t i = 0; i<MAX_NUMBER_OF_PROCESSES;i++)
	{
		if(processes[i].state == OS_PS_READY && i != 0)
			count++;
	}

	//get a random number 
	count = rand()%count;
	//iterate the process array until we get the next process
	for(uint8_t i = 0; i<MAX_NUMBER_OF_PROCESSES;i++){
		//since were using a random number between 0 and count we can just check if the process[i] is ready and not idle process and if its true check if the new count is 0.  
		if(processes[i].state==OS_PS_READY&&i!=0){
			//if it is 0 then return the id
			if(count == 0) return i;
			//else decrement the count
			count--;
		}
	}
	//if there is no process ready then return the idle process id
	return 0;
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
    // This is a presence task
	schedulingInfo.timeSlice--;
	if(schedulingInfo.timeSlice<=0 || !os_isRunnable(&processes[current])){
		ProcessID pid=os_Scheduler_Even(processes,current);
		//if no Process isRunnable, os_Schedular_Even will still return 0 back as the Leeflaufprozess
		schedulingInfo.timeSlice=processes[pid].priority;
		return pid;
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
    // This is a presence task
	for(uint8_t i=1;i<MAX_NUMBER_OF_PROCESSES;i++){
		if(i!=current && os_isRunnable(&processes[current])){
			schedulingInfo.age[i]+=processes[i].priority;
		}
	}
	uint8_t pid=0;
	//processes[0] is Leerlaufprozess, availabe prozess is processes[1] to processes[MAX_NUMBER_OF_PROCESSES-1]?
	for(uint8_t i=1;i<MAX_NUMBER_OF_PROCESSES;i++){
		if(!os_isRunnable(&processes[i])) continue;
		if(pid==0){
			pid=i;
			continue;
		}
		if(schedulingInfo.age[i]>schedulingInfo.age[pid]){
			pid=i;
		}else if(schedulingInfo.age[i]==schedulingInfo.age[pid]){
			if(processes[i].priority>processes[i].priority){
				pid=i;
			}
			/*
			else if(processes[i].priority==processes[i].priority){
				if(i<pid){
					pid=i;
				}
			}
			*/
		}
	}
	schedulingInfo.age[pid]=processes[pid].priority;
    return pid;//if pid=0 here means that no other process isRunnable
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
	if(os_isRunnable(&processes[current]) && current!=0){
		return current;
	}else{
		return os_Scheduler_Even(processes,current);
	}
}

//MultiLevelFeedbackQueue strategy.
ProcessID os_Scheduler_MLFQ(Process const processes[], ProcessID current){
	for (uint8_t id = 0; id < 4; id++) {
		ProcessQueue *queue = MLFQ_getQueue(id);
		uint8_t numInQueue=(queue->head+ MAX_NUMBER_OF_PROCESSES - queue->tail) % MAX_NUMBER_OF_PROCESSES;//tail:7 head:0 
		for (uint8_t j=0;j<numInQueue;j++) {
			ProcessID next = pqueue_getFirst(queue);
			if(processes[next].state==OS_PS_UNUSED){
				pqueue_dropFirst(queue);
			}else if(processes[next].state==OS_PS_READY){
				if(schedulingInfo.mlfqSlices[next]==0){
					pqueue_dropFirst(queue);
					if(id==3){
						pqueue_append(queue, next);
						schedulingInfo.mlfqSlices[next]=MLFQ_getDefaultTimeslice(id);
					}else{
						pqueue_append( MLFQ_getQueue(id+1),next);
						schedulingInfo.mlfqSlices[next]=MLFQ_getDefaultTimeslice(id+1);
					}
					continue;
				}
				schedulingInfo.mlfqSlices[next]--;
				return next;
			}else if(processes[next].state==OS_PS_BLOCKED){
				if(schedulingInfo.mlfqSlices[next]==0 && id<=2){
					pqueue_dropFirst(queue);
					pqueue_append( MLFQ_getQueue(id+1),next);
					schedulingInfo.mlfqSlices[next]=MLFQ_getDefaultTimeslice(id+1);
				}else{
					pqueue_dropFirst(queue);
					pqueue_append(queue, next);
				}
				
			}
		}
	}
	return 0;	
}

void os_initSchedulingInformation(){
	// initialise level queues
	for (uint8_t level = 0; level < 4; level++) {
		pqueue_init(MLFQ_getQueue(level));
	}
	// set initial mlfqSlices
	for (uint8_t id = 1; id < 8; id++) {
		if (os_getProcessSlot(id)->state != OS_PS_UNUSED) {
			uint8_t klass = MLFQ_MapToQueue(os_getProcessSlot(id)->priority);//beide MSBs der Priorität
			pqueue_append(MLFQ_getQueue(klass), id);
			schedulingInfo.mlfqSlices[id] = MLFQ_getDefaultTimeslice(klass);
		}
	}
}

//bool isAnyProcReady(Process const processes[]){}

//Removes a ProcessID from the given ProcessQueue.
void pqueue_removePID(ProcessQueue *queue, ProcessID pid){
	uint8_t oldSize=queue->size;
	for(uint8_t i=0;i<oldSize;i++){//loop the original size times of the queue, to keep the original oder
		if(pqueue_getFirst(queue)!=pid){//if not the pid to be deleted, then move it to the back of queue
			pqueue_append(queue,pqueue_getFirst(queue));
		}
		pqueue_dropFirst(queue);
		if(!pqueue_hasNext(queue)){
			return;
		}
	}
}

//Function that removes the given ProcessID from the ProcessQueues.
void MLFQ_removePID(ProcessID pid){
	for (uint8_t i =0;i<4;i++){
		pqueue_removePID(MLFQ_getQueue(i),pid);
	}
}

//Returns the corresponding ProcessQueue.
ProcessQueue *MLFQ_getQueue(uint8_t queueID){
	if (queueID > 3){
		 return NULL;
	}
	return &schedulingInfo.levelQueues[queueID];
}

//Returns the default number of timeslices for a specific ProcessQueue/priority class.
uint8_t MLFQ_getDefaultTimeslice(uint8_t queueID){
	return 1<<(3 - queueID);////klass:3(1),2(2),1(4),0(8)
}

//Maps a process-priority to a priority class.
uint8_t MLFQ_MapToQueue(Priority prio){
	return prio>>6;//beide MSBs der Priorität
}

//Initializes the given ProcessQueue with a predefined size. 
void pqueue_init(ProcessQueue *queue){
	queue->size=MAX_NUMBER_OF_PROCESSES;
	queue->head=0;
	queue->tail=0;
	for(uint8_t i=0;i<8;i++){
		queue->data[i]=0;
	}
}

//Resets the given ProcessQueue. 
void pqueue_reset(ProcessQueue *queue){
	queue->head = 0;
	queue->tail = 0;
}

//Checks whether there is next a ProcessID.
uint8_t pqueue_hasNext(ProcessQueue *queue){
	return queue->head != queue->tail;
}

//Returns the first ProcessID of the given ProcessQueue.
ProcessID pqueue_getFirst(ProcessQueue *queue){
	if (pqueue_hasNext(queue)){
		return queue->data[queue->tail];
	} else {
		return 0;
	}
}

//Drops the first ProcessID of the given ProcessQueue.
void pqueue_dropFirst(ProcessQueue *queue){
	if (pqueue_hasNext(queue)){
		queue->data[queue->tail] = 0;
		queue->tail = ((queue->tail) + 1) % (queue->size);
	} 
}

//Appends a ProcessID to the given ProcessQueue.
void pqueue_append(ProcessQueue *queue, ProcessID pid){
	queue->data[queue->head] = pid;
	queue->head = (queue->head + 1) % queue->size;
}



















