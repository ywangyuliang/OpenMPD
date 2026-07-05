/* ***************************************************************
 *  Matrix multiplication program: Version ijk
 *
 * RUN:  ./executable n_rowsA n_colsA n_colsB
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
#include <omp.h>

/* Initialize matrices with generic values */

void inicializarMatrizRandom (float *M, int m, int n){
    int i;
    for (i = 0; i < m*n; i++) {
	    M[i] = rand() % 1000;
    }
}

/* Multiply A by B and store the result in C */

void Mult(float *A, float *B, float *C, int fA, int cA, int cB) {
     float acum;
     int i,j,k;
     int cC = cB; /* number of columns in C == columns in B */

#pragma omp parallel for private(acum, j, k) shared(A, B, C, fA, cA, cB, cC)
     for (i=0; i<fA; i++)
        for (j=0; j<cB; j++) {
             acum = 0;
#ifdef GCC
#pragma omp simd
#endif
             for (k=0; k<cA; k++)
                 acum += A[i*cA+k] * B[k*cB+j]; /* A(i,k)*B(k,j) */
             C[i*cC+j] = acum; /* C(i,j) */
        }
}

/** Print the first n elements from the first m rows of matrix M
 *  cM is the number of columns in the matrix **/

void imprimeMat (float *M, int m, int n, int cM){
    int i, j;
    for (i = 0;i<m;i++){
      for (j = 0;j<n;j++){
        printf(" %4f ", M[i*cM+j]);
      }
      printf("\n");
    }
}

int main(int argc, char **argv){
    if (argc <4) {
	printf("Uso: Ejecutable n_filas(A) n_columnas(A) n_columnas(B)\n");
	return(0);
    }

    int F1=atoi(argv[1]); /* Number of rows in A */
    int C1=atoi(argv[2]); /* Number of columns in A */
    int C2=atoi(argv[3]); /* Number of columns in B */
    int F2=C1;		  /* Number of rows in B == number of columns in A */

    printf("**** MatMult: Version i,j,k *********\n");

    float *A;  /* Matrix A */
    float *B;  /* Matrix B */
    float *C;  /* Matrix C */

    struct timeval t, t2;
    double segundos;

   /* Allocate memory and zero matrices A, B and C */

    A = (float *)calloc((F1*C1), sizeof(float));
    B = (float *)calloc((F2*C2), sizeof(float));
    C = (float *)calloc((F1*C2), sizeof(float));

    /* Initialize matrices A and B with random values */

    inicializarMatrizRandom (A, F1, C1);
    inicializarMatrizRandom (B, F2, C2);

    gettimeofday(&t, NULL);
    Mult(A, B, C, F1, C1, C2);
    gettimeofday(&t2, NULL);
    segundos = (((t2.tv_usec - t.tv_usec)/1000000.0f)  + (t2.tv_sec - t.tv_sec));

    printf("\n *******> Duracion total: %f segundos\n\n", segundos);

/* Print a few matrix values  */
      printf("\nValores de la matriz Resultado\n");
      imprimeMat(C, 1, 8, C2);

    free(A);
    free(B);
    free(C);

    return(0);
}
