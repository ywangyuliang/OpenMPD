/* ***************************************************************
 *  Programa de multiplicacion de matrices: Version ijk
 *
 * EJECUCION:  ./ejecutable num_filasA num_columnasA num_filasB
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <string.h>
#include <sys/time.h>

/* Inicializa las matrices en forma generica */

#ifndef BSIZE
#define BSIZE 32
#endif

#if defined(FLOAT)
typedef float real_t;
#define real_t_fmt      "%f "
#endif
#if defined(DOUBLE)
typedef double real_t;
#define real_t_fmt      "%lf "
#endif

#define min(a,b) (a<b?a:b)
#define max(a,b) (a>b?a:b)

void * xcalloc(size_t nmemb, size_t size)
{
        void * mem = calloc(nmemb, size);
        assert(mem);
        return mem;
}
void * MATRIX1D(int T, int I)
{
        return xcalloc(I,T);
}

void Free_M1D(void * M)
{
        free(M);
}

void * MATRIX2D(int T, int I, int J)
{
        void ** M2D;
        int i;
        M2D = xcalloc(I, sizeof(void*));
        M2D[0] = MATRIX1D(T,I*J);
        for (i=1; i < I; i++)
                M2D[i] = ((void*)(M2D[i-1])) + J*T;
        return M2D;
}

void Free_M2D(void * M)
{
        Free_M1D(((void **)M)[0]);
        free(M);
}

void * MATRIX3D(int T, int I, int J, int K)
{
        void *** M3D;
        int i;
        M3D = xcalloc(I, sizeof(void**));
        M3D[0] = MATRIX2D(T,I*J,K);
        for (i=1; i < I; i++)
                M3D[i] = ((void*)(M3D[i-1])) + J*sizeof(void*);
        return M3D;
}

void Free_M3D(void * M)
{
        Free_M2D(((void **)M)[0]);
        free(M);
}


void inicializarMatrizRandom (real_t *M, int m, int n){
    int i;
    int limite = min(100, m+n);
    for (i = 0; i < m*n; i++) {
	    M[i] = rand() % (limite);
    }
}

/* Multiplica las matrices AxB dejando el resultado en C */

void Mult_ijk(real_t *A, real_t *B, real_t *C, int fA, int cA, int cB) {
     register real_t acum;
     int i,j,k;
     int cC = cB; // numero de columnas de C == columnas de B

     for (i=0; i<fA; i++)
        for (j=0; j<cB; j++) {
          acum = 0;
             for (k=0; k<cA; k++)
               acum += A[i*cA+k] * B[k*cB+j]; // A(i,k)*B(k,j)
          C[i*cC+j] = acum; // C(i,j)
        }
}

