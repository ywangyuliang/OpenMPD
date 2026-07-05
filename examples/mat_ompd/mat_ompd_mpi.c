#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <mpi.h>

#define BACK_SIZE N2+1
#define GO_SIZE 1

#define WORKING		100
#define END		10
#define MAGIC		255

int __taskid = -1, __numprocs = -1;
int FGLO=100;

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

void * ReallocMAT1D(void * ptr, int T, int I)
{
	return realloc(ptr, T*I);
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

void * ReallocMAT2D(void * ptr, int T, int I, int J)
{
        void ** M2D;
	void *aux = ((void **)ptr)[0];
        int i;
        M2D = ReallocMAT1D(ptr, sizeof(void*), I);
        M2D[0] = ReallocMAT1D(aux, T, I*J);
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

void * ReallocMAT3D(void * ptr, int T, int I, int J, int K)
{
        void *** M3D;
	void *aux = ((void **)ptr)[0];
        int i;
        M3D = ReallocMAT1D(ptr, sizeof(void**), I);
        M3D[0] = ReallocMAT2D(aux, T, I*J, K);
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

void Mult_ikj(float **matA, float **matB, float **matC, int fA, int cA, int cB) {
    float r;
    int i,j,k;
    int fB = cA;

     /*
      * C must be zero-initialized first
      * for (i=0; i< fC; i++)
      *   for (j=0; j< cC; j++)
      *       C[i*cC+j] = 0;
      */
{
    int __start, __end, __iter;
    int __fA_chunk;
    int __fC_chunk;
    float ** __matA;
    float ** __matC;
    int *displsA = (int *)malloc(__numprocs*sizeof(int));
    int *countsA = (int *)malloc(__numprocs*sizeof(int));
    int *displsC = (int *)malloc(__numprocs*sizeof(int));
    int *countsC = (int *)malloc(__numprocs*sizeof(int));
    int offset;
MPI_Bcast(&fA, 1, MPI_INT, 0, MPI_COMM_WORLD);
MPI_Bcast(&cA, 1, MPI_INT, 0, MPI_COMM_WORLD);
MPI_Bcast(&fB, 1, MPI_INT, 0, MPI_COMM_WORLD);
MPI_Bcast(&cB, 1, MPI_INT, 0, MPI_COMM_WORLD);

/*
 * The correct split is fA * cA across nprocs, but that does not give an
 * exact row count when fA is not a multiple of nprocs.
 * It must match how the i-loop iterations are distributed.
 * fA*cA or fA*cB balance better but split rows, which is inadmissible.
 */
/*
 * if ((fA*cA)%__numprocs == 0)
 *     __fA_chunk = (fA*cA)/__numprocs;
 * else __fA_chunk = (fA*cA)/__numprocs + 1;
 * if ((fA*cB)%__numprocs == 0)
 *     __fC_chunk = (fA*cB)/__numprocs;
 * else __fC_chunk = (fA*cB)/__numprocs + 1;
 */

if (fA%__numprocs == 0)
    __fA_chunk = fA/__numprocs;
else __fA_chunk = fA/__numprocs + 1;
if (__taskid == 0) {
    offset = 0;
    for (i=0; i < __numprocs; i++) {
        countsA[i] = __fA_chunk * cA;
        displsA[i] = offset;
        offset += __fA_chunk * cA;
    }
    countsA[__numprocs-1] = (fA - (__fA_chunk * (__numprocs-1))) * cA;
}
else if (__taskid == __numprocs-1)
	countsA[__taskid] = (fA - (__fA_chunk * (__numprocs-1))) * cA;
     else countsA[__taskid] = __fA_chunk * cA;

if (fA%__numprocs == 0)
    __fC_chunk = fA/__numprocs;
else __fC_chunk = fA/__numprocs + 1;
if (__taskid == 0) {
    offset = 0;
    for (i=0; i < __numprocs; i++) {
        countsC[i] = __fC_chunk * cB;
        displsC[i] = offset;
        offset += __fC_chunk * cB;
    }
    countsC[__numprocs-1] = (fA - (__fC_chunk * (__numprocs-1))) * cB;
}
else if (__taskid == __numprocs-1)
	countsC[__taskid] = (fA - (__fC_chunk * (__numprocs-1))) * cB;
     else countsC[__taskid] = __fC_chunk * cB;

/* The other processes need matA and matB; only root needs matC */
if (__taskid != 0) {
	/* Broadcast can use the same buffer */
    matA = MATRIX2D(sizeof(float), fA, cA);
    matB = MATRIX2D(sizeof(float), fB, cB);
    matC = MATRIX2D(sizeof(float), 1, 1);
}
/* Auxiliary variables are needed to avoid aliasing in gatherv/scatterv */
    __matA = MATRIX2D(sizeof(float), __fA_chunk * __numprocs, cA);
    __matC = MATRIX2D(sizeof(float), __fC_chunk * __numprocs, cB);

float * matAaux = &matA[0][0];
float * matBaux = &matB[0][0];
float * matCaux = &matC[0][0];
float * __matAaux = &__matA[0][0];
float * __matCaux = &__matC[0][0];

MPI_Bcast(matBaux, fB*cB, MPI_FLOAT, 0, MPI_COMM_WORLD);

MPI_Scatterv(matAaux, countsA, displsA, MPI_FLOAT, __matAaux+(__fA_chunk*cA*__taskid), countsA[__taskid] , MPI_FLOAT, 0, MPI_COMM_WORLD);
__iter = (fA - 0) / __numprocs;
if ((fA - 0) % __numprocs != 0)
    __iter++;
__start = ( 0 + __iter * __taskid) ;
__end = __start + __iter ;
if (__end > fA)
    __end = fA;

#pragma omp parallel for private(r)
    for (i=__start; i<__end; i++)
       for (k=0; k<cA; k++) {
          r = __matAaux[i*cA+k];
          for (j=0; j<cB; j++)
               __matCaux[i*cB+j]+= r * matBaux[k*cB+j];
        }
MPI_Gatherv(__matCaux+(__fC_chunk*cB*__taskid), countsC[__taskid], MPI_FLOAT, matCaux, countsC, displsC, MPI_FLOAT, 0, MPI_COMM_WORLD);
}

}

void Mult_ijk(float **matA, float **matB, float **matC, int fA, int cA, int cB) {
    float result;
    int i,j,k;
    int fB = cA;

{
    int __start, __end, __iter;
    int __fA_chunk;
    int __fC_chunk;
    float ** __matA;
    float ** __matC;
    int *displsA = (int *)malloc(__numprocs*sizeof(int));
    int *countsA = (int *)malloc(__numprocs*sizeof(int));
    int *displsC = (int *)malloc(__numprocs*sizeof(int));
    int *countsC = (int *)malloc(__numprocs*sizeof(int));
    int offset;
MPI_Bcast(&fA, 1, MPI_INT, 0, MPI_COMM_WORLD);
MPI_Bcast(&cA, 1, MPI_INT, 0, MPI_COMM_WORLD);
MPI_Bcast(&fB, 1, MPI_INT, 0, MPI_COMM_WORLD);
MPI_Bcast(&cB, 1, MPI_INT, 0, MPI_COMM_WORLD);
if (fA%__numprocs == 0)
    __fA_chunk = fA/__numprocs;
else
     __fA_chunk = fA/__numprocs + 1;
if (__taskid == 0) {
    offset = 0;
    for (i=0; i < __numprocs; i++) {
        countsA[i] = __fA_chunk * cA;
        displsA[i] = offset;
        offset += __fA_chunk * cA;
    }
    countsA[__numprocs-1] = (fA - (__fA_chunk * (__numprocs-1))) * cA;
}
else if (__taskid == __numprocs-1)
	countsA[__taskid] = (fA - (__fA_chunk * (__numprocs-1))) * cA;
     else countsA[__taskid] = __fA_chunk * cA;

if (fA%__numprocs == 0)
    __fC_chunk = fA/__numprocs;
else
     __fC_chunk = fA/__numprocs + 1;
if (__taskid == 0) {
    offset = 0;
    for (i=0; i < __numprocs; i++) {
        countsC[i] = __fC_chunk * cB;
        displsC[i] = offset;
        offset += __fC_chunk * cB;
    }
    countsC[__numprocs-1] = (fA - (__fC_chunk * (__numprocs-1))) * cB;
}
else if (__taskid == __numprocs-1)
	countsC[__taskid] = (fA - (__fC_chunk * (__numprocs-1))) * cB;
     else countsC[__taskid] = __fC_chunk * cB;

if (__taskid != 0) {
	/* Broadcast can use the same buffer */
    matA = MATRIX2D(sizeof(float), fA, cA);
    matB = MATRIX2D(sizeof(float), fB, cB);
    matC = MATRIX2D(sizeof(float), 1, 1);
}
    __matA = MATRIX2D(sizeof(float), __fA_chunk * __numprocs, cA);
    __matC = MATRIX2D(sizeof(float), __fC_chunk * __numprocs, cB);

MPI_Bcast(&matB[0][0], fB*cB, MPI_FLOAT, 0, MPI_COMM_WORLD);

MPI_Scatterv(&matA[0][0], countsA, displsA, MPI_FLOAT, &__matA[__fA_chunk*__taskid][0],  countsA[__taskid], MPI_FLOAT, 0, MPI_COMM_WORLD);
__iter = (fA - 0) / __numprocs;
if ((fA - 0) % __numprocs != 0)
    __iter++;
__start = ( 0 + __iter * __taskid) ;
__end = __start + __iter ;
if (__end > fA)
    __end = fA;

#pragma omp parallel for private(result)
    for (i=__start; i<__end; i++) {
        for (j=0; j<cB; j++) {
            result = 0;
            for (k=0; k<cA; k++) {
                result += __matA[i][k] * matB[k][j];
            }
            __matC[i][j] = result;
        }
    }

MPI_Gatherv(&__matC[__fC_chunk*__taskid][0], countsC[__taskid], MPI_FLOAT, &matC[0][0], countsC, displsC, MPI_FLOAT, 0, MPI_COMM_WORLD);
}

}

int main (int argc, char* argv[])
{
    float ** matA;
    float ** matB;
    float ** matC;
    float ** matC_ikj;
/*
 * float ** matSubA;
 * float ** matSubC;
 * float ** matSubC_ikj;
 * int tag;
 */
    double start,finish;
 /*
  * int begin, end;
  * int n_rows;
  *  int size;
  *  int row=0, col=0;
  */
    int result;
    int F1, C1, F2, C2;
    int i, j, k;

    MPI_Init(&argc, &argv);

    MPI_Comm_size(MPI_COMM_WORLD, &__numprocs);

    MPI_Comm_rank(MPI_COMM_WORLD, &__taskid);

    if (__taskid == 0) {
        if ((argc != 4) && (argc != 5)) {
            printf("Introduce las dimensiones de la matriz A: M x N (separadas por un blanco\n");
            if (scanf("%d %d",&F1, &C1) <= 0)
                    printf("Error al leer las dimensiones de A\n");

            printf("Introduce las dimensiones de la matriz B: M x N (separadas por un blanco\n");
            if (scanf("%d %d",&F2, &C2) <= 0)
	                    printf("Error al leer las dimensiones de B\n");
        }
        else if (argc == 4) {
           F1=atoi(argv[1]); /* Number of rows in A */
           C1=atoi(argv[2]); /* Number of columns in A */
           C2=atoi(argv[3]); /* Number of columns in B */
           F2=C1;            /* Numero de fi */
        }
        else if (argc == 5) {
           F1=atoi(argv[1]); /* Number of rows in A */
           C1=atoi(argv[2]); /* Number of columns in A */
           F2=atoi(argv[3]); /* Number of columns in B */
           C2=atoi(argv[4]);            /* Numero de fi */
           if (C1 != F2)
               printf("Las dimensiones están mal, N1 debe ser igual a M2\n");

        }

	matA = MATRIX2D(sizeof(float), F1, C1); /* M1 * N1 */
	matB = MATRIX2D(sizeof(float), F2, C2);
	matC = MATRIX2D(sizeof(float), F1, C2); /* M1 * N2 */
	matC_ikj = MATRIX2D(sizeof(float), F1, C2); /* M1 * N2 */

        inicializarMatriz (matA, F1, C1);
        inicializarMatriz (matB, F2, C2);
    }

    start=MPI_Wtime();

    Mult_ijk(matA, matB, matC, F1, C1, C2);

    finish=MPI_Wtime();
    if (__taskid == 0)
        printf("Total time using ijk was %f seconds\n", finish-start);

    start=MPI_Wtime();

    Mult_ikj(matA, matB, matC_ikj, F1, C1, C2);

    finish=MPI_Wtime();

    if (__taskid == 0)
        printf("Total time using ikj was %f seconds\n", finish-start);

    if (__taskid == 0)
    {
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
	    printf("Test Failed in ijk!!!\n");
	else if (wrong == 2)
	    printf("Test Failed in ikj!!!\n");
	else printf("Test Passed!!!\n");
    }
    MPI_Finalize();

    return(0);
}
