
#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <mpi.h>

/*
 * Small pipeline example for checking stage ordering.  With N=10 and
 * print-heavy output, it is useful for validation but not for performance
 * comparisons.
 */

int main(int argc, char **argv)
{
int i, N=10;
int a[N], b[N], c[N], d[N], e[N];
int my_rank, n_processes;
MPI_Status status;

MPI_Init(&argc, &argv);

MPI_Comm_size(MPI_COMM_WORLD, &n_processes);

MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

MPI_Bcast(&N, 1, MPI_INT, 0, MPI_COMM_WORLD);

if (my_rank == 0) {
    for (i=0; i<N; i++) {
	a[i] = 13+i;
        MPI_Send (&a[i], 1, MPI_INT, my_rank+1, 0, MPI_COMM_WORLD);
    }
} else if (my_rank == 1) {
    for (i=0; i<N; i++) {
        MPI_Recv (&a[i], 1, MPI_INT, my_rank-1, 0, MPI_COMM_WORLD, &status);
        b[i] = a[i]+1;
        MPI_Send (&b[i], 1, MPI_INT, my_rank+1, 0, MPI_COMM_WORLD);
    }
} else if (my_rank == 2) {
    for (i=0; i<N; i++) {
        MPI_Recv (&b[i], 1, MPI_INT, my_rank-1, 0, MPI_COMM_WORLD, &status);
        c[i] = b[i]+2;
        MPI_Send (&c[i], 1, MPI_INT, my_rank+1, 0, MPI_COMM_WORLD);
    }
} else if (my_rank == 3) {
    for (i=0; i<N; i++) {
        MPI_Recv (&c[i], 1, MPI_INT, my_rank-1, 0, MPI_COMM_WORLD, &status);
        d[i] = c[i]+3;
        MPI_Send (&d[i], 1, MPI_INT, my_rank+1, 0, MPI_COMM_WORLD);
    }
} else if (my_rank == 4) {
    for (i=0; i<N; i++) {
        MPI_Recv(&d[i], 1, MPI_INT, my_rank-1, 0, MPI_COMM_WORLD, &status);
        e[i] = d[i]+4;
        printf("e[%d]: %d\n", i, e[i]);
    }
}

if (my_rank == 1) {
    MPI_Send(b, N, MPI_INT, 0, 0, MPI_COMM_WORLD);
} else if (my_rank == 2) {
    MPI_Send(c, N, MPI_INT, 0, 0, MPI_COMM_WORLD);
} else if (my_rank == 3) {
    MPI_Send(d, N, MPI_INT, 0, 0, MPI_COMM_WORLD);
} else if (my_rank == 4) {
    MPI_Send(e, N, MPI_INT, 0, 0, MPI_COMM_WORLD);
} else if (my_rank == 0) {
    MPI_Recv(b, N, MPI_INT, 1, 0, MPI_COMM_WORLD, &status);
    MPI_Recv(c, N, MPI_INT, 2, 0, MPI_COMM_WORLD, &status);
    MPI_Recv(d, N, MPI_INT, 3, 0, MPI_COMM_WORLD, &status);
    MPI_Recv(e, N, MPI_INT, 4, 0, MPI_COMM_WORLD, &status);

    for (i = 0; i < N; i++) {
        printf("a[%d]: %d b[%d]: %d c[%d]: %d d[%d]: %d e[%d]: %d\n",
               i, a[i], i, b[i], i, c[i], i, d[i], i, e[i]);
    }
}

MPI_Finalize();
}
