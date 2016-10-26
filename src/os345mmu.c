// os345mmu.c - LC-3 Memory Management Unit	03/12/2015
//
//		03/12/2015	added PAGE_GET_SIZE to accessPage()
//
// **************************************************************************
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
#include <assert.h>
#include "os345.h"
#include "os345lc3.h"

#define DEBUG_LEVEL 0

extern void printVMTables(unsigned short int va, unsigned short int pa);

// ***********************************************************************
// mmu variables

// LC-3 memory
unsigned short int memory[LC3_MAX_MEMORY];
bool usingRPTPointer = TRUE;

// statistics
int memAccess;						// memory accesses
int memHits;						// memory hits
int memPageFaults;					// memory faults

int getFrame(int);
int getAvailableFrame(void);
extern TCB tcb[];					// task control block
extern int curTask;					// current task #

//////////////////////////////////
// Find the next page to swap out
int runClock(int notMeFrame) {
	static unsigned short clockRPTPointer = LC3_RPT;
	static unsigned short clockUPTPointer;
	bool done = FALSE;
	int frame, pnum;
	unsigned short int rpte1, upte1, rpte2, upte2, swapEntry;
	// Find the next frame to swap out
	while (!done) {
		rpte1 = memory[clockRPTPointer];
		upte1 = memory[clockUPTPointer];
		// Look at the frame
		if (usingRPTPointer) {
			if (DEFINED(rpte1)) {
				if (REFERENCED(rpte1)) {
					if (DEBUG_LEVEL > 2) printf("\nChecking RPT %x ... %d REFERENCED", clockRPTPointer, FRAME(rpte1));
					memory[clockRPTPointer] = rpte1 = CLEAR_REF(rpte1);
				} else {
					if (DEBUG_LEVEL > 2) printf("\nChecking RPT %x ... %d UNREFERENCED", clockRPTPointer, FRAME(rpte1));
					usingRPTPointer = FALSE;
					memory[clockRPTPointer] = rpte1 = CLEAR_PINNED(rpte1);
					clockUPTPointer = FRAMEADDR(rpte1);
					continue;
				}
			} else {
				if (DEBUG_LEVEL > 3) printf("\nChecking RPT %x ... UNDEFINED", clockRPTPointer);
			}
		} else {
			if (DEFINED(upte1)) {
				memory[clockRPTPointer] = rpte1 = SET_PINNED(rpte1);
				if (REFERENCED(upte1)) {
					if (DEBUG_LEVEL > 2) printf("\n  Checking UPT %x ... %d REFERENCED", clockUPTPointer, FRAME(upte1));
					memory[clockUPTPointer] = upte1 = CLEAR_REF(upte1);
				} else {
					if (DEBUG_LEVEL > 2) printf("\n  Checking UPT %x ... %d UNREFERENCED", clockUPTPointer, FRAME(upte1));
					frame = FRAME(upte1);
					if (frame != notMeFrame) {
						upte2 = memory[clockUPTPointer + 1];
						if (PAGED(upte2)) {
							accessPage(SWAPPAGE(upte2), frame, PAGE_OLD_WRITE);
						} else {
							swapEntry = accessPage(-1, frame, PAGE_NEW_WRITE);
							memory[clockUPTPointer + 1] = SET_PAGED(swapEntry);
						}
						memory[clockUPTPointer] = upte1 = CLEAR_DEFINED(upte1);
						done = TRUE;
					}
				}
			} else {
				if (DEBUG_LEVEL > 3) printf("\n  Checking UPT %x ... UNDEFINED", clockUPTPointer);
			}
		}
		// move clock pointer to next frame
 		if (!usingRPTPointer) {															// if we are looking at a user page table
			clockUPTPointer += 2;														// go to next page table entry
			if (clockUPTPointer%LC3_FRAME_SIZE==0) {									// if you finished the frame
				usingRPTPointer = TRUE;													// go back out to the root page table
				if (!done && !PINNED(rpte1) && DEFINED(rpte1)) {						// if the user page table you just left has no allocated pages, swap it out
					frame = FRAME(rpte1);												// set the frame address of the user page table we left
					if (frame != notMeFrame) {
						rpte2 = memory[clockRPTPointer + 1];
						if (PAGED(rpte2)) {
							accessPage(SWAPPAGE(rpte2), frame, PAGE_OLD_WRITE);
						} else {
							swapEntry = accessPage(-1, frame, PAGE_NEW_WRITE);
							memory[clockRPTPointer + 1] = SET_PAGED(swapEntry);
						}
						memory[clockRPTPointer] = CLEAR_DEFINED(memory[clockRPTPointer]);
						done = TRUE;														// we're done
					}
				}
			}
		}
		if (usingRPTPointer) {															// if we are looking a root page table
			clockRPTPointer += 2;														// go to the next apge table entry
			if (clockRPTPointer >= LC3_RPT_END)											// if we hit the end of root page table area
				clockRPTPointer = LC3_RPT;												// go back to the beginning
		}
	}
	if (DEBUG_LEVEL) printf("\nSwapping out %d", frame);
	// printf("\nFrame %d", frame);
	// printVMTables(0, 0);
	// fflush(stdout);
	if (frame < 192 || frame > 207) {
		int x = 0;
	}
	for (unsigned short int test = LC3_RPT; test < LC3_RPT_END; test += 2) {
		unsigned short int data = memory[test];
		if (frame == FRAME(data) && DEFINED(data)) {
			int x = 0;
		}
	}
	return frame;																		// return the frame number found above
}

