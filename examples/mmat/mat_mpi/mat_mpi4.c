#include "matriz.h"
#include <mpi.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#define BACK_SIZE N2+1
#define GO_SIZE 1

#define WORKING		100
#define END		10
#define MAX_THR		10

void * worker(void * args)
{
    int slave_id = *((int*) args);
    int my_rank;
    int buf[4][BACK_SIZE];
    int root=0;
    int tag;
    int row=0, col=0;
    int i, j, k;
    MPI_Status status;
    MPI_Request request;
    int result;

    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    printf("Slave %d despertando de nodo %d\n", slave_id, my_rank);
    MPI_Recv(buf[slave_id], GO_SIZE, MPI_INT, root, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

    while (status.MPI_TAG == WORKING) {
        row = buf[slave_id][0];

        for (j=0; j<N2; j++) {
	    result = 0;
	    for (k=0; k<N1; k++)
	        result += mat1[row][k] * mat2[k][j];
	    buf[slave_id][j] = result;
        }
        buf[slave_id][N2] = row;
        printf("Calculado fila %d en %d, flujo %d\n", row, my_rank, slave_id);


        MPI_Isend(buf[slave_id], BACK_SIZE, MPI_INT, root, WORKING, MPI_COMM_WORLD, &request);
        MPI_Request_free(&request);
        MPI_Recv(buf[slave_id], GO_SIZE, MPI_INT, root, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    }
    /* printf("Worker %d, flujo %d acabo\n", my_rank, slave_id);*/

    pthread_exit(NULL);
}

int main (int argc, char* argv[])
{
    int v;
    int n_processes;
    int my_rank;
    int n_cpu;
    int root=0;
    int tag;
    int buf[BACK_SIZE];
    int buf2[BACK_SIZE];
    MPI_Status status;
    MPI_Request request;
    int result;
    int recv=0;
    int send=0;
    double start,finish;
    int row=0, col=0;
    int i, j, k;
    pthread_t slaves_id[MAX_THR];
    pthread_attr_t attrib;

    MPI_Init(&argc, &argv); 

    start=MPI_Wtime();

    MPI_Comm_size(MPI_COMM_WORLD, &n_processes);

    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    n_cpu = sysconf(_SC_NPROCESSORS_CONF);

printf("mi numero de cpu's es %d (nodo %d)\n", n_cpu, my_rank);

if (n_cpu > 1) {
    pthread_attr_init(&attrib);

    pthread_attr_setscope(&attrib, PTHREAD_SCOPE_SYSTEM);

    for (i=0; i < n_cpu; i++) 
        pthread_create(&slaves_id[i], &attrib, worker, (void *) &i);

    for (i=0; i < n_cpu; i++) 
	pthread_join(slaves_id[i], NULL);
}
else    if (my_rank == root) {
	tag = WORKING;
printf("soy el maestro\n");

	for (i=1; i < n_processes; i++) {
	    if ((i == 1) || (i == 2)) {
		for (j = 0; j < 4; j++) {
	            buf[0] = row;
	            MPI_Isend(buf, GO_SIZE, MPI_INT, i, tag, MPI_COMM_WORLD, 
		        &request);
	            MPI_Request_free(&request);
	            send++;

	            row++;
		}
	    }
	    else {
	        buf[0] = row;
	        MPI_Isend(buf, GO_SIZE, MPI_INT, i, tag, MPI_COMM_WORLD, 
		    &request);
	        MPI_Request_free(&request);
	        send++;

	        row++;
	    }
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

/* printf("Guardando elemento %d, %d = %d\n", buf[0], buf[1], buf[2]);  */
	    for (j=0; j<N2; j++)
            matR[buf[N2]][j] = buf[j];
	}

	for (i=1; i <= 4; i++) {
	    buf[0] = row;
	    MPI_Isend(buf, GO_SIZE, MPI_INT, 4, tag, MPI_COMM_WORLD, &request);
	    MPI_Request_free(&request);
	}

/* printf("Maestro acabo \n");*/
    }
    else {
    printf("Worker despertando en nodo %d\n", my_rank);
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
printf("Calculado fila %d en %d\n", row, my_rank);

	
	    MPI_Isend(buf, BACK_SIZE, MPI_INT, root, WORKING, MPI_COMM_WORLD, &request);
	    MPI_Request_free(&request);
	    MPI_Recv(buf, GO_SIZE, MPI_INT, root, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
	}
/* printf("Worker %d acabo\n", my_rank); */
    }


    finish=MPI_Wtime();

    MPI_Finalize();

    if (my_rank == 0)
    {
        printf("Total time was %f seconds\n", finish-start);
/*        for (i=0; i<M1; i++) {
            for (j=0; j<N2; j++) {
                printf("%d ", matR[i][j]);
            }
            printf("\n");
        } */
    } 
 
    return(0);
}
