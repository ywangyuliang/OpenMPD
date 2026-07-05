/* **************************************************i***
 * Matrix multiplication program: Version ikj
 *
 * RUN:  ./executable [matrix_dimension]
 *******************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>

/* Initialize matrices with generic values */

void inicializarMatrizRandom (float *M, int m, int n){
    int i;
    for (i = 0; i < m*n; i++) {
	    M[i] = rand() % 10;
    }
}

/* Multiply A by B and store the result in C: i,k,j version */

void Mult(float *A, float *B, float *C, int fA, int cA, int cB) {
     float r;
     int i,j,k;
     int cC = cB; /* number of columns in C == columns in B */
     int fC = fA; /* number of rows in C == rows in A */

     /* C must be zero-initialized first */
     for (i=0; i< fC; i++)
       for (j=0; j< cC; j++)
           C[i*cC+j] = 0;

     for (i=0; i<fA; i++)
        for (k=0; k<cA; k++) {
          r = A[i*cA+k];
          for (j=0; j<cB; j++)
               C[i*cC+j]+= r * B[k*cB+j];
        }
}

/** Print the first n elements from the first m rows of matrix M
    cM is the number of columns in the matrix **/
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

    printf("**** MatMult: Version i,k,j *********\n");

    float *A;  /* Matrix A */
    float *B;  /* Matrix B */
    float *C;  /* Matrix C */

    int m;
    /*
     * int i,j,k, m;
     * float acum;
     */

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

m=10;
    if (m<10){ /* Print matrix values for small inputs */
      printf("\nValores de la matriz A\n");
      imprimeMat(A, 4, 5, C1);
      printf("\nValores de la matriz B\n");
      imprimeMat(B, 5, 3, C2);
      printf("\nValores de la matriz C\n");
      imprimeMat(C, 4, 3, C2);
    }

    printf("\nDuracion total: %f segundos\n\n", segundos);

    free(A);
    free(B);
    free(C);

    return(0);
}
