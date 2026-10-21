#include <stdio.h>

int main(void)
{
    int x = 0;
    int y = -1;
    #pragma omp cluster
    {
        #pragma omp task_async depend(out:x)
        { x = 10; }
        #pragma omp task_async depend(in:x) depend(out:y)
        { y = x; }
        #pragma omp task_async depend(out:x)
        { x = 20; }
        #pragma omp taskwait
    }
    printf("y = %d (expected 10)\n", y);
    return 0;
}
