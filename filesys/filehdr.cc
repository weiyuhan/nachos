// filehdr.cc 
//	Routines for managing the disk file header (in UNIX, this
//	would be called the i-node).
//
//	The file header is used to locate where on disk the 
//	file's data is stored.  We implement this as a fixed size
//	table of pointers -- each entry in the table points to the 
//	disk sector containing that portion of the file data
//	(in other words, there are no indirect or doubly indirect 
//	blocks). The table size is chosen so that the file header
//	will be just big enough to fit in one disk sector, 
//
//      Unlike in a real system, we do not keep track of file permissions, 
//	ownership, last modification date, etc., in the file header. 
//
//	A file header can be initialized in two ways:
//	   for a new file, by modifying the in-memory data structure
//	     to point to the newly allocated data blocks
//	   for a file already on disk, by reading the file header from disk
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#include "system.h"
#include "filehdr.h"

//----------------------------------------------------------------------
// FileHeader::Allocate
// 	Initialize a fresh file header for a newly created file.
//	Allocate data blocks for the file out of the map of free disk blocks.
//	Return FALSE if there are not enough free blocks to accomodate
//	the new file.
//
//	"freeMap" is the bit map of free disk sectors
//	"fileSize" is the bit map of free disk sectors
//----------------------------------------------------------------------

bool
FileHeader::Allocate(BitMap *freeMap, int fileSize)
{ 
    //printf("fileSize %d\n", fileSize);
    numBytes = fileSize;
    numSectors  = divRoundUp(fileSize, SectorSize);
    //printf("numSectors: %d\n", numSectors);
    if(numSectors <= NumDirect - 2)
    {
        if (freeMap->NumClear() < numSectors)
    	   return FALSE;		// not enough space

        for (int i = 0; i < numSectors; i++)
    	   dataSectors[i] = freeMap->Find();
    }
    else if(numSectors <= NumDirect - 2 + SectorsInSector)
    {
        if(freeMap->NumClear() < numSectors + 1)
            return FALSE;
        int left = numSectors - (NumDirect-2);
        for (int i = 0; i < (NumDirect-1); i++) // direct
            dataSectors[i] = freeMap->Find();

        int sectorTable[SectorsInSector] = {};
        for(int i = 0; i < left; i++) // indirect
            sectorTable[i] = freeMap->Find();
        synchDisk->WriteSector(dataSectors[NumDirect-2],
            (char*)sectorTable);
    }
    else if(numSectors <= NumDirect - 2 + SectorsInSector
        + SectorsInSector * SectorsInSector)
    {
        int left = numSectors - (NumDirect-2) - SectorsInSector;
        int indirectSectors = divRoundUp(left, SectorsInSector);
        if(freeMap->NumClear() < numSectors + 1 + indirectSectors)
            return FALSE;
        for (int i = 0; i < NumDirect; i++) // direct
            dataSectors[i] = freeMap->Find();

        int sectorTable[SectorsInSector] = {};
        for(int i = 0; i < SectorsInSector; i++) // indirect
        {
            sectorTable[i] = freeMap->Find();
        }
        synchDisk->WriteSector(dataSectors[NumDirect-2],
            (char*)sectorTable);

        int InsectorTable[SectorsInSector] = {};
        for(int i = 0; i < indirectSectors; i++)
        {
            InsectorTable[i] = freeMap->Find();
        }
        synchDisk->WriteSector(dataSectors[NumDirect-1],
            (char*)InsectorTable);
        for(int i = 0; i < indirectSectors; i++)
        {
            int fillup = left <= SectorsInSector? left:SectorsInSector;
            left -= SectorsInSector;

            int InInsectorTable[SectorsInSector] = {};
            for(int j = 0; j < fillup; j++)
                InInsectorTable[j] = freeMap->Find();
            synchDisk->WriteSector(InsectorTable[i], 
                (char*)InInsectorTable);
        }
    }
    return TRUE;
}

//----------------------------------------------------------------------
// FileHeader::Deallocate
// 	De-allocate all the space allocated for data blocks for this file.
//
//	"freeMap" is the bit map of free disk sectors
//----------------------------------------------------------------------

void 
FileHeader::Deallocate(BitMap *freeMap)
{
    if(numSectors <= NumDirect - 2)
    {
        for (int i = 0; i < numSectors; i++) 
        {
            ASSERT(freeMap->Test((int) dataSectors[i]));  // ought to be marked!
            freeMap->Clear((int) dataSectors[i]);
        }
    }
    else if(numSectors <= NumDirect - 2 + SectorsInSector)
    {
        int left = numSectors - (NumDirect-2);

        int sectorTable[SectorsInSector] = {};
        synchDisk->ReadSector(dataSectors[NumDirect-2],
            (char*)sectorTable);
        for(int i = 0; i < left; i++) // indirect
        {
            ASSERT(freeMap->Test((int) sectorTable[i]));  // ought to be marked!
            freeMap->Clear((int) sectorTable[i]);
        }

        for (int i = 0; i < (NumDirect-1); i++) // direct
        {
            ASSERT(freeMap->Test((int) dataSectors[i]));  // ought to be marked!
            freeMap->Clear((int) dataSectors[i]);
        }
    }
    else
    {
        int left = numSectors - (NumDirect-2) - SectorsInSector;
        int indirectSectors = divRoundUp(left, SectorsInSector); 

        int sectorTable[SectorsInSector] = {};
        synchDisk->ReadSector(dataSectors[NumDirect-2],
            (char*)sectorTable);
        for(int i = 0; i < SectorsInSector; i++) // indirect
        {
            ASSERT(freeMap->Test((int) sectorTable[i]));  // ought to be marked!
            freeMap->Clear((int) sectorTable[i]);
        }

        int InsectorTable[SectorsInSector] = {};
        synchDisk->ReadSector(dataSectors[NumDirect-1],
            (char*)InsectorTable);
        for(int i = 0; i < indirectSectors; i++)
        {
            int fillup = left <= SectorsInSector? left:SectorsInSector;
            left -= SectorsInSector;

            int InInsectorTable[SectorsInSector] = {};
            synchDisk->ReadSector(InsectorTable[i], 
                (char*)InInsectorTable);
            for(int j = 0; j < fillup; j++)
            {
                ASSERT(freeMap->Test((int) InInsectorTable[i]));  // ought to be marked!
                freeMap->Clear((int) InInsectorTable[i]);
            }

            ASSERT(freeMap->Test((int) InsectorTable[i]));  // ought to be marked!
            freeMap->Clear((int) InsectorTable[i]);
        }



        for (int i = 0; i < NumDirect; i++) // direct
        {
            ASSERT(freeMap->Test((int) dataSectors[i]));  // ought to be marked!
            freeMap->Clear((int) dataSectors[i]);
        }
    }
}

