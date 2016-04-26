// scheduler.h 
//	Data structures for the thread dispatcher and scheduler.
//	Primarily, the list of threads that are ready to run.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "copyright.h"
#include "list.h"
#include "thread.h"

#define MaxThreadNum 128

// The following class defines the scheduler/dispatcher abstraction -- 
// the data structures and operations needed to keep track of which 
// thread is running, and which threads are ready but not running.

class Scheduler {
  public:
    Scheduler();			// Initialize list of ready threads 
    ~Scheduler();			// De-allocate ready list
    void ThreadStatus();
    int AddThread(Thread *thread);
    void RemoveThread(Thread *thread);
    void ReadyToRun(Thread* thread);	// Thread can be dispatched.
    Thread* FindNextToRun();		// Dequeue first thread on the ready 
					// list, if any, and return thread.
    void Run(Thread* nextThread);	// Cause nextThread to start running
    void Print();			// Print contents of ready list
#ifdef USER_PROGRAM
    bool isActive(int tid){return threadMap->Test(tid);}

    int getExitNum(int tid){return exitNum[tid];}

    void setExitNum(int tid, int num){exitNum[tid] = num;}
#endif
    List *readyList;    // queue of threads that are ready to run,
                // but not running
    List *suspendList;
  private: 	
    List *allList; // all the threads
#ifdef USER_PROGRAM
    #include "bitmap.h"
    BitMap* threadMap;
    Thread** threadEntry;
    int exitNum[MaxThreadNum];
#endif
};

#endif // SCHEDULER_H
