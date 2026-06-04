#include<stdlib.h>
#include<stdio.h>
#include<math.h>
#include<omp.h>
#include<complex.h>
#include<tgmath.h>
#include <sys/time.h>
#include <mpi.h>

//#define DIM 1024
#define DIM 8192

typedef struct{
	unsigned char bl,gr,re;
} color;

int __taskid = -1, __numprocs = -1;

// #pragma omp declare cluster  
// typedef struct{
// 	unsigned char bl,gr,re;
// } color;
// #pragma omp end declare cluster 

MPI_Datatype MPIcolor_t;

void __Declare_MPI_Type_color () { 
    int blocklengths[3];
    MPI_Datatype old_types[3];
    MPI_Aint disp[3];
    MPI_Aint lb;
    MPI_Aint extent;
    blocklengths[0]= 1;
    blocklengths[1]= 1;
    blocklengths[2]= 1;
    old_types[0]= MPI_UNSIGNED_CHAR;
    old_types[1]= MPI_UNSIGNED_CHAR;
    old_types[2]= MPI_UNSIGNED_CHAR;
    // Cambios para adaptarse a nuevas funciones de MPI
    MPI_Type_get_extent(MPI_UNSIGNED_CHAR, &lb, &extent);
    disp[0]= lb;
//printf("** Disp[0] = %d, Extent = %d\n", disp[0], extent);
    MPI_Type_get_extent(MPI_UNSIGNED_CHAR, &lb, &extent);
    disp[1]= disp[0] + extent;
//printf("** Disp[1] = %d, lb = %d, Extent = %d\n", disp[1], lb, extent);
    MPI_Type_get_extent(MPI_UNSIGNED_CHAR, &lb, &extent);
    disp[2]= disp[1] + extent;
//printf("** Disp[2] = %d, , lb = %d, Extent = %d\n", disp[2], lb, extent);
    /**/
    MPI_Type_create_struct(3,blocklengths, disp, old_types, &MPIcolor_t);
    MPI_Type_commit(&MPIcolor_t);
}

void Declare_MPI_Types () {
  __Declare_MPI_Type_color ();
  return;
}

void tga_write ( int w, int h, color rgb[], char *filename );

color fcolor(int iter,int num_its){
        color c;

// Poner un color dependiente del no. de iteraciones
/*        c.re = 255;
        c.gr = (iter*20)%255;
        c.bl = (iter*20)%255;
*/
        c.re = (iter*20+0)%255;
        c.gr = (iter*20+85)%255;
        c.bl = (iter*20+170)%255;
        return c;
}

int explode (float _Complex z0, float _Complex c, float radius, int n)
{
int k=1;
float modul;

z0 = (z0*z0)+c;
modul = cabsf(z0);

while ((k<=n) && (modul<=radius)){ 
		z0 = (z0*z0)+c;
		modul = cabsf(z0);
                k++;
}
return k;
}

float _Complex mapPoint(int width,int height,float radius,int x,int y){
	float _Complex c;
	int l = (width<height)?width:height;
	float re = 2*radius*(x - width/2.0)/l;
        float im = 2*radius*(y - height/2.0)/l;
	c = re+im*I;
        return c;
}

color *juliaSet(int width,int height,float _Complex c,float radius,int iter){
	int x,y,i;
	float _Complex z0;
	int k=0;
	int count=0;

	color *rgb;

if (__taskid == 0) {
        rgb = calloc (width*height, sizeof(color));
}
//#pragma omp cluster broad(width, height, c, radius, iter) gather(rgb[height*width]:chunk(width))
//#pragma omp teams distribute dist_schedule(static,1)

{
MPI_Bcast(&width, 1, MPI_INT, 0, MPI_COMM_WORLD);
MPI_Bcast(&height, 1, MPI_INT, 0, MPI_COMM_WORLD);
MPI_Bcast(&c, sizeof(float _Complex ), MPI_PACKED, 0, MPI_COMM_WORLD);
MPI_Bcast(&radius, 1, MPI_FLOAT, 0, MPI_COMM_WORLD);
MPI_Bcast(&iter, 1, MPI_INT, 0, MPI_COMM_WORLD);
//Esto es por si se envía la parte real e imaginaria como dos floats en vez 
//	de como un complejo
// float re, im;
// if (__taskid == 0) {
//     re = crealf(c);
//     im = cimagf(c);
// }
// MPI_Bcast(&re, 1, MPI_FLOAT, 0, MPI_COMM_WORLD);
// MPI_Bcast(&im, 1, MPI_FLOAT, 0, MPI_COMM_WORLD);

// c=re+im*I;
// MPIcolor * __rgb = ( MPIcolor * ) malloc ( height * width * sizeof (MPIcolor));
color * __rgb = ( color * ) malloc ( height * width * sizeof(color));
int __start = __taskid * 1; // x chunk
int __end = height;
int __step = __numprocs * 1; // x chunk

printf("JuliaSet MPI: %d, %d, %f, %f, %ld, %f, %d. Task %d, from %d to %d, ++(%d)\n", width, height,creal(c),cimag(c),sizeof(c),radius,iter,__taskid,__start,__end,__step);

#pragma omp parallel for private (x,y,k,i,z0) shared(rgb,width,height) num_threads(20) schedule(dynamic,1)
	for(x=__start;x<__end;x+=__step){
		k= x*width;
#pragma omp simd
        	for(y=0;y<width;y++){
			z0 = mapPoint(width,height,radius,x,y);
			i = explode (z0, c, radius, iter);

			if (i<iter) { // Si esta fuera del Jc,
				      // se pinta en color dependiente del #iteraciones
				__rgb[k+y] = fcolor(i,iter);
				count++;
			}
		}
	}
printf("Taskid %d, Elementos fuera de Jc %d de %d\n", __taskid, count, width*height/__numprocs);

int *displs_rgb = (int *)malloc(__numprocs*sizeof(int));
int *counts_rgb = (int *)malloc(__numprocs*sizeof(int));
int offset_rgb = 0;

while (offset_rgb < height * width) {
  if (__taskid == 0) {
    for (i=0; i < __numprocs; i++) {
        if (offset_rgb < height*width) {
            counts_rgb[i] = width; // chunk
            displs_rgb[i] = offset_rgb;
            offset_rgb += width;
        }
        else {
            counts_rgb[i] = 0;
            displs_rgb[i] = height*width;
        }
//printf("Task %d: counts_rgb[%d]: %d displs_rgb[%d]: %d\n", __taskid, i, counts_rgb[i], i, displs_rgb[i]);
    }
  }
  else {
        if (offset_rgb + width * __taskid < height *width) {
            counts_rgb[__taskid] = width; // chunk
            displs_rgb[__taskid] = offset_rgb + width * __taskid;
            offset_rgb += width * __numprocs;
        }
        else {
            counts_rgb[__taskid] = 0;
            displs_rgb[__taskid] = height*width;
            offset_rgb += width * __numprocs;
        }
//printf("Task %d: counts_rgb[%d]: %d displs_rgb[%d]: %d\n", __taskid, __taskid, counts_rgb[__taskid], __taskid, displs_rgb[__taskid]);
  }
  MPI_Gatherv(__rgb+displs_rgb[__taskid], counts_rgb[__taskid], MPIcolor_t,
	      rgb, counts_rgb, displs_rgb, MPIcolor_t, 0, MPI_COMM_WORLD);
}

}
	return rgb;
}

