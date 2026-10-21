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
    }

    printf("before broadcast: x = %d\n", x);
    fflush(stdout);

    #pragma omp cluster broad(x)
    {
        printf("broadcast: x = %d\n", x);
    }

    return 0;
}
