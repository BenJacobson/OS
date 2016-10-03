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
#include <stdarg.h>

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
static void my_printf_isr(void);

// **********************************************************************
// **********************************************************************
// global semaphores

extern Semaphore* keyboard;						// keyboard semaphore
extern Semaphore* charReady;					// character has been entered
extern Semaphore* inBufferReady;				// input buffer ready semaphore
extern Semaphore* deltaClockMutex;				// mutex for delta clock access

extern Semaphore* tics10secs;					// 10 second semaphore
extern Semaphore* tics1sec;						// 1 second semaphore
extern Semaphore* tics10thsec;					// 1/10 second semaphore
extern char inChar;								// last entered character
extern int charFlag;							// 0 => buffered input
extern int inBufIndx;							// input pointer into input buffer
extern char inBuffer[INBUF_SIZE+1];				// character input buffer
extern int cmdBufIndx;							// input pointer into current command from command buffer
extern char* cmdBuffer[CMDBUF_SIZE];			// previous buffer size
extern int myPrintfBufIndx;						// input index to myprintf buffer
extern char myPrintfBuffer[MYPRINTFBUF_SIZE+1];	// stdout printf buffer

extern void printPrompt(void);

extern DCEvent* DCHead;

extern time_t oldTime1;							// old 1sec time
extern time_t oldTime10;						// old 10sec time
extern clock_t myClkTime;
extern clock_t myOldClkTime;

extern int pollClock;							// current clock()
extern int lastPollClock;						// last pollClock

extern int superMode;							// system mode


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
	if ((inChar = GET_CHAR) > 0) {
	  keyboard_isr();
	}

	// timer interrupt
	timer_isr();

	// Buffered printing
	my_printf_isr();

	return;
} // end pollInterrupts


// **********************************************************************
// Save input buffer for recall
//
static void saveBuffer() {
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
static void keyboard_isr() {
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
			#if GCC == 1      // Linux/Unix systems send 3 char codes when an arrow is pressed
			case BEGIN_ARROW:
			{
				GET_CHAR;     // Throw away the second character
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
							putchar(CR);
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
							putchar(CR);
							printPrompt();
							printf("%s", inBuffer);
						}
						break;
					}

					case UP_ARROW:
					{
						if (cmdBufIndx < CMDBUF_SIZE-1 && cmdBuffer[cmdBufIndx+1] != 0) {
							// erase line
							putchar(CR);
							printPrompt();
							for (int i=strlen(inBuffer); i >= 0; i--) printf(" ");
							// load command
							strcpy(inBuffer, cmdBuffer[++cmdBufIndx]);
							inBufIndx = strlen(inBuffer);
							// print line
							putchar(CR);
							printPrompt();
							printf("%s", inBuffer);
						}
						break;
					}

			#if GCC == 1			// End Linux/Unix arrow key handling
				}
				break;
			}
			#endif
			// End Arrow Key Hndling

			case BACKSPACE:
			{
				if (inBufIndx > 0) {
					// remove character
					inBufIndx--;
					for (int cpyIndx = inBufIndx; cpyIndx < INBUF_SIZE && inBuffer[cpyIndx] != 0; cpyIndx++)
						inBuffer[cpyIndx] = inBuffer[cpyIndx+1];
					// clear line
					putchar(CR);
					printPrompt();
					printf("%s", inBuffer);
					printf(" ");
					// print correct line
					putchar(CR);
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
					putchar(CR);
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
static void timer_isr() {
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
		// dec delta clock
		if (DCHead) {
			DCHead->ticksLeft--;
			while (DCHead && !DCHead->ticksLeft) {
				SEM_SIGNAL(DCHead->sem);
				DCEvent* usedEvent = DCHead;
				DCHead = DCHead->next;
				free(usedEvent);
			}
		}
	}

	return;
} // end timer_isr


// **********************************************************************
// my printf interrupt service routine
//
static void my_printf_isr() {
	if (myPrintfBufIndx != 0) {
		printf("%s", myPrintfBuffer);
		myPrintfBufIndx = 0;
	}
} // end my_printf_isr

void my_printf(char* fmt, ...)
{
	va_list arg_ptr;
	char pBuffer[128];
	char* s = pBuffer;

	va_start(arg_ptr, fmt);
	vsprintf(pBuffer, fmt, arg_ptr);

	while (*s && myPrintfBufIndx < MYPRINTFBUF_SIZE)
		myPrintfBuffer[myPrintfBufIndx++] = *s++;
	myPrintfBuffer[myPrintfBufIndx] = 0;

	va_end(arg_ptr);
} // end my_printf

// **********************************************************************
// insert semaphore into delta clock
//
void insertDeltaClock(int ticks, Semaphore* sem) {
	SEM_WAIT(deltaClockMutex);
		// set up iteration pointers
		DCEvent* lastEvent = 0;		
		DCEvent* currEvent = DCHead;
		// walk list until find where to insert
		while (currEvent && currEvent->ticksLeft < ticks) {
			ticks -= currEvent->ticksLeft;
			lastEvent = currEvent;
			currEvent = currEvent->next;
		}
		// create the new event
		DCEvent* newEvent = malloc(sizeof(DCEvent));
		newEvent->next = currEvent;
		newEvent->ticksLeft = ticks;
		newEvent->sem = sem;
		// update ticks for next tasks relative to new task
		if (currEvent)
			currEvent->ticksLeft -= ticks;
		// link in the back
		if (lastEvent)
			lastEvent->next = newEvent;
		else
			DCHead = newEvent;
	SEM_SIGNAL(deltaClockMutex);
} // end insertDeltaClock
