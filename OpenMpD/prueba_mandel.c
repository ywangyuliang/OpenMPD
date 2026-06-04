#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <omp.h>
#include <sys/time.h>
#include <string.h>

#define DIM 2048

int main ( );
int explode ( float x, float y, int count_max );
int ppma_write ( char *output_filename, int xsize, int ysize, int *r,
  int *g, int *b );
int ppma_write_data ( FILE *file_out, int xsize, int ysize, int *r,
  int *g, int *b );
int ppma_write_header ( FILE *file_out, char *output_filename, int xsize,
  int ysize, int rgb_max );

/******************************************************************************/
int main ( )
/******************************************************************************/
/*
  Purpose:

    MAIN is the main program for MANDELBROT.

  Discussion:

    MANDELBROT computes an image of the Mandelbrot set.

  Licensing:

    This code is distributed under the GNU LGPL license.

  Author:

    John Burkardt

  Local Parameters:

    Local, integer COUNT_MAX, the maximum number of iterations taken
    for a particular pixel.
*/
{
  int c;
  int c_max;
  int *count;
  int count_max = 8000; // numero iteraciones
  char *filename = "mandelbrot.ppm";
  int i,j;
  int m = DIM; // Ventana de DIMxDIM
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
/*
  Carry out the iteration for each pixel, determining COUNT.
*/
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

// Usa schedule (static,1) para que las filas más pesadas se repartan
// uniformemente, en MPI y OpenMP
#pragma omp cluster alloc(count[m*n])
{
#pragma omp teams distribute reduction(max:c_max) dist_schedule(static,1)
  #pragma omp parallel for simd private(j,x,y) schedule(static,1) 
  for ( i = 0; i < m; i++ ){
    y = ( ( float ) (     i     ) * y_max
        + ( float ) ( m - i - 1 ) * y_min )
        / ( float ) ( m     - 1 );

    for ( j = 0; j < n; j++ )
    {
      x = ( ( float ) (     j     ) * x_max
          + ( float ) ( n - j - 1 ) * x_min )
          / ( float ) ( n     - 1 );

      count[i*n+j] = explode ( x, y, count_max );
      if ( c_max < count[i*n+j] ) {
        c_max = count[i*n+j];
      }
    }
  }
/*
  Set the image data.
*/
#pragma omp cluster update broad(c_max)
} // Fin cluster-1 

printf("c_max %d\n", c_max);

// Usa chunk:n para repartir filas enteras para que coincida con el 
// schedule, pero complica mucho el gather, que va por bloques, no filas
#pragma omp cluster alloc(r[m*n], g[m*n], b[m*n]) gather(r[m*n]:chunk(n), g[m*n]:chunk(n), b[m*n]:chunk(n))
{
#pragma omp teams distribute dist_schedule(static,1)
  #pragma omp parallel for simd private(j,c) schedule(guided)
  for ( i = 0; i < m; i++ ){
    for ( j = 0; j < n; j++ )
    {
      if ( count[i*n+j] % 2 == 1 )
      {
        r[i*n+j] = 255;
        g[i*n+j] = 255;
        b[i*n+j] = 255;
      }
      else
      {
        c = ( int ) ( 255.0 * sqrt ( sqrt ( sqrt (
          ( ( float ) ( count[i*n+j] ) / ( float ) ( c_max ) ) ) ) ) );
        r[i*n+j] = 3 * c / 5;
        g[i*n+j] = 3 * c / 5;
        b[i*n+j] = c;
      }
    }
  }
} // Fin cluster-2

#ifdef _OPENMP
		run_time = omp_get_wtime() - start_time;
		printf("\n Tiempo Mandelbrot = %f seconds\n", run_time);
#else
		gettimeofday(&tv_end, NULL);
		tiempo_trans=(tv_end.tv_sec - tv_start.tv_sec) * 1000000 +
		  (tv_end.tv_usec - tv_start.tv_usec); //en us
		printf("Tiempo Mandelbrot = %f segundos\n", tiempo_trans/1000000);
#endif


/*
  Write an image file.
*/
//  ppma_write ( filename, m, n, r, g, b );

  printf ( "\n" );
  printf ( "  ASCII PPM image data stored in \"%s\".\n", filename );
/*
  Free memory.
*/
  free ( b );
  free ( count );
  free ( g );
  free ( r );
/*
  Terminate.
*/
  printf ( "\n" );
  printf ( "MANDELBROT\n" );
  printf ( "  Normal end of execution.\n" );
  printf ( "\n" );

  return 0;
}
/******************************************************************************/
#pragma omp cluster
int explode ( float x, float y, int count_max )
/******************************************************************************/
/*
  Purpose:

    EXPLODE reports the step when the Mandelbrot iteration at (x,y) "explodes".

  Discussion:

    We assume that the iteration has exploded if an iterate leaves the
    rectangle -2 <= x <= +2, -2 <= y <= +2.

  Licensing:

    This code is distributed under the GNU LGPL license.

  Author:

    John Burkardt

  Parameters:

    Input, float X, Y, the coordinates of the point.
  
    Input, int COUNT_MAX, the maximum number of steps to consider.

    Output, int EXPLODE, the step on which the iteration 'exploded',
    or 0 if it did not explode in COUNT_MAX steps.
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
/******************************************************************************/

