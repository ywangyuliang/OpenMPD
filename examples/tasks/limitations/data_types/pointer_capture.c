#include <stdio.h>

int read_value(int *pointer)
{
    return *pointer;
}

int main(void)
{
    int x = 7;
    int *pointer = &x;
    int result = 0;

    #pragma omp cluster
    {
        #pragma omp task_async depend(out:result)
        { result = read_value(pointer); }
        #pragma omp taskwait
    }

    printf("result = %d (expected 7)\n", result);
    return 0;
}
