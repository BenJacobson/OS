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

Semaphore* parkEntrance;					// counting sempaphore to restrict
Semaphore* wakeupDriver;					// binary semaphore to signal a driver
Semaphore* needDriver;						// binary semaphore to signal driver for a car 
Semaphore* getSeat;							// binary semaphore to mutex getting a seat
Semaphore* needPassenger;					// binary semaphore to signal visitor from car to board
Semaphore* needTicket;						// binary semaphore to signal driver to sell a ticket
Semaphore* tickets;							// counting semaphore of tickets available to sell
Semaphore* getTicket;						// binary semaphore to mutex who can ask for a ticket
Semaphore* museum;							// counting semaphore of people in the museum
Semaphore* giftShop;						// counting semaphore of people in the gift shop
Semaphore* needGift;						// binary semaphore to signal that a visitor needs a gift
Semaphore* purchaseGift;					// binary semaphore to signal ready to purchase gift
Semaphore* cashierAvailable;				// binary semaphore for driver to signal he is available to the visitor for gift purchase
Semaphore* driverMutex;						// binary semaphore to allow one visitor to wake up a driver at a time
Semaphore* buyTicket;						// binary semaphore to signal driver that ticket was tacken
Semaphore* ticketReady;						// binary semaphore to signal that a ticket is ready to the visitor
Semaphore* mailboxMutex;					// binary semaphore to allow one access to the mailbox at a time
Semaphore* mailboxReady;					// binary semaphore to signal that a semaphore is in the mailbox
Semaphore* mailAcquired;					// binary semaphore to signal that the semaphore has been received from the mailbox
Semaphore* semaphoreMailbox;				// global semaphore variable for passing semaphores between tasks
Semaphore* rideOverPassengers[NUM_CARS][NUM_SEATS+1];	// binary semaphore buffer to hold passengers' semaphores to signal when ride is over

// ***********************************************************************
// project 3 functions and tasks


