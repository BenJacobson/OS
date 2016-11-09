// os345.c - OS Kernel	09/12/2013
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
#include "os345pqueue.h"
#include "os345signals.h"
#include "os345config.h"
#include "os345lc3.h"
#include "os345fat.h"

// **********************************************************************
//	local prototypes
//
void pollInterrupts(void);
static int scheduler(void);
static int createFairShareSchedule(void);
static int dispatcher(void);
int decDC(int argc, char* argv[]);
int sysKillTask(int taskID);
static int initOS(void);

// **********************************************************************
// **********************************************************************
// global semaphores

Semaphore* semaphoreList;			// linked list of active semaphores

Semaphore* keyboard;				// keyboard semaphore
Semaphore* charReady;				// character has been entered
Semaphore* inBufferReady;			// input buffer ready semaphore
Semaphore* deltaClockMutex;			// mutex for delta clock access

Semaphore* tics10secs;				// 10 second semaphore
Semaphore* tics1sec;				// 1 second semaphore
Semaphore* tics10thsec;				// 1/10 second semaphore

// **********************************************************************
// **********************************************************************
// global system variables

DCEvent* DCHead;						// head of the Delta Clock linked list
TCB tcb[MAX_TASKS];						// task control block
Semaphore* taskSems[MAX_TASKS];			// task semaphore
jmp_buf k_context;						// context of kernel stack
jmp_buf reset_context;					// context of kernel stack
volatile void* temp;					// temp pointer used in dispatcher

int schedulerMode;						// scheduler mode
int superMode;							// system mode
int curTask;							// current task #
long swapCount;							// number of re-schedule cycles
char inChar;							// last entered character
int charFlag;							// 0 => buffered input
int inBufIndx;							// input index into input buffer
char inBuffer[INBUF_SIZE+1];			// character input buffer
int cmdBufIndx;							// input index to command buffer
char* cmdBuffer[CMDBUF_SIZE];			// previous command buffer
int myPrintfBufIndx;					// input index to myprintf buffer
char myPrintfBuffer[MYPRINTFBUF_SIZE+1];	// stdout printf buffer
Message messages[NUM_MESSAGES];			// process message buffers

int pollClock;							// current clock()
int lastPollClock;						// last pollClock
bool diskMounted;						// disk has been mounted

time_t oldTime1;						// old 1sec time
time_t oldTime10;						// old 10sec time
clock_t myClkTime;
clock_t myOldClkTime;
PQueue readyQueue;						// ready priority queue

// **********************************************************************
// **********************************************************************
// global system functions


// **********************************************************************
// **********************************************************************
// OS startup
//
// 1. Init OS
// 2. Define reset longjmp vector
// 3. Define global system semaphores
// 4. Create CLI task
// 5. Enter scheduling/idle loop
//
int main(int argc, char* argv[])
{
	// save context for restart (a system reset would return here...)
	int resetCode = setjmp(reset_context);
	superMode = TRUE;						// supervisor mode

	switch (resetCode)
	{
		case POWER_DOWN_QUIT:				// quit
			powerDown(0);
			printf("\nGoodbye!!");
			return 0;

		case POWER_DOWN_RESTART:			// restart
			powerDown(resetCode);
			printf("\nRestarting system...\n");

		case POWER_UP:						// startup
			break;

		default:
			printf("\nShutting down due to error %d", resetCode);
			powerDown(resetCode);
			return resetCode;
	}

	// output header message
	printf("%s", STARTUP_MSG);

	// initalize OS
	if ( resetCode = initOS()) return resetCode;

	// global/system semaphores
	charReady = createSemaphore("charReady", BINARY, 0);
	inBufferReady = createSemaphore("inBufferReady", BINARY, 0);
	keyboard = createSemaphore("keyboard", BINARY, 1);
	deltaClockMutex = createSemaphore("dcMutex", BINARY, 1);
	tics10secs = createSemaphore("tics10secs", COUNTING, 0);
	tics1sec = createSemaphore("tics1sec", BINARY, 0);
	tics10thsec = createSemaphore("tics10thsec", BINARY, 0);

	// schedule CLI task
	createTask("myShell",			// task name
					P1_shellTask,	// task
					MED_PRIORITY,	// task priority
					1,				// task time slice
					argc,			// task arg count
					argv);			// task argument pointers

	// schedule delta clock task
	createTask("decDeltaClock",			// task name
					decDC,				// task
					VERY_HIGH_PRIORITY,	// task priority
					1,					// task time slice
					0,					// task arg count
					0);					// task argument pointers

	// Scheduling loop
	// 1. Check for asynchronous events (character inputs, timers, etc.)
	// 2. Choose a ready task to schedule
	// 3. Dispatch task
	// 4. Loop (forever!)

	while(1)									// scheduling loop
	{
		for(int i=0; i<100000; i++);
		// check for character / timer interrupts
		pollInterrupts();

		// schedule highest priority ready task
		if ((curTask = scheduler()) < 0) continue;

		// dispatch curTask, quit OS if negative return
		if (dispatcher() < 0) break;
	}											// end of scheduling loop

	// exit os
	longjmp(reset_context, POWER_DOWN_QUIT);
	return 0;
} // end main



