#include "syscall.h"

int
main()
{
    int a, b, ret, index;
    a = GetMsg();
    b = GetMsg();
    ret = 1;
    index = 0;
    while(index < b)
    {
        ret = ret * a;
        index++;
    }
    PutMsg(ret, 0);
    Exit(1024);
}