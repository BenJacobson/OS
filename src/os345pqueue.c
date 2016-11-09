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
#include <stdio.h>

#include "os345.h"
#include "os345pqueue.h"

extern TCB tcb[];

// **********************************************************************
// **********************************************************************
// Queue functions

// **********************************************************************
// **********************************************************************
// Enqueue Task
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
} // end enqueueTask

// **********************************************************************
// **********************************************************************
// Dequeue Task
TID dequeueTask(PQueue pqueue, int useFairSchedule) {
	if (pqueue[0] == 0) {
		return -1;
	} else if (useFairSchedule) {				// fair share
		for (int i=pqueue[0]; i>0; i--) {
			if (tcb[pqueue[i]].timeLeft) {
				tcb[pqueue[i]].timeLeft--;
				TID nextTask = pqueue[i];
				for (int j=i; j<=pqueue[0]; j++) {
					pqueue[j] = pqueue[j+1];
				}
				pqueue[0]--;
				return nextTask;
			}
		}
		return -1;
	} else {									// round robin priority
		return pqueue[pqueue[0]--];
	}
} // end dequeueTask

// **********************************************************************
// **********************************************************************
// Remove Task
void removeTask(PQueue pqueue, TID taskID) {
	int i, size = *pqueue;
	if (size) {
		for (i=1; i<=size; i++) {
			if (pqueue[i] == taskID) {
				break;
			}
		}
		while (i<size) {
			pqueue[i] = pqueue[i+1];
			i++;
		}
		(*pqueue)--;
	}
} // end removeTask

// **********************************************************************
// **********************************************************************
// New PQueue
PQueue newPQueue() {
    PQueue pqueue = (PQueue)malloc(MAX_TASKS * sizeof(int));
	pqueue[0] = 0;
	return pqueue;
} // end newPQueue

// **********************************************************************
// **********************************************************************
// Print PQueue
void printPQueue(PQueue pqueue) {
	printf("\nPrinting Queue");
    for (int i=pqueue[0]; i>0; i--) {
		printf("\n%d", pqueue[i]);
	}
	printf("\nEnd");
} // end newPQueue
