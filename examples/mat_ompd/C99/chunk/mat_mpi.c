#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <mpi.h>

#define BACK_SIZE N2+1
#define GO_SIZE 1

#define WORKING		100
#define END		10
#define MAGIC		63

void imprimeMat (int cM, float mat[][cM], int F, int C){
    int i, j;
    for (i=0; i<F; i++) {
        for (j=0; j<C; j++) {
            printf(" %4f ", mat[i][j]);
        }
        printf("\n");
    }
}

void inicializarMatriz (int F, int C, float mat[F][C]){
    int i, j;
    for (i=0; i<F; i++)
        for (j=0; j<C; j++)
            mat[i][j] = (i*F+j) % MAGIC;
}

void Mult_ikj(int fA, int cA, int cB, float matA[fA][cA], float matB[cA][cB], float matC[fA][cB]) {
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
          r = matA[i][k];
          for (j=0; j<cB; j++)
               matC[i][j]+= r * matB[k][j];
        }
}

void Mult_ijk(int fA, int cA, int cB, float matA[fA][cA], float matB[cA][cB], float matC[fA][cB]) {
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

	printf("Worker %d: A %dx%d y B %dx%d\n", rank, F1, C1, F2, C2);
    }

    MPI_Bcast(dim, 4, MPI_INT, root, MPI_COMM_WORLD);

    if (rank != 0) {
	F1 = dim[0];
	C1 = dim[1];
	F2 = dim[2];
	C2 = dim[3];
    }

    float (*matA)[C1];
    float (*matB)[C2];
    float (*matC)[C2];
    float (*matC_ikj)[C2];
    float (*matSubA)[C1];
    float (*matSubC)[C2];
    float (*matSubC_ikj)[C2];
    int F1aux = n_rows*n_processes;

    if (rank == 0)  {
        matB =     calloc(F2,   C2*sizeof(float)); /* M2 * N2 */
        matA =     calloc(F1aux,C1*sizeof(float)); /* M1 * N1 */
        matC =     calloc(F1aux,C2*sizeof(float)); /* M1 * N2 */
        matC_ikj = calloc(F1aux,C2*sizeof(float)); /* M1 * N2 */

        inicializarMatriz (F1, C1, matA);
        inicializarMatriz (F2, C2, matB);
    }

    start=MPI_Wtime();

    if (rank != 0) {
        if (F1%n_processes == 0)
	    n_rows = F1/n_processes;
        else n_rows = F1/n_processes + 1;

        matB =     calloc(F2,C2*sizeof(float)); /* M2 * N2 */
        matC =     calloc(F1,1*sizeof(float)); /* 1 * 1 */
        matC_ikj = calloc(F1,1*sizeof(float)); /* 1 * 1 */

    }

    MPI_Bcast(matB, F2*C2, MPI_FLOAT, root, MPI_COMM_WORLD);

    begin = n_rows * rank;
    end = begin + n_rows;
    if (end > F1)
	end = F1;

    matSubA =     calloc(n_rows,C1*sizeof(float)); /* n_rows * C1 */
    matSubC =     calloc(n_rows,C2*sizeof(float)); /* n_rows * C2 */
    matSubC_ikj = calloc(n_rows,C2*sizeof(float)); /* n_rows * C2 */

    MPI_Scatter(matA, n_rows*C1, MPI_FLOAT, matSubA, n_rows*C1, MPI_FLOAT, root, MPI_COMM_WORLD);

    Mult_ijk(n_rows, C1, C2, matSubA, matB, matSubC);

    MPI_Gather(matSubC, n_rows*C2, MPI_FLOAT, matC, n_rows*C2, MPI_FLOAT, root, MPI_COMM_WORLD);
    finish=MPI_Wtime();
    if (rank == 0)
        printf("Total time using ijk was %f seconds\n", finish-start);

    start=MPI_Wtime();

    MPI_Scatter(matA, n_rows*C1, MPI_FLOAT, matSubA, n_rows*C1, MPI_FLOAT, root, MPI_COMM_WORLD);

    Mult_ikj(n_rows, C1, C2, matSubA, matB, matSubC_ikj);

    MPI_Gather(matSubC_ikj, n_rows*C2, MPI_FLOAT, matC_ikj, n_rows*C2, MPI_FLOAT, root, MPI_COMM_WORLD);
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
