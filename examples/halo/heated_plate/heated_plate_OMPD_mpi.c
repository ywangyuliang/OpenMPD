# include <stdlib.h>
# include <stdio.h>
# include <math.h>
# include <time.h>
# include <sys/time.h>
# include <omp.h>
# include <mpi.h>
# include <assert.h>

int __taskid = -1, __numprocs = -1;

/* **************************************************************************** */
int main ( int argc, char *argv[] )
/*
 * Purpose:
 *
 *   MAIN is the main program for HEATED_PLATE
 *
 * Discussion:
 *
 *   This code solves the steady state heat equation on a rectangular region
 *
 *   The sequential version of this program needs approximately
 *   18/epsilon iterations to complete
 *
 *   The physical region, and the boundary conditions, are suggested
 *   by this diagram;
 *
 *                  W = 0
 *            +------------------+
 *            |                  |
 *   W = 100  |                  | W = 100
 *            |                  |
 *            +------------------+
 *                  W = 100
 *
 *   The region is covered with a grid of M by N nodes, and an N by N
 *   array W is used to record the temperature.  The correspondence between
 *   array indices and locations in the region is suggested by giving the
 *   indices of the four corners:
 *
 *                 I = 0
 *         [0][0]-------------[0][N-1]
 *            |                  |
 *     J = 0  |                  |  J = N-1
 *            |                  |
 *       [M-1][0]-----------[M-1][N-1]
 *                 I = M-1
 *
 *   The steady state solution to the discrete heat equation satisfies the
 *   following condition at an interior grid point:
 *
 *     W[Central] = (1/4) * ( W[North] + W[South] + W[East] + W[West] )
 *
 *   where "Central" is the index of the grid point, "North" is the index
 *   of its immediate neighbor to the "north", and so on
 *
 *   Given an approximate solution of the steady state heat equation, a
 *   "better" solution is given by replacing each interior point by the
 *   average of its 4 neighbors - in other words, by using the condition
 *   as an ASSIGNMENT statement:
 *
 *     W[Central]  <=  (1/4) * ( W[North] + W[South] + W[East] + W[West] )
 *
 *   If this process is repeated often enough, the difference between successive
 *   estimates of the solution will go to zero
 *
 *   This program carries out such an iteration, using a tolerance specified by
 *   the user, and writes the final estimate of the solution to a file that can
 *   be used for graphic processing
 *
 * Licensing:
 *
 *   This code is distributed under the GNU LGPL license
 *
 * Modified:
 *
 *   22 July 2008
 *
 * Author:
 *
 *   Original C version by Michael Quinn
 *   C++ version by John Burkardt
 *
 * Reference:
 *
 *   Michael Quinn,
 *   Parallel Programming in C with MPI and OpenMP,
 *   McGraw-Hill, 2004,
 *   ISBN13: 978-0071232654,
 *   LC: QA76.73.C15.Q55
 *
 * Parameters:
 *
 *   Commandline argument 1, double EPSILON, the error tolerance
 *
 *   Commandline argument 2, char *OUTPUT_FILENAME, the name of the file into which
 *   the steady state solution is written when the program has completed
 *
 * Local parameters:
 *
 *   Local, double DIFF, the norm of the change in the solution from one iteration
 *   to the next
 *
 *   Local, double MEAN, the average of the boundary values, used to initialize
 *   the values of the solution in the interior
 *
 *   Local, double U[M][N], the solution at the previous iteration
 *
 *   Local, double W[M][N], the solution computed at the latest iteration
 */
{
#ifdef _OPENMP
  double start_time, run_time;
#else
  struct timeval tv_start, tv_end;
  double run_time;
#endif

  int M = 500;
  int N = 500;
  double diff;
  double aux_diff;
  double epsilon;
  int i;
  int iterations;
  int iterations_print;
  int j;
  double mean;
  FILE *output;
  char output_filename[80];
  int success;
  double u[M][N];
  double w[M][N];
  MPI_Request reqs;
  MPI_Status status;

MPI_Init(&argc,&argv);
MPI_Comm_size(MPI_COMM_WORLD,&__numprocs);
MPI_Comm_rank(MPI_COMM_WORLD,&__taskid);

if (__taskid == 0) {

  printf ( "\n" );
  printf ( "HEATED_PLATE <epsilon> <fichero-salida>\n" );
  printf ( "  C/serie version\n" );
  printf ( "  A program to solve for the steady state temperature distribution\n" );
  printf ( "  over a rectangular plate.\n" );
  printf ( "\n" );
  printf ( "  Spatial grid of %d by %d points.\n", M, N );

/* Read EPSILON from the command line or the user */
  epsilon = atof(argv[1]);
  printf("The iteration will be repeated until the change is <= %lf\n", epsilon);
  diff = epsilon;
/* Read OUTPUT_FILE from the command line or the user */
  success = sscanf ( argv[2], "%s", output_filename );
  if ( success != 1 )
    {
        printf ( "\n" );
        printf ( "HEATED_PLATE\n" );
        printf ( " Error en la lectura del nombre del fichero de salida\n");
        return 1;
    }

 printf("  The steady state solution will be written to %s\n", output_filename);

/* Set the boundary values, which don't change */
  mean = 0.0;
  for ( i = 1; i < M - 1; i++ )
  {
	  w[i][0] = 100.0;
      mean += w[i][0];
  }
  for ( i = 1; i < M - 1; i++ )
  {
	w[i][N-1] = 100.0;
      mean += w[i][N-1];
  }

  for ( j = 0; j < N; j++ )
  {
	w[M-1][j] = 100.0;
      mean += w[M-1][j];
  }

  for ( j = 0; j < N; j++ )
  {
      w[0][j] = 0.0;
      mean += w[0][j];
  }

/* Average the boundary values, to come up with a reasonable initial value for the interior */
  mean = mean / ( double ) ( 2 * M + 2 * N - 4 );

  printf ( "\n" );
  printf ( "  MEAN = %lf\n", mean );

/*  Initialize the interior solution to the mean value */
  for ( i = 1; i < M - 1; i++ )
    for ( j = 1; j < N - 1; j++ )
	w[i][j] = mean;

/* iterate until the  new solution W differs from the old solution U by no more than EPSILON */

  iterations = 0;
  iterations_print = 2;
  printf ( "\n" );
  printf ( " Iteration  Change\n" );
  printf ( "\n" );

#ifdef _OPENMP
  start_time = omp_get_wtime();
#else
  gettimeofday(&tv_start, NULL);
#endif
}

/* Distribute n rows to each process */
{
MPI_Bcast(&epsilon, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
MPI_Bcast(&diff, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
MPI_Bcast(&M, 1, MPI_INT, 0, MPI_COMM_WORLD);
MPI_Bcast(&N, 1, MPI_INT, 0, MPI_COMM_WORLD);

/* alloc: u[M][N] */
double __u[M][N];

/* Gather needs a separate w buffer because MPI buffers must not be aliased. */
double __w[M][N];

if (__taskid == 0)
  for ( i = 0; i < M; i++ )
      for ( j = 0; j < N; j++ )
        __w[i][j] = w[i][j];
/* With halos, broadcast the full matrix to avoid separate scatter and halo exchanges. */
MPI_Bcast(&__w[0][0], M*N, MPI_DOUBLE, 0, MPI_COMM_WORLD);
  while ( epsilon <= diff )
  {
/* Determine the new estimate of the solution at the interior points The new solution U is the average of north, south, east and west neighbors */

  #pragma omp parallel for private(j)
  for ( i = 0; i < M; i++ )
      for ( j = 0; j < N; j++ )
        __u[i][j] = __w[i][j];

/* Determine the new estimate of the solution at the interior points The new solution W is the average of north, south, east and west neighbors */

    diff = 0.0;
{
double __diff = 0.0;

/* Include the halo above and below so rows line up with the iterations. */
int first = 1 - 1; /* first - halo */
int last = M - 1 + 1;  /* last - halo */
int __iter = -1;
int __start = -1;
int __end = -1;

__iter = ((last - first)  / __numprocs);
if (__taskid < ((last -first) % __numprocs))
    __iter++;
__start =  first + __iter * __taskid ;
if (__taskid >= ((last - first) % __numprocs))
    __start += ((last - first) % __numprocs);
__end = __start + __iter ;
if (__taskid == (__numprocs-1)) assert (__end == last);
if (__taskid == 0) __start += 1; /* halo */
if (__taskid == (__numprocs-1)) __end -= 1; /* halo */

    #pragma omp parallel for private(aux_diff, j) reduction(max:__diff)
    for ( i = __start; i < __end; i++ )
    {
      for ( j = 1; j < N - 1; j++ )
      {
        __w[i][j] = ( __u[i-1][j] + __u[i+1][j] + __u[i][j-1] + __u[i][j+1] ) * 0.25;
        aux_diff = fabs ( __w[i][j] - __u[i][j] );
	__diff = __diff < aux_diff ? aux_diff : __diff;
      }

    }
MPI_Allreduce(&__diff, &diff, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);
}
    iterations++;
{
int __chunk;
int __start_w;
int __end_w;
/* chunk is the number of rows to update and distribute Each 1 represents one halo row */
__chunk = (M / __numprocs);
if (__taskid < (M % __numprocs))
    __chunk++;
__start_w =  __chunk * __taskid;
if (__taskid >= (M % __numprocs))
    __start_w += M % __numprocs;
__end_w = __start_w + __chunk ;
if (__taskid == 0) __start_w = __start_w - 1; /* Subtract the halo */
if (__taskid == (__numprocs-1)) __end_w = __end_w - 1; /* Subtract the halo */
if (__taskid == (__numprocs-1)) assert (__end_w == (M-1));
    if(__taskid > 0)
      MPI_Isend(__w[__start_w], 1*N, MPI_DOUBLE, __taskid - 1, 100, MPI_COMM_WORLD, &reqs);

    if(__taskid < __numprocs - 1)
      MPI_Isend(__w[__end_w-1], 1*N, MPI_DOUBLE, __taskid + 1, 101, MPI_COMM_WORLD, &reqs);

    if(__taskid > 0)
      MPI_Recv(__w[__start_w-1], 1*N, MPI_DOUBLE, __taskid - 1, 101, MPI_COMM_WORLD, &status);

    if(__taskid < __numprocs - 1)
      MPI_Recv(__w[__end_w], 1*N, MPI_DOUBLE, __taskid + 1, 100, MPI_COMM_WORLD, &status);
}
if (__taskid == 0) {
    if ( iterations == iterations_print )
    {
	    printf ( "  %8d  %lg\n", iterations, diff );
      iterations_print = 2 * iterations_print;
    }
}
  } /* end while epsilon */

/* Gather w */
int __chunk_w;
int *displs_w = (int *)malloc(__numprocs*sizeof(int));
int *counts_w = (int *)malloc(__numprocs*sizeof(int));

__chunk_w = (M / __numprocs);
if (__taskid < (M % __numprocs))
    counts_w[__taskid] = (__chunk_w+1) * N;
else
    counts_w[__taskid] = __chunk_w * N;
displs_w[__taskid] = __chunk_w * N * __taskid;
if (__taskid < (M % __numprocs))
    displs_w[__taskid] += __taskid * N;
else  displs_w[__taskid] += (M % __numprocs) * N;

if (__taskid == 0) {
    for (i=0; i < __numprocs; i++)
        if (i < (M % __numprocs))
            counts_w[i] = (__chunk_w+1) * N;
        else
            counts_w[i] = __chunk_w * N;
    displs_w[0] = 0;
    for (i=1; i < __numprocs; i++)
        if (i <= (M % __numprocs))
            displs_w[i] = displs_w[i-1] + (__chunk_w+1) * N;
        else
            displs_w[i] = displs_w[i-1] + __chunk_w * N;
     assert ((displs_w[__numprocs-1] + counts_w[__numprocs-1]) == M*N);
}

MPI_Gatherv((&__w[0][0])+displs_w[__taskid], counts_w[__taskid], MPI_DOUBLE, &w[0][0], counts_w, displs_w, MPI_DOUBLE, 0, MPI_COMM_WORLD);

}

if (__taskid == 0) {

#ifdef _OPENMP
  run_time = omp_get_wtime() - start_time;
#else
  gettimeofday(&tv_end, NULL);
  run_time=(tv_end.tv_sec - tv_start.tv_sec) * 1000000 +
        (tv_end.tv_usec - tv_start.tv_usec); /* microseconds */
  run_time = run_time/1000000; /* seconds */
#endif

  printf ( "\n" );
  printf ( "  %8d  %lg\n", iterations, diff );
  printf ( "\n" );
  printf ( "  Error tolerance achieved.\n" );
  printf("\n Tiempo version Secuencial = %lg s\n", run_time);

/* Write the solution to the output file */
  output = fopen(output_filename, "wt");

  fprintf(output, "%d\n", M);
  fprintf(output, "%d\n", N);

  for ( i = 0; i < M; i++ )
  {
    for ( j = 0; j < N; j++)
    {
	fprintf(output, "%lg ", w[i][j]);
    }
    fprintf(output, "\n");
  }
  fclose(output);

  printf ( "\n" );
  printf ( " Solucion escrita en el fichero %s\n", output_filename );
  printf ( "\n" );
  printf ( "HEATED_PLATE_Serie:\n" );
  printf ( "  Normal end of execution.\n" );
}

  MPI_Finalize();

  return 0;
}
