#include <stdio.h>

int main(void)
{
    int i;
    int x = 10;

    #pragma omp cluster
    {
        for (i = 0; i < 8; i++) {
            x = 10 + i;
            #pragma omp task_async depend(in:x)
            { printf("initial %d = %d\n", i, x); }
        }
        #pragma omp task_async depend(out:x)
        { x = 30; }
        for (i = 0; i < 8; i++) {
            x = 100 + i;
            #pragma omp task_async depend(in:x)
            { printf("produced %d = %d\n", i, x); }
        }
        #pragma omp taskwait
    }
    return 0;
}
