#include<stdlib.h>
#include<stdio.h>
#include<math.h>
#include<omp.h>
#include<complex.h>
#include<tgmath.h>
#include <sys/time.h>

#define DIM 4096

#pragma omp declare cluster  
typedef struct{
	unsigned char bl,gr,re;
} color;
#pragma omp end declare cluster 

void tga_write ( int w, int h, color rgb[], char *filename );

#pragma omp cluster
color fcolor(int iter,int num_its){
        color c;

// Poner un color dependiente del no. de iteraciones
    c.re = 255;
    c.gr = (iter*20)%255;
    c.bl = (iter*20)%255;
    return c;
}

#pragma omp cluster
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

#pragma omp cluster
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
#pragma omp cluster broad(radius, iter) gather(rgb[height*width]:chunk(width)) 
{
	rgb = calloc (width*height, sizeof(color));
#pragma omp teams distribute dist_schedule(static,1)

#pragma omp parallel for private (x,y,k,i,z0) shared(rgb,width,height)
	for(x=0;x<height;x++){
		k= x*width;
#pragma omp simd
        	for(y=0;y<width;y++){
			z0 = mapPoint(width,height,radius,x,y);
			i = explode (z0, c, radius, iter);

			if (i<iter) { // Si esta fuera del Jc,
				   // se pinta en color dependiente del #iteraciones
				rgb[k+y] = fcolor(i,iter);
				count++;
			}
		}
	}
}
	printf("Elementos fuera de Jc %d de %d\n",count, width*height);
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

 
	if(argC != 6) {
		printf("Uso : %s\n", "<dim de la ventana, partes real e imaginaria de c, radio, iteraciones>");
		exit(1);
	}
		width = atoi(argV[1]);
		height = width; // La ventana es cuadrada
		if (width >DIM) {
                   printf("El tamanyo de la ventana deben ser menor que 1024\n");
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
	#pragma omp cluster broad(width, height, c)
	rgb = juliaSet(width,height,c,atof(argV[4]), atoi(argV[5]));

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