// **********************************************************************
// **********************************************************************
// scheduler
//
static int scheduler() {
	int nextTask = dequeueTask(readyQueue, schedulerMode);
	if (nextTask == -1) {
		if (schedulerMode) {
			nextTask = createFairShareSchedule();
		} else {
			return -1;
		}
	}

	// Pause SIGSTOP tasks
	if (tcb[nextTask].signal & mySIGSTOP) {
		if (nextTask == 0) {
			sigClear(0, mySIGSTOP); // Do not allow shell to pause
		} else {
			enqueueTask(readyQueue, nextTask);
			return -1;
		}
	}

	return nextTask;
} // end scheduler


static int createFairShareSchedule() {
	for (int i=readyQueue[0]; i>0; i--) {
		tcb[readyQueue[i]].timeLeft = 20;
	}
	return 0;
}

// **********************************************************************
// **********************************************************************
// dispatch curTask
//
static int dispatcher() {
	int result;

	// schedule task
	switch(tcb[curTask].state)
	{
		case S_NEW:
		{
			// new task
			printf("\nNew Task[%d] %s", curTask, tcb[curTask].name);
			tcb[curTask].state = S_RUNNING;	// set task to run state

			// save kernel context for task SWAP's
			if (setjmp(k_context))
			{
				superMode = TRUE;					// supervisor mode
				break;								// context switch to next task
			}

			// move to new task stack (leave room for return value/address)
			temp = (int*)tcb[curTask].stack + (STACK_SIZE-8);
			SET_STACK(temp);
			superMode = FALSE;						// user mode

			// begin execution of new task, pass argc, argv
			result = (*tcb[curTask].task)(tcb[curTask].argc, tcb[curTask].argv);

			// task has completed
			if (result) printf("\nTask[%d] returned %d", curTask, result);
			else        printf("\nTask[%d] returned %d", curTask, result);
			tcb[curTask].state = S_EXIT;			// set task to exit state
			enqueueTask(readyQueue, curTask);

			// return to kernal mode
			longjmp(k_context, 1);					// return to kernel
		}

		case S_READY:
		{
			tcb[curTask].state = S_RUNNING;			// set task to run
			// fall through to running
		}

		case S_RUNNING:
		{
			if (setjmp(k_context))
			{
				// SWAP executed in task
				superMode = TRUE;					// supervisor mode
				break;								// return from task
			}
			if (signals()) break;
			longjmp(tcb[curTask].context, 3); 		// restore task context
		}

		case S_BLOCKED:
		{
			// printf("\ntask %d is blocked", curTask);
			break;
		}

		case S_EXIT:
		{
			if (curTask == 0) return -1;			// if CLI, then quit scheduler
			// release resources and kill task
			sysKillTask(curTask);					// kill current task
			break;
		}

		default:
		{
			printf("Unknown Task[%d] State", curTask);
			longjmp(reset_context, POWER_DOWN_ERROR);
		}
	}
	return 0;
} // end dispatcher


// **********************************************************************
// **********************************************************************
// Do a context switch to next task.

// 1. If scheduling task, return (setjmp returns non-zero value)
// 2. Else, save current task context (setjmp returns zero value)
// 3. Set current task state to READY
// 4. Enter kernel mode (longjmp to k_context)

void swapTask()
{
	assert("SWAP Error" && !superMode);		// assert user mode

	// increment swap cycle counter
	swapCount++;

	// check time slices
	if (tcb[curTask].state == S_RUNNING && ++(tcb[curTask].timeSliceCount) < tcb[curTask].timeSlice) {
		return;
	}
	tcb[curTask].timeSliceCount = 0;

	// either save current task context or schedule task (return)
	if (setjmp(tcb[curTask].context))
	{
		superMode = FALSE;					// user mode
		return;
	}

	// context switch - move task state to ready
	if (tcb[curTask].state == S_RUNNING) {
		tcb[curTask].state = S_READY;
		enqueueTask(readyQueue, curTask);
	}

	// move to kernel mode (reschedule)
	longjmp(k_context, 2);
} // end swapTask

