// os345tasks.c - OS create/kill task	08/08/2013
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>
#include <time.h>
#include <assert.h>

#include "os345.h"
#include "os345signals.h"
#include "os345pqueue.h"


extern TCB tcb[];							// task control block
extern int curTask;							// current task #

extern PQueue readyQueue;					// queue of tasks waiting to run

extern int superMode;						// system mode
extern Semaphore* semaphoreList;			// linked list of active semaphores
extern Semaphore* taskSems[MAX_TASKS];		// task semaphore


// **********************************************************************
// **********************************************************************
// create task
int createTask(char* name,						// task name
				int (*task)(int, char**),		// task address
				int priority,					// task priority
				int timeSlice,					// task time slice
				int argc,						// task argument count
				char* argv[])					// task argument pointers
{
	assert("Invalid time slice" && timeSlice > 0);
	int tid;

	// find an open tcb entry slot
	for (tid = 0; tid < MAX_TASKS; tid++)
	{
		if (tcb[tid].name == 0)
		{
			char buf[8];

			// create task semaphore
			if (taskSems[tid]) deleteSemaphore(&taskSems[tid]);
			sprintf(buf, "task%d", tid);
			taskSems[tid] = createSemaphore(buf, 0, 0);
			taskSems[tid]->taskNum = 0;	// assign to shell

			// copy task name
			tcb[tid].name = (char*)malloc(strlen(name)+1);
			strcpy(tcb[tid].name, name);

			// set task address and other parameters
			tcb[tid].task = task;			// task address
			tcb[tid].state = S_NEW;			// NEW task state
			tcb[tid].priority = priority;	// task priority
			tcb[tid].timeSlice = timeSlice;	// time slices given
			tcb[tid].timeSliceCount = 0;	// time slices used this swap
			tcb[tid].parent = curTask;		// parent
			tcb[tid].argc = argc;			// argument count

			tcb[tid].argv = malloc(sizeof(char*) * (argc+1));			// argument pointer
			for (int i = 0; i < argc; i++)
			{
				tcb[tid].argv[i] = malloc(sizeof(char) * (strlen(argv[i])+1));
				strcpy(tcb[tid].argv[i], argv[i]);
			}

			tcb[tid].event = 0;				// suspend semaphore
			tcb[tid].RPT = 0;					// root page table (project 5)
			tcb[tid].cdir = CDIR;			// inherit parent cDir (project 6)

			// define task signals
			createTaskSigHandlers(tid);

			// Each task must have its own stack and stack pointer.
			tcb[tid].stack = malloc(STACK_SIZE * sizeof(int));

			enqueueTask(readyQueue, tid);

			if (!superMode) swapTask();				// do context switch (if not cli)
			return tid;							// return tcb index (curTask)
		}
	}
	// tcb full!
	return -1;
} // end createTask



// **********************************************************************
// **********************************************************************
// kill task
//
//	taskID == -1 => kill all non-shell tasks
//
static void exitTask(int taskID);
int killTask(int taskID)
{
	if (taskID < 0 || taskID >= NUM_SYS_TASKS)			// don't terminate shell
	{
		if (taskID < 0)			// kill all tasks
		{
			int tid;
			for (tid = 1; tid < MAX_TASKS; tid++)
			{
				if (tcb[tid].name) exitTask(tid);
			}
		}
		else
		{
			// terminate individual task
			if (!tcb[taskID].name) return 1;
			exitTask(taskID);	// kill individual task
		}
	}
	if (!superMode) SWAP;
	return 0;
} // end killTask

static void exitTask(int taskID)
{
	assert("exitTaskError" && tcb[taskID].name);

	if (tcb[taskID].state == S_BLOCKED) {
		removeTask(tcb[taskID].event->blockedQueue, taskID);
	}
	tcb[taskID].state = S_EXIT;			// EXIT task state
	return;
} // end exitTask



// **********************************************************************
// system kill task
//
int sysKillTask(int taskID)
{
	
	Semaphore* sem = semaphoreList;
	Semaphore** semLink = &semaphoreList;
	// Print task name
	printf("\nKill Task[%d] %s", taskID, tcb[taskID].name);
	// assert that you are not pulling the rug out from under yourself!
	assert("sysKillTask Error" && tcb[taskID].name && superMode);

	// signal task terminated
	semSignal(taskSems[taskID]);

	// free argv
	for (int i=0; i < tcb[taskID].argc; i++) {
		free(tcb[taskID].argv[i]);
		tcb[taskID].argv[i]= 0;
	}
	free(tcb[taskID].argv);
	tcb[taskID].argv = 0;

	// look for any semaphores created by this task
	while(sem = *semLink)
	{
		if(sem->taskNum == taskID)
		{
			// semaphore found, delete from list, release memory
			deleteSemaphore(semLink);
		}
		else
		{
			// move to next semaphore
			semLink = (Semaphore**)&sem->semLink;
		}
	}

	// ?? delete task from system queues

	tcb[taskID].name = 0;			// release tcb slot
	return 0;
} // end sysKillTask
