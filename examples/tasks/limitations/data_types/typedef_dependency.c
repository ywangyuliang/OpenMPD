#include <stdio.h>

typedef int Integer;

int main(void)
{
    Integer x = 7;
    int result = 0;

    #pragma omp cluster
    {
        #pragma omp task_async depend(in:x) depend(out:result)
        { result = x; }
        #pragma omp taskwait
    }

    printf("result = %d (expected 7)\n", result);
    return 0;
}