// **********************************************************************
// insert semaphore into delta clock
//
void insertDeltaClock(unsigned int ticks, Semaphore* sem) {
	SEM_WAIT(deltaClockMutex);												SWAP;
		// set up iteration pointers
		DCEvent* lastEvent = 0;												SWAP;
		DCEvent* currEvent = DCHead;										SWAP;
		// do not allow an insert of 0 time
		if (!ticks) { ticks = 1; }											SWAP;
		// walk list until find where to insert
		while (currEvent && currEvent->ticksLeft < ticks) {					SWAP;
			ticks -= currEvent->ticksLeft;									SWAP;
			lastEvent = currEvent;											SWAP;
			currEvent = currEvent->next;									SWAP;
		}
		// create the new event
		DCEvent* newEvent = malloc(sizeof(DCEvent));						SWAP;
		newEvent->next = currEvent;											SWAP;
		newEvent->ticksLeft = ticks;										SWAP;
		newEvent->sem = sem;												SWAP;
		// update ticks for next tasks relative to new task
		if (currEvent) {
			currEvent->ticksLeft -= ticks;									SWAP;
		}
		// link in the back
		if (lastEvent) {													SWAP;
			lastEvent->next = newEvent;										SWAP;
		} else {															SWAP;
			DCHead = newEvent;												SWAP;
		}
	SEM_SIGNAL(deltaClockMutex);											SWAP;
} // end insertDeltaClock

// **********************************************************************
// decrement delta clock task
//
int decDC(int argc, char* argv[]) {
	while(1) {
		SEM_WAIT(tics10thsec);												SWAP;
		SEM_WAIT(deltaClockMutex);											SWAP;
		if (DCHead) {														SWAP;
			DCHead->ticksLeft--;											SWAP;
			while (DCHead && !DCHead->ticksLeft) {							SWAP;
				SEM_SIGNAL(DCHead->sem);									SWAP;
				DCEvent* usedEvent = DCHead;								SWAP;
				DCHead = DCHead->next;										SWAP;
				free(usedEvent);											SWAP;
			}
		}
		SEM_SIGNAL(deltaClockMutex);										SWAP;
	}
	return 0;
} // end decDC


// **********************************************************************
// **********************************************************************
// system utility functions
// **********************************************************************
// **********************************************************************

// **********************************************************************
// **********************************************************************
// initialize operating system
static int initOS()
{
	int i;

	// make any system adjustments (for unblocking keyboard inputs)
	INIT_OS

	// reset system variables
	curTask = 0;						// current task #
	swapCount = 0;						// number of scheduler cycles
	schedulerMode = 0;					// default scheduler
	inChar = 0;							// last entered character
	charFlag = 0;						// 0 => buffered input
	inBufIndx = 0;						// input pointer into input buffer
	semaphoreList = 0;					// linked list of active semaphores
	DCHead = 0;							// linked list of clock events
	diskMounted = 0;					// disk has been mounted
	srand(time(NULL));					// seed random function

	// malloc ready queue
	readyQueue = newPQueue();
	if (readyQueue == NULL) return 99;

	// capture current time
	lastPollClock = clock();			// last pollClock
	time(&oldTime1);
	time(&oldTime10);

	// init system tcb's
	for (i=0; i<MAX_TASKS; i++)
	{
		tcb[i].name = NULL;				// tcb
		taskSems[i] = NULL;				// task semaphore
	}

	// initialize lc-3 memory
	initLC3Memory(LC3_MEM_FRAME, 0xF800>>6);

	return 0;
} // end initOS



// **********************************************************************
// **********************************************************************
// Causes the system to shut down. Use this for critical errors
void powerDown(int code)
{
	int i;
	printf("\nPowerDown Code %d", code);

	// release all system resources.
	printf("\nRecovering Task Resources...");

	// kill all tasks
	for (i = MAX_TASKS-1; i >= 0; i--)
		if(tcb[i].name) sysKillTask(i);

	// delete all semaphores
	while (semaphoreList)
		deleteSemaphore(&semaphoreList);

	// free ready queue
	free(readyQueue);

	// ?? release any other system resources
	// ?? deltaclock (project 3)

	RESTORE_OS
	return;
} // end powerDown
