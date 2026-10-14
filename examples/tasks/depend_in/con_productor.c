#include <stdio.h>

int main(void)
{
    int x = 10;

    #pragma omp cluster
    {
        #pragma omp task_async depend(out:x)
        { x = 30; }

        #pragma omp task_async depend(in:x)
        { printf("x = %d\n", x); }

        #pragma omp taskwait
    }

    return 0;
}
