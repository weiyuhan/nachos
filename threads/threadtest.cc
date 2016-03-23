// threadtest.cc 
//	Simple test case for the threads assignment.
//
//	Create two threads, and have them context switch
//	back and forth between themselves by calling Thread::Yield, 
//	to illustratethe inner workings of the thread system.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "elevatortest.h"
#include "synch.h"
#include "synchlist.h"

// testnum is set in main.cc
int testnum = 1;
int locknum = 0;
Lock* lock;
Condition* condition;
Condition* notfull;
Condition* notempty;
SynchList* synchlist;
Semaphore* mutex;
Semaphore* full;
Semaphore* empty;
Barrier* barrier;
RWLock* rwlock;
//----------------------------------------------------------------------
// SimpleThread
// 	Loop 5 times, yielding the CPU to another ready thread 
//	each iteration.
//
//	"which" is simply a number identifying the thread, for debugging
//	purposes.
//----------------------------------------------------------------------

void
SimpleThread(int which)
{
    int num;
    currentThread->Print();
    for (num = 0; num < 5; num++) {
	printf("*** thread %d looped %d times\n", which, num);
        currentThread->Yield();
    }
}

//----------------------------------------------------------------------
// PrintThread
//  print the status of currentThread
//----------------------------------------------------------------------
void PrintThread(int dummy)
{
    currentThread->Print();
    //printf("\n\n");
}

void TickThread(int dummy)
{
    for(int i = 0; i <1000; i++)
        interrupt->OneTick();
}

void LockThread(int dummy)
{
    lock->Acquire();
    for(int i = 0; i <20; i++)
    {
        for(int j = 0; j <20; j++)
            interrupt->OneTick();
        printf("tid: %d ,locknum = %d\n", currentThread->gettid(), locknum);
        locknum++;
    }
    lock->Release();
}

void ConditionThread(int dummy)
{
    lock->Acquire();
    for(int i = 1; i < 21; i++)
    {
        printf("tid: %d ,locknum = %d\n", currentThread->gettid(), locknum);
        locknum++;
        if(i % 5 == 0 && dummy == 0)
        {
            condition->Wait(lock);
            condition->Signal(lock);
        }
        if(i % 5 == 0 && dummy == 1)
        {
            condition->Signal(lock);
            condition->Wait(lock);
        }
    }
    lock->Release();
}

void producerThread(int dummy)
{
    int i = 0;
    while(true)
    {
        if( i % 7 == 0)
            currentThread->Yield();
        for(int j = 0; j < 10; j++)
            interrupt->OneTick();
        i++;
        lock->Acquire();
        if(locknum == 10)
        {
            printf("wait for consumer\n");
            notfull->Wait(lock);
        }
        locknum++;
        printf("producer: now item %d\n", locknum);
        if(locknum == 1)
        {
            notempty->Signal(lock);   
        }
        lock->Release();
    }
}

void consumerThread(int dummy)
{
    int i = 0;
    while(true)
    {
        if(i % 13 == 0 )
            currentThread->Yield();
        for(int j = 0; j < 10; j++)
            interrupt->OneTick();
        i++;
        lock->Acquire();
        if(locknum == 0)
        {
            printf("wait for producer\n");
            notempty->Wait(lock);
        }
        locknum--;
        printf("consumer: now item %d\n", locknum);
        if(locknum == 9)
        {
            notfull->Signal(lock);   
        }
        lock->Release();
    }
}

void producerThreadSem(int dummy)
{
    int i = 0;
    while(true)
    {
        if( i % 7 == 0)
            currentThread->Yield();
        for(int j = 0; j < 10; j++)
            interrupt->OneTick();
        i++;
        empty->P();
        mutex->P();
        locknum++;
        printf("producer: now item %d\n", locknum);
        mutex->V();
        full->V();
    }
}

void consumerThreadSem(int dummy)
{
    int i = 0;
    while(true)
    {
        if(i % 13 == 0 )
            currentThread->Yield();
        for(int j = 0; j < 10; j++)
            interrupt->OneTick();
        i++;
        full->P();
        mutex->P();
        locknum--;
        printf("consumer: now item %d\n", locknum);
        mutex->V();
        empty->V();
    }
}

void BarrierThread(int dummy)
{
    for(int i = 0; i < 10; i++)
    {
        for(int j = 0; j < 10; j++)
            interrupt->OneTick();
        if(i == 7 || i == 3)
            barrier->Wait();
        printf("thread %d print %d\n", currentThread->gettid(), i);
    }
}

void ReaderThread(int dummy)
{
    while(true)
    {
        for(int j = 0; j < 10; j++)
            interrupt->OneTick();
        rwlock->Read_start();
        printf("Reader %d is reading, locknum is %d\n", 
            currentThread->gettid(), locknum);
        rwlock->Read_end();
    }
}

void WriterThread(int dummy)
{
    while(true)
    {
        for(int j = 0; j < 20; j++)
                interrupt->OneTick();
        rwlock->Write_start();
        locknum++;
        printf("Writer %d is writing, locknum is %d\n", 
            currentThread->gettid(), locknum);
        rwlock->Write_end();
    }
}
//----------------------------------------------------------------------
// ThreadTest1
// 	Set up a ping-pong between two threads, by forking a thread 
//	to call SimpleThread, and then calling SimpleThread ourselves.
//----------------------------------------------------------------------

