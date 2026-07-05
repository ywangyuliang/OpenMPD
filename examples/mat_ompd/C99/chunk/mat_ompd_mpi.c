#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <mpi.h>

#define BACK_SIZE N2+1
#define GO_SIZE 1

#define WORKING		100
#define END		10
#define MAGIC		63

int __taskid = -1, __numprocs = -1;

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
    int fB = cA;

     /*
      * C must be zero-initialized first
      * for (i=0; i< fC; i++)
      *   for (j=0; j< cC; j++)
      *       C[i*cC+j] = 0;
      */
{
MPI_Bcast(&fA, 1, MPI_INT, 0, MPI_COMM_WORLD);
MPI_Bcast(&cA, 1, MPI_INT, 0, MPI_COMM_WORLD);
MPI_Bcast(&fB, 1, MPI_INT, 0, MPI_COMM_WORLD);
MPI_Bcast(&cB, 1, MPI_INT, 0, MPI_COMM_WORLD);

/* The other processes need matA, matB and matC */
if (__taskid != 0) {
    matA =     calloc(fA,cA*sizeof(float)); /* M1 * N1 */
    matB =     calloc(fB,cB*sizeof(float)); /* M2 * N2 */
    matC =     calloc(fA,cB*sizeof(float)); /* M1 * N2 */
}
{
int *__displsmatA = (int *)malloc(sizeof(int)*__numprocs);
int *__countsmatA = (int *)malloc(sizeof(int)*__numprocs);
int __offsetmatA = 0;
/* Auxiliary variables are needed to avoid aliasing in gatherv/scatterv */
float (*__matA)[cA];
if (__taskid == 0)
  __matA = calloc(fA,cA*sizeof(float));

if (__taskid == 0)
  memcpy(__matA, matA, fA * cA * sizeof(float));

while (__offsetmatA < fA*cA) {
  if (__taskid == 0) {
    for (int __gather = 0; __gather < __numprocs; __gather++) {
        if (__offsetmatA < fA*cA) {
            __countsmatA[__gather] = cA; /* chunk  */
            __displsmatA[__gather] = __offsetmatA;
            __offsetmatA += cA;
        }
        else {
            __countsmatA[__gather] = 0;
            __displsmatA[__gather] = fA*cA;
        }
    }
  }
  else {
        if (__offsetmatA + __taskid * cA < fA*cA) {
            __countsmatA[__taskid] = cA; /* chunk  */
            __displsmatA[__taskid] = __offsetmatA + __taskid * cA;
            __offsetmatA += __numprocs * cA;
        }
        else {
            __countsmatA[__taskid] = 0;
            __displsmatA[__taskid] = fA*cA;
            __offsetmatA += __numprocs * cA;
        }
  }

/*
 * printf("task %d: countsmatA[%d] = %d, displsmatA[%d] = %d\n", __taskid, __taskid, __countsmatA[__taskid], __taskid, __displsmatA[__taskid]);
 * 	for (i=1; i < __numprocs; i++)
 * / 	    printf("task %d: countsA[%d] = %d, displsA[%d] = %d\n", __taskid, i, __countsmatA[i], i, __displsmatA[i]);
 */

  MPI_Scatterv(__matA, __countsmatA, __displsmatA, MPI_FLOAT, &matA[0][0] + __displsmatA[__taskid], __countsmatA[__taskid], MPI_FLOAT, 0, MPI_COMM_WORLD);

}
/*
 * 	printf("Scatter task %d: matA[5][1] %f matA[9][1] %f\n", __taskid, matA[5][1], matA[9][1]);
 * 	printf("Scatter task %d:  __matA[5][1] %f __matA[9][1] %f\n", __taskid, __matA[5][1], __matA[9][1]);
 * 	printf("Scatter task %d: matA[4][1] %f matA[8][1] %f\n", __taskid, matA[4][1], matA[8][1]);
 */

}

/*
 * printf("task %d: countsC[%d] = %d, displsC[%d] = %d\n", __taskid, __taskid, countsC[__taskid], __taskid, displsC[__taskid]);
 * 	for (i=1; i < __numprocs; i++)
 * 	    printf("task %d: countsC[%d] = %d, displsC[%d] = %d\n", __taskid, i, countsC[i], i, displsC[i]);
 */

/* Broadcast can use the same buffer */
MPI_Bcast(matB, fB*cB, MPI_FLOAT, 0, MPI_COMM_WORLD);
	/* printf("Scatter task %d: matA[1][1] %f __matA[1][1] %f matA[4][1] %f __matA[4][1] %f\n", __taskid, matA[1][1], __matA[1][1], matA[4][1], __matA[4][1]); */
{
	int __i;
	int __start = 0 + __taskid * 1;   /* chunk 1 */
	int __end = fA;
	int __step = __numprocs * 1 * 1; /* chunk 1 y step 1   */
#pragma omp parallel for private(r)
	for (__i=__start;__i< __end; __i+= __step) {
	   for (i=__i;i< (__i+1*1); i+= 1) {
		for (k=0; k<cA; k++) {
		r = matA[i][k];
		for (j=0; j<cB; j++)
		matC[i][j]+= r * matB[k][j];
	}
          }
	}
}
{
int *__displsmatC = (int *)malloc(sizeof(int)*__numprocs);
int *__countsmatC = (int *)malloc(sizeof(int)*__numprocs);
int __offsetmatC = 0;
float (*__matC)[cB];
if (__taskid == 0)
  __matC = calloc(fA,cB*sizeof(float));

while (__offsetmatC < fA*cB) {
  if (__taskid == 0) {
    for (int __gather = 0; __gather < __numprocs; __gather++) {
        if (__offsetmatC < fA*cB) {
            __countsmatC[__gather] = cB; /* chunk  */
            __displsmatC[__gather] = __offsetmatC;
            __offsetmatC += cB;
        }
        else {
            __countsmatC[__gather] = 0;
            __displsmatC[__gather] = fA*cB;
        }
    }
  }
  else {
        if (__offsetmatC + __taskid * cB < fA*cB) {
            __countsmatC[__taskid] = cB; /* chunk  */
            __displsmatC[__taskid] = __offsetmatC + __taskid * cB;
            __offsetmatC += __numprocs * cB;
        }
        else {
            __countsmatC[__taskid] = 0;
            __displsmatC[__taskid] = fA*cB;
            __offsetmatC += __numprocs * cB;
        }
  }

  MPI_Gatherv(&matC[0][0] + __displsmatC[__taskid], __countsmatC[__taskid], MPI_FLOAT, __matC, __countsmatC, __displsmatC, MPI_FLOAT, 0, MPI_COMM_WORLD);

}
  if (__taskid == 0)
	memcpy(matC, __matC, fA * cB * sizeof(float));
}
/*
 * 	printf("Gather task %d: matC[1][1] %f __matC[1][1] %f matC[4][1] %f __matC[4][1] %f\n", __taskid, matC[1][1], __matC[1][1], matC[4][1], __matC[4][1]);
 * 	printf("Gather task %d: matC[5][1] %f matC[9][1] %f\n", __taskid, matC[5][1], matC[9][1]);
 * 	printf("Gather task %d:  __matC[5][1] %f __matC[9][1] %f\n", __taskid, __matC[5][1], __matC[9][1]);
 */

}

}

