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

     // Es necesario inicializar previamente C a cero
     // for (i=0; i< fC; i++)
     //   for (j=0; j< cC; j++)
     //       C[i*cC+j] = 0;
{
    int __start, __end, __iter;
    int __fA_chunk;
    int __fC_chunk;
    int *displsA = (int *)malloc(__numprocs*sizeof(int));
    int *countsA = (int *)malloc(__numprocs*sizeof(int));
    int *displsC = (int *)malloc(__numprocs*sizeof(int));
    int *countsC = (int *)malloc(__numprocs*sizeof(int));
    int offset;
// #pragma omp cluster broad(fA, cA, fB, cB) scatter(matA[fA*cA]:chunk(n*cA)) broad(matB[fB*cB]) gather(matC[fA*cB]:chunk(n*cB))

MPI_Bcast(&fA, 1, MPI_INT, 0, MPI_COMM_WORLD);
MPI_Bcast(&cA, 1, MPI_INT, 0, MPI_COMM_WORLD);
MPI_Bcast(&fB, 1, MPI_INT, 0, MPI_COMM_WORLD);
MPI_Bcast(&cB, 1, MPI_INT, 0, MPI_COMM_WORLD);

//  El reparto correcto es fA * cA entre nprocs, pero eso no genera un
//  n de filas exacto si fA no es múltiplo de nprocs
//  Debe ser coherente con el reparto de las iteraciones del bucle en i
//  Con fA*cA o fA*cB genera mejor equilibrio, pero parte filas, inadmisible.
// if ((fA*cA)%__numprocs == 0)
//     __fA_chunk = (fA*cA)/__numprocs;
// else __fA_chunk = (fA*cA)/__numprocs + 1;
// if ((fA*cB)%__numprocs == 0)
//     __fC_chunk = (fA*cB)/__numprocs;
// else __fC_chunk = (fA*cB)/__numprocs + 1;

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
else if (__taskid == __numprocs-1) {
	countsA[__taskid] = (fA - (__fA_chunk * __taskid)) * cA;
     }
     else {
	countsA[__taskid] = __fA_chunk * cA;
     }

// printf("task %d: countsA[%d] = %d, displsA[%d] = %d\n", __taskid, __taskid, countsA[__taskid], __taskid, displsA[__taskid]);
// if (__taskid == 0) 
// 	for (i=1; i < __numprocs; i++)
// 	    printf("task %d: countsA[%d] = %d, displsA[%d] = %d\n", __taskid, i, countsA[i], i, displsA[i]);

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
else if (__taskid == __numprocs-1) {
	countsC[__taskid] = (fA - (__fC_chunk * (__numprocs-1))) * cB;
     }
     else {
	countsC[__taskid] = __fC_chunk * cB;
     }

// printf("task %d: countsC[%d] = %d, displsC[%d] = %d\n", __taskid, __taskid, countsC[__taskid], __taskid, displsC[__taskid]);
// if (__taskid == 0) 
// 	for (i=1; i < __numprocs; i++)
// 	    printf("task %d: countsC[%d] = %d, displsC[%d] = %d\n", __taskid, i, countsC[i], i, displsC[i]);

// Los otros procesos necesitaran matA y matB, solo el root matC
if (__taskid != 0) {
	// Broadcast puede serla misma
    matA =     calloc(fA,cA*sizeof(float)); // M1 * N1
    matB =     calloc(fB,cB*sizeof(float)); // M2 * N2
    matC =     calloc(fA,cB*sizeof(float)); // M1 * N2
}
// Para evitar alias en gatherv y scatterv necesitamos variables auxiliares
    float (*__matA)[cA];
    float (*__matC)[cB];
    __matA =     calloc(__fA_chunk * __numprocs,cA*sizeof(float)); // M1 * N1
    __matC =     calloc(__fC_chunk * __numprocs,cB*sizeof(float)); // M1 * N2


MPI_Bcast(matB, fB*cB, MPI_FLOAT, 0, MPI_COMM_WORLD);

MPI_Scatterv(matA, countsA, displsA, MPI_FLOAT, &(__matA[0][0])+(__fA_chunk*cA*__taskid), countsA[__taskid] , MPI_FLOAT, 0, MPI_COMM_WORLD);

// if (__taskid == 0)
	// printf("Scatter task %d: matA[1][1] %f __matA[1][1] %f matA[4][1] %f __matA[4][1] %f\n", __taskid, matA[1][1], __matA[1][1], matA[4][1], __matA[4][1]);
// if (__taskid == 0)
	// printf("Scatter task %d: matA[5][1] %f matA[9][1] %f\n", __taskid, matA[5][1], matA[9][1]);
// if (__taskid == 1)
	// printf("Scatter task %d:  __matA[5][1] %f __matA[9][1] %f\n", __taskid, __matA[5][1], __matA[9][1]);

// #pragma omp teams distribute 
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
          r = __matA[i][k];
          for (j=0; j<cB; j++)
               __matC[i][j]+= r * matB[k][j];
        }
