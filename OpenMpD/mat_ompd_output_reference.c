#include <string.h>
#include <assert.h>
#include <mpi.h>
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

void DeclareTypesMPI();

int __taskid = -1, __numprocs = -1;
int n = 1;

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
     int cC = cB; 
     int fC = fA; 
     int fB = cA;

     
     
     
     










MPI_Bcast(&fA, 1, MPI_INT, 0, MPI_COMM_WORLD);
MPI_Bcast(&cA, 1, MPI_INT, 0, MPI_COMM_WORLD);
MPI_Bcast(&fB, 1, MPI_INT, 0, MPI_COMM_WORLD);
MPI_Bcast(&cB, 1, MPI_INT, 0, MPI_COMM_WORLD);

{
	int __offset = 0;
	int *__displs = (int *) malloc(sizeof(int) * __numprocs);
	int *__counts = (int *) malloc(sizeof(int) * __numprocs);
	float *__matAAux;
	if (__taskid == 0) {
		__matAAux = (float *) malloc(sizeof(float)*fA*cA);
		memcpy(__matAAux, matA, sizeof(float)*fA*cA);
	}

	while (__offset < fA*cA) {
		if (__taskid == 0) {
			for (int __gather = 0; __gather < __numprocs; __gather++) {
				if (__offset < fA*cA) {
					__counts[__gather] = n*cA;
					__displs[__gather] = __offset;
					__offset += n*cA;
				}
				else {
					__counts[__gather] = 0;
					__displs[__gather] = fA*cA;
				}
			}
		}
		else {
			if (__offset + __taskid*n*cA < fA*cA) {
				__counts[__taskid] = n*cA;
				__displs[__taskid] = __offset + __taskid*n*cA;
				__offset += __numprocs*n*cA;
			}
			else {
				__counts[__taskid] = 0;
				__displs[__taskid] = fA*cA;
				__offset += __numprocs*n*cA;
			}
		}

		MPI_Scatterv(__matAAux, __counts, __displs, MPI_FLOAT, &matA[0]+__displs[__taskid], __counts[__taskid], MPI_FLOAT, 0, MPI_COMM_WORLD);
	}
}

MPI_Bcast(matB, fB*cB, MPI_FLOAT, 0, MPI_COMM_WORLD);