void Mult_ijk(int fA, int cA, int cB, float matA[fA][cA], float matB[cA][cB], float matC[fA][cB]) {
    float result;
    int i,j,k;
    int fB = cA;
MPI_Bcast(&fA, 1, MPI_INT, 0, MPI_COMM_WORLD);
MPI_Bcast(&cA, 1, MPI_INT, 0, MPI_COMM_WORLD);
MPI_Bcast(&fB, 1, MPI_INT, 0, MPI_COMM_WORLD);
MPI_Bcast(&cB, 1, MPI_INT, 0, MPI_COMM_WORLD);

if (__taskid != 0) {
	/* Broadcast can use the same buffer */
    matA =     calloc(fA,cA*sizeof(float)); /* M1 * N1 */
    matB =     calloc(fB,cB*sizeof(float)); /* M2 * N2 */
    matC =     calloc(fA,cB*sizeof(float)); /* M1 * N2 */
}
{
int *__displsmatA = (int *)malloc(sizeof(int)*__numprocs);
int *__countsmatA = (int *)malloc(sizeof(int)*__numprocs);
int __offsetmatA = 0;
/* Auxiliary variables are needed to avoid aliasing in gatherv/scatterv */
float (*__matA)[cA];
if (__taskid == 0)
  __matA = calloc(fA,cA*sizeof(float));

if (__taskid == 0)
  memcpy(__matA, matA, fA * cA * sizeof(float));

while (__offsetmatA < fA*cA) {
  if (__taskid == 0) {
    for (int __gather = 0; __gather < __numprocs; __gather++) {
        if (__offsetmatA < fA*cA) {
            __countsmatA[__gather] = cA; /* chunk  */
            __displsmatA[__gather] = __offsetmatA;
            __offsetmatA += cA;
        }
        else {
            __countsmatA[__gather] = 0;
            __displsmatA[__gather] = fA*cA;
        }
    }
  }
  else {
        if (__offsetmatA + __taskid * cA < fA*cA) {
            __countsmatA[__taskid] = cA; /* chunk  */
            __displsmatA[__taskid] = __offsetmatA + __taskid * cA;
            __offsetmatA += __numprocs * cA;
        }
        else {
            __countsmatA[__taskid] = 0;
            __displsmatA[__taskid] = fA*cA;
            __offsetmatA += __numprocs * cA;
        }
  }

/*
 * printf("task %d: countsmatA[%d] = %d, displsmatA[%d] = %d\n", __taskid, __taskid, __countsmatA[__taskid], __taskid, __displsmatA[__taskid]);
 * 	for (i=1; i < __numprocs; i++)
 * 	    printf("task %d: countsA[%d] = %d, displsA[%d] = %d\n", __taskid, i, __countsmatA[i], i, __displsmatA[i]);
 */

  MPI_Scatterv(__matA, __countsmatA, __displsmatA, MPI_FLOAT, &matA[0][0] + __displsmatA[__taskid], __countsmatA[__taskid], MPI_FLOAT, 0, MPI_COMM_WORLD);

}
/*
 * 	printf("Scatter task %d: matA[5][1] %f matA[9][1] %f\n", __taskid, matA[5][1], matA[9][1]);
 * 	printf("Scatter task %d:  __matA[5][1] %f __matA[9][1] %f\n", __taskid, __matA[5][1], __matA[9][1]);
 * 	printf("Scatter task %d: matA[4][1] %f matA[8][1] %f\n", __taskid, matA[4][1], matA[8][1]);
 */

}

MPI_Bcast(matB, fB*cB, MPI_FLOAT, 0, MPI_COMM_WORLD);
/*
 * 	printf("Scatter task %d: matA[1][1] %f __matA[1][1] %f matA[4][1] %f __matA[4][1] %f\n", __taskid, matA[1][1], __matA[1][1], matA[4][1], __matA[4][1]);
 * 	printf("Scatter task %d: matA[5][1] %f matA[9][1] %f\n", __taskid, matA[5][1], matA[9][1]);
 * 	printf("Scatter task %d:  __matA[5][1] %f __matA[9][1] %f\n", __taskid, __matA[5][1], __matA[9][1]);
 */

/*  printf("__matA[5][1] %f, __matA[5][2] %f __matA[5][3] %f __matA[5][4] %f __matA[5][5] %f\n", __matA[5][1], __matA[5][2], __matA[5][3], __matA[5][4], __matA[5][5]); */
{
        int __i;
        int __start = 0 + __taskid * 1;   /* chunk 1 */
        int __end = fA;
        int __step = __numprocs * 1 * 1; /* chunk 1 y step 1 */
#pragma omp parallel for private(result)
    for (__i=__start; __i<__end; __i+=__step) {
        for (i=__i;i< (__i+1*1); i+= 1) {
            for (j=0; j<cB; j++) {
                result = 0.0;
                for (k=0; k<cA; k++) {
                    result += matA[i][k] * matB[k][j];
                }
                matC[i][j] = result;
            }
	}
    }
}
{
int *__displsmatC = (int *)malloc(sizeof(int)*__numprocs);
int *__countsmatC = (int *)malloc(sizeof(int)*__numprocs);
int __offsetmatC = 0;
float (*__matC)[cB];
if (__taskid == 0)
  __matC = calloc(fA,cB*sizeof(float));

while (__offsetmatC < fA*cB) {
  if (__taskid == 0) {
    for (int __gather = 0; __gather < __numprocs; __gather++) {
        if (__offsetmatC < fA*cB) {
            __countsmatC[__gather] = cB; /* chunk  */
            __displsmatC[__gather] = __offsetmatC;
            __offsetmatC += cB;
        }
        else {
            __countsmatC[__gather] = 0;
            __displsmatC[__gather] = fA*cB;
        }
    }
  }
  else {
        if (__offsetmatC + __taskid * cB < fA*cB) {
            __countsmatC[__taskid] = cB; /* chunk  */
            __displsmatC[__taskid] = __offsetmatC + __taskid * cB;
            __offsetmatC += __numprocs * cB;
        }
        else {
            __countsmatC[__taskid] = 0;
            __displsmatC[__taskid] = fA*cB;
            __offsetmatC += __numprocs * cB;
        }
  }

  MPI_Gatherv(&matC[0][0] + __displsmatC[__taskid], __countsmatC[__taskid], MPI_FLOAT, __matC, __countsmatC, __displsmatC, MPI_FLOAT, 0, MPI_COMM_WORLD);

}
if (__taskid == 0)
	memcpy(matC, __matC, fA * cB * sizeof(float));
 /*
  * 	printf("Gather task %d: matC[0][1] %f __matC[0][1] %f matC[4][1] %f __matC[4][1] %f\n", __taskid, matC[0][1], __matC[0][1], matC[4][1], __matC[4][1]);
  * 	printf("Gather task %d: matC[5][1] %f matC[9][1] %f\n", __taskid, matC[5][1], matC[9][1]);
  * 	printf("Gather task %d:  __matC[5][1] %f __matC[9][1] %f\n", __taskid, __matC[5][1], __matC[9][1]);
  */
}

}

