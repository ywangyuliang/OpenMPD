#include <stdio.h>
#include <mpi.h>

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

    printf("before barrier: x = %d\n", x);
    fflush(stdout);
    MPI_Barrier(MPI_COMM_WORLD);
    printf("after barrier\n");

    return 0;
}
