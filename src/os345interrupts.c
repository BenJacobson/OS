// os345interrupts.c - pollInterrupts	08/08/2013
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
#include "os345config.h"
#include "os345signals.h"

// **********************************************************************
//	local prototypes
//
void pollInterrupts(void);
static void saveBuffer(void);
static void keyboard_isr(void);
static void timer_isr(void);

// **********************************************************************
// **********************************************************************
// global semaphores

extern Semaphore* keyboard;				// keyboard semaphore
extern Semaphore* charReady;			// character has been entered
extern Semaphore* inBufferReady;		// input buffer ready semaphore

extern Semaphore* tics10secs;			// 10 second semaphore
extern Semaphore* tics1sec;				// 1 second semaphore
extern Semaphore* tics10thsec;			// 1/10 second semaphore

extern char inChar;						// last entered character
extern int charFlag;					// 0 => buffered input
extern int inBufIndx;					// input pointer into input buffer
extern char inBuffer[INBUF_SIZE+1];		// character input buffer
extern int cmdBufIndx;					// input pointer into current command from command buffer
extern char* cmdBuffer[CMDBUF_SIZE+1];	// previous buffer size

extern void printPrompt(void);

extern time_t oldTime1;					// old 1sec time
extern time_t oldTime10;				// old 10sec time
extern clock_t myClkTime;
extern clock_t myOldClkTime;

extern int pollClock;					// current clock()
extern int lastPollClock;				// last pollClock

extern int superMode;					// system mode

extern int* readyQueue;						// queue of tasks waiting to run
extern int enqueueTask(int* queue, int taskID);	// pass the queue to add to and the taskID of the task to add
extern void printQueue(int* queue);


// **********************************************************************
// **********************************************************************
// simulate asynchronous interrupts by polling events during idle loop
//
void pollInterrupts(void)
{
	// check for task monopoly
	pollClock = clock();
	assert("Timeout" && ((pollClock - lastPollClock) < MAX_CYCLES));
	lastPollClock = pollClock;

	// check for keyboard interrupt
	if ((inChar = GET_CHAR) > 0)
	{
	  keyboard_isr();
	}

	// timer interrupt
	timer_isr();

	return;
} // end pollInterrupts


// **********************************************************************
// Save input buffer for recall
//
static void saveBuffer()
{
	if (cmdBuffer[CMDBUF_SIZE-1] != 0)
		free(cmdBuffer[CMDBUF_SIZE-1]);
	for (int i = CMDBUF_SIZE - 1; i > 0; i--)
	{
		cmdBuffer[i] = cmdBuffer[i-1];
	}
	char* commandString = malloc(strlen(inBuffer) + 1);
	strcpy(commandString, inBuffer);
	cmdBuffer[0] = commandString;
	return;
}


// **********************************************************************
// keyboard interrupt service routine
//

static void upArrowHandler() {

}

static void downArrowHandler() {
	
}

static void leftArrowHandler() {
	
}

static void rightArrowHandler() {
	
}

