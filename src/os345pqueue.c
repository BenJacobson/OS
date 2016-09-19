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
	for (int i = 0; i < MAX_TASKS; i++) {
		if (pqueue[i] == -1) {
			pqueue[i] = taskID;
			return 0;			
		}
	}
	return -1;
}

// **********************************************************************
// **********************************************************************
// dequeue
TID dequeueTask(PQueue pqueue) {
	int i, taskIndex = -1;
	TID taskID = -1;
	Priority taskPriority = -1;
	for (i = 0; i < MAX_TASKS && pqueue[i] != -1; i++) {
		if (pqueue[i] != -1 && tcb[pqueue[i]].priority > taskPriority) {
			taskIndex = i;
			taskID = pqueue[i];
			taskPriority = tcb[pqueue[i]].priority;
		}
	}
	if (taskIndex != -1) {
		for (i = taskIndex; i < MAX_TASKS-1 && pqueue[i] != -1; i++) {
			pqueue[i] = pqueue[i+1];
		}
	}
	pqueue[MAX_TASKS-1] = -1;		// set the last task to empty to cover the case when you dequeue a full PQueue
	return taskID;
}

// **********************************************************************
// **********************************************************************
// new PQueue
PQueue newPQueue(int size) {
	assert("Positive PQueue size" && size > 0);
    PQueue pqueue = (PQueue)malloc(size * sizeof(int));
	for (int i=0; i<MAX_TASKS; i++)
		pqueue[i] = -1;
	return pqueue;
}
