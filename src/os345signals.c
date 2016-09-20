// os345signal.c - signals
// ***********************************************************************
// **   DISCLAMER ** DISCLAMER ** DISCLAMER ** DISCLAMER ** DISCLAMER   **
// **                                                                   **
// ** The code given here is the basis for the CS345 projects.          **
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
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>
#include <assert.h>
#include "os345.h"
#include "os345signals.h"

extern TCB tcb[];							// task control block
extern int curTask;							// current task #

// ***********************************************************************
// ***********************************************************************
//	Call all pending task signal handlers
//
//	return 1 if task is NOT to be scheduled.
//
int signals(void)
{
	if (tcb[curTask].signal)
	{
		if (*(tcb[curTask].name) == 'T') {
			int x = 0;
		}
		// SIGCONT
		if (tcb[curTask].signal & mySIGCONT)
		{
			sigClear(curTask, mySIGCONT);
			(*tcb[curTask].sigContHandler)();
			tcb[curTask].state = S_READY;
		}
		// SIGINT
		if (tcb[curTask].signal & mySIGINT)
		{
			sigClear(curTask, mySIGINT);
			(*tcb[curTask].sigIntHandler)();
		}
		// SIGERM
		if (tcb[curTask].signal & mySIGTERM)
		{
			sigClear(curTask, mySIGTERM);
			(*tcb[curTask].sigTermHandler)();
		}
		// SIGTSTP
		if (tcb[curTask].signal & mySIGTSTP)
		{
			sigClear(curTask, mySIGTSTP);
			(*tcb[curTask].sigTstpHandler)();
		}
	}
	return 0;
}


// **********************************************************************
// **********************************************************************
//	Register task signal handlers
//
int sigAction(void (*sigHandler)(void), int sig)
{
	switch (sig)
	{
		case mySIGINT:
		{
			tcb[curTask].sigIntHandler = sigHandler;		// mySIGINT handler
			return 0;
		}
	}
	return 1;
}


// **********************************************************************
//	sigSignal - send signal to task(s)
//
//	taskID = task (-1 = all tasks)
//	sig = signal
//
int sigSignal(int taskID, int sig)
{
	// check for task
	if ((taskID >= 0) && tcb[taskID].name)
	{
		tcb[taskID].signal |= sig;
		return 0;
	}
	else if (taskID == -1)
	{
		for (taskID=0; taskID<MAX_TASKS; taskID++)
		{
			sigSignal(taskID, sig);
		}
		return 0;
	}
	// error
	return 1;
}

// **********************************************************************
//	sigClear - clear signal for task(s)
//
//	taskID = task (-1 = all tasks)
//	sig = signal
//
int sigClear(int taskID, int sig)
{
	// check for task
	if (taskID >= 0 && taskID < MAX_TASKS)
	{
		tcb[taskID].signal &= ~sig;
		return 0;
	}
	else if (taskID == -1)
	{
		for (taskID=0; taskID<MAX_TASKS; taskID++)
		{
			sigClear(taskID, sig);
		}
		return 0;
	}
	// error
	return 1;
}


// **********************************************************************
// **********************************************************************
//	Default signal handlers
//
void defaultSigContHandler(void)			// task mySIGCONT handler
{
	return;
}

void defaultSigIntHandler(void)				// task mySIGINT handler
{
	return;
}

void defaultSigKillHandler(void)			// task mySIGKILL handler
{
	killTask(curTask);
	return;
}

void defaultSigTermHandler(void)			// task mySIGTERM handler
{
	killTask(curTask);
	return;
}

void defaultSigTstpHandler(void)			// task mySIGTSTP handler
{
	sigSignal(-1, mySIGSTOP);
	return;
}

void createTaskSigHandlers(int tid)
{
	tcb[tid].signal = 0;
	if (tid)
	{	// inherit parent signal handlers
		tcb[tid].sigContHandler = tcb[curTask].sigContHandler;
		tcb[tid].sigIntHandler = tcb[curTask].sigIntHandler;
		tcb[tid].sigKillHandler = tcb[curTask].sigKillHandler;
		tcb[tid].sigTermHandler = tcb[curTask].sigTermHandler;
		tcb[tid].sigTstpHandler = tcb[curTask].sigTstpHandler;
	}
	else
	{	// otherwise use defaults
		tcb[tid].sigContHandler = defaultSigContHandler;
		tcb[tid].sigIntHandler = defaultSigIntHandler;
		tcb[tid].sigKillHandler = defaultSigKillHandler;
		tcb[tid].sigTermHandler = defaultSigTermHandler;
		tcb[tid].sigTstpHandler = defaultSigTstpHandler;

	}
}
