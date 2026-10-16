#include <stdio.h>

int main(int argc, char **argv)
{
    int x = 0;

    #pragma omp cluster
    {
        #pragma omp cluster data broad(x)

        #pragma omp task_async depend(out:x)
        {
            x = 42;
        }

        #pragma omp taskwait
    }

    printf("x = %d\n", x);
    return 0;
}
