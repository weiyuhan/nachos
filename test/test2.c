#include "syscall.h"

int num;

void run()
{
    Print(num, 'd');
    num = 100;
    Print("it's Me!", 's');
    Print(num, 'd');
    Exit(100);
}

int
main()
{
    /*
    SpaceId sd;
    int exitNum;
    Print("Exec sort", 's');
    sd = Exec("sort");
    Yield();
    Print("Wait For sort", 's');
    exitNum = Join(sd);
    Print(exitNum, 'd');
    */
    Print("befork fork", 's');
    num = 10;
    Print(num, 'd');
    Fork(run);
    Print("after fork", 's');
    Print(num, 'd');
    Exit(0);
}