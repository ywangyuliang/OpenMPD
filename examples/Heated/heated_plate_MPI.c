# include <stdlib.h>
# include <stdio.h>
# include <math.h>
# include <time.h>
# include <sys/time.h>
# include <omp.h>
#include "mpi.h"

# define M 500
# define N 500
#define MASTER 0
#define COMM MPI_COMM_WORLD
#define MAX(A,B)  ((A)>(B)?(A):(B))  /* Max macro */

int id;
double epsilon;
double diff;

int steady_state(int n_rows, int n_procs, double w[n_rows][N], double u[n_rows][N])
{
  int i, j;
  int iterations = 0;
  int iterations_print = 1;
  double aux_diff;
  diff = epsilon;
  MPI_Request reqs;
  MPI_Status status;

  while(epsilon <= diff)
  {
    for(i = 0; i < n_rows; ++i)
      for(j = 0; j < N; ++j)
        u[i][j] = w[i][j];

    aux_diff = 0.0;

    #pragma omp parallel for reduction(max:aux_diff)
    for ( i = 1; i < n_rows - 1; ++i)
    {
      for ( j = 1; j < N - 1; ++j)
      {
        w[i][j] = ( u[i-1][j] + u[i+1][j] + u[i][j-1] + u[i][j+1] ) * 0.25;
        double aux_fabs = fabs ( w[i][j] - u[i][j] );
        aux_diff = aux_diff < aux_fabs ? aux_fabs : aux_diff;
      }
    }
    MPI_Allreduce(&aux_diff, &diff, 1, MPI_DOUBLE, MPI_MAX, COMM);

    if(id > 0)
      MPI_Isend(w[1], N, MPI_DOUBLE, id - 1, 100, COMM, &reqs);

    if(id < n_procs - 1)
      MPI_Isend(w[n_rows - 2], N, MPI_DOUBLE, id + 1, 101, COMM, &reqs);

    if(id > 0)
      MPI_Recv(w[0], N, MPI_DOUBLE, id - 1, 101, COMM, &status);

    if(id < n_procs - 1)
      MPI_Recv(w[n_rows - 1], N, MPI_DOUBLE, id + 1, 100, COMM, &status);

    iterations++;
    if(id == MASTER)
    {
      if ( iterations == iterations_print )
      {
        printf ( "  %8d  %lg\n", iterations, diff );
        iterations_print = 2 * iterations_print;
      }
    }
  }
  return iterations;
}

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

  int i;
  int iterations;
  int iterations_print;
  int j;
  double mean;
  FILE *output;
  char output_filename[80];
  int success;
  int n_procs;
  double u[M][N];
  double w[M][N];
  MPI_Request reqs;
  MPI_Status status;

/* Read EPSILON from the command line or the user */
  epsilon = atof(argv[1]);
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

/*  Initialize the interior solution to the mean value */
  for ( i = 1; i < M - 1; i++ )
    for ( j = 1; j < N - 1; j++ )
	w[i][j] = mean;

/* iterate until the  new solution W differs from the old solution U by no more than EPSILON */

#ifdef _OPENMP
  start_time = omp_get_wtime();
#else
  gettimeofday(&tv_start, NULL);
#endif

  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &n_procs); /* Getting # of processes */
  MPI_Comm_rank(MPI_COMM_WORLD, &id);     /* Getting my rank */

  int n_rows = ceil(1.0 * M / n_procs);
  int offset = id * n_rows;
  n_rows = (offset + n_rows >= M) ? (M - offset) : n_rows;
  int my_rows = n_rows;

  if (n_procs > 1) {
      n_rows += (id == 0 || id == n_procs - 1) ? 1 : 2;
      offset = MAX(0, offset - 1);
  }

  double u2[n_rows][N];
  double w2[n_rows][N];

  for(i = 0; i < n_rows; ++i)
    for(j = 0; j < N; j++)
      w2[i][j] = w[offset + i][j];

  iterations = steady_state(n_rows, n_procs, w2, u2);

  if(id == MASTER)
  {
    for(i = 0; i < n_rows; ++i)
      for(j = 0; j < N; ++j)
        w[offset + i][j] = w2[i][j];

    for(i = 1; i < n_procs; i++)
    {
      n_rows = ceil(1.0 * M / n_procs);
      offset = i * n_rows;
      n_rows = (offset + n_rows >= M) ? (M - offset) : n_rows;
      MPI_Recv(w[offset], N * n_rows, MPI_DOUBLE, i, 102, COMM, &status);
    }
  }
  else
  {
    MPI_Isend(w2[1], N * my_rows, MPI_DOUBLE, MASTER, 102, COMM, &reqs);
  }

  MPI_Finalize();
  if(id == 0)
  {

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
  return 0;
}