void
ThreadTest1()
{
    DEBUG('t', "Entering ThreadTest1");

    Thread *t = new Thread("forked thread");
    t->Fork(SimpleThread, (void*)1);
    SimpleThread(0);
}

//----------------------------------------------------------------------
// ThreadTest2
//  Loop 130 times to create a printThread
//----------------------------------------------------------------------

void
ThreadTest2()
{
    DEBUG('t', "Entering ThreadTest2");
    for(int i = 0; i < 130; i++)
    {
        Thread *t = new Thread("forked thread", testnum);
        if(t->gettid() == -1)
        {
            printf("can't fork!\n");
            continue;
        }
        t->Fork(PrintThread, (void*)1);
    }
    PrintThread(0);
    scheduler->ThreadStatus();
}

void
ThreadTest3()
{
    DEBUG('t', "Entering ThreadTest3");
    for(int i = 0; i < 20; i++)
    {
        Thread *t = NULL;
        if(i % 2 == 0)
            t = new Thread("forked thread", 1, 9);
        else
            t = new Thread("forked thread", 2, 11);
        if(t->gettid() == -1)
        {
            printf("can't fork!\n");
            continue;
        }
        t->Fork(PrintThread, (void*)t->gettid());
    }
    PrintThread(0);
    scheduler->ThreadStatus();
}

void
ThreadTest4()
{
    DEBUG('t', "Entering ThreadTest4");
    for(int i = 0; i < 4; i++)
    {
        Thread *t = new Thread("forked thread", testnum, 11);
        if(t->gettid() == -1)
        {
            printf("can't fork!\n");
        }
        t->Fork(TickThread, (void*)1);
    }
    scheduler->Print();
    TickThread(1);
}

ThreadTest5()
{
    lock = new Lock("lock");
    condition = new Condition("condition");
    for(int i = 0; i < 2; i++)
    {
        Thread *t = new Thread("forked thread", testnum, 5);
        if(t->gettid() == -1)
        {
            printf("can't fork!\n");
        }
        t->Fork(ConditionThread, (void*)i);
    }
}

ThreadTest6()
{
    lock = new Lock("lock");
    notfull = new Condition("notfull");
    notempty = new Condition("notempty");
    Thread *producer = new Thread("producer", testnum, 5);
    if(producer->gettid() == -1)
    {
        printf("can't fork!\n");
    }
    producer->Fork(producerThread, (void*)1);
    Thread *consumer = new Thread("consumer", testnum, 5);
    if(consumer->gettid() == -1)
    {
        printf("can't fork!\n");
    }
    consumer->Fork(consumerThread, (void*)1);
}

ThreadTest7()
{
    mutex = new Semaphore("mutex", 1);
    empty = new Semaphore("empty", 10);
    full = new Semaphore("full", 0);
    Thread *producer = new Thread("producer", testnum, 5);
    if(producer->gettid() == -1)
    {
        printf("can't fork!\n");
    }
    producer->Fork(producerThreadSem, (void*)1);
    Thread *consumer = new Thread("consumer", testnum, 5);
    if(consumer->gettid() == -1)
    {
        printf("can't fork!\n");
    }
    consumer->Fork(consumerThreadSem, (void*)1);
}

ThreadTest8()
{
    barrier = new Barrier("barrier", 5);
    for(int i = 0; i < 5; i++)
    {
        Thread *t = new Thread("forked thread", testnum, 5);
        if(t->gettid() == -1)
        {
            printf("can't fork!\n");
        }
        t->Fork(BarrierThread, (void*)i);
    }
}

ThreadTest9()
{
    rwlock = new RWLock("rw");
    locknum = 0;
    for(int i = 0; i < 5 ; i++)
    {
        Thread *t = new Thread("forked thread", testnum, 5);
        if(t->gettid() == -1)
        {
            printf("can't fork!\n");
        }
        t->Fork(ReaderThread, (void*)i);
    }
    for(int i = 0; i < 2 ; i++)
    {
        Thread *t = new Thread("forked thread", testnum, 5);
        if(t->gettid() == -1)
        {
            printf("can't fork!\n");
        }
        t->Fork(WriterThread, (void*)i);
    }
}
//----------------------------------------------------------------------
// ThreadTest
// 	Invoke a test routine.
//----------------------------------------------------------------------

void
ThreadTest()
{
    switch (testnum) {
    case 1:
	ThreadTest1();
	break;
    case 2:
    ThreadTest2();
    break;
    case 3:
    ThreadTest3();
    break;
    case 4:
    ThreadTest4();
    break;
    case 5:
    ThreadTest5();
    break;
    case 6:
    ThreadTest6();
    break;
    case 7:
    ThreadTest7();
    break;
    case 8:
    ThreadTest8();
    break;
    case 9:
    ThreadTest9();
    break;
    default:
	printf("No test specified.\n");
	break;
    }
}

