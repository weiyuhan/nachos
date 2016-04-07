// addrspace.cc 
//	Routines to manage address spaces (executing user programs).
//
//	In order to run a user program, you must:
//
//	1. link with the -N -T 0 option 
//	2. run coff2noff to convert the object file to Nachos format
//		(Nachos object code format is essentially just a simpler
//		version of the UNIX executable object code format)
//	3. load the NOFF file into the Nachos file system
//		(if you haven't implemented the file system yet, you
//		don't need to do this last step)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "addrspace.h"
#include "noff.h"   
#ifdef HOST_SPARC
#include <strings.h>
#endif

char* itoa(int num)
{
    char* _ret = new char[10];
    int i = 0;
    if(num == 0)
    {
        _ret[0] = '0';
        i++;
    }
    while(num > 0)
    {
        int left = num % 10;
        char tmp = '0' + left;
        _ret[i] = tmp;
        i++;
        num = num / 10;
    }
    _ret[i] = '\0';
    //printf("%s\n", _ret);
    return _ret;
}

//----------------------------------------------------------------------
// SwapHeader
// 	Do little endian to big endian conversion on the bytes in the 
//	object file header, in case the file was generated on a little
//	endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------

static void 
SwapHeader (NoffHeader *noffH)
{
	noffH->noffMagic = WordToHost(noffH->noffMagic);
	noffH->code.size = WordToHost(noffH->code.size);
	noffH->code.virtualAddr = WordToHost(noffH->code.virtualAddr);
	noffH->code.inFileAddr = WordToHost(noffH->code.inFileAddr);
	noffH->initData.size = WordToHost(noffH->initData.size);
	noffH->initData.virtualAddr = WordToHost(noffH->initData.virtualAddr);
	noffH->initData.inFileAddr = WordToHost(noffH->initData.inFileAddr);
	noffH->uninitData.size = WordToHost(noffH->uninitData.size);
	noffH->uninitData.virtualAddr = WordToHost(noffH->uninitData.virtualAddr);
	noffH->uninitData.inFileAddr = WordToHost(noffH->uninitData.inFileAddr);
}

//----------------------------------------------------------------------
// AddrSpace::AddrSpace
// 	Create an address space to run a user program.
//	Load the program from a file "executable", and set everything
//	up so that we can start executing user instructions.
//
//	Assumes that the object code file is in NOFF format.
//
//	First, set up the translation from program memory to physical 
//	memory.  For now, this is really simple (1:1), since we are
//	only uniprogramming, and we have a single unsegmented page table
//
//	"executable" is the file containing the object code to load into memory
//----------------------------------------------------------------------

AddrSpace::AddrSpace(OpenFile *executable)
{
    LoadSwapSpace(executable);

// first, set up the translation 
    TLBMissCount = 0;
    PageFaultCount = 0;
#ifdef TLB_FIFO
    TLBFIFO_List = new List;
#endif

}

//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
// 	Dealloate an address space.  Nothing for now!
//----------------------------------------------------------------------

AddrSpace::~AddrSpace()
{
    //machine->leftPages += maxPagesinMem;
    for(int i = 0; i < NumPhysPages; i++)
    {
        if(machine->reversePageTable[i].valid &&
            machine->reversePageTable[i].ownerThread == (void*) threadToBeDestroyed)
        {
            pageMap->Clear(i);
            machine->reversePageTable[i].valid = FALSE;
        }
    }
#ifdef TLB_FIFO
    delete TLBFIFO_List;
#endif
    delete swap;
    fileSystem->Remove(itoa(threadToBeDestroyed->gettid()));

}

//----------------------------------------------------------------------
// AddrSpace::InitRegisters
// 	Set the initial values for the user-level register set.
//
// 	We write these directly into the "machine" registers, so
//	that we can immediately jump to user code.  Note that these
//	will be saved/restored into the currentThread->userRegisters
//	when this thread is context switched out.
//----------------------------------------------------------------------

void
AddrSpace::InitRegisters()
{
    int i;

    for (i = 0; i < NumTotalRegs; i++)
	machine->WriteRegister(i, 0);

    // Initial program counter -- must be location of "Start"
    machine->WriteRegister(PCReg, 0);	

    // Need to also tell MIPS where next instruction is, because
    // of branch delay possibility
    machine->WriteRegister(NextPCReg, 4);

   // Set the stack register to the end of the address space, where we
   // allocated the stack; but subtract off a bit, to make sure we don't
   // accidentally reference off the end!
    machine->WriteRegister(StackReg, numPages * PageSize - 16);
    DEBUG('a', "Initializing stack register to %d\n", numPages * PageSize - 16);
}

//----------------------------------------------------------------------
// AddrSpace::SaveState
// 	On a context switch, save any machine state, specific
//	to this address space, that needs saving.
//
//	For now, nothing!
//----------------------------------------------------------------------

void AddrSpace::SaveState() 
{
    for(int i = 0; i < TLBSize; i++)
    {
        machine->tlb[i].valid = FALSE;
        machine->tlb[i].dirty = FALSE;
        machine->tlb[i].readOnly = FALSE;
    }
}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
// 	On a context switch, restore the machine state so that
//	this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------



void AddrSpace::RestoreState() 
{

}

void AddrSpace::LoadSwapSpace(OpenFile *executable)
{
    NoffHeader noffH;
    unsigned int i, size;

    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) && 
        (WordToHost(noffH.noffMagic) == NOFFMAGIC))
        SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);


// how big is address space?
    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size 
            + UserStackSize;    // we need to increase the size
                        // to leave room for the stack
    numPages = divRoundUp(size, PageSize);
    size = numPages * PageSize;

    DEBUG('a', "loadint swap space, num pages %d, size %d\n", 
                numPages, size);

    char* Buffer = new char[size];
    memset(Buffer,0,sizeof(Buffer));

    if(noffH.code.size > 0)
    {
        executable->ReadAt(Buffer, noffH.code.size, noffH.code.inFileAddr);
    }
    if(noffH.initData.size > 0)
    {
        executable->ReadAt(Buffer + noffH.code.size, noffH.initData.size, 
            noffH.initData.inFileAddr);
    }
    char* swapname = itoa(currentThread->gettid());
    fileSystem->Create(swapname, size);
    swap = fileSystem->Open(swapname);
    swap->WriteAt(Buffer,size,0);

    delete [] Buffer;
}