MPI_Gatherv(&(__matC[0][0])+(__fC_chunk*cB*__taskid), countsC[__taskid], MPI_FLOAT, matC, countsC, displsC, MPI_FLOAT, 0, MPI_COMM_WORLD); 
// if (__taskid == 0)
// 	printf("Gather task %d: matC[1][1] %f __matC[1][1] %f matC[4][1] %f __matC[4][1] %f\n", __taskid, matC[1][1], __matC[1][1], matC[4][1], __matC[4][1]);
// if (__taskid == 0)
// 	printf("Gather task %d: matC[5][1] %f matC[9][1] %f\n", __taskid, matC[5][1], matC[9][1]);
// if (__taskid == 1)
// 	printf("Gather task %d:  __matC[5][1] %f __matC[9][1] %f\n", __taskid, __matC[5][1], __matC[9][1]);

}

}

void Mult_ijk(int fA, int cA, int cB, float matA[fA][cA], float matB[cA][cB], float matC[fA][cB]) {
    float result;
    int i,j,k;
    int fB = cA;

{
    int __start, __end, __iter;
    int __fA_chunk;
    int __fC_chunk;
    int *displsA = (int *)malloc(__numprocs*sizeof(int));
    int *countsA = (int *)malloc(__numprocs*sizeof(int));
    int *displsC = (int *)malloc(__numprocs*sizeof(int));
    int *countsC = (int *)malloc(__numprocs*sizeof(int));
    int offset;

// #pragma omp cluster broad(fA, cA, fB, cB) scatter(matA[fA][cA]:chunk(n*cA)) broad(matB[fB][cB]) gather(matC[fA][cB]:chunk(n*cB))
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
else if (__taskid == __numprocs-1) {
	countsA[__taskid] = (fA - (__fA_chunk * (__numprocs-1))) * cA;
     }
     else {
	countsA[__taskid] = __fA_chunk * cA;
     }

// printf("task %d: countsA[%d] = %d, displsA[%d] = %d\n", __taskid, __taskid, countsA[__taskid], __taskid, displsA[__taskid]);
// if (__taskid == 0) 
// 	for (i=1; i < __numprocs; i++)
// 	    printf("task %d: countsA[%d] = %d, displsA[%d] = %d\n", __taskid, i, countsA[i], i, displsA[i]);


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
else if (__taskid == __numprocs-1) {
	countsC[__taskid] = (fA - (__fC_chunk * (__numprocs-1))) * cB;
     }
     else {
	countsC[__taskid] = __fC_chunk * cB;
     }


// printf("task %d: countsC[%d] = %d, displsC[%d] = %d\n", __taskid, __taskid, countsC[__taskid], __taskid, displsC[__taskid]);
// if (__taskid == 0) 
// 	for (i=1; i < __numprocs; i++)
// 	    printf("task %d: countsC[%d] = %d, displsC[%d] = %d\n", __taskid, i, countsC[i], i, displsC[i]);

if (__taskid != 0) {
	// Broadcast puede serla misma
    matA =     calloc(fA,cA*sizeof(float)); // M1 * N1
    matB =     calloc(fB,cB*sizeof(float)); // M2 * N2
    matC =     calloc(fA,cB*sizeof(float)); // M1 * N2
}
// Para evitar alias en gatherv y scatterv necesitamos variables auxiliares
    float (*__matA)[cA];
    float (*__matC)[cB];
    __matA =     calloc(__fA_chunk * __numprocs,cA*sizeof(float)); // M1 * N1
    __matC =     calloc(__fC_chunk * __numprocs,cB*sizeof(float)); // M1 * N2

MPI_Bcast(matB, fB*cB, MPI_FLOAT, 0, MPI_COMM_WORLD);

MPI_Scatterv(matA, countsA, displsA, MPI_FLOAT, &(__matA[0][0])+(__fA_chunk*cA*__taskid),  countsA[__taskid], MPI_FLOAT, 0, MPI_COMM_WORLD);

// if (__taskid == 0)
// 	printf("Scatter task %d: matA[1][1] %f __matA[1][1] %f matA[4][1] %f __matA[4][1] %f\n", __taskid, matA[1][1], __matA[1][1], matA[4][1], __matA[4][1]);
// if (__taskid == 0)
// 	printf("Scatter task %d: matA[5][1] %f matA[9][1] %f\n", __taskid, matA[5][1], matA[9][1]);
// if (__taskid == 1)
// 	printf("Scatter task %d:  __matA[5][1] %f __matA[9][1] %f\n", __taskid, __matA[5][1], __matA[9][1]);

// #pragma omp teams distribute 
__iter = (fA - 0) / __numprocs;
if ((fA - 0) % __numprocs != 0)
    __iter++;
__start = ( 0 + __iter * __taskid) ;
__end = __start + __iter ;
if (__end > fA)
    __end = fA;

//if (__taskid == 1)
//  printf("__matA[5][1] %f, __matA[5][2] %f __matA[5][3] %f __matA[5][4] %f __matA[5][5] %f\n", __matA[5][1], __matA[5][2], __matA[5][3], __matA[5][4], __matA[5][5]);
#pragma omp parallel for private(result)
    for (i=__start; i<__end; i++) {
        for (j=0; j<cB; j++) {
            result = 0.0;
            for (k=0; k<cA; k++) {
                result += __matA[i][k] * matB[k][j];
            }
            __matC[i][j] = result;
        }
    }

MPI_Gatherv(&(__matC[0][0])+(__fC_chunk*cB*__taskid), countsC[__taskid], MPI_FLOAT, matC, countsC, displsC, MPI_FLOAT, 0, MPI_COMM_WORLD); 
// if (__taskid == 0)
// 	printf("Gather task %d: matC[1][1] %f __matC[1][1] %f matC[4][1] %f __matC[4][1] %f\n", __taskid, matC[1][1], __matC[1][1], matC[4][1], __matC[4][1]);
// if (__taskid == 0)
// 	printf("Gather task %d: matC[5][1] %f matC[9][1] %f\n", __taskid, matC[5][1], matC[9][1]);
// if (__taskid == 1)
// 	printf("Gather task %d:  __matC[5][1] %f __matC[9][1] %f\n", __taskid, __matC[5][1], __matC[9][1]);

}

}


