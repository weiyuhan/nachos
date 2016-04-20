#include "copyright.h"
#include "synchconsole.h"

static void
SynchConsoleReadAvail (int arg)
{
    SynchConsole* console = (SynchConsole *)arg;

    console->ReadAvail();
}

static void
SynchConsoleWriteDone (int arg)
{
    SynchConsole* console = (SynchConsole *)arg;

    console->WriteDone();
}

SynchConsole::SynchConsole(char *readFile, char *writeFile)
{
	read = new Semaphore("synchconsole read", 0);
	write = new Semaphore("synchconsole write", 0);
    lock = new Lock("synch console lock");
    console = new Console(NULL, NULL, SynchConsoleReadAvail, 
    	SynchConsoleWriteDone, (int) this);
}

SynchConsole::~SynchConsole()
{
    delete console;
    delete lock;
    delete write;
    delete read;
}

void 
SynchConsole::PutChar(char ch)
{
    lock->Acquire();
    console->PutChar(ch);
    write->P();
    //printf("putchar: %c\n", ch);	
    lock->Release();
}

char 
SynchConsole::GetChar()
{
	char ch = EOF;
	lock->Acquire();
	read->P();	
    ch = console->GetChar();
    //printf("getchar: %c\n", ch);
    lock->Release();
    return ch;
}
    
void 
SynchConsole::ReadAvail()
{
	read->V();
}

void 
SynchConsole::WriteDone()
{
	write->V();
}

