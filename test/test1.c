#include "syscall.h"

int
main()
{
	OpenFileId fd;
	char buffer[5];
	int readNum;


    Create("testFile");
    fd = Open("testFile");
    Write("fuck", 4, fd);
    Close(fd);

    fd = Open("testFile");
    readNum = Read(buffer, 100, fd);
    Print(readNum, 'd');
    buffer[4] = '\0';
    Print(buffer, 's');
    Close(fd);
    Exit(0);
}