int getFrame(int notMeFrame) {
	int frame, i;
	// look for unused frames
	frame = getAvailableFrame();
	if (frame >=0) {
		if (DEBUG_LEVEL) printf("\nFound free frame %d", frame);
		return frame;
	}
	// find a frame to swap out
	return runClock(notMeFrame);
}

// **************************************************************************
// **************************************************************************
// LC3 Memory Management Unit
// Virtual Memory Process
// **************************************************************************
//           ___________________________________Frame defined
//          / __________________________________Dirty frame
//         / / _________________________________Referenced frame
//        / / / ________________________________Pinned in memory
//       / / / /     ___________________________
//      / / / /     /                 __________frame # (0-1023) (2^10)
//     / / / /     /                 / _________page defined
//    / / / /     /                 / /       __page # (0-4096) (2^12)
//   / / / /     /                 / /       /
//  / / / /     / 	             / /       /
// F D R P - - f f|f f f f f f f f|S - - - p p p p|p p p p p p p p

unsigned short int *getMemAdr(int va, int rwFlg) {
	unsigned short int pa, i, *frameEntryPointer;
	int rpta, rpte1, rpte2;
	int upta, upte1, upte2;
	int rptFrame, uptFrame;

	memAccess += 3;				// access the RPT, the UPT, and the data frame
	memHits++;					// RPT is always a hit

	if (DEBUG_LEVEL > 2) printf("\nAccess to %x", va);

	// turn off virtual addressing for system RAM
	if (va < 0x3000)
		return &memory[va];
	// get physical address
	rpta = tcb[curTask].RPT + RPTI(va);		// root page table address
	rpte1 = memory[rpta];					// FDRP__ffffffffff
	rpte2 = memory[rpta+1];					// S___pppppppppppp
	if (DEFINED(rpte1))	{					// rpte defined
		memHits++;
		rpte1 = SET_PINNED(rpte1);
	} else if (PAGED(rpte2)) {				// rpte in swap space
		if (DEBUG_LEVEL) printf("\nBring the upt frame back from swap space");
		memPageFaults++;
		rptFrame = getFrame(-1);
		rpte1 = FRAME(rptFrame);
		rpte1 = SET_DEFINED(rpte1);
		rpte1 = SET_REF(rpte1);
		rpte1 = SET_PINNED(rpte1);
		rpte2 = CLEAR_PAGED(rpte2);
		accessPage(SWAPPAGE(rpte2), FRAME(rpte1), PAGE_READ);
	} else {								// get frame for upt
		if (DEBUG_LEVEL) printf("\nGet a new frame for UPT");
		memPageFaults++;
		rptFrame = getFrame(-1);
		int rptFrameAddr = FRAMEADDR(rptFrame);
		memset(&memory[rptFrameAddr], 0, sizeof(memory[0]) * LC3_FRAME_SIZE);
		// frameEntryPointer = &memory[FRAMEADDR(rptFrame)];
		// for (i=0; i<LC3_FRAME_SIZE; i++)
		// 	*frameEntryPointer++ = 0;
		rpte1 |= FRAME(rptFrame);
		rpte1 = SET_DEFINED(rpte1);
	}
	memory[rpta] = SET_REF(rpte1);			// set rpt frame access bit

	upta = (FRAME(rpte1)<<6) + UPTI(va);	// user page table address
	upte1 = memory[upta]; 					// FDRP__ffffffffff
	upte2 = memory[upta+1]; 				// S___pppppppppppp
	if (DEFINED(upte1))	{					// upte defined
		memHits++;
	} else if (PAGED(upte2)) {				// upte in swap space
		if (DEBUG_LEVEL) printf("\nBring the data frame back from swap space");
		uptFrame = getFrame(FRAME(rpte1));
		memPageFaults++;
		upte1 = FRAME(uptFrame);
		upte1 = SET_DEFINED(upte1);
		upte1 = SET_REF(upte1);
		upte2 = CLEAR_PAGED(upte2);
		accessPage(SWAPPAGE(upte2), FRAME(upte1), PAGE_READ);
	} else {								// get frame for data
		if (DEBUG_LEVEL) printf("\nGet a new frame for data");
		memPageFaults++;
		uptFrame = getFrame(FRAME(rpte1));
		upte1 |= FRAME(uptFrame);
		upte1 = SET_DEFINED(upte1);
	}
	memory[upta] = SET_REF(upte1); 			// set upt frame access bit
	int physical_address = (FRAME(upte1)<<6) + FRAMEOFFSET(va);
	if (DEBUG_LEVEL > 2) printVMTables(va, physical_address);
	return &memory[physical_address];
} // end getMemAdr


