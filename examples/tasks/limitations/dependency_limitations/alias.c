#include <stdio.h>

int main(void)
{
    int x = 0;
    int *p = &x;
    int y = -1;
    #pragma omp cluster
    {
        #pragma omp task_async depend(out:x)
        { x = 10; }
        #pragma omp task_async depend(in:p[0:1]) depend(out:y)
        { y = p[0]; }
        #pragma omp taskwait
    }
    printf("y = %d (expected 10)\n", y);
    return 0;
}
