// os345pqueue.h	09/15/2016
//
//
//
#ifndef __os345pqueue_h__
#define __os345pqueue_h__

// ***********************************************************************
// PQueue definitions
typedef int* PQueue;					// Priority Queue


// ***********************************************************************
// PQueue prototypes
int enqueueTask(PQueue pqueue, TID taskID);
TID dequeueTask(PQueue pqueue);
PQueue newPQueue();

#endif//__os345pqueue_h__