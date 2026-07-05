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
     int cC = cB; /* number of columns in C == columns in B */

     /*
      * C must be zero-initialized first
      * for (i=0; i< fC; i++)
      *   for (j=0; j< cC; j++)
      *       C[i*cC+j] = 0;
      */

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
    float ** matSubA;
    float ** matSubC;
    float ** matSubC_ikj;
    int n_processes;
    int rank;
    int root=0;
/*    int tag; */
    double start,finish;
    int begin, end;
    int n_rows;
/*
 * int size;
 * int row=0, col=0;
 */
    int result;
    int F1, C1, F2, C2;
    int i, j, k;
    int dim[4];

    MPI_Init(&argc, &argv);

    MPI_Comm_size(MPI_COMM_WORLD, &n_processes);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == 0) {
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

	dim[0] = F1;
	dim[1] = C1;
	dim[2] = F2;
	dim[3] = C2;

        if (F1%n_processes == 0)
	    n_rows = F1/n_processes;
        else n_rows = F1/n_processes + 1;

	matA = MATRIX2D(sizeof(float), n_rows*n_processes, C1); /* M1 * N1 */
	matB = MATRIX2D(sizeof(float), F2, C2);
	matC = MATRIX2D(sizeof(float), n_rows*n_processes, C2); /* M1 * N2 */
	matC_ikj = MATRIX2D(sizeof(float), n_rows*n_processes, C2); /* M1 * N2 */

        inicializarMatriz (matA, F1, C1);
        inicializarMatriz (matB, F2, C2);
printf("Worker %d: A %dx%d y B %dx%d\n", rank, F1, C1, F2, C2);
    }

    start=MPI_Wtime();

    MPI_Bcast(dim, 4, MPI_INT, root, MPI_COMM_WORLD);

    if (rank != 0) {
	F1 = dim[0];
	C1 = dim[1];
	F2 = dim[2];
	C2 = dim[3];

        if (F1%n_processes == 0)
	    n_rows = F1/n_processes;
        else n_rows = F1/n_processes + 1;

	matB = MATRIX2D(sizeof(float), F2, C2);
/* 	matC = MATRIX2D(sizeof(float), n_rows*n_processes, C2); */
	matC = MATRIX2D(sizeof(float), 1, 1);
	matC_ikj = MATRIX2D(sizeof(float), 1, 1);
    }

    MPI_Bcast(&matB[0][0], F2*C2, MPI_FLOAT, root, MPI_COMM_WORLD);

    begin = n_rows * rank;
    end = begin + n_rows;
    if (end > F1)
	end = F1;

    matSubA = MATRIX2D(sizeof(float), n_rows, C1);
    matSubC = MATRIX2D(sizeof(float), n_rows, C2);
    matSubC_ikj = MATRIX2D(sizeof(float), n_rows, C2);

    MPI_Scatter(&matA[0][0], n_rows*C1, MPI_FLOAT, &matSubA[0][0], n_rows*C1, MPI_FLOAT, root, MPI_COMM_WORLD);

    Mult_ijk(matSubA, matB, matSubC, n_rows, C1, C2);

    MPI_Gather(&matSubC[0][0], n_rows*C2, MPI_FLOAT, &matC[0][0], n_rows*C2, MPI_FLOAT, root, MPI_COMM_WORLD);
    finish=MPI_Wtime();
    if (rank == 0)
        printf("Total time using ijk was %f seconds\n", finish-start);

    start=MPI_Wtime();

    MPI_Scatter(&matA[0][0], n_rows*C1, MPI_FLOAT, &matSubA[0][0], n_rows*C1, MPI_FLOAT, root, MPI_COMM_WORLD);

    Mult_ikj(&matSubA[0][0], &matB[0][0], &matSubC_ikj[0][0], n_rows, C1, C2);

    MPI_Gather(&matSubC_ikj[0][0], n_rows*C2, MPI_FLOAT, &matC_ikj[0][0], n_rows*C2, MPI_FLOAT, root, MPI_COMM_WORLD);
    finish=MPI_Wtime();

    if (rank == 0)
        printf("Total time using ikj was %f seconds\n", finish-start);
    MPI_Finalize();

    if (rank == 0)
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

    return(0);
}
