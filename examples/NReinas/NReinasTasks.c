/*
 * PCM. Advanced Architectures Course 16/17 ETSISI 08/02/17
 * nqueens: sequential N-Queens problem
 */
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/time.h>

#define TRUE 1
#define FALSE 0
#define MAX_REINAS 25

/* Global variables */

/*
 * Indexed by column; stores the row containing the queen
 * int reinaEnFila[MAX_REINAS];
 */

int numReinas, soluciones=0;

/* -------------------------------------------------------------------+ */
int aceptable (int reinaFila, int reinaColumna, int reinaEnFila[]) {
  int col;

  if (reinaEnFila[reinaColumna]!=0)
    return FALSE; /* Queen in the same column */
  for(col=1; col<=numReinas; col++)
    /* A queen on the same diagonal is not acceptable */
    if ( ((reinaEnFila[col] != 0)
       && (abs(reinaEnFila[col]-reinaFila))==(abs(reinaColumna-col))))
      return FALSE;
  return TRUE;
}

/* -------------------------------------------------------------------+ */
void NReinas(int reina, int reinaEnFilaGlobal[]) {
  int fila,columna,col,i;

#pragma omp parallel
{
int reinaEnFila[MAX_REINAS];
for (i=0; i<MAX_REINAS; i++)
  reinaEnFila[i] = reinaEnFilaGlobal[i];

#pragma omp single
  for(col=1;col<=numReinas;col++) {
    if(aceptable(reina,col,reinaEnFila)) {
      reinaEnFila[col]=reina; /* Queen i is always placed in row i */
      if (reina==numReinas) {
        if (soluciones==0) {
          printf("\n\n");
          for (fila=1;fila<=numReinas;fila++) {
            for(columna=1;columna<=numReinas;columna++) {
              if (fila!=reinaEnFila[columna]) printf(" *");
              else printf(" Q");
            }
            printf("\n");
          }
        }
#pragma omp atomic
	soluciones++;
      }
      else
#pragma omp task if ((numReinas - reina) > 7)
	      NReinas(reina+1, reinaEnFila);
      reinaEnFila[col]=0;
    }
  }
}
}

/* -------------------------------------------------------------------+ */
int main (int argc, char *argv[]) {
  int i;
  struct timeval tv_start, tv_end;
  double time;
  int reinaEnFilaGlobal[MAX_REINAS];

  for (i=0; i<MAX_REINAS; i++)
    reinaEnFilaGlobal[i] = 0;
  soluciones=0;
  if (argc != 2) {
    printf ("Uso: nreinas numReinas\n");
    return 0;
  }
  numReinas = atoi(argv[1]);
  if (numReinas >= MAX_REINAS) {
    printf ("Error: El numero de reinas no puede superar %d\n",(MAX_REINAS-1));
    return 0;
  }
  gettimeofday (&tv_start, NULL);
  NReinas(1, reinaEnFilaGlobal);
  gettimeofday (&tv_end, NULL);
  printf("Numero de soluciones: %d \n",soluciones);
  time = (tv_end.tv_sec - tv_start.tv_sec) * 1000000 +
      (tv_end.tv_usec - tv_start.tv_usec); /* microseconds */
  printf ("Tiempo total => %f seg\n", time/1000000);

  return 0;
}
