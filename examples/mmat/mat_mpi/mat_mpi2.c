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
    int buf[BACK_SIZE];
    int buf2[BACK_SIZE];
    MPI_Status status;
    MPI_Request request;
    int recv=0;
    int send=0;
    double start,finish;
    int row=0, col=0;
    int result;
    int i, j, k;

    MPI_Init(&argc, &argv); 

    start=MPI_Wtime();

    MPI_Comm_size(MPI_COMM_WORLD, &n_processes);

    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);




    if (my_rank == root) {
	tag = WORKING;

	for (i=1; i < n_processes; i++) {
	    buf[0] = row;
	    MPI_Isend(buf, GO_SIZE, MPI_INT, i, tag, MPI_COMM_WORLD, &request);
	    MPI_Request_free(&request);
	    send++;

	    row++;
	}

	while (recv < M1) {
	    MPI_Recv(buf, BACK_SIZE, MPI_INT, MPI_ANY_SOURCE, WORKING, MPI_COMM_WORLD, &status);
	    recv++;

	    buf2[0] = row;
	    MPI_Isend(buf2, GO_SIZE, MPI_INT, status.MPI_SOURCE, tag, MPI_COMM_WORLD, &request);
	    MPI_Request_free(&request);
	    send++;
	    if (send == M1)
		tag = END;
	    row++;

	    for (j=0; j<N2; j++)
            matR[buf[N2]][j] = buf[j];
	}
    }
    else {
	MPI_Recv(buf, GO_SIZE, MPI_INT, root, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

	while (status.MPI_TAG == WORKING) {
	    row = buf[0];

	    for (j=0; j<N2; j++) {
                result = 0;
                for (k=0; k<N1; k++)
                    result += mat1[row][k] * mat2[k][j];
                buf[j] = result;
	    }
	    buf[N2] = row;

	
	    MPI_Isend(buf, BACK_SIZE, MPI_INT, root, WORKING, MPI_COMM_WORLD, &request);
	    MPI_Request_free(&request);
	    MPI_Recv(buf, GO_SIZE, MPI_INT, root, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
	}
    }


    finish=MPI_Wtime();

    MPI_Finalize();

    if (my_rank == 0)
    {
        printf("Total time was %f seconds\n", finish-start);
/*
 * for (i=0; i<M1; i++) {
 *     for (j=0; j<N2; j++) {
 *         printf("%d ", matR[i][j]);
 *     }
 *     printf("\n");
 * }
 */
    } 
 
    return(0);
}
