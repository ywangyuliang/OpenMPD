# include <math.h>
# include <stdio.h>
# include <stdlib.h>
# include <time.h>
# include <omp.h>
# include <mpi.h>
# include <assert.h>
#include <sys/time.h>

#define DIM 8192

int __taskid = -1, __numprocs = -1;

int main ( );
int explode ( float x, float y, int count_max );
int ppma_write ( char *output_filename, int xsize, int ysize, int *r,
  int *g, int *b );
int ppma_write_data ( FILE *file_out, int xsize, int ysize, int *r,
  int *g, int *b );
int ppma_write_header ( FILE *file_out, char *output_filename, int xsize,
  int ysize, int rgb_max );

int main (int argc, char **argv )
/*
 * Purpose:
 *
 *   MAIN is the main program for MANDELBROT.
 *
 * Discussion:
 *
 *   MANDELBROT computes an image of the Mandelbrot set.
 *
 * Licensing:
 *
 *   This code is distributed under the GNU LGPL license.
 *
 * Author:
 *
 *   John Burkardt
 *
 * Local Parameters:
 *
 *   Local, integer COUNT_MAX, the maximum number of iterations taken
 *   for a particular pixel
 */
{
  int c;
  int c_max;
  int *count;
  int count_max = 8000; /* iteration count */
  char *filename = "mandelbrot.ppm";
  int i,j;
  int m = DIM; /* DIMxDIM window */
  int n = DIM;
  int *r, *g, *b;
  float x,y;
  float x_max =   1.25;
  float x_min = - 2.25;
  float y_max =   1.75;
  float y_min = - 1.75;

#ifdef _OPENMP
double start_time, run_time;
#else
struct timeval tv_start, tv_end;
float tiempo_trans;
#endif

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &__numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &__taskid);

if (__taskid == 0) {
  printf ( "\n" );
  printf ( "MANDELBROT\n" );
  printf ( "  C version\n" );
  printf ( "\n" );
  printf ( "  Create an ASCII PPM image of the Mandelbrot set.\n" );
  printf ( "\n" );
  printf ( "  For each point C = X + i*Y\n" );
  printf ( "  with X range [%f,%f]\n", x_min, x_max );
  printf ( "  and  Y range [%f,%f]\n", y_min, y_max );
  printf ( "  carry out %d iterations of the map\n", count_max );
  printf ( "  Z(n+1) = Z(n)^2 + C.\n" );
  printf ( "  If the iterates stay bounded (norm less than 2)\n" );
  printf ( "  then C is taken to be a member of the set.\n" );
  printf ( "\n" );
  printf ( "  An ASCII PPM image of the set is created using\n" );
  printf ( "    N = %d pixels in the X direction and\n", n );
  printf ( "    N = %d pixels in the Y direction.\n", n );
  count = ( int * ) malloc ( m * n * sizeof ( int ) );

#ifdef _OPENMP
 start_time = omp_get_wtime();
#else
 gettimeofday(&tv_start, NULL);
#endif

  c_max = 0;
  r = ( int * ) malloc ( m * n * sizeof ( int ) );
  g = ( int * ) malloc ( m * n * sizeof ( int ) );
  b = ( int * ) malloc ( m * n * sizeof ( int ) );
}

