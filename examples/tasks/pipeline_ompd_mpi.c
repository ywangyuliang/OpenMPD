
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>


int main(int argc, char **argv)
{
int i, N=10;
int a[N], b[N], c[N], d[N], e[N];

// Init
int __taskid = -1, __numprocs = -1;
MPI_Status __status;

MPI_Init(&argc,&argv);
MPI_Comm_size(MPI_COMM_WORLD,&__numprocs);
MPI_Comm_rank(MPI_COMM_WORLD,&__taskid);

if (__taskid == 0) {
}	


//#pragma omp cluster teams num_teams(5)
{
if (__numprocs != 5) {
	printf("Error, num_teams debe ser 5\n");
	MPI_Finalize();
        return(-1);
}

for (i=0; i<N; i++) {
//#pragma omp task_async depend(out:a[i:1])  
if (__taskid == 0) {
// Un recv de cada proceso del que dependa (in:xxx)
    {
	a[i] = 13+ i;
    }
// Un send a cada proceso que dependa de a[] (in:a[i:1]), en esta caso el 1
    MPI_Send (&a[i], 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
}
//#pragma omp task_async depend(in:a[i:1]) depend(out:b[i:1])  
if (__taskid == 1) {
// Un recv de cada proceso del que dependa (in:a[i:1]) en este caso el 0
    MPI_Recv (&a[i], 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &__status);
    {
	b[i] = a[i] + 1;
    }
// Un send a cada proceso que dependa de b[] (in:b[i:1]), en esta caso el 2
    MPI_Send (&b[i], 1, MPI_INT, 2, 0, MPI_COMM_WORLD);
}
//#pragma omp task_async depend(in:b[i:1]) depend(out:c[i:1])  
if (__taskid == 2) {
    MPI_Recv (&b[i], 1, MPI_INT, 1, 0, MPI_COMM_WORLD, &__status);
    {
	c[i] = b[i] + 2;
    }
    MPI_Send (&c[i], 1, MPI_INT, 3, 0, MPI_COMM_WORLD);
}
//#pragma omp task_async depend(in:c[i:1]) depend(out:d[i:1]) 
if (__taskid == 3) {
    MPI_Recv (&c[i], 1, MPI_INT, 2, 0, MPI_COMM_WORLD, &__status);
    {
	d[i] = c[i] + 3;
    }
    MPI_Send (&d[i], 1, MPI_INT, 4, 0, MPI_COMM_WORLD);
}
//#pragma omp task_async depend(in:d[i:1]) 
if (__taskid == 4) {
    MPI_Recv (&d[i], 1, MPI_INT, 3, 0, MPI_COMM_WORLD, &__status);
    {
	e[i] = d[i] + 4;
	printf("e[%d]: %d\n", i, e[i]);
    }
}
} // for
} // Cluster

if (__taskid == 0) {
}

MPI_Finalize();
return(0);
} // main
