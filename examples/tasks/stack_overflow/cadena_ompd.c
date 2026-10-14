#include <stdio.h>

void cadena(int n)
{
    if (n <= 0) return;  // Caso base

    #pragma omp cluster
    {
        #pragma omp task_async
        { cadena(n - 1); }

        #pragma omp taskwait
    }
}

int main(int argc, char **argv)
{
    #pragma omp cluster
    {
        cadena(100000);
    }

    printf("Terminado\n");
    return 0;
}