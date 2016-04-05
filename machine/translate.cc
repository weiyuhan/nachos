// translate.cc 
//	Routines to translate virtual addresses to physical addresses.
//	Software sets up a table of legal translations.  We look up
//	in the table on every memory reference to find the true physical
//	memory location.
//
// Two types of translation are supported here.
//
//	Linear page table -- the virtual page # is used as an index
//	into the table, to find the physical page #.
//
//	Translation lookaside buffer -- associative lookup in the table
//	to find an entry with the same virtual page #.  If found,
//	this entry is used for the translation.
//	If not, it traps to software with an exception. 
//
//	In practice, the TLB is much smaller than the amount of physical
//	memory (16 entries is common on a machine that has 1000's of
//	pages).  Thus, there must also be a backup translation scheme
//	(such as page tables), but the hardware doesn't need to know
//	anything at all about that.
//
//	Note that the contents of the TLB are specific to an address space.
//	If the address space changes, so does the contents of the TLB!
//
// DO NOT CHANGE -- part of the machine emulation
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "machine.h"
#include "addrspace.h"
#include "system.h"

// Routines for converting Words and Short Words to and from the
// simulated machine's format of little endian.  These end up
// being NOPs when the host machine is also little endian (DEC and Intel).

unsigned int
WordToHost(unsigned int word) {
#ifdef HOST_IS_BIG_ENDIAN
	 register unsigned long result;
	 result = (word >> 24) & 0x000000ff;
	 result |= (word >> 8) & 0x0000ff00;
	 result |= (word << 8) & 0x00ff0000;
	 result |= (word << 24) & 0xff000000;
	 return result;
#else 
	 return word;
#endif /* HOST_IS_BIG_ENDIAN */
}

unsigned short
ShortToHost(unsigned short shortword) {
#ifdef HOST_IS_BIG_ENDIAN
	 register unsigned short result;
	 result = (shortword << 8) & 0xff00;
	 result |= (shortword >> 8) & 0x00ff;
	 return result;
#else 
	 return shortword;
#endif /* HOST_IS_BIG_ENDIAN */
}

unsigned int
WordToMachine(unsigned int word) { return WordToHost(word); }

unsigned short
ShortToMachine(unsigned short shortword) { return ShortToHost(shortword); }


//----------------------------------------------------------------------
// Machine::ReadMem
//      Read "size" (1, 2, or 4) bytes of virtual memory at "addr" into 
//	the location pointed to by "value".
//
//   	Returns FALSE if the translation step from virtual to physical memory
//   	failed.
//
//	"addr" -- the virtual address to read from
//	"size" -- the number of bytes to read (1, 2, or 4)
//	"value" -- the place to write the result
//----------------------------------------------------------------------

bool
Machine::ReadMem(int addr, int size, int *value)
{
    int data;
    ExceptionType exception;
    int physicalAddress;
    
    DEBUG('a', "Reading VA 0x%x, size %d\n", addr, size);
    
    exception = Translate(addr, &physicalAddress, size, FALSE, FALSE);
    if(exception == TLBMissException)
    {
    	//machine->RaiseException(exception, addr);
    	DEBUG('a', "find VA 0x%x in pagrtable\n", addr, size);
    	exception = Translate(addr, &physicalAddress, size, FALSE, TRUE);
    	if(exception == PageFaultException)
    	{
    		DEBUG('a', "load VA 0x%x into memory\n", addr, size);
    		machine->RaiseException(exception, addr);
    		exception = Translate(addr, &physicalAddress, size, FALSE, TRUE);
    		if (exception != NoException) {
				machine->RaiseException(exception, addr);
				return FALSE;
    		}
    	}
    	else if (exception != NoException) {
			machine->RaiseException(exception, addr);
			return FALSE;
    	}
    	machine->RaiseException(TLBMissException, addr);
    }else if (exception != NoException) {
	machine->RaiseException(exception, addr);
	return FALSE;
    }
    switch (size) {
      case 1:
	data = machine->mainMemory[physicalAddress];
	*value = data;
	break;
	
      case 2:
	data = *(unsigned short *) &machine->mainMemory[physicalAddress];
	*value = ShortToHost(data);
	break;
	
      case 4:
	data = *(unsigned int *) &machine->mainMemory[physicalAddress];
	*value = WordToHost(data);
	break;

      default: ASSERT(FALSE);
    }
    
    DEBUG('a', "\tvalue read = %8.8x\n", *value);
    return (TRUE);
}

//----------------------------------------------------------------------
// Machine::WriteMem
//      Write "size" (1, 2, or 4) bytes of the contents of "value" into
//	virtual memory at location "addr".
//
//   	Returns FALSE if the translation step from virtual to physical memory
//   	failed.
//
//	"addr" -- the virtual address to write to
//	"size" -- the number of bytes to be written (1, 2, or 4)
//	"value" -- the data to be written
//----------------------------------------------------------------------