/* Use schedule(static,1) so heavier rows are distributed evenly across MPI and OpenMP */
{

if (__taskid != 0)
  count = ( int * ) malloc ( m * n * sizeof ( int ) );
int __c_max = 0;

int __start = __taskid * 1; /* x chunk */
int __end = m;
int __step = __numprocs * 1; /* x chunk */

  #pragma omp parallel for simd private(j,x,y) schedule(static,1) reduction(max:__c_max) num_threads(20)
  for ( i = __start; i < __end; i+= __step )
  {
    y = ( ( float ) (     i     ) * y_max
        + ( float ) ( m - i - 1 ) * y_min )
        / ( float ) ( m     - 1 );

    for ( j = 0; j < n; j++ )
    {
      x = ( ( float ) (     j     ) * x_max
          + ( float ) ( n - j - 1 ) * x_min )
          / ( float ) ( n     - 1 );

      count[i*n+j] = explode ( x, y, count_max );
      if ( __c_max < count[i*n+j] ) {
        __c_max = count[i*n+j];
      }
    }
  }

MPI_Reduce(&__c_max, &c_max, 1, MPI_INT, MPI_MAX, 0, MPI_COMM_WORLD);
MPI_Bcast(&c_max, 1, MPI_INT, 0, MPI_COMM_WORLD);

} /* End cluster 1 */
{
/* Use chunk:n to distribute whole rows and match the schedule; gather still works by blocks rather than rows. */
int *  __r = ( int * ) malloc ( m * n * sizeof ( int ) );
int *  __g = ( int * ) malloc ( m * n * sizeof ( int ) );
int *  __b = ( int * ) malloc ( m * n * sizeof ( int ) );

int __start = __taskid * 1; /* x chunk */
int __end = m;
int __step = __numprocs * 1; /* x chunk */

  #pragma omp parallel for simd private(i,c) schedule(static,1) num_threads(20)
  for ( i = __start; i < __end; i+= __step )
  {
    for ( j = 0; j < n; j++ )
    {
      if ( count[i*n+j] % 2 == 1 )
      {
        __r[i*n+j] = 255;
        __g[i*n+j] = 255;
        __b[i*n+j] = 255;
      }
      else
      {
        c = ( int ) ( 255.0 * sqrt ( sqrt ( sqrt (
          ( ( float ) ( count[i*n+j] ) / ( float ) ( c_max ) ) ) ) ) );
        __r[i*n+j] = 3 * c / 5;
        __g[i*n+j] = 3 * c / 5;
        __b[i*n+j] = c;
      }
    }
  }

int *__displs_r = (int *)malloc(__numprocs*sizeof(int));
int *__counts_r = (int *)malloc(__numprocs*sizeof(int));
int *__displs_g = (int *)malloc(__numprocs*sizeof(int));
int *__counts_g = (int *)malloc(__numprocs*sizeof(int));
int *__displs_b = (int *)malloc(__numprocs*sizeof(int));
int *__counts_b = (int *)malloc(__numprocs*sizeof(int));
int __offset_r = 0;
int __offset_g = 0;
int __offset_b = 0;

while (__offset_r < m * n) {
  if (__taskid == 0) {
    for (i=0; i < __numprocs; i++) {
	if (__offset_r < m*n) {
            __counts_r[i] = n; /* chunk */
            __displs_r[i] = __offset_r;
            __offset_r += n;
	}
	else {
	    __counts_r[i] = 0;
            __displs_r[i] = m*n;
	}
    }
  }
  else {
	if (__offset_r + n * __taskid < m *n) {
            __counts_r[__taskid] = n; /* chunk */
            __displs_r[__taskid] = __offset_r + n * __taskid;
	    __offset_r += n * __numprocs;
	}
	else {
	    __counts_r[__taskid] = 0;
	    __displs_r[__taskid] = m*n;
	    __offset_r += n * __numprocs;
	}
  }
  MPI_Gatherv(__r+__displs_r[__taskid], __counts_r[__taskid], MPI_INT, r, __counts_r, __displs_r, MPI_INT, 0, MPI_COMM_WORLD);
} /* end while-1 */

while (__offset_g < m * n) {
  if (__taskid == 0) {
    for (i=0; i < __numprocs; i++) {
	if (__offset_g < m*n) {
            __counts_g[i] = n; /* chunk */
            __displs_g[i] = __offset_g;
            __offset_g += n;
	}
	else {
	    __counts_g[i] = 0;
            __displs_g[i] = m*n;
	}
    }
  }
  else {
	if (__offset_g + n * __taskid < m *n) {
            __counts_g[__taskid] = n; /* chunk */
            __displs_g[__taskid] = __offset_g + n * __taskid;
	    __offset_g += n * __numprocs;
	}
	else {
	    __counts_g[__taskid] = 0;
	    __displs_g[__taskid] = m*n;
	    __offset_g += n * __numprocs;
	}
  }
  MPI_Gatherv(__g+__displs_g[__taskid], __counts_g[__taskid], MPI_INT, g, __counts_g, __displs_g, MPI_INT, 0, MPI_COMM_WORLD);
} /* end while-2 */

while (__offset_b < m * n) {
  if (__taskid == 0) {
    for (i=0; i < __numprocs; i++) {
	if (__offset_b < m*n) {
            __counts_b[i] = n; /* chunk */
            __displs_b[i] = __offset_b;
            __offset_b += n;
	}
	else {
	    __counts_b[i] = 0;
            __displs_b[i] = m*n;
	}
    }
  }
  else {
	if (__offset_b + n * __taskid < m *n) {
            __counts_b[__taskid] = n; /* chunk */
            __displs_b[__taskid] = __offset_b + n * __taskid;
	    __offset_b += n * __numprocs;
	}
	else {
	    __counts_b[__taskid] = 0;
	    __displs_b[__taskid] = m*n;
	    __offset_b += n * __numprocs;
	}
  }
  MPI_Gatherv(__b+__displs_b[__taskid], __counts_b[__taskid], MPI_INT, b, __counts_b, __displs_b, MPI_INT, 0, MPI_COMM_WORLD);
} /* end while-3 */

} /* End cluster 2 */

