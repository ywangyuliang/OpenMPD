#include <stdio.h>
#include <stdlib.h>

void chain(int depth)
{
    if (depth <= 0) {
        return;
    }

    #pragma omp cluster
    {
        #pragma omp task_async
        { chain(depth - 1); }
        #pragma omp taskwait
    }
}

int main(int argc, char **argv)
{
    int depth = 100000;
    if (argc > 1) {
        depth = atoi(argv[1]);
    }

    #pragma omp cluster
    {
        chain(depth);
    }

    printf("Completed a chain of %d tasks\n", depth);
    return 0;
}