static void keyboard_isr()
{
	// assert system mode
	assert("keyboard_isr Error" && superMode);

	semSignal(charReady);					// SIGNAL(charReady) (No Swap)
	if (charFlag == 0)
	{
		switch (inChar)
		{
			// Signals
			case CTRL_X:
			{
				inBufIndx = 0;
				inBuffer[0] = 0;
				cmdBufIndx = -1;
				sigSignal(0, mySIGINT);		// interrupt task 0
				semSignal(inBufferReady);	// SEM_SIGNAL(inBufferReady)
				break;
			}

			case CTRL_W:
			{
				printf("%c", CR);
				break;
				sigSignal(-1, mySIGTSTP);
				break;
			}

			case CTRL_R:
			{
				sigClear(-1, mySIGTSTP);
				sigClear(-1, mySIGSTOP);
				sigSignal(-1, mySIGCONT);
				break;
			}

			// Begin Arrow Key Handling
			#if GCC == 1
			case BEGIN_ARROW:
			{
				GET_CHAR;
				switch (GET_CHAR) {
			#endif

				case LEFT_ARROW:
				{
					if (inBufIndx > 0)
						inBufIndx--;
					break;
				}

				case RIGHT_ARROW:
				{
					if (inBufIndx < INBUF_SIZE && inBufIndx < strlen(inBuffer))
						inBufIndx++;
					break;
				}

				case DOWN_ARROW:
				{
					if (cmdBufIndx >= 0) {
						// erase line
						printf("%c", CR);
						printPrompt();
						for (int i=strlen(inBuffer); i >= 0; i--) printf(" ");
						// load command
						cmdBufIndx--;
						if (cmdBufIndx >= 0) {
							strcpy(inBuffer, cmdBuffer[cmdBufIndx]);
							inBufIndx = strlen(inBuffer);
						} else {
							inBufIndx = 0;
							*inBuffer = 0;
						}
						// print line
						printf("%c", CR);
						printPrompt();
						printf(inBuffer);
					}
					break;
				}

				case UP_ARROW:
				{
					if (cmdBufIndx < CMDBUF_SIZE-1 && cmdBuffer[cmdBufIndx+1] != 0) {
						// erase line
						printf("%c", CR);
						printPrompt();
						for (int i=strlen(inBuffer); i >= 0; i--) printf(" ");
						// load command
						strcpy(inBuffer, cmdBuffer[++cmdBufIndx]);
						inBufIndx = strlen(inBuffer);
						// print line
						printf("%c", CR);
						printPrompt();
						printf(inBuffer);
					}
					break;
				}

			#if GCC == 1
				}
				break;
			}
			#endif						// End Arrow Key Hndling

			case BACKSPACE:
			{
				if (inBufIndx > 0) {
					// remove character
					inBufIndx--;
					for (int cpyIndx = inBufIndx; cpyIndx < INBUF_SIZE && inBuffer[cpyIndx] != 0; cpyIndx++)
						inBuffer[cpyIndx] = inBuffer[cpyIndx+1];
					// clear line
					printf("%c", CR);
					printPrompt();
					printf("%s", inBuffer);
					printf(" ");
					// print correct line
					printf("%c", CR);
					printPrompt();
					printf("%s", inBuffer);
				}
				break;
			}

			// submit the text
			case '\r':
			{

			}
			case '\n':
			{
				saveBuffer();
				cmdBufIndx = -1;
				inBufIndx = 0;				// EOL, signal line ready
				semSignal(inBufferReady);	// SIGNAL(inBufferReady)
				break;
			}

			// print the character
			default:
			{
				if (inBufIndx < INBUF_SIZE) {
					int bufLen = strlen(inBuffer);
					for (int cpyIndx = bufLen; cpyIndx > 0 && cpyIndx != inBufIndx; cpyIndx--)
						inBuffer[cpyIndx] = inBuffer[cpyIndx-1];
					inBuffer[inBufIndx++] = inChar;
					inBuffer[bufLen+1] = 0;
					// print correct line
					printf("%c", CR);
					printPrompt();
					printf("%s", inBuffer);
					// printf("%x", inChar);	// echo key code
				}
			}
		}
	}
	else
	{
		// single character mode
		inBufIndx = 0;
		inBuffer[inBufIndx] = 0;
	}
	return;
} // end keyboard_isr


// **********************************************************************
// timer interrupt service routine
//
static void timer_isr()
{
	time_t currentTime;						// current time

	// assert system mode
	assert("timer_isr Error" && superMode);

	// capture current time
  	time(&currentTime);

  	// 10 second timer
  	if ((currentTime - oldTime10) >= 10) {
  		// signal 10 seconds
  		semSignal(tics10secs);
  		oldTime10 += 10;
	}

  	// 1 second timer
  	if ((currentTime - oldTime1) >= 1) {
		// signal 1 second
		semSignal(tics1sec);
		oldTime1 += 1;
  	}

	// sample fine clock
	myClkTime = clock();
	if ((myClkTime - myOldClkTime) >= ONE_TENTH_SEC) {
		myOldClkTime = myOldClkTime + ONE_TENTH_SEC;   // update old
		semSignal(tics10thsec);
	}

	// ?? add other timer sampling/signaling code here for project 2


	return;
} // end timer_isr
