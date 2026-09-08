#include <stdio.h>

int main(int argc, char const *argv[])
{
    int hello = 0;
    struct hello;

    void;
    int;

    /* test statement expression */
    int ss = (      {int a = 5; int sum = a + 5; sum;}    );
    ss = ( /* aaa */     {int a = 5; int sum = a + 5; sum;} /*aaa*/);
    ss = (     
        {int a = 5; int sum = a + 5; sum;}
    );

    printf("hellow world\n");
    return 0;
}
