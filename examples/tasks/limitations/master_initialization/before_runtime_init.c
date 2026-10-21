#include <stdio.h>

int main(int argc, char **argv)
{
    int x = 0;

    printf("Before runtime initialization\n");
    fflush(stdout);

    #pragma omp cluster
    {
        #pragma omp task_async depend(out:x)
        {
            x = 42;
        }

        #pragma omp taskwait
    }

    printf("After taskwait: x = %d\n", x);
    return 0;
}
