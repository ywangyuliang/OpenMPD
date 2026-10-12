#include <stdio.h>

int main(void)
{
    long y = 10000000000L;
    double z = 2.5;

    #pragma omp cluster
    {
        #pragma omp task_async depend(in:y,z)
        { printf("initial y = %ld, z = %.1f\n", y, z); }
        #pragma omp task_async depend(out:y,z)
        { y = 10000000030L; z = 3.5; }
        #pragma omp task_async depend(in:y,z)
        { printf("produced y = %ld, z = %.1f\n", y, z); }
        #pragma omp taskwait
    }
    return 0;
}
