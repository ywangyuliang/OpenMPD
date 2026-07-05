#include<stdlib.h>
#include<stdio.h>
#include<math.h>
#include<omp.h>
#include<complex.h>
#include<tgmath.h>
#include<sys/time.h>
#include"mpi.h"
#define DIM 4096

typedef struct{
	unsigned char bl,gr,re;
} color;

void tga_write ( int w, int h, color rgb[], char *filename );

color fcolor(int iter,int num_its)
{
    color c;

    c.re = 255;
    c.gr = (iter*20)%255;
    c.bl = (iter*20)%255;
    return c;
}

int explode (float _Complex z0, float _Complex c, float radius, int n)
{
    int k=1;
    float modul;
    float real;
    float imag;

    z0 = (z0*z0)+c;
    real=crealf(z0);
    imag=cimagf(z0);
    modul = sqrtf(real*real + imag*imag);

    while ((k<=n) && (modul<=radius)){
        z0 = (z0*z0)+c;
        real=crealf(z0);
        imag=cimagf(z0);
        modul = sqrtf(real*real + imag*imag);
        k++;
    }
    return k;
}

float _Complex mapPoint(int width,int height,float radius,int x,int y)
{
    float _Complex c;
    int l = (width<height)?width:height;
    float re = 2*radius*(x - width/2.0)/l;
    float im = 2*radius*(y - height/2.0)/l;

    c = re+im*I;
    return c;
}

color *juliaSet(int width,int height,float _Complex c,float radius,int n,int myrank, int nProcesos){
	int x,y,i;
	float _Complex z0;
	int k=0;

	color * rgb;
	rgb = calloc (width*(height/nProcesos), sizeof(color));

	#pragma omp parallel for private (x,y,k,i,z0) shared(rgb,width,height)
	for(x=(height/nProcesos)*myrank; x<(height/nProcesos)*(myrank+1); x++){

		#pragma omp simd
        for(y=0;y<width;y++){
			z0 = mapPoint(width,height,radius,x,y);
			i = explode (z0, c, radius, n);

			if (i<n)
				rgb[k]=fcolor(i,n);
			k++;
		}
	}
	return rgb;
}

int main(int argC, char* argV[])
{
int width, height;
float _Complex c;
color *rgb;
color *rgbfinal;
int myrank, nProcesos;

MPI_Init(&argC,&argV);
MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
MPI_Comm_size(MPI_COMM_WORLD, &nProcesos);

MPI_Datatype color_type;

int blocklengths[1];
blocklengths[0]= 3;
MPI_Aint disp[1];
disp[0]=0;
MPI_Datatype old_types[1];
old_types[0]= MPI_UNSIGNED_CHAR;
MPI_Type_create_struct(1,blocklengths, disp, old_types, &color_type);
MPI_Type_commit(&color_type);

#ifdef _OPENMP
double start_time, end_time;
#else
struct timeval tv_start, tv_end;
float tiempo_trans;
#endif

	if(argC != 6) {
		if(myrank==0){
		printf("Uso : %s\n", "<dim de la ventana, partes real e imaginaria de c, radio, iteraciones>");
		}
		exit(1);

	}
	else{
		width = atoi(argV[1]);
		height = width; /* Square window */
		if (width >DIM) {
		if(myrank==0){
                   printf("El tamanyo de la ventana deben ser menor que 1024\n");
                }
			exit(1);
                }
		float re = atof(argV[2]);
                float im = atof(argV[3]);

                c=re+im*I;
	}
if(myrank==0){

	printf("JuliaSet: %d, %d, %f, %f, %f, %d\n", width, height,creal(c),cimag(c),atof(argV[4]),atoi(argV[5]));
#ifdef _OPENMP
	start_time = omp_get_wtime();
#else
	gettimeofday(&tv_start, NULL);
#endif
}

if(myrank==0){
         rgbfinal= calloc(width*height, sizeof(color));
}
	rgb = juliaSet(width,height,c,atof(argV[4]), atoi(argV[5]),myrank,nProcesos);

	MPI_Gather(rgb, width*(height/nProcesos), color_type, rgbfinal, width*(height/nProcesos), color_type, 0, MPI_COMM_WORLD);

	MPI_Barrier(MPI_COMM_WORLD);

if(myrank==0){

#ifdef _OPENMP
	end_time = omp_get_wtime();
	printf ( "Tiempo Julia = %f segundos\n",end_time-start_time);
#else
	gettimeofday(&tv_end, NULL);
	tiempo_trans=(tv_end.tv_sec - tv_start.tv_sec) * 1000000 +
	  (tv_end.tv_usec - tv_start.tv_usec); /* microseconds */
	printf("Tiempo Julia = %f segundos\n", tiempo_trans/1000000);
#endif
}

if(myrank==0){

	tga_write ( width, height, rgbfinal, "julia_set.tga"  );

	printf ( "\n" );
	printf ( "JULIA_SET. Finalizado\n");
}

	free(rgb);
if(myrank==0){
	free(rgbfinal);
}
MPI_Finalize();
	return 0;
}

void tga_write ( int w, int h, color rgb[], char *filename )
/*
 * Purpose:
 *
 *   TGA_WRITE writes a TGA or TARGA graphics file of the data.
 *
 * Licensing:
 *
 *   This code is distributed under the GNU LGPL license.
 *
 * Modified:
 *
 *   06 March 2017
 *
 * Parameters:
 *
 *   Input, int W, H, the width and height of the image.
 *
 *   Input, unsigned char RGB[W*H*3], the pixel data.
 *
 *   Input, char *FILENAME, the name of the file to contain the screenshot
 */
{
  FILE *file_unit;
  unsigned char header1[12] = { 0,0,2,0,0,0,0,0,0,0,0,0 };
  unsigned char header2[6] = { w%256, w/256, h%256, h/256, 24, 0 };
/* Create the file */
  file_unit = fopen ( filename, "wb" );
/* Write the headers */
  fwrite ( header1, sizeof ( unsigned char ), 12, file_unit );
  fwrite ( header2, sizeof ( unsigned char ), 6, file_unit );
/* Write the image data */
  fwrite ( rgb, sizeof ( unsigned char ), 3 * w * h, file_unit );
/* Close the file */
  fclose ( file_unit );

  printf ( "\n" );
  printf ( "TGA_WRITE:\n" );
  printf ( "  Graphics data saved as '%s'\n", filename );

  return;
}
