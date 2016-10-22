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

// ***********************************************************************
// mmu variables

// LC-3 memory
unsigned short int memory[LC3_MAX_MEMORY];

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
int runClock() {
	static bool usingRPTPointer = TRUE;
	static unsigned short clockRPTPointer = LC3_RPT;
	static unsigned short clockUPTPointer;
	bool done = FALSE;
	int frame;
	unsigned short int rptEntry, uptEntry, swapEntry;
	while (!done) {
		rptEntry = memory[clockRPTPointer];
		uptEntry = memory[clockUPTPointer];
		// check what I'm pointing at
		if (usingRPTPointer) {
			if (DEFINED(rptEntry)) {
				if (REFERENCED(rptEntry)) {
					memory[clockRPTPointer] = rptEntry = CLEAR_REF(rptEntry);
				} else {
					usingRPTPointer = FALSE;
					clockUPTPointer = FRAMEADDR(rptEntry);
					continue;
				}
			}
		} else {
			if (DEFINED(uptEntry)) {
				if (REFERENCED(uptEntry)) {
					memory[clockUPTPointer] = uptEntry = CLEAR_REF(uptEntry);
				} else {
					swapEntry = memory[clockUPTPointer + 1];
					memory[clockUPTPointer + 1] = SET_PAGED(swapEntry);
					frame = FRAME(uptEntry);
					done = TRUE;
				}
			}
		}
		// move clock pointer
 		if (!usingRPTPointer) {											// if we are looking at a user page table
			clockUPTPointer += 2;										// go to next page table entry
			if (clockUPTPointer%LC3_FRAME_SIZE==0) {					// if you finished the frame
				usingRPTPointer = TRUE;									// go back out to the root page table
				if (!done && !PINNED(rptEntry)) {						// if the user page table you just left has no allocated pages, swap it out
					swapEntry = memory[clockRPTPointer + 1];
					memory[clockRPTPointer + 1] = SET_PAGED(swapEntry);
					frame = FRAME(rptEntry);							// set the frame address of the user page table we left
					done = TRUE;										// we're done
				}
			}
		}
		if (usingRPTPointer) {											// if we are looking a root page table
			clockRPTPointer += 2;										// go to the next apge table entry
			if (clockRPTPointer >= LC3_RPT_END)							// if we hit the end of root page table area
				clockRPTPointer = LC3_RPT;								// go back to the beginning
		}
	}
	return frame;														// return the frame number found above
}

int getFrame(int notme) {
	int frame, frameMem, i;
	frame = getAvailableFrame();
	if (frame >=0)
		return frame;

	frame = runClock();
	accessPage( -1, frame, PAGE_NEW_WRITE);

	frameMem = FRAMEADDR(frame);
	for (i=0; i<LC3_FRAME_SIZE; i++)
		memory[frameMem++] = 0;
	return frame;
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
	unsigned short int pa;
	int rpta, rpte1, rpte2;
	int upta, upte1, upte2;
	int rptFrame, uptFrame;

	// turn off virtual addressing for system RAM
	if (va < 0x3000) return &memory[va];
	// get physical address
	rpta = tcb[curTask].RPT + RPTI(va);		// root page table address
	rpte1 = memory[rpta];					// FDRP__ffffffffff
	rpte2 = memory[rpta+1];					// S___pppppppppppp
	if (DEFINED(rpte1))	{					// rpte defined
		rpte1 = SET_PINNED(rpte1);
	} else if (PAGED(rpte2)) {				// rpte in swap space
		printf("\n!!!PAGED!!!");
	} else {								// get frame for upt
		rptFrame = getFrame(-1);
		rpte1 |= FRAME(rptFrame);
		rpte1 = SET_DEFINED(rpte1);
		rpte1 = SET_PINNED(rpte1);
	}
	memory[rpta] = SET_REF(rpte1);			// set rpt frame access bit

	upta = (FRAME(rpte1)<<6) + UPTI(va);	// user page table address
	upte1 = memory[upta]; 					// FDRP__ffffffffff
	upte2 = memory[upta+1]; 				// S___pppppppppppp
	if (DEFINED(upte1))	{					// upte defined
		// ? do nothing ?
	} else if (PAGED(upte2)) {				// upte in swap space
		printf("\n!!!PAGED!!!");
	} else {								// get frame for data
		uptFrame = getFrame(-1);
		upte1 |= FRAME(uptFrame);
		upte1 = SET_DEFINED(upte1);
	}
	memory[upta] = SET_REF(upte1); 			// set upt frame access bit
	int physical_address = (FRAME(upte1)<<6) + FRAMEOFFSET(va);
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
			//printf("\n    (%d) Write frame %d (memory[%04x]) to page %d", p.PID, frame, frame<<6, pnum);
			memcpy(&swapMemory[pnum<<6], &memory[frame<<6], 1<<7);
			pageWrites++;
			return pnum;

		case PAGE_READ:                    	// read
			//printf("\n    (%d) Read page %d into frame %d (memory[%04x])", p.PID, pnum, frame, frame<<6);
			memcpy(&memory[frame<<6], &swapMemory[pnum<<6], 1<<7);
			pageReads++;
			return pnum;

		case PAGE_FREE:                   // free page
			printf("\nPAGE_FREE not implemented");
			break;
   }
   return pnum;
} // end accessPage
