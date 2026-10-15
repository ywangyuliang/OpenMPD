#include <stdio.h>

int main(void)
{
    int x = 10;
    int y;

    #pragma omp cluster
    {
        #pragma omp task_async depend(out:x,y)
        { x = 30; y = 40; }
        #pragma omp task_async depend(in:x,y)
        { printf("x = %d, y = %d\n", x, y); }
        #pragma omp taskwait
        #pragma omp task_async depend(in:x,y)
        { printf("completed x = %d, y = %d\n", x, y); }
        #pragma omp taskwait
    }
    return 0;
}
