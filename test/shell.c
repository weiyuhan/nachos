#include "syscall.h"


int
main()
{
    SpaceId newProc;
    OpenFileId input = ConsoleInput;
    OpenFileId output = ConsoleOutput;
    char prompt[2], ch, buffer[60], cmd[20], arg[40];
    int bufferSize, bufferIndex, cmdIndex, argIndex;


    prompt[0] = '-';
    prompt[1] = '-';

    while( 1 )
    {
    	Write(prompt, 2, output);

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
			if(argIndex > 0 && StrCmp("exec", cmd, 4) == 0)
			{
				newProc = Exec(arg);
				Join(newProc);
			}
			if(StrCmp("ls", cmd, 2) == 0)
			{
				LS();
			}
			if(StrCmp("quit", cmd, 4) == 0)
			{
				Halt();
			}
			if(StrCmp("path", cmd, 4) == 0)
			{
				Path();
			}
		}
    }
}