// **************************************************************************
// **************************************************************************
// set frames available from sf to ef
//    flg = 0 -> clear all others
//        = 1 -> just add bits
//
void setFrameTableBits(int flg, int sf, int ef)
{	int i, data;
	int adr = LC3_FBT-1;             // index to frame bit table
	int fmask = 0x0001;              // bit mask

	// 1024 frames in LC-3 memory
	for (i=0; i<LC3_FRAMES; i++)
	{	if (fmask & 0x0001)
		{  fmask = 0x8000;
			adr++;
			data = (flg)?MEMWORD(adr):0;
		}
		else fmask = fmask >> 1;
		// allocate frame if in range
		if ( (i >= sf) && (i < ef)) data = data | fmask;
		MEMWORD(adr) = data;
	}
	return;
} // end setFrameTableBits


// **************************************************************************
// get frame from frame bit table (else return -1)
int getAvailableFrame()
{
	int i, data;
	int adr = LC3_FBT - 1;				// index to frame bit table
	int fmask = 0x0001;					// bit mask

	for (i=0; i<LC3_FRAMES; i++)		// look thru all frames
	{	if (fmask & 0x0001)
		{  fmask = 0x8000;				// move to next work
			adr++;
			data = MEMWORD(adr);
		}
		else fmask = fmask >> 1;		// next frame
		// deallocate frame and return frame #
		if (data & fmask)
		{  MEMWORD(adr) = data & ~fmask;
			return i;
		}
	}
	return -1;
} // end getAvailableFrame



// **************************************************************************
// read/write to swap space
int accessPage(int pnum, int frame, int rwnFlg)
{
	static int nextPage;						// swap page size
	static int pageReads;						// page reads
	static int pageWrites;						// page writes
	static unsigned short int swapMemory[LC3_MAX_SWAP_MEMORY];

	if ((nextPage >= LC3_MAX_PAGE) || (pnum >= LC3_MAX_PAGE))
	{
		printf("\nVirtual Memory Space Exceeded!  (%d)", LC3_MAX_PAGE);
		exit(-4);
	}
	switch(rwnFlg)
	{
		case PAGE_INIT:                    		// init paging
			memAccess = 0;						// memory accesses
			memHits = 0;						// memory hits
			memPageFaults = 0;					// memory faults
			nextPage = 0;						// disk swap space size
			pageReads = 0;						// disk page reads
			pageWrites = 0;						// disk page writes
			return 0;

		case PAGE_GET_SIZE:                    	// return swap size
			return nextPage;

		case PAGE_GET_READS:                   	// return swap reads
			return pageReads;

		case PAGE_GET_WRITES:                    // return swap writes
			return pageWrites;

		case PAGE_GET_ADR:                    	// return page address
			return (int)(long)(&swapMemory[pnum<<6]);

		case PAGE_NEW_WRITE:                   // new write (Drops thru to write old)
			pnum = nextPage++;

		case PAGE_OLD_WRITE:                   // write
			if (DEBUG_LEVEL) printf("\n    (%d) Write frame %d (memory[%04x]) to page %d", curTask, frame, frame<<6, pnum);
			memcpy(&swapMemory[pnum<<6], &memory[frame<<6], 1<<7);
			pageWrites++;
			return pnum;

		case PAGE_READ:                    	// read
			if (DEBUG_LEVEL) printf("\n    (%d) Read page %d into frame %d (memory[%04x])", curTask, pnum, frame, frame<<6);
			memcpy(&memory[frame<<6], &swapMemory[pnum<<6], 1<<7);
			pageReads++;
			return pnum;

		case PAGE_FREE:                   // free page
			printf("\nPAGE_FREE not implemented");
			break;
   }
   return pnum;
} // end accessPage
