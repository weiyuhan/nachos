// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"

//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2. 
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	are in machine.h.
//----------------------------------------------------------------------

void SysHalt()
{
    DEBUG('a', "Shutdown, initiated by user program.\n");
    interrupt->Halt();
}

void SysExit()
{
    int exitAddress = machine->ReadRegister(4);
    int exitNum;
    machine->ReadMem(exitAddress, 1, &exitNum);
    printf("EXIT NUM : %d\n", exitAddress);
    printf("Total TLB miss : %d\n",
            currentThread->space->TLBMissCount);
    printf("Total Page Fault : %d\n",
            currentThread->space->PageFaultCount);
    currentThread->Print();
    currentThread->Finish();
}

void SysCreate()
{
    int nameAddress = machine->ReadRegister(4);
    int value;
    int count = 0;
    char name[20];
    do
    {
        machine->ReadMem(nameAddress++, 1, &value);
        name[count++] = (char)value;
    }while(value != 0);

    if(!fileSystem->Create(name))
        machine->WriteRegister(2, 0);
    machine->WriteRegister(2, 1);
}

void SysOpen()
{
    int nameAddress = machine->ReadRegister(4);
    int value;
    int count = 0;
    char name[20];
    do
    {
        machine->ReadMem(nameAddress++, 1, &value);
        name[count++] = (char)value;
    }while(value != 0);

    OpenFileId fileId = fileSystem->OpenAFile(name);
    machine->WriteRegister(2, fileId);
}

void SysClose()
{
    OpenFileId fileId = (OpenFileId)machine->ReadRegister(4);
    fileSystem->CloseFile((int)fileId);
}

void SysWrite()
{

    int bufferAddress = machine->ReadRegister(4);
    int size = (int)machine->ReadRegister(5);
    int value;
    int count = 0;
    char* buffer = new char[size];
    while(count < size)
    {
        machine->ReadMem(bufferAddress++, 1, &value);
        buffer[count] = (char)value;
    }

    OpenFileId fileId = (OpenFileId)machine->ReadRegister(6);
    int numWrite = fileSystem->WriteFile(buffer, size, fileId);
    machine->WriteRegister(2, numWrite);

    delete buffer;
}

void SysRead()
{
    int bufferAddress = machine->ReadRegister(4);
    int size = (int)machine->ReadRegister(5);
    int value;
    int count = 0;
    char* buffer = new char[size];
    while(count < size)
    {
        machine->ReadMem(bufferAddress++, 1, &value);
        buffer[count] = (char)value;
    }

    OpenFileId fileId = (OpenFileId)machine->ReadRegister(6);
    int numRead = fileSystem->ReadFile(buffer, size, fileId);
    machine->WriteRegister(2, numRead);

    delete buffer;
}

void SysPrint()
{
    int* contentAddress = machine->ReadRegister(4);
    char type = (char)machine->ReadRegister(5);
    int value;
    int count = 0;
    switch(type)
    {
        case 'd':
            machine->ReadMem(contentAddress, 1, &value);
            printf("%d\n", value);
            break;
        case 's':
            char buffer[100];
            do
            {
                machine->ReadMem(contentAddress++, 1, &value);
                buffer[count++] = (char)value;
            }while(value != 0);
            printf("%s\n", buffer);
            break;
        case 'x':
            machine->ReadMem(contentAddress, 1, &value);
            printf("%x\n", value);
            break;
        case 'c':
            machine->ReadMem(contentAddress, 1, &value);
            printf("%c\n", (char)value);
            break;
        default:
            break;
    }
}

void
ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);

    if (which == SyscallException)
    {
        switch(type)
        {
            case SC_Halt:
                SysHalt();
                break;
            case SC_Exit:
                SysExit();
                break;
            case SC_Create:
                SysCreate();
                break;
            case SC_Open:
                SysOpen();
                break;
            case SC_Close:
                SysClose();
                break;
            case SC_Write:
                SysWrite();
                break;
            case SC_Read:
                SysRead();
                break;
            case SC_Print:
                SysPrint();
                break;
            default:
                printf("Unexpected syscall %d\n", type);
                ASSERT(FALSE);
        }
    } else if ((which == TLBMissException))
    {
    	DEBUG('a', "TLB Miss.\n");
        int addr = machine->ReadRegister(BadVAddrReg);
        machine->TLBLoad(addr);
        currentThread->space->TLBMissCount++;
    }
    else if ((which == PageFaultException))
    {
        DEBUG('a', "handle PageFault\n");
        int addr = machine->ReadRegister(BadVAddrReg);
        machine->PageLoad(addr);
        currentThread->space->PageFaultCount++;
        stats->numPageFaults++;
    }
    else{
	printf("Unexpected user mode exception %d %d\n", which, type);
	ASSERT(FALSE);
    }
}
