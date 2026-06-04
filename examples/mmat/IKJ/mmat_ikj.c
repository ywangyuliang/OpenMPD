/* ***************************************************************
 *  Programa de multiplicacion de matrices: Version ikj
 *
 * EJECUCION:  ./ejecutable n_filasA n_columnasA n_columnasB
 ******************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>

/* Inicializa las matrices en forma generica */

void inicializarMatrizRandom(float *M, int m, int n)
{
  int i;
  for (i = 0; i < m * n; i++)
  {
    M[i] = rand() % 1000;
  }
}

/* Multiplica las matrices AxB dejando el resultado en C */

void Mult(float *A, float *B, float *C, int fA, int cA, int cB)
{
  float acum;
  int i, j, k;
  int cC = cB; // numero de columnas de C == columnas de B

  for (i = 0; i < fA; i++)
    for (k = 0; k < cA; k++)
    {
      acum = A[i * cA + k];
      for (j = 0; j < cB; j++)
        C[i * cC + j] += acum * B[k * cB + j]; // A(i,k)*B(k,j)
    }
}

/** Imprime los n primeros elementos de las m primeras filas de la matriz M por pantalla
 *  cM es el numero de columnas de la matriz **/

void imprimeMat(float *M, int m, int n, int cM)
{
  int i, j;
  for (i = 0; i < m; i++)
  {
    for (j = 0; j < n; j++)
    {
      printf(" %4f ", M[i * cM + j]);
    }
    printf("\n");
  }
}

int main(int argc, char **argv)
{
  if (argc < 4)
  {
    printf("Uso: Ejecutable n_filas(A) n_columnas(A) n_columnas(B)\n");
    return (0);
  }

  int F1 = atoi(argv[1]); // Numero de filas de A
  int C1 = atoi(argv[2]); // Numero de columnas de A
  int C2 = atoi(argv[3]); // Numero de columnas de B
  int F2 = C1;            // Numero de filas de B == numero de columnas de A

  printf("**** MatMult: Version i,k,j *********\n");

  float *A; // Matriz A
  float *B; // Matriz B
  float *C; // Matriz C

  struct timeval t, t2;
  double segundos;

  // Se reserva memoria e inicializa a 0 las matrices A, B y C.

  A = (float *)calloc((F1 * C1), sizeof(float));
  B = (float *)calloc((F2 * C2), sizeof(float));
  C = (float *)calloc((F1 * C2), sizeof(float));

  // Inicializacion de las matrices A y B con valores aleatorios

  inicializarMatrizRandom(A, F1, C1);
  inicializarMatrizRandom(B, F2, C2);

  // Multiplicacion de A por B, almacenando el resultado en C

  gettimeofday(&t, NULL);
  Mult(A, B, C, F1, C1, C2);
  gettimeofday(&t2, NULL);
  segundos = (((t2.tv_usec - t.tv_usec) / 1000000.0f) + (t2.tv_sec - t.tv_sec));

  printf("\n *******> Duracion total: %f segundos\n\n", segundos);

  // Se muestran algunos valores de las matrices
//  printf("\nValores de la matriz A\n");
//  imprimeMat(A, 4, 5, C1);
//  printf("\nValores de la matriz B\n");
//  imprimeMat(B, 5, 3, C2);
  printf("\nValores de la matriz Resultado\n");
  imprimeMat(C, 1, 8, C2);

  // Se libera el espacio previamente reservado.
  free(A);
  free(B);
  free(C);

  return (0);
}
