#include <stdio.h>
#include <mpi.h>

/* Ejecutar con 2 procesos MPI */

int main(int argc, char **argv)
{
    printf("El bucle original solo visita: 0, 2, 4, 6, 8\n");

    #pragma omp cluster
    {
        #pragma omp distribute
        for (int i = 0; i < 10; i += 2) {
            printf("Proceso %d: i = %d\n", __taskid, i);
        }
    }
    return 0;
}