if (__taskid == 0) {
#ifdef _OPENMP
		run_time = omp_get_wtime() - start_time;
		printf("\n Tiempo Mandelbrot = %f seconds\n", run_time);
#else
		gettimeofday(&tv_end, NULL);
		tiempo_trans=(tv_end.tv_sec - tv_start.tv_sec) * 1000000 +
		  (tv_end.tv_usec - tv_start.tv_usec); /* microseconds */
		printf("Tiempo Mandelbrot = %f segundos\n", tiempo_trans/1000000);
#endif
  ppma_write ( filename, m, n, r, g, b );

  printf ( "\n" );
  printf ( "  ASCII PPM image data stored in \"%s\".\n", filename );
  free ( b );
  free ( count );
  free ( g );
  free ( r );
  printf ( "\n" );
  printf ( "MANDELBROT\n" );
  printf ( "  Normal end of execution.\n" );
  printf ( "\n" );
}

  MPI_Finalize();

  return 0;
}

int explode ( float x, float y, int count_max )
/*
 * Purpose:
 *
 *   EXPLODE reports the step when the Mandelbrot iteration at (x,y) "explodes".
 *
 * Discussion:
 *
 *   We assume that the iteration has exploded if an iterate leaves the
 *   rectangle -2 <= x <= +2, -2 <= y <= +2.
 *
 * Licensing:
 *
 *   This code is distributed under the GNU LGPL license.
 *
 * Author:
 *
 *   John Burkardt
 *
 * Parameters:
 *
 *   Input, float X, Y, the coordinates of the point.
 *
 *   Input, int COUNT_MAX, the maximum number of steps to consider.
 *
 *   Output, int EXPLODE, the step on which the iteration 'exploded',
 *   or 0 if it did not explode in COUNT_MAX steps
 */
{
  int k=1;
  int value;
  double x1;
  double x2=0.0;
  double y1;
  double y2=0.0;

  value = 0;

  x1 = x;
  y1 = y;

  for (k=1 ; k <= count_max; k++)
	{
    x2 = x1 * x1 - y1 * y1 + x;
    y2 = 2.0 * x1 * y1 + y;

    if ( (x2 < 2.0) && (x2 > -2.0) && ( y2 < 2.0) && (y2 > -2.0)) {
        x1 = x2;
        y1 = y2;
    }
    else {
        value = k;
        break;
    }
  }
  return value;
}

int ppma_write ( char *output_filename, int xsize, int ysize, int *r,
  int *g, int *b )