int main(int argC, char* argV[])
{
int width, height;
float _Complex c;
color *rgb;

#ifdef _OPENMP
double start_time, end_time;
#else
struct timeval tv_start, tv_end;
float tiempo_trans;
#endif

    MPI_Init(&argC, &argV);
    MPI_Comm_size(MPI_COMM_WORLD, &__numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &__taskid);

// #pragma omp declare cluster  
 Declare_MPI_Types();

if (__taskid == 0) {
 
	if(argC != 6) {
		printf("Uso : %s\n", "<dim de la ventana, partes real e imaginaria de c, radio, iteraciones>");
		exit(1);
	}
	else{
		width = atoi(argV[1]);
		height = width; // La ventana es cuadrada
//		height = 3*width/4; // Prueba para usar  x!=y
		if (width >DIM) {
                   printf("El tamanyo de la ventana debe ser menor que 1024\n");
                   exit(1);
                }
		float re = atof(argV[2]);
                float im = atof(argV[3]);

                c=re+im*I;

	printf("JuliaSet: %d, %d, %f, %f, %f, %d\n", width, height,creal(c),cimag(c),atof(argV[4]),atoi(argV[5]));
#ifdef _OPENMP
	start_time = omp_get_wtime();
#else
	gettimeofday(&tv_start, NULL);
#endif
	}
}
	rgb = juliaSet(width,height,c,atof(argV[4]), atoi(argV[5]));

if (__taskid == 0) {

#ifdef _OPENMP
	end_time = omp_get_wtime();
	printf ( "Tiempo Julia = %f segundos\n",end_time-start_time);
#else
	gettimeofday(&tv_end, NULL);
	tiempo_trans=(tv_end.tv_sec - tv_start.tv_sec) * 1000000 +
	  (tv_end.tv_usec - tv_start.tv_usec); //en us
	printf("Tiempo Julia = %f segundos\n", tiempo_trans/1000000);
#endif
	
	tga_write ( width, height, rgb, "julia_set.tga" );

  	printf ( "\n" );
  	printf ( "JULIA_SET. Finalizado\n");

  	free(rgb);
}

	 MPI_Finalize();

  	return 0;
}

/******************************************************************************/
void tga_write ( int w, int h, color rgb[], char *filename )
/******************************************************************************/
/*
  Purpose:

    TGA_WRITE writes a TGA or TARGA graphics file of the data.

  Licensing:

    This code is distributed under the GNU LGPL license.

  Modified:

    06 March 2017

  Parameters:

    Input, int W, H, the width and height of the image.

    Input, unsigned char RGB[W*H*3], the pixel data.

    Input, char *FILENAME, the name of the file to contain the screenshot.
*/
{
  FILE *file_unit;
  unsigned char header1[12] = { 0,0,2,0,0,0,0,0,0,0,0,0 };
  unsigned char header2[6] = { w%256, w/256, h%256, h/256, 24, 0 };
/* 
  Create the file.
*/
  file_unit = fopen ( filename, "wb" );
/*
  Write the headers.
*/
  fwrite ( header1, sizeof ( unsigned char ), 12, file_unit );
  fwrite ( header2, sizeof ( unsigned char ), 6, file_unit );
/*
  Write the image data.
*/
  fwrite ( rgb, sizeof ( unsigned char ), 3 * w * h, file_unit );
/*
  Close the file.
*/
  fclose ( file_unit );

  printf ( "\n" );
  printf ( "TGA_WRITE:\n" );
  printf ( "  Graphics data saved as '%s'\n", filename );

  return;
}
