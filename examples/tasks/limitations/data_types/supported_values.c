#include <stdio.h>

int main(void)
{
    int i = 2;
    long l = 3;
    double d = 4.0;
    int a[1] = {11};
    int result = 0;

    #pragma omp cluster
    {
        #pragma omp task_async depend(in:i,l,d,a[0:1]) depend(out:result)
        { result = i + (int)l + (int)d + a[0]; }
        #pragma omp taskwait
    }

    printf("result = %d (expected 20)\n", result);
    return 0;
}
