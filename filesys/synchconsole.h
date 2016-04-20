#include "copyright.h"

#ifndef SYNCHCONSOLE_H
#define SYNCHCONSOLE_H

#include "console.h"
#include "synch.h"

class SynchConsole {
  public:
    SynchConsole(char *readFile, char *writeFile);    		// Initialize a synchronous disk,
					// by initializing the raw Disk.
    ~SynchConsole();			// De-allocate the synch disk data
    
    void PutChar(char ch);

    char GetChar();
    
    void ReadAvail();

    void WriteDone();	

  private:
    Console *console;		  		
    Semaphore *read;
    Semaphore *write; 		
    Lock *lock;		  		
};

#endif // SYNCHCONSOLE_H