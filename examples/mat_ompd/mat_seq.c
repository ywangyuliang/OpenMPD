#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <sys/time.h>


#define BACK_SIZE N2+1
#define GO_SIZE 1

#define WORKING		100
#define END		10
#define MAGIC		255

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


void imprimeMat (float **mat, int F, int C){
    int i, j;
    for (i=0; i<F; i++) {
        for (j=0; j<C; j++) {
	    printf(" %4f ", mat[i][j]);
	}
	printf("\n");
    }
}

void inicializarMatriz (float **mat, int F, int C){
    int i, j;
    for (i=0; i<F; i++) 
        for (j=0; j<C; j++) 
  	    mat[i][j] = (i*F+j) % MAGIC;
}

void Mult_ikj(float *matA, float *matB, float *matC, int fA, int cA, int cB) {
     float r;
     int i,j,k;
     int cC = cB; // numero de columnas de C == columnas de B
     int fC = fA; // numero de columnas de C == columnas de B

     // Es necesario inicializar previamente C a cero
     // for (i=0; i< fC; i++)
     //   for (j=0; j< cC; j++)
     //       C[i*cC+j] = 0;

     for (i=0; i<fA; i++)
        for (k=0; k<cA; k++) {
          r = matA[i*cA+k];
          for (j=0; j<cB; j++)
               matC[i*cC+j]+= r * matB[k*cB+j];
        }
}

void Mult_ijk(float **matA, float **matB, float **matC, int fA, int cA, int cB) {
    float result;
    int i,j,k;
    int cC = cB; // numero de columnas de C == columnas de B
    int fC = fA; // numero de columnas de C == columnas de B

    for (i=0; i<fA; i++) {
        for (j=0; j<cB; j++) {
            result = 0;
            for (k=0; k<cA; k++) {
                result += matA[i][k] * matB[k][j];
            }
            matC[i][j] = result;
        }
    }
}


int main (int argc, char* argv[])
{
    float ** matA;
    float ** matB;
    float ** matC;
    float ** matC_ikj;
    int result;
    int F1, C1, F2, C2;
    int i, j, k;
    struct timeval t, t2;
    double segundos;


    	if ((argc != 4) && (argc != 5)) {
            printf("Introduce las dimensiones de la matriz A: M x N (separadas por un blanco\n");
            if (scanf("%d %d",&F1, &C1) <= 0)
                    printf("Error al leer las dimensiones de A\n");
            printf("Introduce las dimensiones de la matriz B: M x N (separadas por un blanco\n");
            if (scanf("%d %d",&F2, &C2) <= 0)
                    printf("Error al leer las dimensiones de B\n");
	}
	else if (argc == 4) {
           F1=atoi(argv[1]); // Numero de filas de A
           C1=atoi(argv[2]); // Numero de columnas de A
           C2=atoi(argv[3]); // Numero de columnas de B
           F2=C1;            // Numero de filas de B
	} 
        else if (argc == 5) {	
           F1=atoi(argv[1]); // Numero de filas de A
           C1=atoi(argv[2]); // Numero de columnas de A
           F2=atoi(argv[3]); // Numero de filas de B
           C2=atoi(argv[4]); // Numero de columnas de B
           if (C1 != F2)
	       printf("Las dimensiones están mal, N1 debe ser igual a M2\n");

	}
	printf("Producto A(%d, %d) x B(%d, %d)\n", F1, C1, F2, C2);

	matA = MATRIX2D(sizeof(float), F1, C1); // M1 * N1
	matB = MATRIX2D(sizeof(float), F2, C2);
	matC = MATRIX2D(sizeof(float), F1, C2); // M1 * N2
	matC_ikj = MATRIX2D(sizeof(float), F1, C2); // M1 * N2

	inicializarMatriz (matA, F1, C1);
	inicializarMatriz (matB, F2, C2);

    gettimeofday(&t, NULL);
    Mult_ijk(matA, matB, matC, F1, C1, C2);
    gettimeofday(&t2, NULL);
    segundos = (((t2.tv_usec - t.tv_usec)/1000000.0f)  + (t2.tv_sec - t.tv_sec));
    printf("Total time using ijk was %f seconds\n", segundos);
    
    gettimeofday(&t, NULL);
    Mult_ikj(&matA[0][0], &matB[0][0], &matC_ikj[0][0], F1, C1, C2);
    gettimeofday(&t2, NULL);
    segundos = (((t2.tv_usec - t.tv_usec)/1000000.0f)  + (t2.tv_sec - t.tv_sec));
    printf("Total time using ikj was %f seconds\n", segundos);
    

	int wrong = 0;
        for (i=0; i<F1; i++) {
            for (j=0; j<C2; j++) {
                result = 0;
                for (k=0; k<C1; k++) {
                    result += matA[i][k] * matB[k][j];
                }
                if (matC[i][j] != result)
		    wrong = 1;
                if (matC_ikj[i][j] != result)
		    wrong = 2;
            }
        } 
	if (wrong == 1)
	    printf("Test Failed in ijk!!\n");
	else if (wrong == 2)
	    printf("Test Faile in ikj!!!\n");
	else printf("Test Passed!!!\n");

    if (F1<10){ //Si las matrices son pequeñas, se muestran los valores por pantalla
      printf("\nValores de la matriz A\n");
      imprimeMat(matA, 4, 5);
      printf("\nValores de la matriz B\n");
      imprimeMat(matB, 5, 3);
      printf("\nValores de la matriz C\n");
      imprimeMat(matC, 4, 3);
    }
 
    return(0);
}
