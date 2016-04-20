#include "copyright.h"

#ifndef SYNCHCONSOLE_H
#define SYNCHCONSOLE_H

#include "console.h"
#include "synch.h"
#include "openfile.h"

class SynchConsole {
  public:
    SynchConsole(char *readFile, char *writeFile, bool usePipe = FALSE);    		// Initialize a synchronous disk,
					// by initializing the raw Disk.
    ~SynchConsole();			// De-allocate the synch disk data
    
    void PutChar(char ch);

    char GetChar();

    void PutCharPipe(char ch);

    char GetCharPipe();
    
    void ReadAvail();

    void WriteDone();	

  private:
    bool pipe;
    OpenFile* pipeFileread;
    OpenFile* pipeFilewrite;
    Console *console;		  		
    Semaphore *read;
    Semaphore *write; 		
    Lock *lock;	
    Condition *pipeAvail;	  		
};

#endif // SYNCHCONSOLE_H