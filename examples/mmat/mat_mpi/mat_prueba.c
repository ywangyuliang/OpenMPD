#include "matriz_float.h"
#include <stdio.h>
#include <mpi.h>

int main(int argc, char *argv[])
{
    int i,j,k;
    int result;
    double start,finish;
    int my_rank;
    int n_processes;

    MPI_Init(&argc, &argv);

    MPI_Comm_size(MPI_COMM_WORLD, &n_processes);

    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    start=MPI_Wtime();

    for (i=0; i<M1; i++) {
        for (j=0; j<N2; j++) {
            result = 0;
            for (k=0; k<N1; k++) {
                result += mat1[i][k] * mat2[k][j];
            matR[i][j] = result;
            }
/*         printf("%d ", matR[i][j]);   */
        }
/*      printf("\n");   */
    }

    finish=MPI_Wtime();

    MPI_Finalize();

    printf("Total time was %f seconds\n", finish-start);

    return(0);

}