// ***********************************************************************
// ***********************************************************************
// visitor task
int visitorTask(int argc, char* argv[]) {
	char buf[32];
	int myID = atoi(argv[1]);
	// create visotor's personal semaphore
	sprintf(buf, "myVisitorSemaphore%d", myID);							SWAP;
	Semaphore* myVisitorSemaphore = createSemaphore(buf, BINARY, 0);	SWAP;
	// wait 0-10 seconds before trying to enter park
	insertDeltaClock(rand()%100, myVisitorSemaphore);					SWAP;
	SEM_WAIT(myVisitorSemaphore);										SWAP;
	// Update park
	SEM_WAIT(parkMutex);												SWAP;
	myPark.numOutsidePark++;											SWAP;
	SEM_SIGNAL(parkMutex);												SWAP;
	// Wait to enter park
	SEM_WAIT(parkEntrance);												SWAP;
	// Update park
	SEM_WAIT(parkMutex);												SWAP;
	myPark.numOutsidePark--;											SWAP;
	myPark.numInPark++;													SWAP;
	myPark.numInTicketLine++;											SWAP;
	SEM_SIGNAL(parkMutex);												SWAP;
	// Wait to get a ticket
	SEM_WAIT(getTicket);												SWAP;
	SEM_WAIT(driverMutex);												SWAP;
	SEM_SIGNAL(needTicket);												SWAP;
	SEM_SIGNAL(wakeupDriver);											SWAP;
	SEM_WAIT(ticketReady);												SWAP;
	SEM_SIGNAL(buyTicket);												SWAP;
	SEM_SIGNAL(getTicket);												SWAP;
	// Update park
	SEM_WAIT(parkMutex);												SWAP;
	myPark.numInTicketLine--;											SWAP;
	myPark.numInMuseumLine++;											SWAP;
	myPark.numTicketsAvailable--;										SWAP;
	SEM_SIGNAL(parkMutex);												SWAP;
	// Wait in line for the museum
	SEM_WAIT(museum);													SWAP;
	// Update park
	SEM_WAIT(parkMutex);												SWAP;
	myPark.numInMuseumLine--;											SWAP;
	myPark.numInMuseum++;												SWAP;
	SEM_SIGNAL(parkMutex);												SWAP;
	// Stay in the museum for 0-3 seconds
	insertDeltaClock(rand()%30, myVisitorSemaphore);					SWAP;
	SEM_WAIT(myVisitorSemaphore);										SWAP;
	SEM_SIGNAL(museum);													SWAP;
	// Update park
	SEM_WAIT(parkMutex);												SWAP;
	myPark.numInMuseum--;												SWAP;
	myPark.numInCarLine++;												SWAP;
	SEM_SIGNAL(parkMutex);												SWAP;
	// Wait in line for the car ride
	SEM_WAIT(getSeat);													SWAP;
	SEM_WAIT(needPassenger);											SWAP;
	SEM_SIGNAL(getSeat);												SWAP;
	// Get in the car
	SEM_WAIT(mailboxMutex);												SWAP;
	semaphoreMailbox = myVisitorSemaphore;								SWAP;
	SEM_SIGNAL(mailboxReady);											SWAP;
	SEM_WAIT(mailAcquired);												SWAP;
	SEM_SIGNAL(mailboxMutex);											SWAP;
	SEM_SIGNAL(tickets);												SWAP;
	// Update park
	SEM_WAIT(parkMutex);												SWAP;
	myPark.numInCarLine--;												SWAP;
	myPark.numInCars++;													SWAP;
	myPark.numTicketsAvailable++;										SWAP;
	SEM_SIGNAL(parkMutex);												SWAP;
	// wait until the ride is over
	SEM_WAIT(myVisitorSemaphore);										SWAP;
	// Update park
	SEM_WAIT(parkMutex);												SWAP;
	myPark.numInCars--;													SWAP;
	myPark.numInGiftLine++;												SWAP;
	SEM_SIGNAL(parkMutex);												SWAP;
	// Wait in line for the gift shop
	SEM_WAIT(giftShop);													SWAP;
	// Update park
	SEM_WAIT(parkMutex);												SWAP;
	myPark.numInGiftLine--;												SWAP;
	myPark.numInGiftShop++;												SWAP;
	SEM_SIGNAL(parkMutex);												SWAP;
	// Stay in the giftShop for 0-3 seconds
	insertDeltaClock(rand()%30, myVisitorSemaphore);					SWAP;
	SEM_WAIT(myVisitorSemaphore);										SWAP;
	SEM_SIGNAL(giftShop);												SWAP;
	// Purchase a gift
	SEM_WAIT(driverMutex);												SWAP;
	SEM_SIGNAL(needGift);												SWAP;
	SEM_SIGNAL(wakeupDriver);											SWAP;
	SEM_WAIT(cashierAvailable);											SWAP;
	SEM_SIGNAL(purchaseGift);											SWAP;
	// Update park
	SEM_WAIT(parkMutex);												SWAP;
	myPark.numInGiftShop--;												SWAP;
	myPark.numInPark--;													SWAP;
	myPark.numExitedPark++;												SWAP;
	SEM_SIGNAL(parkMutex);												SWAP;
	// leave the park
	SEM_SIGNAL(parkEntrance);											SWAP;
	return 0;
} // end visitor task


// ***********************************************************************
// ***********************************************************************
// car task
int carTask(int argc, char* argv[]) {
	int carID = atoi(argv[1]);
	while (1) {
		for (int i=0; i<NUM_SEATS; i++) {
			SEM_WAIT(fillSeat[carID]);										SWAP; // wait for available seat
			SEM_SIGNAL(needPassenger);										SWAP; // signal for visitor
			SEM_WAIT(mailboxReady);											SWAP;
			rideOverPassengers[carID][i] = semaphoreMailbox;				SWAP;
			SEM_SIGNAL(mailAcquired);										SWAP;
			if (i == NUM_SEATS-1) {											SWAP;
				SEM_WAIT(driverMutex);										SWAP;
				SEM_SIGNAL(needDriver);										SWAP;
				SEM_SIGNAL(wakeupDriver);									SWAP;
				SEM_WAIT(mailboxReady);										SWAP;
				rideOverPassengers[carID][NUM_SEATS] = semaphoreMailbox;	SWAP;
				SEM_SIGNAL(mailAcquired);									SWAP;
			}
			SEM_SIGNAL(seatFilled[carID]);									SWAP; // signal next seat ready
		}
		SEM_WAIT(rideOver[carID]);											SWAP; // wait for ride over

		for (int i=0; i<=NUM_SEATS; i++) {
			SEM_SIGNAL(rideOverPassengers[carID][i]);						SWAP;
		}
	}
	return 0;
} // end car task