bool
Machine::WriteMem(int addr, int size, int value)
{
    ExceptionType exception;
    int physicalAddress;
     
    DEBUG('a', "Writing VA 0x%x, size %d, value 0x%x\n", addr, size, value);

    exception = Translate(addr, &physicalAddress, size, TRUE, FALSE);
    if(exception == TLBMissException) // TLB Miss
    {
    	exception = Translate(addr, &physicalAddress, size, TRUE, TRUE);
    	if(exception == PageFaultException) // page fault
    	{
    		machine->RaiseException(exception, addr); // load page
    		exception = Translate(addr, &physicalAddress, size, TRUE, TRUE);
    		if (exception != NoException) {
				machine->RaiseException(exception, addr);
				return FALSE;
    		}
    	}
    	else if (exception != NoException) {
			machine->RaiseException(exception, addr);
			return FALSE;
    	}
    	machine->RaiseException(TLBMissException, addr);
    }else if (exception != NoException) {
	machine->RaiseException(exception, addr);
	return FALSE;
    }
    switch (size) {
      case 1:
	machine->mainMemory[physicalAddress] = (unsigned char) (value & 0xff);
	break;

      case 2:
	*(unsigned short *) &machine->mainMemory[physicalAddress]
		= ShortToMachine((unsigned short) (value & 0xffff));
	break;
      
      case 4:
	*(unsigned int *) &machine->mainMemory[physicalAddress]
		= WordToMachine((unsigned int) value);
	break;
	
      default: ASSERT(FALSE);
    }
    
    return TRUE;
}

//----------------------------------------------------------------------
// Machine::Translate
// 	Translate a virtual address into a physical address, using 
//	either a page table or a TLB.  Check for alignment and all sorts 
//	of other errors, and if everything is ok, set the use/dirty bits in 
//	the translation table entry, and store the translated physical 
//	address in "physAddr".  If there was an error, returns the type
//	of the exception.
//
//	"virtAddr" -- the virtual address to translate
//	"physAddr" -- the place to store the physical address
//	"size" -- the amount of memory being read or written
// 	"writing" -- if TRUE, check the "read-only" bit in the TLB
//----------------------------------------------------------------------

ExceptionType
Machine::Translate(int virtAddr, int* physAddr, int size, bool writing, bool usePageTable)
{
    int i;
    unsigned int vpn, offset;
    TranslationEntry *entry;
    unsigned int pageFrame;

    DEBUG('a', "\tTranslate 0x%x, %s: ", virtAddr, writing ? "write" : "read");

// check for alignment errors
    if (((size == 4) && (virtAddr & 0x3)) || 
    	((size == 2) && (virtAddr & 0x1)))
    {
		DEBUG('a', "alignment problem at %d, size %d!\n", virtAddr, size);
		return AddressErrorException;
    }
    
    // we must have either a TLB or a page table, or both!
    //ASSERT(tlb == NULL || pageTable == NULL);	
    ASSERT(tlb != NULL || pageTable != NULL);	

// calculate the virtual page number, and offset within the page,
// from the virtual address
    vpn = (unsigned) virtAddr / PageSize;
    offset = (unsigned) virtAddr % PageSize;
    
    if (usePageTable) 
    {		// => page table => vpn is index into table
		if (vpn >= pageTableSize) 
		{
		    DEBUG('a', "virtual page # %d too large for page table size %d!\n", 
				virtAddr, pageTableSize);
		    return AddressErrorException;
		} 
		else if (!pageTable[vpn].valid) 
		{
		    DEBUG('a', "virtual page # %d is invalid!\n", virtAddr);
		    return PageFaultException;
		}
		entry = &pageTable[vpn];
		entry->use = TRUE;
		if(writing)
			entry->dirty = TRUE;
		entry->lastUseTime = stats->totalTicks;
    } 
    else 
    {
        for (entry = NULL, i = 0; i < TLBSize; i++)
        {
    	    if (tlb[i].valid && (tlb[i].virtualPage == vpn)) 
    	    {
				entry = &tlb[i];			// FOUND!
			#ifdef TLB_LRU
				tlb[i].use = true;
				tlb[i].lastUseTime = stats->totalTicks;
			#endif
				pageTable[vpn].use = TRUE;
				if(writing)
					pageTable[vpn].dirty = TRUE;
				pageTable[vpn].lastUseTime = stats->totalTicks;
				break;
		    }
		}
		if (entry == NULL) 
		{				// not found
	    	DEBUG('a', "*** no valid TLB entry found for this virtual page!\n");
	    	return TLBMissException;		
	    	// really, this is a TLB fault,
							// the page may be in memory,
							// but not in the TLB
		}
    }

    if (entry->readOnly && writing) 
    {	// trying to write to a read-only page
		DEBUG('a', "%d mapped read-only at %d in TLB!\n", virtAddr, i);
		return ReadOnlyException;
    }
    pageFrame = entry->physicalPage;

    // if the pageFrame is too big, there is something really wrong! 
    // An invalid translation was loaded into the page table or TLB. 
    if (pageFrame >= NumPhysPages) 
    { 
		DEBUG('a', "*** frame %d > %d!\n", pageFrame, NumPhysPages);
		return BusErrorException;
    }
    entry->use = TRUE;		// set the use, dirty bits
    if (writing)
		entry->dirty = TRUE;
    *physAddr = pageFrame * PageSize + offset;
    ASSERT((*physAddr >= 0) && ((*physAddr + size) <= MemorySize));
    DEBUG('a', "phys addr = 0x%x\n", *physAddr);
    return NoException;
}