{
int __iter;
int __start;
int __end;
__iter = __numprocs * n;
__start = 0 + __taskid * n;
__end = fA;

	for (int __distribSched = __start; __distribSched < __end; __distribSched += __iter) {
	for (int __distrib = __distribSched; __distrib < __distribSched + n; __distrib++){if(__distrib >= __end) {continue;}
	#pragma omp parallel for private(r)
	        for (k=0; k<cA; k++) {
	          r = matA[__distrib*cA+k];
	          for (j=0; j<cB; j++)
	               matC[__distrib*cC+j]+= r * matB[k*cB+j];
	        }
}
	}
}
{
	int __offset = 0;
	int *__displs = (int *) malloc(sizeof(int) * __numprocs);
	int *__counts = (int *) malloc(sizeof(int) * __numprocs);
	float *__matCAux;
	if (__taskid == 0) {
		__matCAux = (float *) malloc(sizeof(float)*fA*cB);
	}

	while (__offset < fA*cB) {
		if (__taskid == 0) {
			for (int __gather = 0; __gather < __numprocs; __gather++) {
				if (__offset < fA*cB) {
					__counts[__gather] = n*cB;
					__displs[__gather] = __offset;
					__offset += n*cB;
				}
				else {
					__counts[__gather] = 0;
					__displs[__gather] = fA*cB;
				}
			}
		}
		else {
			if (__offset + __taskid*n*cB < fA*cB) {
				__counts[__taskid] = n*cB;
				__displs[__taskid] = __offset + __taskid*n*cB;
				__offset += __numprocs*n*cB;
			}
			else {
				__counts[__taskid] = 0;
				__displs[__taskid] = fA*cB;
				__offset += __numprocs*n*cB;
			}
		}

		MPI_Gatherv(&matC[0]+__displs[__taskid], __counts[__taskid], MPI_FLOAT, __matCAux, __counts, __displs, MPI_FLOAT, 0, MPI_COMM_WORLD);
	}
	if (__taskid == 0) {
		memcpy(matC, __matCAux, sizeof(float)*fA*cB);
	}
}

	}

	void Mult_ijk(float **matA, float **matB, float **matC, int fA, int cA, int cB) {
	    float result;
	    int i,j,k;
	    int cC = cB; 
	    int fC = fA; 
	    int fB = cA;



MPI_Bcast(&fA, 1, MPI_INT, 0, MPI_COMM_WORLD);
MPI_Bcast(&cA, 1, MPI_INT, 0, MPI_COMM_WORLD);
MPI_Bcast(&fB, 1, MPI_INT, 0, MPI_COMM_WORLD);
MPI_Bcast(&cB, 1, MPI_INT, 0, MPI_COMM_WORLD);

{
	int __offset = 0;
	int *__displs = (int *) malloc(sizeof(int) * __numprocs);
	int *__counts = (int *) malloc(sizeof(int) * __numprocs);
	float *__matAAux;
	if (__taskid == 0) {
		__matAAux = (float *) malloc(sizeof(float)*fA*cA);
		memcpy(__matAAux, &matA[0][0], sizeof(float)*fA*cA);
	}

	while (__offset < fA*cA) {
		if (__taskid == 0) {
			for (int __gather = 0; __gather < __numprocs; __gather++) {
				if (__offset < fA*cA) {
					__counts[__gather] = n*cA;
					__displs[__gather] = __offset;
					__offset += n*cA;
				}
				else {
					__counts[__gather] = 0;
					__displs[__gather] = fA*cA;
				}
			}
		}
		else {
			if (__offset + __taskid*n*cA < fA*cA) {
				__counts[__taskid] = n*cA;
				__displs[__taskid] = __offset + __taskid*n*cA;
				__offset += __numprocs*n*cA;
			}
			else {
				__counts[__taskid] = 0;
				__displs[__taskid] = fA*cA;
				__offset += __numprocs*n*cA;
			}
		}

		MPI_Scatterv(__matAAux, __counts, __displs, MPI_FLOAT, &matA[0][0]+__displs[__taskid], __counts[__taskid], MPI_FLOAT, 0, MPI_COMM_WORLD);
	}
}

MPI_Bcast(&matB[0][0], fB*cB, MPI_FLOAT, 0, MPI_COMM_WORLD);

{
int __iter;
int __start;
int __end;
__iter = __numprocs * n;
__start = 0 + __taskid * n;
__end = fA;

	for (int __distribSched = __start; __distribSched < __end; __distribSched += __iter) {
	for (int __distrib = __distribSched; __distrib < __distribSched + n; __distrib++){if(__distrib >= __end) {continue;}
	#pragma omp parallel for private(result)
	        for (j=0; j<cB; j++) {
	            result = 0;
	            for (k=0; k<cA; k++) {
	                result += matA[__distrib][k] * matB[k][j];
	            }
	            matC[__distrib][j] = result;
	        }
	    }
	}
}
{
	int __offset = 0;
	int *__displs = (int *) malloc(sizeof(int) * __numprocs);
	int *__counts = (int *) malloc(sizeof(int) * __numprocs);
	float *__matCAux;
	if (__taskid == 0) {
		__matCAux = (float *) malloc(sizeof(float)*fA*cB);
	}

	while (__offset < fA*cB) {
		if (__taskid == 0) {
			for (int __gather = 0; __gather < __numprocs; __gather++) {
				if (__offset < fA*cB) {
					__counts[__gather] = n*cB;
					__displs[__gather] = __offset;
					__offset += n*cB;
				}
				else {
					__counts[__gather] = 0;
					__displs[__gather] = fA*cB;
				}
			}
		}
		else {
			if (__offset + __taskid*n*cB < fA*cB) {
				__counts[__taskid] = n*cB;
				__displs[__taskid] = __offset + __taskid*n*cB;
				__offset += __numprocs*n*cB;
			}
			else {
				__counts[__taskid] = 0;
				__displs[__taskid] = fA*cB;
				__offset += __numprocs*n*cB;
			}
		}

		MPI_Gatherv(&matC[0][0]+__displs[__taskid], __counts[__taskid], MPI_FLOAT, __matCAux, __counts, __displs, MPI_FLOAT, 0, MPI_COMM_WORLD);
	}
	if (__taskid == 0) {
		memcpy(&matC[0][0], __matCAux, sizeof(float)*fA*cB);
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


	MPI_Init(&argc,&argv);
	MPI_Comm_size(MPI_COMM_WORLD,&__numprocs);
	MPI_Comm_rank(MPI_COMM_WORLD,&__taskid);
	DeclareTypesMPI();
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
	           F1=atoi(argv[1]); 
	           C1=atoi(argv[2]); 
	           C2=atoi(argv[3]); 
	           F2=C1;            
		} 
	        else if (argc == 5) {	
	           F1=atoi(argv[1]); 
	           C1=atoi(argv[2]); 
	           F2=atoi(argv[3]); 
	           C2=atoi(argv[4]); 
	           if (C1 != F2)
		       printf("Las dimensiones están mal, N1 debe ser igual a M2\n");

		}
}
MPI_Bcast(&F1, 1, MPI_INT, 0, MPI_COMM_WORLD);
MPI_Bcast(&C1, 1, MPI_INT, 0, MPI_COMM_WORLD);
MPI_Bcast(&F2, 1, MPI_INT, 0, MPI_COMM_WORLD);
MPI_Bcast(&C2, 1, MPI_INT, 0, MPI_COMM_WORLD);

if (__taskid == 0) {

}
	{
		matA = MATRIX2D(sizeof(float), F1, C1); 
		matB = MATRIX2D(sizeof(float), F2, C2);
		matC = MATRIX2D(sizeof(float), F1, C2); 
		matC_ikj = MATRIX2D(sizeof(float), F1, C2); 
	}
if (__taskid == 0) {

		inicializarMatriz (matA, F1, C1);
		inicializarMatriz (matB, F2, C2);

	    gettimeofday(&t, NULL);
}
	        Mult_ijk(matA, matB, matC, F1, C1, C2);
if (__taskid == 0) {
	    gettimeofday(&t2, NULL);
	    segundos = (((t2.tv_usec - t.tv_usec)/1000000.0f)  + (t2.tv_sec - t.tv_sec));
	    printf("Total time using ijk was %f seconds\n", segundos);
	    
	    gettimeofday(&t, NULL);
}
	        Mult_ikj(&matA[0][0], &matB[0][0], &matC_ikj[0][0], F1, C1, C2);
if (__taskid == 0) {
	    gettimeofday(&t2, NULL);
	    segundos = (((t2.tv_usec - t.tv_usec)/1000000.0f)  + (t2.tv_sec - t.tv_sec));
	    printf("Total time using ikj was %f seconds\n", segundos);
	    

}
		int wrong = 0;
if (__taskid == 0) {
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

	    if (F1<10){ 
	      printf("\nValores de la matriz A\n");
	      imprimeMat(matA, 4, 5);
	      printf("\nValores de la matriz B\n");
	      imprimeMat(matB, 5, 3);
	      printf("\nValores de la matriz C\n");
	      imprimeMat(matC, 4, 3);
	    }
	 
}
	MPI_Finalize();
    return(0);
}


void DeclareTypesMPI() {
}
