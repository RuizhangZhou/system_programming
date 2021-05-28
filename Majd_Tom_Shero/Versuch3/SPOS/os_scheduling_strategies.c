#include "os_scheduling_strategies.h"
#include "defines.h"

#include <stdlib.h>



/*!
 *  Reset the scheduling information for a specific strategy
 *  This is only relevant for RoundRobin and InactiveAging
 *  and is done when the strategy is changed through os_setSchedulingStrategy
 *
 * \param strategy  The strategy to reset information for
 */
void os_resetSchedulingInformation(SchedulingStrategy strategy) {
    if(strategy == OS_SS_ROUND_ROBIN){
		schedulingInfo.timeSlice = os_getProcessSlot(os_getCurrentProc())->priority; 
	} else schedulingInfo.timeSlice = 0;
	if(strategy == OS_SS_INACTIVE_AGING){
		for (int i = 0; i < MAX_NUMBER_OF_PROCESSES; i++)
		{
			os_resetProcessSchedulingInformation(i);
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
	//set the count/next process id to current + 1
	int count = current+1;
	while(1){
		//if were back at the current id return the current if its ready again, if not then the idle process
		if(count == current){
			if (processes[count].state == OS_PS_READY) return current;
			else return 0;
		}
		//if its not the its not the current process and not idle then look if its ready
		if(processes[count].state == OS_PS_READY && count!= 0) return count;
		//go to next
		count++;
		//don't get out of bounds
		count = count % MAX_NUMBER_OF_PROCESSES;
	}
	return 0;
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
    //count the currently ready process excluding the idle process
	int count = 0;
	for (int i = 0; i<MAX_NUMBER_OF_PROCESSES;i++)
	{
		if(processes[i].state == OS_PS_READY && i != 0)
			count++;
	}

	//get a random number 
	count = rand()%count;
	//iterate the process array until we get the next process
	for(int i = 0; i<MAX_NUMBER_OF_PROCESSES;i++){
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
	// wait if timeSlice > 0 
	if(schedulingInfo.timeSlice > 0) {
		if (processes[current].state != OS_PS_READY && processes[current].state != OS_PS_RUNNING) return 0; 
		return current;
	}
	ProcessID pid = os_Scheduler_Even(processes, current);
	
	schedulingInfo.timeSlice = processes[pid].priority;
	
	
	if (processes[pid].state != OS_PS_READY && processes[pid].state != OS_PS_RUNNING)
		return 0; 
	return pid;
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
    for (uint8_t i = 1; i < MAX_NUMBER_OF_PROCESSES; i++) {
	    if (i != current && processes[i].state == OS_PS_READY) {
		    schedulingInfo.age[i] += processes[i].priority;
	    }
    }
	
	// variable for oldest process 
	ProcessID pid = 1;
	
	// iterate over processes and store the oldest
	for (uint8_t i = 2; i < MAX_NUMBER_OF_PROCESSES; i++)
	{
		if (processes[i].state != OS_PS_READY && processes[i].state != OS_PS_RUNNING) continue;
		
		// check for maximal age
		if (schedulingInfo.age[i] > schedulingInfo.age[pid]) {
			pid = i;
		} 
		// check for age evenness 
		if (schedulingInfo.age[i] == schedulingInfo.age[pid]){
			if (processes[i].priority == processes[pid].priority) {
				pid = i < pid ? i : pid;
			} else {
				pid = processes[i].priority > processes[pid].priority ? i : pid;
			}
		}		
		
	}
	
	if (processes[pid].state != OS_PS_READY && processes[pid].state != OS_PS_RUNNING)
		return 0;
	
	schedulingInfo.age[pid] = processes[pid].priority;
	return pid;
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
    if(processes[current].state == OS_PS_READY) return current;
    return os_Scheduler_Even(processes,	current);
}