//----------------------------------------------------------------------
// FileHeader::FetchFrom
// 	Fetch contents of file header from disk. 
//
//	"sector" is the disk sector containing the file header
//----------------------------------------------------------------------

void
FileHeader::FetchFrom(int sector)
{   
    this->sector = sector;
    synchDisk->ReadSector(sector, (char *)this);
}

//----------------------------------------------------------------------
// FileHeader::WriteBack
// 	Write the modified contents of the file header back to disk. 
//
//	"sector" is the disk sector to contain the file header
//----------------------------------------------------------------------

void
FileHeader::WriteBack(int sector)
{
    this->sector = sector;
    synchDisk->WriteSector(sector, (char *)this); 
}

//----------------------------------------------------------------------
// FileHeader::ByteToSector
// 	Return which disk sector is storing a particular byte within the file.
//      This is essentially a translation from a virtual address (the
//	offset in the file) to a physical address (the sector where the
//	data at the offset is stored).
//
//	"offset" is the location within the file of the byte in question
//----------------------------------------------------------------------

int
FileHeader::ByteToSector(int offset)
{
    return LogicalSectorToSector(offset/SectorSize);
}

int 
FileHeader::LogicalSectorToSector(int logi)
{
    if(logi >= numSectors)
        return -1;
    if(logi < NumDirect - 2)
    {
        return dataSectors[logi];
    }
    else if(logi < NumDirect - 2 + SectorsInSector)
    {
        int left = logi - (NumDirect-2);

        int sectorTable[SectorsInSector] = {};
        synchDisk->ReadSector(dataSectors[NumDirect-2],
            (char*)sectorTable);

        return sectorTable[left];
    }
    else
    {
        int left = logi - (NumDirect-2) - SectorsInSector;
        int indirectSectors = left/SectorsInSector;
        int offset = left % SectorsInSector;

        int InsectorTable[SectorsInSector] = {};
        synchDisk->ReadSector(dataSectors[NumDirect-1],
            (char*)InsectorTable);

        int InInsectorTable[SectorsInSector] = {};
        synchDisk->ReadSector(InsectorTable[indirectSectors],
            (char*)InInsectorTable);
        return InInsectorTable[offset];
    }
    return -1;
}
//----------------------------------------------------------------------
// FileHeader::FileLength
// 	Return the number of bytes in the file.
//----------------------------------------------------------------------

int
FileHeader::FileLength()
{
    return numBytes;
}

//----------------------------------------------------------------------
// FileHeader::Print
// 	Print the contents of the file header, and the contents of all
//	the data blocks pointed to by the file header.
//----------------------------------------------------------------------

void
FileHeader::Print()
{
    int i, j, k;
    char *data = new char[SectorSize];

    printf("FileHeader contents.  File size: %d.  ", numBytes);
    printf("CreateTime: %s.  ", getCreateTime());
    printf("LastAccess: %s.  ", getLastAccessTime());
    printf("LastModify: %s.  ", getLastModifyTime());
    printf("File blocks:\n");
    for (i = 0; i < numSectors; i++)
	   printf("%d ", LogicalSectorToSector(i));
    printf("\nFile contents:\n");
    for (i = k = 0; i < numSectors; i++) 
    {
	    synchDisk->ReadSector(LogicalSectorToSector(i), data);
        for (j = 0; (j < SectorSize) && (k < numBytes); j++, k++) 
        {
    	    if ('\040' <= data[j] && data[j] <= '\176')   // isprint(data[j])
    		printf("%c", data[j]);
                else
    		printf("\\%x", (unsigned char)data[j]);
    	}
        printf("\n"); 
    }
    delete [] data;
}

void FileHeader::setCreateTime()
{
    time_t rawtime;
    time(&rawtime);
    createTime = rawtime;
    lastAccessTime = rawtime;
    lastModifyTime = rawtime;
}
void FileHeader::setLastAccessTime()
{
    time_t rawtime;
    time(&rawtime);
    lastAccessTime = rawtime;
    WriteBack(sector);
}
void FileHeader::setLastModifyTime()
{
    time_t rawtime;
    time(&rawtime);
    lastAccessTime = rawtime;
    lastModifyTime = rawtime;
    WriteBack(sector);
}
char* FileHeader::getCreateTime()
{
    struct tm* timeinfo;
    timeinfo = localtime (&createTime);
    return asctime(timeinfo);
}
char* FileHeader::getLastAccessTime()
{
    struct tm* timeinfo;
    timeinfo = localtime (&lastAccessTime);
    return asctime(timeinfo);
}
char* FileHeader::getLastModifyTime()
{
    struct tm* timeinfo;
    timeinfo = localtime (&lastModifyTime);
    return asctime(timeinfo);
}