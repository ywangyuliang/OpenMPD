#include "matriz.h"
#include <mpi.h>
#include <stdio.h>
#include <unistd.h>

#define BACK_SIZE N2+1
#define GO_SIZE 1

#define WORKING		100
#define END		10

int main (int argc, char* argv[])
{
    int v;
    int n_processes;
    int my_rank;
    int root=0;
    int tag;
    int buf[M1*N2];
    MPI_Status status;
    MPI_Request request;
    int recv=0;
    int send=0;
    double start,finish;
    int begin, end;
    int size;
    int n_rows;
    int row=0, col=0;
    int result;
    int i, j, k;

    MPI_Init(&argc, &argv); 

    start=MPI_Wtime();

    MPI_Comm_size(MPI_COMM_WORLD, &n_processes);

    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);


/* printf("mi numero de cpu's es %d (nodo %d)\n", sysconf(_SC_NPROCESSORS_CONF), my_rank); */

    if (M1%n_processes == 0)
	n_rows = M1/n_processes;
    else n_rows = M1/n_processes + 1;

    begin = n_rows * my_rank;
    end = begin + n_rows;
    if (end > M1)
	end = M1;

/* printf("Worker %d de %d a %d\n", my_rank, begin, end);*/

    for (i=begin; i<end; i++) {
        for (j=0; j<N2; j++) {
            result = 0;
            for (k=0; k<N1; k++) {
                result += mat1[i][k] * mat2[k][j];
            matR[i][j] = result;
            }
        }
    }

    if (my_rank == root) {
        while (recv < (M1 - (end - begin))) {
            MPI_Recv(buf, BACK_SIZE, MPI_INT, MPI_ANY_SOURCE, WORKING, MPI_COMM_WORLD, &status);
            recv++;

/* printf("Guardando elemento %d, %d = %d\n", buf[0], buf[1], buf[2]);  */
            for (j=0; j<N2; j++)
                matR[buf[N2]][j] = buf[j];
        }

/*printf("Maestro acabo \n");  */
    }
    else {
	for (i=begin; i<end; i++) {
	    for (j=0; j<N2; j++)
		buf[j] = matR[i][j];
            buf[N2] = i;
/* printf("Calculado elemento %d, %d = %d en %d\n", row, col, result, my_rank); */


            MPI_Send(buf, BACK_SIZE, MPI_INT, root, WORKING, MPI_COMM_WORLD);
	}
/*printf("Worker %d acabo\n", my_rank);  */
    }

    finish=MPI_Wtime();

    MPI_Finalize();

/* printf("Worker %d de %d a %d\n", my_rank, begin, end); */
    if (my_rank == 0)
    {
        printf("Total time was %f seconds\n", finish-start);
        for (i=0; i<M1; i++) {
            for (j=0; j<N2; j++) {
/*                printf("%d ", matR[i][j]); */
            }
/*            printf("\n"); */
        } 
    } 
 
    return(0);
}