/*
 * Purpose:
 *
 *   PPMA_WRITE writes the header and data for an ASCII portable pixel map file.
 *
 * Example:
 *
 *   P3
 *   # feep.ppm
 *   4 4
 *   15
 *    0  0  0    0  0  0    0  0  0   15  0 15
 *    0  0  0    0 15  7    0  0  0    0  0  0
 *    0  0  0    0  0  0    0 15  7    0  0  0
 *   15  0 15    0  0  0    0  0  0    0  0  0
 *
 * Licensing:
 *
 *   This code is distributed under the GNU LGPL license.
 *
 * Modified:
 *
 *   28 February 2003
 *
 * Author:
 *
 *   John Burkardt
 *
 * Parameters:
 *
 *   Input, char *OUTPUT_FILENAME, the name of the file to contain the ASCII
 *   portable pixel map data.
 *
 *   Input, int XSIZE, YSIZE, the number of rows and columns of data.
 *
 *   Input, int *R, *G, *B, the arrays of XSIZE by YSIZE data values.
 *
 *   Output, int PPMA_WRITE, is
 *   true, if an error was detected, or
 *   false, if the file was written
 */
{
  int *b_index;
  int error;
  FILE *file_out;
  int *g_index;
  int i;
  int j;
  int *r_index;
  int rgb_max;
/* Open the output file */
  file_out = fopen ( output_filename, "wt" );

  if ( ! file_out )
  {
    printf ( "\n" );
    printf ( "PPMA_WRITE - Fatal error!\n" );
    printf ( "  Cannot open the output file \"%s\".\n", output_filename );
    return 1;
  }
/* Compute the maximum */
  rgb_max = 0;
  r_index = r;
  g_index = g;
  b_index = b;

  for ( j = 0; j < ysize; j++ )
  {
    for ( i = 0; i < xsize; i++ )
    {
      if ( rgb_max < *r_index )
      {
        rgb_max = *r_index;
      }
      r_index = r_index + 1;

      if ( rgb_max < *g_index )
      {
        rgb_max = *g_index;
      }
      g_index = g_index + 1;

      if ( rgb_max < *b_index )
      {
        rgb_max = *b_index;
      }
      b_index = b_index + 1;
    }
  }
/* Write the header */
  error = ppma_write_header ( file_out, output_filename, xsize, ysize, rgb_max );

  if ( error )
  {
    printf ( "\n" );
    printf ( "PPMA_WRITE - Fatal error!\n" );
    printf ( "  PPMA_WRITE_HEADER failed.\n" );
    return 1;
  }
/* Write the data */
  error = ppma_write_data ( file_out, xsize, ysize, r, g, b );

  if ( error )
  {
    printf ( "\n" );
    printf ( "PPMA_WRITE - Fatal error!\n" );
    printf ( "  PPMA_WRITE_DATA failed.\n" );
    return 1;
  }
/* Close the file */
  fclose ( file_out );

  return 0;
}

int ppma_write_data ( FILE *file_out, int xsize, int ysize, int *r,
  int *g, int *b )

/*
 * Purpose:
 *
 *   PPMA_WRITE_DATA writes the data for an ASCII portable pixel map file.
 *
 * Licensing:
 *
 *   This code is distributed under the GNU LGPL license.
 *
 * Author:
 *
 *   John Burkardt
 *
 * Parameters:
 *
 *   Input, ofstream &FILE_OUT, a pointer to the file to contain the ASCII
 *   portable pixel map data.
 *
 *   Input, int XSIZE, YSIZE, the number of rows and columns of data.
 *
 *   Input, int *R, *G, *B, the arrays of XSIZE by YSIZE data values.
 *
 *   Output, int PPMA_WRITE_DATA, is
 *   true, if an error was detected, or
 *   false, if the data was written
 */
{
  int *b_index;
  int *g_index;
  int i;
  int j;
  int *r_index;
  int rgb_num;

  r_index = r;
  g_index = g;
  b_index = b;
  rgb_num = 0;

  for ( j = 0; j < ysize; j++ )
  {
    for ( i = 0; i < xsize; i++ )
    {
      fprintf ( file_out, "%d  %d  %d", *r_index, *g_index, *b_index );
      rgb_num = rgb_num + 3;
      r_index = r_index + 1;
      g_index = g_index + 1;
      b_index = b_index + 1;

      if ( rgb_num % 12 == 0 || i == xsize - 1 || rgb_num == 3 * xsize * ysize )
      {
        fprintf ( file_out, "\n" );
      }
      else
      {
        fprintf ( file_out, " " );
      }
    }
  }
  return 0;
}

int ppma_write_header ( FILE *file_out, char *output_filename, int xsize,
  int ysize, int rgb_max )

/*
 * Purpose:
 *
 *   PPMA_WRITE_HEADER writes the header of an ASCII portable pixel map file.
 *
 * Licensing:
 *
 *   This code is distributed under the GNU LGPL license.
 *
 * Author:
 *
 *   John Burkardt
 *
 * Parameters:
 *
 *   Input, FILE *FILE_OUT, a pointer to the file to contain the ASCII
 *   portable pixel map data.
 *
 *   Input, char *OUTPUT_FILENAME, the name of the file.
 *
 *   Input, int XSIZE, YSIZE, the number of rows and columns of data.
 *
 *   Input, int RGB_MAX, the maximum RGB value.
 *
 *   Output, int PPMA_WRITE_HEADER, is
 *   true, if an error was detected, or
 *   false, if the header was written
 */
{
  fprintf ( file_out, "P3\n" );
  fprintf ( file_out, "# %s created by ppma_write.c.\n", output_filename );
  fprintf ( file_out, "%d  %d\n", xsize, ysize );
  fprintf ( file_out, "%d\n", rgb_max );

  return 0;
}
