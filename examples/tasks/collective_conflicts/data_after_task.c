#include <stdio.h>

int main(int argc, char **argv)
{
    int x = 0;

    #pragma omp cluster
    {
        #pragma omp task_async depend(out:x)
        {
            x = 42;
        }

        #pragma omp taskwait
        #pragma omp cluster data broad(x)
    }

    printf("x = %d\n", x);
    return 0;
}
