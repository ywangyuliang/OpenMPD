#include <stdio.h>

int main(void)
{
    float x = 7.0f;
    int result = 0;

    #pragma omp cluster
    {
        #pragma omp task_async depend(in:x) depend(out:result)
        { result = (int)x; }
        #pragma omp taskwait
    }

    printf("result = %d (expected 7)\n", result);
    return 0;
}