int main (int argc, char* argv[])
{
//    float ** matSubA;
//    float ** matSubC;
//    float ** matSubC_ikj;
//    int tag;
//    MPI_Status status;
    double start,finish;
 //   int begin, end;
 //   int n_rows;
//    int size;
//    int row=0, col=0;
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
           F1=atoi(argv[1]); // Numero de filas de A
           C1=atoi(argv[2]); // Numero de columnas de A
           C2=atoi(argv[3]); // Numero de columnas de B
           F2=C1;            // Numero de fi
        }
        else if (argc == 5) {
           F1=atoi(argv[1]); // Numero de filas de A
           C1=atoi(argv[2]); // Numero de columnas de A
           F2=atoi(argv[3]); // Numero de columnas de B
           C2=atoi(argv[4]);            // Numero de fi
           if (C1 != F2)
               printf("Las dimensiones están mal, N1 debe ser igual a M2\n");

        }


    float (*matA)[C1];
    float (*matB)[C2];
    float (*matC)[C2];
    float (*matC_ikj)[C2];

    if (__taskid == 0) {
        matA =     calloc(F1,C1*sizeof(float)); // M1 * N1
        matB =     calloc(F2,C2*sizeof(float)); // M2 * N2
        matC =     calloc(F1,C2*sizeof(float)); // M1 * N2
        matC_ikj = calloc(F1,C2*sizeof(float)); // M1 * N2
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
//			printf("result %f, matC[%d][%d] %f\n", result, i, j, matC[i][j]);
		    wrong = 1;
		}
                if (matC_ikj[i][j] != result) {
//			printf("result %f, matC_ikj[%d][%d] %f\n", result, i, j, matC_ikj[i][j]);
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