int main (int argc, char* argv[])
{
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
    float result;
    int F1, C1, F2, C2;
    int i, j, k;

    MPI_Init(&argc, &argv);

    MPI_Comm_size(MPI_COMM_WORLD, &__numprocs);

    MPI_Comm_rank(MPI_COMM_WORLD, &__taskid);

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

    float (*matA)[C1];
    float (*matB)[C2];
    float (*matC)[C2];
    float (*matC_ikj)[C2];

    if (__taskid == 0) {
        matA =     calloc(F1,C1*sizeof(float)); /* M1 * N1 */
        matB =     calloc(F2,C2*sizeof(float)); /* M2 * N2 */
        matC =     calloc(F1,C2*sizeof(float)); /* M1 * N2 */
        matC_ikj = calloc(F1,C2*sizeof(float)); /* M1 * N2 */
        inicializarMatriz (F1, C1, matA);
        inicializarMatriz (F2, C2, matB);
    }

    start=MPI_Wtime();

    Mult_ijk(F1, C1, C2, matA, matB, matC);

    finish=MPI_Wtime();
    if (__taskid == 0)
        printf("Total time using ijk was %f seconds\n", finish-start);

    start=MPI_Wtime();

    Mult_ikj(F1, C1, C2, matA, matB, matC_ikj);

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
                if (matC[i][j] != result) {
/* 			printf("result %f, matC[%d][%d] %f\n", result, i, j, matC[i][j]); */
		    wrong = 1;
		}
                if (matC_ikj[i][j] != result) {
/* 			printf("result %f, matC_ikj[%d][%d] %f\n", result, i, j, matC_ikj[i][j]); */
		    wrong = 2;
		}
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