// ***********************************************************************
// ***********************************************************************
// Get the number of the car about to embark
int getLeavingCar() {
	for (int i=0; i<NUM_CARS; i++) {
		if (myPark.cars[i].location == 33) {
			return i+2;
		}
	}
	return 0;
} // end getLeavingCar

// ***********************************************************************
// ***********************************************************************
// driver task
int driverTask(int argc, char* argv[])
{
	char buf[32];
	Semaphore* driverDone;
	int myID = atoi(argv[1]);							SWAP;	// get unique drive id
	// create driver's own semaphore
	sprintf(buf, "driverDone%d", myID); 				SWAP;
	driverDone = createSemaphore(buf, BINARY, 0);		SWAP; 	// create notification event
	// loop waiting for tasks
	while(1) {
		myPark.drivers[myID] = 0;						SWAP;
		SEM_WAIT(wakeupDriver);							SWAP;	// goto sleep
		if (SEM_TRYLOCK(needDriver)) {					SWAP;	// i’m awake  - driver needed?
			myPark.drivers[myID] = getLeavingCar();		SWAP;
			SEM_SIGNAL(driverMutex);					SWAP;
			SEM_WAIT(mailboxMutex);						SWAP;
				semaphoreMailbox = driverDone;			SWAP;	// pass notification semaphore
				SEM_SIGNAL(mailboxReady);				SWAP;
				SEM_WAIT(mailAcquired);					SWAP;
			SEM_SIGNAL(mailboxMutex);					SWAP;
			SEM_WAIT(driverDone);						SWAP;	// drive ride
		} else if (SEM_TRYLOCK(needTicket)) {			SWAP;	// someone need ticket?
			myPark.drivers[myID] = -1;					SWAP;
			SEM_SIGNAL(driverMutex);					SWAP;
			SEM_WAIT(tickets);							SWAP;	// wait for ticket (counting)
			SEM_SIGNAL(ticketReady);					SWAP;
			SEM_WAIT(buyTicket);						SWAP;
		} else if (SEM_TRYLOCK(needGift)) {				SWAP;
			myPark.drivers[myID] = 1;					SWAP;		
			SEM_SIGNAL(driverMutex);					SWAP;
			SEM_SIGNAL(cashierAvailable);				SWAP;
			SEM_WAIT(purchaseGift);						SWAP;
		} else {										SWAP;
			SEM_SIGNAL(driverMutex);					SWAP;
			break;
		}
	}
	return 0;
} // end driverTask


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

	// Initialize other semaphores
	parkEntrance = createSemaphore("parkEntance", COUNTING, MAX_IN_PARK);
	wakeupDriver = createSemaphore("wakeupDriver", BINARY, 0);
	needDriver = createSemaphore("needDriver", BINARY, 0); 
	needTicket = createSemaphore("needTicket", BINARY, 0);
	tickets = createSemaphore("tickets", COUNTING, MAX_TICKETS);
	getTicket = createSemaphore("getTicket", BINARY, 1);
	museum = createSemaphore("museum", COUNTING, MAX_IN_MUSEUM);
	giftShop = createSemaphore("giftShop", COUNTING, MAX_IN_GIFTSHOP);
	needGift = createSemaphore("needGift", BINARY, 0);
	purchaseGift = createSemaphore("purchaseGift", BINARY, 0);
	cashierAvailable = createSemaphore("cashierAvailable", BINARY, 0);
	driverMutex = createSemaphore("driverMutex", BINARY, 1);
	ticketReady = createSemaphore("ticketReady", BINARY, 0);
	buyTicket = createSemaphore("buyTicket", BINARY, 0);
	getSeat = createSemaphore("getSeat", BINARY, 1);
	needPassenger = createSemaphore("needPassenger", BINARY, 0);
	mailboxMutex = createSemaphore("mailboxMutex", BINARY, 1);
	mailboxReady = createSemaphore("mailboxReady", BINARY, 0);
	mailAcquired = createSemaphore("mailAcquired", BINARY, 0);

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
