// os345p3.c - Jurassic Park
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>
#include <time.h>
#include <assert.h>
#include "os345.h"
#include "os345park.h"

// ***********************************************************************
// project 3 variables

// Jurassic Park
extern JPARK myPark;
extern Semaphore* parkMutex;						// protect park access
extern Semaphore* fillSeat[NUM_CARS];				// (signal) seat ready to fill
extern Semaphore* seatFilled[NUM_CARS];				// (wait) passenger seated
extern Semaphore* rideOver[NUM_CARS];				// (signal) ride over
extern DCEvent* DCHead;


// ***********************************************************************
// project 3 functions and tasks


// ***********************************************************************
// ***********************************************************************
// project3 command
int P3_project3(int argc, char* argv[])
{
	char buf1[32], buf2[32];
	char* newArgv[2];

	// start park
	sprintf(buf1, "jurassicPark");
	newArgv[0] = buf1;
	createTask(buf1,				// task name
		jurassicTask,				// task
		MED_PRIORITY,				// task priority
		1,							// task time slice
		1,							// task argc
		newArgv);					// task argv

	// wait for park to get initialized...
	while (!parkMutex) SWAP;
	printf("\nStart Jurassic Park...");

	// visitor tasks
	for (int i=0; i<NUM_VISITORS; i++) {
		sprintf(buf1, "visitorTask%d", i);
		newArgv[0] = buf1;
		sprintf(buf2, "%d", i);
		newArgv[1] = buf2;
		createTask(buf1,
			visitorTask,
			MED_PRIORITY,
			1,
			2,
			newArgv);
	}

	// car tasks
	for (int i=0; i<NUM_CARS; i++) {
		sprintf(buf1, "carTask%d", i);
		newArgv[0] = buf1;
		sprintf(buf2, "%d", i);
		newArgv[1] = buf2;
		createTask(buf1,
			carTask,
			MED_PRIORITY,
			1,
			2,
			newArgv);
	}

	// driver tasks
	for (int i=0; i<NUM_DRIVERS; i++) {
		sprintf(buf1, "driverTask%d", i);
		newArgv[0] = buf1;
		sprintf(buf2, "%d", i);
		newArgv[1] = buf2;
		createTask(buf1,
			driverTask,
			MED_PRIORITY,
			1,
			2,
			newArgv);
	}

	return 0;
} // end project3



// ***********************************************************************
// ***********************************************************************
// delta clock command
int P3_dc(int argc, char* argv[])
{
	printf("\nDelta Clock");
	DCEvent* event = DCHead;
	while (event) {
		printf("\n%d %s", event->ticksLeft, event->sem->name);
		event = event->next;
	}
	return 0;
} // end CL3_dc