void Machine::TLBLoad(int virtAddr)
{
	int vpn = (unsigned) virtAddr / PageSize;
	if (vpn >= pageTableSize) {
	    DEBUG('a', "virtual page # %d too large for page table size %d! in tlb\n", 
			virtAddr, pageTableSize);
	    return;
	} else if (!pageTable[vpn].valid) {
	    DEBUG('a', "virtual page # %d is invalid! in tlb\n", virtAddr);
	    return;
	}
	int tlbindex = FindTLBindex();
	tlb[tlbindex].virtualPage = pageTable[vpn].virtualPage;
	tlb[tlbindex].physicalPage = pageTable[vpn].physicalPage;
	tlb[tlbindex].valid = pageTable[vpn].valid;
	tlb[tlbindex].readOnly = pageTable[vpn].readOnly;
	tlb[tlbindex].use = pageTable[vpn].use; 
	tlb[tlbindex].dirty = pageTable[vpn].dirty;
}

int Machine::FindTLBindex()
{
	int index = -1;
	for(int i = 0; i < TLBSize; i++)
	{
		if(!tlb[i].valid)
		{
			index = i;
		}
	}
#ifdef TLB_FIFO
	if(index == -1)
	{
		DEBUG('a', "replace a tlb entry by FIFO");
		index = (int)currentThread->space->TLBFIFO_List->Remove();
	}
	currentThread->space->TLBFIFO_List->Append((void*)index);
#endif
#ifdef TLB_LRU
	int lastedtime = tlb[0].lastUseTime;
	if(index == -1)
	{
		DEBUG('a', "replace a tlb entry by LRU");
		index = 0;
		for(int i = 1; i < TLBSize; i++)
		{
			if(tlb[i].use && tlb[i].lastUseTime < lastedtime)
			{
				lastedtime = tlb[i].lastUseTime;
				index = i;
			}
		}
	}
	tlb[index].use = true;
	tlb[index].lastUseTime = stats->totalTicks;
#endif
	return index;
}


void Machine::PageSwap()
{
	if(currentThread->space->PagesinMem < currentThread->space->maxPagesinMem)
		return;
	int index = -1;
	int lastedtime = -1;
	for(int i = 0; i < pageTableSize; i++)
	{
		if(pageTable[i].valid)
		{
			if(lastedtime < 0)
			{
				lastedtime = pageTable[i].lastUseTime;
				index = i;
				continue;
			}
			//printf("%d : %d, %d\n", i, pageTable[i].lastUseTime, lastedtime);
			if(pageTable[i].lastUseTime < lastedtime)
			{
				index = i;
				lastedtime = pageTable[i].lastUseTime;
			}
		}
	}
	//printf("%d\n", index);
	DEBUG("a", "swap out page : %d\n",index);
	if(pageTable[index].dirty)
	{
		currentThread->space->swap->WriteAt(
			&(machine->mainMemory[pageTable[index].physicalPage*PageSize])
			, PageSize, pageTable[index].virtualPage * PageSize);
	}
	pageTable[index].dirty = FALSE;
	pageTable[index].valid = FALSE;
	pageMap->Clear(pageTable[index].physicalPage);

	for(int i = 0; i < TLBSize; i++)
	{
		if(tlb[i].valid && tlb[i].virtualPage == pageTable[index].virtualPage)
		{
			tlb[i].valid = FALSE;
			break;
		}
	}
	currentThread->space->PagesinMem--;
}

void Machine::PageLoad(int virtAddr)
{
	int vpn = virtAddr / PageSize;
	PageSwap();
	pageTable[vpn].physicalPage = pageMap->Find();

	currentThread->space->swap->ReadAt(
		&(machine->mainMemory[pageTable[vpn].physicalPage * PageSize]),
		PageSize, vpn*PageSize);

	currentThread->space->PagesinMem++;

	pageTable[vpn].valid = TRUE;
	pageTable[vpn].virtualPage = vpn;
	pageTable[vpn].use = TRUE;
	pageTable[vpn].dirty = FALSE;
	pageTable[vpn].readOnly = FALSE;

	pageTable[vpn].lastUseTime = stats->totalTicks;
}