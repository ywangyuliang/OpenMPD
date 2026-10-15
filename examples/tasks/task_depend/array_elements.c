#include <stdio.h>

int main(void)
{
    int i;
    int a[4] = {10, 11, 12, 13};
    int b[4];

    #pragma omp cluster
    {
        for (i = 0; i < 4; i++) {
            #pragma omp task_async depend(in:a[i:1])
            { printf("initial a[%d] = %d\n", i, a[i]); }
            a[i] = 20 + i;
            #pragma omp task_async depend(in:a[i:1])
            { printf("changed a[%d] = %d\n", i, a[i]); }
            #pragma omp task_async depend(out:a[i:1],b[i:1])
            { a[i] = 30 + i; b[i] = 40 + i; }
            #pragma omp task_async depend(in:a[i:1],b[i:1])
            { printf("produced a[%d] = %d, b[%d] = %d\n", i, a[i], i, b[i]); }
        }
        #pragma omp taskwait
    }
    return 0;
}
