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
    // This is a presence task
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
		//dont get out of bounds
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
    //#warning IMPLEMENT STH. HERE
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
    // This is a presence task
    return 0;
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
    return 0;
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
    return 0;
}
