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

int main (int argc, char* argv[])
{
    int ** matA;
    int ** matB;
    int ** matC;
    int ** matSubA;
    int ** matSubC;
    int n_processes;
    int rank;
    int root=0;
    int tag;
    MPI_Status status;
    double start,finish;
    int begin, end;
    int size;
    int n_rows;
    int row=0, col=0;
    int result;
    int M1, N1, M2, N2;
    int i, j, k;
    int dim[3];

    MPI_Init(&argc, &argv); 

    MPI_Comm_size(MPI_COMM_WORLD, &n_processes);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == 0) {
        printf("Introduce las dimensiones de la matriz A: M x N (separadas por un blanco\n");
        scanf("%d %d",&M1, &N1);
        printf("Introduce las dimensiones de la matriz B: M x N (separadas por un blanco\n");
        scanf("%d %d",&M2, &N2);

        if (N1 != M2)
	    printf("Las dimensiones están mal, N1 debe ser igual a M2\n");
	dim[0] = M1;
	dim[1] = N1;
	dim[2] = N2;

        if (M1%n_processes == 0)
	    n_rows = M1/n_processes;
        else n_rows = M1/n_processes + 1;

	matA = MATRIX2D(sizeof(int), n_rows*n_processes, N1); // M1 * N1
	matB = MATRIX2D(sizeof(int), M2, N2);
	matC = MATRIX2D(sizeof(int), n_rows*n_processes, N2); // M1 * N2

        for (i=0; i<M1; i++) 
	    for (j=0; j<N1; j++) 
		matA[i][j] = (i*N1+j) % MAGIC;
        for (i=0; i<M2; i++) 
	    for (j=0; j<N2; j++) 
		matB[i][j] = (i*N2+j) % MAGIC;
    }

    
    start=MPI_Wtime();

    MPI_Bcast(dim, 3, MPI_INT, root, MPI_COMM_WORLD);

    if (rank != 0) {
	M1 = dim[0];
	N1 = dim[1];
	M2 = dim[1];
	N2 = dim[2];
	matB = MATRIX2D(sizeof(int), M2, N2);
//	matC = MATRIX2D(sizeof(int), M1, N2);
	matC = MATRIX2D(sizeof(int), 1, 1);
    }

    MPI_Bcast(&matB[0][0], M2*N2, MPI_INT, root, MPI_COMM_WORLD);

    if (M1%n_processes == 0)
	n_rows = M1/n_processes;
    else n_rows = M1/n_processes + 1;

    begin = n_rows * rank;
    end = begin + n_rows;
    if (end > M1)
	end = M1;

    matSubA = MATRIX2D(sizeof(int), n_rows, N1);
    matSubC = MATRIX2D(sizeof(int), n_rows, N2);

    MPI_Scatter(&matA[0][0], n_rows*N1, MPI_INT, &matSubA[0][0], n_rows*N1, MPI_INT, root, MPI_COMM_WORLD); 

    for (i=0; i<n_rows; i++) {
        for (j=0; j<N2; j++) {
            result = 0;
            for (k=0; k<N1; k++) {
                result += matSubA[i][k] * matB[k][j];
            }
            matSubC[i][j] = result;
        }
    }
printf("Worker %d ends: de %d a %d\n", rank, begin, end);

MPI_Barrier(MPI_COMM_WORLD);

    MPI_Gather(&matSubC[0][0], n_rows*N2, MPI_INT, &matC[0][0], n_rows*N2, MPI_INT, root, MPI_COMM_WORLD); 

    finish=MPI_Wtime();

    MPI_Finalize();

    if (rank == 0)
    {
	int wrong = 0;
        printf("Total time was %f seconds\n", finish-start);
        for (i=0; i<M1; i++) {
            for (j=0; j<N2; j++) {
                result = 0;
                for (k=0; k<N1; k++) {
                    result += matA[i][k] * matB[k][j];
                }
                if (matC[i][j] != result)
		    wrong = 1;
            }
        } 
	if (wrong)
	    printf("Test Failed!!!\n");
	else printf("Test Passed!!!\n");
    } 
 
    return(0);
}
