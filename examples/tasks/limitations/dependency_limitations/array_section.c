#include <stdio.h>

int main(void)
{
    int a[2] = {0, 0};
    #pragma omp cluster
    {
        #pragma omp task_async depend(out:a[0:2])
        { a[0] = 10; a[1] = 20; }
        #pragma omp taskwait
    }
    printf("a = %d, %d (expected 10, 20)\n", a[0], a[1]);
    return 0;
}
