// synch.cc 
//	Routines for synchronizing threads.  Three kinds of
//	synchronization routines are defined here: semaphores, locks 
//   	and condition variables (the implementation of the last two
//	are left to the reader).
//
// Any implementation of a synchronization routine needs some
// primitive atomic operation.  We assume Nachos is running on
// a uniprocessor, and thus atomicity can be provided by
// turning off interrupts.  While interrupts are disabled, no
// context switch can occur, and thus the current thread is guaranteed
// to hold the CPU throughout, until interrupts are reenabled.
//
// Because some of these routines might be called with interrupts
// already disabled (Semaphore::V for one), instead of turning
// on interrupts at the end of the atomic operation, we always simply
// re-set the interrupt state back to its original value (whether
// that be disabled or enabled).
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "synch.h"
#include "system.h"

//----------------------------------------------------------------------
// Semaphore::Semaphore
// 	Initialize a semaphore, so that it can be used for synchronization.
//
//	"debugName" is an arbitrary name, useful for debugging.
//	"initialValue" is the initial value of the semaphore.
//----------------------------------------------------------------------

Semaphore::Semaphore(char* debugName, int initialValue)
{
    name = debugName;
    value = initialValue;
    queue = new List;
}

//----------------------------------------------------------------------
// Semaphore::Semaphore
// 	De-allocate semaphore, when no longer needed.  Assume no one
//	is still waiting on the semaphore!
//----------------------------------------------------------------------

Semaphore::~Semaphore()
{
    delete queue;
}

//----------------------------------------------------------------------
// Semaphore::P
// 	Wait until semaphore value > 0, then decrement.  Checking the
//	value and decrementing must be done atomically, so we
//	need to disable interrupts before checking the value.
//
//	Note that Thread::Sleep assumes that interrupts are disabled
//	when it is called.
//----------------------------------------------------------------------

void
Semaphore::P()
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);	// disable interrupts
    
    while (value == 0) { 			// semaphore not available
    //printf("go to sleep\n");
	queue->Append((void *)currentThread);	// so go to sleep
	currentThread->Sleep();
    //printf("waitup\n");
    } 
    value--; 					// semaphore available, 
						// consume its value
    
    (void) interrupt->SetLevel(oldLevel);	// re-enable interrupts
}

//----------------------------------------------------------------------
// Semaphore::V
// 	Increment semaphore value, waking up a waiter if necessary.
//	As with P(), this operation must be atomic, so we need to disable
//	interrupts.  Scheduler::ReadyToRun() assumes that threads
//	are disabled when it is called.
//----------------------------------------------------------------------

void
Semaphore::V()
{
    Thread *thread;
    IntStatus oldLevel = interrupt->SetLevel(IntOff);

    thread = (Thread *)queue->Remove();
    if (thread != NULL)	   // make thread ready, consuming the V immediately
	scheduler->ReadyToRun(thread);
    value++;
    (void) interrupt->SetLevel(oldLevel);
}

// Dummy functions -- so we can compile our later assignments 
// Note -- without a correct implementation of Condition::Wait(), 
// the test case in the network assignment won't work!
Lock::Lock(char* debugName) 
{
    name = debugName;
    lock = new Semaphore(debugName,1);
    lockingThread = NULL;
}
Lock::~Lock() 
{
    delete lock;
}
void Lock::Acquire() 
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    lock->P();
    lockingThread = currentThread;
    (void) interrupt->SetLevel(oldLevel);
}
void Lock::Release() 
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    //ASSERT(isHeldByCurrentThread())
    lock->V();
    lockingThread = NULL;
    (void) interrupt->SetLevel(oldLevel);
}

bool Lock::isHeldByCurrentThread()
{
    return currentThread == lockingThread;
}

Condition::Condition(char* debugName) 
{
    name = debugName;
    waitingList = new List;
}
Condition::~Condition() 
{
    delete waitingList;
}
void Condition::Wait(Lock* conditionLock) 
{ 
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    conditionLock->Release();
    waitingList->Append((void*)currentThread);
    currentThread->Sleep();
    conditionLock->Acquire();
    (void) interrupt->SetLevel(oldLevel);
}
void Condition::Signal(Lock* conditionLock) 
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    Thread* nextThread = (Thread*)waitingList->Remove();
    if(nextThread != NULL)
        scheduler->ReadyToRun(nextThread);
    (void) interrupt->SetLevel(oldLevel); 
}
void Condition::Broadcast(Lock* conditionLock) 
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    Thread* nextThread;
    while((nextThread = (Thread*)waitingList->Remove()) != NULL)
    {
        scheduler->ReadyToRun(nextThread);
    }
    (void) interrupt->SetLevel(oldLevel); 
}


Barrier::Barrier(char* debugName, int num)
{
    name = debugName;
    maxNum = num;
    currentNum = 0;
    mutex = new Lock("mutex");
    arrival = new Condition("arrival");
}
Barrier::~Barrier()
{
    delete arrival;
    delete mutex;
}
void Barrier::Wait()
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    mutex->Acquire();
    currentNum++;
    if(currentNum == maxNum)
    {
        printf("barrier arrival, Release\n");
        arrival->Broadcast(mutex);
        currentNum = 0;
    }
    else
    {
        printf("thread: %d wait at barrier %s\n", 
            currentThread->gettid(), name);
        arrival->Wait(mutex);
    }
    mutex->Release();
    (void) interrupt->SetLevel(oldLevel);
}

RWLock::RWLock(char* debugName)
{
    name = debugName;
    mutex = new Lock("mutex");
    W = new Lock("W");
    readCount = 0;
}

RWLock::~RWLock()
{
    delete mutex;
    delete W;
}

void
RWLock::Read_start()
{
    mutex->Acquire();
    readCount++;
    if(readCount == 1)
        W->Acquire();
    mutex->Release();
}

void
RWLock::Read_end()
{
    mutex->Acquire();
    readCount--;
    if(readCount == 0)
        W->Release();
    mutex->Release();
}

void
RWLock::Write_start()
{
    W->Acquire();
}

void
RWLock::Write_end()
{
    W->Release();
}