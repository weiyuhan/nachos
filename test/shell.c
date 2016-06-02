#include "syscall.h"

int strcmp(char* s1, char* s2, int len)
{
	int index = 0;
	while(index < len)
	{
		if(s1[index] == '\0' || s2[index] == '\0')
			return 1; 
		if(s1[index] != s2[index])
			return 1;
		index++;
	}
	return 0;
}

int parseInt(char* s,int len)
{
	int ret, index;
	ret = 0;
	index = 0;
	while(index < len)
	{
		ret = ret + (s[index] - '0');
		ret = ret * 10;
		index++;
	}
	ret = ret / 10;
	return ret;
}


int
main()
{
    SpaceId newProc;
    OpenFileId input = ConsoleInput;
    OpenFileId output = ConsoleOutput;
    char prompt[2], ch, buffer[60], cmd[20], arg[40];
    char* quit, ls, exec, path, echo, mkdir, rm, create, open, cd, close, read, write; 
    int bufferSize, bufferIndex, cmdIndex, argIndex;
    OpenFileId fd;


    prompt[0] = '-';
    prompt[1] = '-';

    while( 1 )
    {
    	Print("nachos://  ", 's');

		bufferSize = 0;
		
		do 
		{
		
		    Read(&buffer[bufferSize], 1, input);

		} while( buffer[bufferSize++] != '\n' );

		buffer[--bufferSize] = '\0';


		bufferIndex = 0;
		cmdIndex = 0;
		while(bufferIndex < bufferSize && buffer[bufferIndex] != '\0' 
			&& buffer[bufferIndex] != ' ')
		{
			cmd[cmdIndex++] = buffer[bufferIndex++];
		}
		cmd[cmdIndex] = '\0';

		while(bufferIndex < bufferSize && buffer[bufferIndex] == ' ')
		{
			bufferIndex++;
		}

		argIndex = 0;
		while(bufferIndex < bufferSize && buffer[bufferIndex] != '\0' 
			&& buffer[bufferIndex] != ' ')
		{
			arg[argIndex++] = buffer[bufferIndex++];
		}
		arg[argIndex] = '\0';

		if(cmdIndex > 0) 
		{
			if(argIndex > 0 && strcmp("exec", cmd, 4) == 0)
			{
				newProc = Exec(arg);
				if(newProc < 0)
				{
					Print("unable to run ", 's');
					Println(arg, 's');
					continue;
				}
				Print("SpaceId : ", 's');
				Println(newProc, 'd');
				Join(newProc);
			}
			if(strcmp("join", cmd, 4) == 0)
			{
				if(newProc >= 0)
					Join(newProc);
			}
			if(strcmp("ls", cmd, 2) == 0)
			{
				LS();
			}
			if(strcmp("quit", cmd, 4) == 0)
			{
				Exit(0);
			}
			if(strcmp("halt", cmd, 4) == 0)
			{
				Halt();
			}
			if(strcmp("path", cmd, 4) == 0)
			{
				Path();
			}
			if(strcmp("help", cmd, 4) == 0)
			{
				Println("ls,echo,mkdir,cd,rm is the same as linux", 's');
				Println("create a new file with: create [filename]", 's');
				Println("open a file with: open [filename]", 's');
				Println("close a file with: close [filename]", 's');
				Println("write an opened file with: write [content]", 's');
				Println("read an opened file with: read", 's');
				Println("close an opened file with: close", 's');
				Println("show current path with: path", 's');
				Println("exit system with: exit", 's');
				Println("halt system with: halt", 's');
			}
			if(argIndex > 0 && strcmp("echo", cmd, 4) == 0)
			{
				Println(arg, 's');
			}
			if(argIndex > 0 && strcmp("mkdir", cmd, 5) == 0)
			{
				MKDir(arg);
			}
			if(argIndex > 0 && strcmp("cd", cmd, 2) == 0)
			{
				CDDir(arg);
			}
			if(argIndex > 0 && strcmp("rm", cmd, 2) == 0)
			{
				Remove(arg);
			}
			if(argIndex > 0 && strcmp("create", cmd, 6) == 0)
			{
				Create(arg);
			}
			if(argIndex > 0 && strcmp("open", cmd, 4) == 0)
			{
				fd = Open(arg);
			}
			if(strcmp("close", cmd, 5) == 0)
			{
				Close(fd);
			}
			if(argIndex > 0 && strcmp("write", cmd, 5) == 0)
			{
				Write(arg, argIndex, fd);
			}
			if(strcmp("read", cmd, 4) == 0)
			{
				int readNum = Read(arg, 40, fd);
				arg[readNum] = '\0';
				Println(arg, 's');
			}
			if(strcmp("send", cmd, 4) == 0)
			{
				int num = parseInt(arg, argIndex);
				PutMsg(num, newProc);
			}
			if(strcmp("get", cmd, 3) == 0)
			{
				int num = GetMsg();
				Println(num, 'd');
			}
			if(strcmp("ts", cmd, 2) == 0)
			{
				TS();
			}
		}
    }
}