void Mult_ikj(real_t *A, real_t *B, real_t *C, int fA, int cA, int cB) {
     register real_t r;
     int i,j,k;
     int cC = cB; // numero de columnas de C == columnas de B
     int fC = fA; // numero de filas de C == filas de B

     // Es necesario inicializar previamente C a cero
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


void BS_5F_MMikj(real_t **A, real_t **B, real_t ** C, int fA, int cA, int cB){

   int i, j , jj, k , kk ;
   register int jjMax = 0.0 , kkMax = 0.0;
   int cC = cB; // numero de columnas de C == columnas de B
   int fC = fA; // numero de filas de C == filas de B

   for (jj=0; jj<cB; jj+= BSIZE) {
       jjMax = min(jj+BSIZE,cB);
       for (kk=0; kk<cA; kk+= BSIZE) {
           kkMax = min(kk+BSIZE,cA);
           for (i=0; i<fA; i++) {
               for (k = kk ; k < kkMax ; k++) {
                   for (j=jj; j < jjMax; j++) {
                      C[i][j]  +=  A[i][k] * B[k][j];
                   }
               }
           }
      }
   }
}

void BS_6F_MMikj(real_t **A, real_t **B, real_t ** C, int fA, int cA, int cB){

   int i, j, k, ii, jj, kk ;
   register int iiMax = 0.0, jjMax = 0.0 , kkMax = 0.0;
   int cC = cB; // numero de columnas de C == columnas de B
   int fC = fA; // numero de filas de C == filas de B

   for(int ii=0; ii<fA; ii+=BSIZE) {
       iiMax = min(ii+BSIZE,fA);
       for (kk=0; kk<cA; kk+= BSIZE) {
           kkMax = min(kk+BSIZE,cA);
           for (jj=0; jj<cB; jj+= BSIZE) {
               jjMax = min(jj+BSIZE,cB);
                   for(i=ii; i<iiMax; i++) {
                       for (k = kk ; k < kkMax ; k++) {
                           for (j=jj; j < jjMax; j++) {
                              C[i][j]  +=  A[i][k] * B[k][j];
                           }
                       }
                   }
          }
      }
   }
}

void BS_5F_MMijk(real_t **A, real_t **B, real_t ** C, int fA, int cA, int cB){

   int cC = cB; // numero de columnas de C == columnas de B
   int fC = fA; // numero de filas de C == filas de B
   register int iiMax = 0.0, jjMax = 0.0 , kkMax = 0.0;
   register real_t acum;
    
       for(int jj=0; jj<cB; jj+=BSIZE) {
           jjMax = min(jj+BSIZE,cB);
           for(int kk=0; kk<cA; kk+=BSIZE) {
               kkMax = min(kk+BSIZE,cA);
               for(int i=0; i<fA; i++) {
                   for(int j=jj; j<jjMax; j++) {
                       acum = 0;
                       for(int k=kk; k<kkMax; k++) 
			   C[i][j] += A[i][k]*B[k][j];
//		       C[i][j] += acum;

                   }
               }
           }
       }
}

void BS_6F_MMijk(real_t **A, real_t **B, real_t ** C, int fA, int cA, int cB){

   int cC = cB; // numero de columnas de C == columnas de B
   int fC = fA; // numero de filas de C == filas de B
   register int iiMax = 0.0, jjMax = 0.0 , kkMax = 0.0;
   register real_t acum;
    
   for(int ii=0; ii<fA; ii+=BSIZE) {
       iiMax = min(ii+BSIZE,fA);
       for(int jj=0; jj<cB; jj+=BSIZE) {
           jjMax = min(jj+BSIZE,cB);
           for(int kk=0; kk<cA; kk+=BSIZE) {
               kkMax = min(kk+BSIZE,cA);
               for(int i=ii; i<iiMax; i++) {
                   for(int j=jj; j<jjMax; j++) {
                       acum = 0;
                       for(int k=kk; k<kkMax; k++) 
			   C[i][j] += A[i][k]*B[k][j];
//		       C[i][j] += acum;

                   }
               }
           }
       }
   }
}

/** Imprime los n primeros elementos de las m primeras filas de la matriz M por pantalla
 *  cM es el numero de columnas de la matriz **/

void imprimeMat (real_t **M, int m, int n){
    int i, j;
    for (i = 0;i<m;i++){
      for (j = 0;j<n;j++){
        printf(" %4f ", M[i][j]);
      }
      printf("\n");
    }
}

void  diffMMat(real_t **C1, real_t ** C2, int m, int n) {
    int i, j;
    for (i = 0;i<m;i++){
      for (j = 0;j<n;j++){
        if (abs(C1[i][j] - C2[i][j]) > 1.0) {
	    printf("DIFERENTES!!!! %d %d: %f\n", i, j, C1[i][j] - C2[i][j]);
	    return;
	}
      }
    }
    printf("Iguales\n");
}

int main(int argc, char **argv){
    if (argc <4) {
	printf("Uso: Ejecutable n_filas(A) n_columnas(A) n_columnas(B)\n");
	return(0);
    }
  
    int FA=atoi(argv[1]); // Numero de filas de A
    int CA=atoi(argv[2]); // Numero de columnas de A
    int CB=atoi(argv[3]); // Numero de columnas de B
    int FB=CA;		  // Numero de filas de B == numero de columnas de A
  
    real_t **A; //Matriz A
    real_t **B; //Matriz B
    real_t **C1, **C2;  //Matriz C
    
    //    int i,j,k, m;
    //    real_t acum;
    
    struct timeval t, t2;
    double segundos;
    
   // Se reserva memoria e inicializa a 0 las matrices A, B y C.
    
    A = MATRIX2D(sizeof(real_t), FA, CA);
    B = MATRIX2D(sizeof(real_t), FB, CB);
    C1 = MATRIX2D(sizeof(real_t), FA, CB);
    C2 = MATRIX2D(sizeof(real_t), FA, CB);
    
    // Inicializacion de las matrices A y B con valores aleatorios
    
    inicializarMatrizRandom (A[0], FA, CA);
    inicializarMatrizRandom (B[0], FB, CB);
    
// Se muestran algunos valores de las matrices 
//    printf("\nValores de la matriz A\n");
//    imprimeMat(A, F1, C1);
//    printf("\nValores de la matriz B\n");
//    imprimeMat(B, F2, C2);
    // Multiplicacion de A por B, almacenando el resultado en C
  
    gettimeofday(&t, NULL);
    Mult_ijk(A[0], B[0], C1[0], FA, CA, CB);
    gettimeofday(&t2, NULL);
    segundos = (((t2.tv_usec - t.tv_usec)/1000000.0f) + (t2.tv_sec - t.tv_sec));

    printf("**** MatMult: Version i,j,k *********\n");
    printf("*******> Duracion total: %f segundos\n\n", segundos);

//    printf("\nValores de la matriz Resultado ijk\n");
//    imprimeMat(C1, 4, 4);
//    memset(C1[0], 0, F1*C2*sizeof(real_t));

    // Multiplicacion de A por B, almacenando el resultado en C
  
    gettimeofday(&t, NULL);
    Mult_ikj(A[0], B[0], C2[0], FA, CA, CB);
    gettimeofday(&t2, NULL);
    segundos = (((t2.tv_usec - t.tv_usec)/1000000.0f) + (t2.tv_sec - t.tv_sec));

    printf("**** MatMult: Version i,k,j *********\n");
    printf("*******> Duracion total: %f segundos\n\n", segundos);

//    printf("\nValores de la matriz Resultado ikj\n");
//    imprimeMat(C2, 4, 4);
    diffMMat(C1, C2, FA, CB);
    memset(C2[0], 0, FA*CB*sizeof(real_t));

    // Multiplicacion de A por B, almacenando el resultado en C
  
    gettimeofday(&t, NULL);
    BS_6F_MMijk(A, B, C2, FA, CA, CB);
    gettimeofday(&t2, NULL);
    segundos = (((t2.tv_usec - t.tv_usec)/1000000.0f) + (t2.tv_sec - t.tv_sec));

    printf("**** MatMult: Version BSIZE 6 Fors i,j,k *********\n");
    printf("*******> Duracion total: %f segundos\n\n", segundos);

//    printf("\nValores de la matriz Resultado BSIZE 6 For ijk\n");
//    imprimeMat(C2, 4, 4);
    diffMMat(C1, C2, FA, CB);
    memset(C2[0], 0, FA*CB*sizeof(real_t));

    // Multiplicacion de A por B, almacenando el resultado en C
  
    gettimeofday(&t, NULL);
    BS_5F_MMijk(A, B, C2, FA, CA, CB);
    gettimeofday(&t2, NULL);
    segundos = (((t2.tv_usec - t.tv_usec)/1000000.0f) + (t2.tv_sec - t.tv_sec));

    printf("**** MatMult: Version BSIZE 5 Fors i,j,k *********\n");
    printf("*******> Duracion total: %f segundos\n\n", segundos);

//    printf("\nValores de la matriz Resultado BSIZE 5 For ijk\n");
//    imprimeMat(C2, 4, 4);
    diffMMat(C1, C2, FA, CB);
    memset(C2[0], 0, FA*CB*sizeof(real_t));

    // Multiplicacion de A por B, almacenando el resultado en C
  
    gettimeofday(&t, NULL);
    BS_6F_MMikj(A, B, C2, FA, CA, CB);
    gettimeofday(&t2, NULL);
    segundos = (((t2.tv_usec - t.tv_usec)/1000000.0f) + (t2.tv_sec - t.tv_sec));

    printf("**** MatMult: Version BSIZE 6 Fors i,k,j *********\n");
    printf("*******> Duracion total: %f segundos\n\n", segundos);

//    printf("\nValores de la matriz Resultado BSIZE 6 For ikj\n");
//    imprimeMat(C2, 4, 4);
    diffMMat(C1, C2, FA, CB);
    memset(C2[0], 0, FA*CB*sizeof(real_t));

    // Multiplicacion de A por B, almacenando el resultado en C
  
    gettimeofday(&t, NULL);
    BS_5F_MMikj(A, B, C2, FA, CA, CB);
    gettimeofday(&t2, NULL);
    segundos = (((t2.tv_usec - t.tv_usec)/1000000.0f) + (t2.tv_sec - t.tv_sec));

    printf("**** MatMult: Version BSIZE 5 Fors i,k,j *********\n");
    printf("*******> Duracion total: %f segundos\n\n", segundos);

//    printf("\nValores de la matriz Resultado BSIZE 5 For ikj\n");
//    imprimeMat(C2, 4, 4);
    diffMMat(C1, C2, FA, CB);
    memset(C2[0], 0, FA*CB*sizeof(real_t));

    // Se libera el espacio previamente reservado.
    Free_M2D(A);
    Free_M2D(B);
    Free_M2D(C1);
    Free_M2D(C2);
    
    return(0);    
}
