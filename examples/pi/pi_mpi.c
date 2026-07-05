#include <stdio.h>
#include <math.h>
#include "mpi.h"

double f(a)
double a;
{
    return (4.0 / (1.0 + a*a));
}

int main(argc,argv)
int argc;
char *argv[];
{
    int done = 0, n, myid;
    unsigned int i;
    int numprocs;
    double PI25DT = 3.141592653589793238462643;
    double mypi, pi, h, sum, x;
    double startwtime, endwtime;
    int  namelen;
    char processor_name[MPI_MAX_PROCESSOR_NAME];

    MPI_Init(&argc,&argv);
    MPI_Comm_size(MPI_COMM_WORLD,&numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD,&myid);
    MPI_Get_processor_name(processor_name,&namelen);

    fprintf(stderr,"Process# %d with name %s on %d processors\n",
	    myid, processor_name, numprocs);

    n = 0;
    while (!done)
    {
        if (myid == 0)
        {
            printf("Number of intervals: %d (0 quits)\n", n);
            if (scanf("%d",&n) <= 0)
		printf("Error en el número de intervalos\n");

	    startwtime = MPI_Wtime();
        }
        MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);
        if (n == 0)
            done = 1;
        else
        {
            h   = 1.0 / (double) n;
            sum = 0.0;
            for (i = myid + 1; i <= n; i += numprocs)
            {
                x = h * ((double)i - 0.5);
                sum += f(x);
            }
            mypi = h * sum;

            MPI_Reduce(&mypi, &pi, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

            if (myid == 0)
	    {
                printf("pi is approximately %.16f, Relative Error is %16.8e\n",
                       pi, (double)100 * (pi - PI25DT)/PI25DT);
		endwtime = MPI_Wtime();
		printf("wall clock time = %f\n", endwtime-startwtime);	       
	    }
        }
    }
    MPI_Finalize();

    return 0;
}
