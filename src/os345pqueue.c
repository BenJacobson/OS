// os345pqueue.c - OS Kernel	09/15/2016
// ***********************************************************************
// **   DISCLAMER ** DISCLAMER ** DISCLAMER ** DISCLAMER ** DISCLAMER   **
// **                                                                   **
// ** The code given here is the basis for the BYU CS345 projects.      **
// ** It comes "as is" and "unwarranted."  As such, when you use part   **
// ** or all of the code, it becomes "yours" and you are responsible to **
// ** understand any algorithm or method presented.  Likewise, any      **
// ** errors or problems become your responsibility to fix.             **
// **                                                                   **
// ** NOTES:                                                            **
// ** -Comments beginning with "// ??" may require some implementation. **
// ** -Tab stops are set at every 3 spaces.                             **
// ** -The function API's in "OS345.h" should not be altered.           **
// **                                                                   **
// **   DISCLAMER ** DISCLAMER ** DISCLAMER ** DISCLAMER ** DISCLAMER   **
// ***********************************************************************

#include <setjmp.h>
#include <assert.h>
#include <stdlib.h>

#include "os345.h"
#include "os345pqueue.h"

extern TCB tcb[];

// **********************************************************************
// **********************************************************************
// Queue functions

// **********************************************************************
// **********************************************************************
// enqueue
int enqueueTask(PQueue pqueue, TID taskID) {
	assert("Positive taskID" && taskID >= 0);
	assert("Queue overflow" && pqueue[0] < MAX_TASKS - 2);

	Priority insertPriority = tcb[taskID].priority;
	int numTasks = pqueue[0]++;
	// Find insert position
	while (numTasks) {
		pqueue[numTasks+1] = pqueue[numTasks];
		if (insertPriority > tcb[pqueue[numTasks+1]].priority) {
			pqueue[numTasks+1] = taskID;
			return taskID;
		}
		numTasks--;
	}
	// insert in lowest spot
	pqueue[1] = taskID;
	return taskID;
}

// **********************************************************************
// **********************************************************************
// dequeue
TID dequeueTask(PQueue pqueue) {
	if (pqueue[0])
		return pqueue[pqueue[0]--];
	else
		return -1;
}

// **********************************************************************
// **********************************************************************
// new PQueue
PQueue newPQueue() {
    PQueue pqueue = (PQueue)malloc(MAX_TASKS * sizeof(int));
	return pqueue;
}
