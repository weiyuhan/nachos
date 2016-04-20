#include "copyright.h"
#include "synchconsole.h"
#include "filesys.h"
#include "system.h"

#define pipeFileName "pipe"

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

SynchConsole::SynchConsole(char *readFile, char *writeFile, bool usePipe = FALSE)
{
	read = new Semaphore("synchconsole read", 0);
	write = new Semaphore("synchconsole write", 0);
    lock = new Lock("synch console lock");
    console = new Console(readFile, writeFile, SynchConsoleReadAvail, 
    	SynchConsoleWriteDone, (int) this);
    pipe = usePipe;
    if(usePipe)
    {
        if (!fileSystem->Create(pipeFileName, 0)) 
        {
            printf("Perf test: can't create pipe\n");
            pipe = FALSE;
            return;
        }
        pipeFileread = fileSystem->Open(pipeFileName);
        if (pipeFileread == NULL) 
        {
            printf("Perf test: unable to open pipe\n");
            pipe = FALSE;
            return;
        }
        pipeFilewrite = fileSystem->Open(pipeFileName);
        if (pipeFilewrite == NULL) 
        {
            printf("Perf test: unable to open pipe\n");
            pipe = FALSE;
            return;
        }
        pipeAvail = new Condition("pipeAvail");
    }
}

SynchConsole::~SynchConsole()
{
    if(pipe)
    {
        delete pipeFileread;
        delete pipeFilewrite;
        fileSystem->Remove(pipeFileName);
    }
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
SynchConsole::PutCharPipe(char ch)
{
    if(!pipe)
        return;
    lock->Acquire();
    while(pipeFileread->Read(&ch,1) == 0)
    {
        pipeAvail->Wait(lock);
    }
    printf("Thread %d: %s put ch: %c\n", currentThread->gettid(),
            currentThread->getName(), ch);
    console->PutChar(ch);
    write->P();
    //printf("putchar: %c\n", ch);  
    lock->Release();
}

char 
SynchConsole::GetCharPipe()
{
    char ch = EOF;
    if(!pipe)
        return ch;
    lock->Acquire();
    read->P();  
    ch = console->GetChar();
    printf("Thread %d: %s get ch: %c\n", currentThread->gettid(),
            currentThread->getName(), ch);
    if(pipe)
    {
        pipeFilewrite->Write(&ch, 1);
        pipeAvail->Signal(lock);
    }

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