int ppma_write ( char *output_filename, int xsize, int ysize, int *r,
  int *g, int *b )

/******************************************************************************/
/*
  Purpose:

    PPMA_WRITE writes the header and data for an ASCII portable pixel map file.

  Example:

    P3
    # feep.ppm
    4 4
    15
     0  0  0    0  0  0    0  0  0   15  0 15
     0  0  0    0 15  7    0  0  0    0  0  0
     0  0  0    0  0  0    0 15  7    0  0  0
    15  0 15    0  0  0    0  0  0    0  0  0

  Licensing:

    This code is distributed under the GNU LGPL license.

  Modified:

    28 February 2003

  Author:

    John Burkardt

  Parameters:

    Input, char *OUTPUT_FILENAME, the name of the file to contain the ASCII
    portable pixel map data.

    Input, int XSIZE, YSIZE, the number of rows and columns of data.

    Input, int *R, *G, *B, the arrays of XSIZE by YSIZE data values.

    Output, int PPMA_WRITE, is
    true, if an error was detected, or
    false, if the file was written.
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
/*
  Open the output file.
*/
  file_out = fopen ( output_filename, "wt" );

  if ( ! file_out )
  {
    printf ( "\n" );
    printf ( "PPMA_WRITE - Fatal error!\n" );
    printf ( "  Cannot open the output file \"%s\".\n", output_filename );
    return 1;
  }
/*
  Compute the maximum.
*/
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
/*
  Write the header.
*/
  error = ppma_write_header ( file_out, output_filename, xsize, ysize, rgb_max );

  if ( error )
  {
    printf ( "\n" );
    printf ( "PPMA_WRITE - Fatal error!\n" );
    printf ( "  PPMA_WRITE_HEADER failed.\n" );
    return 1;
  }
/*
  Write the data.
*/
  error = ppma_write_data ( file_out, xsize, ysize, r, g, b );

  if ( error )
  {
    printf ( "\n" );
    printf ( "PPMA_WRITE - Fatal error!\n" );
    printf ( "  PPMA_WRITE_DATA failed.\n" );
    return 1;
  }
/*
  Close the file.
*/
  fclose ( file_out );

  return 0;
}
/******************************************************************************/

int ppma_write_data ( FILE *file_out, int xsize, int ysize, int *r,
  int *g, int *b )

/******************************************************************************/
/*
  Purpose:

    PPMA_WRITE_DATA writes the data for an ASCII portable pixel map file.

  Licensing:

    This code is distributed under the GNU LGPL license.


  Author:

    John Burkardt

  Parameters:

    Input, ofstream &FILE_OUT, a pointer to the file to contain the ASCII
    portable pixel map data.

    Input, int XSIZE, YSIZE, the number of rows and columns of data.

    Input, int *R, *G, *B, the arrays of XSIZE by YSIZE data values.

    Output, int PPMA_WRITE_DATA, is
    true, if an error was detected, or
    false, if the data was written.
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
/******************************************************************************/

int ppma_write_header ( FILE *file_out, char *output_filename, int xsize,
  int ysize, int rgb_max )

/******************************************************************************/
/*
  Purpose:

    PPMA_WRITE_HEADER writes the header of an ASCII portable pixel map file.

  Licensing:

    This code is distributed under the GNU LGPL license.

  Author:

    John Burkardt

  Parameters:

    Input, FILE *FILE_OUT, a pointer to the file to contain the ASCII
    portable pixel map data.

    Input, char *OUTPUT_FILENAME, the name of the file.

    Input, int XSIZE, YSIZE, the number of rows and columns of data.

    Input, int RGB_MAX, the maximum RGB value.

    Output, int PPMA_WRITE_HEADER, is
    true, if an error was detected, or
    false, if the header was written.
*/
{
  fprintf ( file_out, "P3\n" );
  fprintf ( file_out, "# %s created by ppma_write.c.\n", output_filename );
  fprintf ( file_out, "%d  %d\n", xsize, ysize );
  fprintf ( file_out, "%d\n", rgb_max );

  return 0;
}