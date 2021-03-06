// progtest.cc 
//	Test routines for demonstrating that Nachos can load
//	a user program and execute it.  
//
//	Also, routines for testing the Console hardware device.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "addrspace.h"
#include "synch.h"

extern void PrintThread(int dummy);

//----------------------------------------------------------------------
// StartProcess
// 	Run a user program.  Open the executable, load it into
//	memory, and jump to it.
//----------------------------------------------------------------------

void
StartProcess(char *filename)
{

    OpenFile *executable = fileSystem->Open(filename);
    AddrSpace *space;

    if (executable == NULL) {
	printf("Unable to open file %s\n", filename);
	return;
    }
    space = new AddrSpace(executable);    
    currentThread->space = space;

    delete executable;			// close file

    space->InitRegisters();		// set the initial register values
    space->RestoreState();		// load page table register

    machine->Run();			// jump to the user progam
    ASSERT(FALSE);			// machine->Run never returns;
					// the address space exits
					// by doing the syscall "exit"
}

#ifdef FILESYS

#include "synchconsole.h"

static SynchConsole* console;

void useConsole(int dummy)
{
    char ch;
    while(console != NULL) 
    {
        ch = console->GetCharPipe();   // echo it!
        console->PutChar(ch);
        if (ch == 'q') 
        {
            delete console;
            console = NULL;
            return;  // if q, quit
        }
        if (ch == 't') // ts
        {
            scheduler->ThreadStatus();
        }
        currentThread->Yield();
    }
}


void 
ConsoleTest (char *in, char *out)
{
    char ch;

    console = new SynchConsole(in, out, TRUE);
    
    
    Thread *t = new Thread("forked thread", 1);
    if(t->gettid() == -1)
    {
        printf("can't fork!\n");
    }
    t->Fork(useConsole, 0);
    

    while(console != NULL) 
    {
        ch = console->GetChar();
        console->PutCharPipe(ch);
        if (ch == 'q') 
        {
            delete console;
            console = NULL;
            return;  // if q, quit
        }
        if (ch == 't') // ts
        {
            scheduler->ThreadStatus();
        }
        currentThread->Yield();
    }
}

#else

#include "console.h"
// Data structures needed for the console test.  Threads making
// I/O requests wait on a Semaphore to delay until the I/O completes.

static Console *console;
static Semaphore *readAvail;
static Semaphore *writeDone;

//----------------------------------------------------------------------
// ConsoleInterruptHandlers
// 	Wake up the thread that requested the I/O.
//----------------------------------------------------------------------

static void ReadAvail(int arg) { readAvail->V(); }
static void WriteDone(int arg) { writeDone->V(); }

//----------------------------------------------------------------------
// ConsoleTest
// 	Test the console by echoing characters typed at the input onto
//	the output.  Stop when the user types a 'q'.
//  Print Threads Status when the user types a 't'
//----------------------------------------------------------------------

void 
ConsoleTest (char *in, char *out)
{
    char ch;

    console = new Console(in, out, ReadAvail, WriteDone, 0);
    readAvail = new Semaphore("read avail", 0);
    writeDone = new Semaphore("write done", 0);
    
    for (;;) {
    	readAvail->P();		// wait for character to arrive
    	ch = console->GetChar();
    	console->PutChar(ch);	// echo it!
    	writeDone->P() ;        // wait for write to finish
    	if (ch == 'q') return;  // if q, quit
        if (ch == 't') // ts
        {
            scheduler->ThreadStatus();
        }
        if(ch == 'c') // create 130 thread
        {
            #ifdef TLB_FIFO
            printf("hahaha\n");
            #endif
            Thread *t = new Thread("forked thread", 1);
            if(t->gettid() == -1)
            {
                printf("can't fork!\n");
                continue;
            }
            char* filename = "halt.coff";
            t->Fork(StartProcess, (void*)filename);
        }

        if(ch >= '0' && ch <= '9')
        {
            int priority = ch - '0';
            Thread *t = new Thread("forked thread", 1, priority);
            if(t->gettid() == -1)
            {
                printf("can't fork!\n");
                continue;
            }
            t->Fork(PrintThread, (void*)1);
        }
    }
}

#endif