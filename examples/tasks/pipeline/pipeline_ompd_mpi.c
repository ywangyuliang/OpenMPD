
#include <stdio.h>
#include <stdlib.h>
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

/* Init */
int __taskid = -1, __numprocs = -1;
MPI_Status __status;

MPI_Init(&argc,&argv);
MPI_Comm_size(MPI_COMM_WORLD,&__numprocs);
MPI_Comm_rank(MPI_COMM_WORLD,&__taskid);

if (__taskid == 0) {
}
{
if (__numprocs != 5) {
	printf("Error, num_teams debe ser 5\n");
	MPI_Finalize();
        return(-1);
}

for (i=0; i<N; i++) {
if (__taskid == 0) {
/* Receive from each process this task depends on (in:xxx) */
    {
	a[i] = 13+ i;
    }
/* Send to each process that depends on a[] (in:a[i:1]); here, process 1 */
    MPI_Send (&a[i], 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
}
if (__taskid == 1) {
/* Receive from each process this task depends on (in:a[i:1]); here, process 0 */
    MPI_Recv (&a[i], 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &__status);
    {
	b[i] = a[i] + 1;
    }
/* Send to each process that depends on b[] (in:b[i:1]); here, process 2 */
    MPI_Send (&b[i], 1, MPI_INT, 2, 0, MPI_COMM_WORLD);
}
if (__taskid == 2) {
    MPI_Recv (&b[i], 1, MPI_INT, 1, 0, MPI_COMM_WORLD, &__status);
    {
	c[i] = b[i] + 2;
    }
    MPI_Send (&c[i], 1, MPI_INT, 3, 0, MPI_COMM_WORLD);
}
if (__taskid == 3) {
    MPI_Recv (&c[i], 1, MPI_INT, 2, 0, MPI_COMM_WORLD, &__status);
    {
	d[i] = c[i] + 3;
    }
    MPI_Send (&d[i], 1, MPI_INT, 4, 0, MPI_COMM_WORLD);
}
if (__taskid == 4) {
    MPI_Recv (&d[i], 1, MPI_INT, 3, 0, MPI_COMM_WORLD, &__status);
    {
	e[i] = d[i] + 4;
	printf("e[%d]: %d\n", i, e[i]);
    }
}
}
} /* Cluster */

if (__taskid == 0) {
}

MPI_Finalize();
return(0);
} /* main */
