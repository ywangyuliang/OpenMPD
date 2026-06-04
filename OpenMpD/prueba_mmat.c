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
     int cC = cB; // numero de columnas de C == columnas de B
     int fC = fA; // numero de filas de C == filas de A
     int fB = cA;
     // Es necesario inicializar previamente C a cero
     // for (i=0; i< fC; i++)
     //   for (j=0; j< cC; j++)
     //       C[i*cC+j] = 0;

//  Si no se lleva cuidado la división de la matriz, al ser 1D, no coincide con las filas
//  Lo normal es dividir fA*cA entre nprocs, pero eso no tiene que coincidir con un nº de filas exacto
//  Debe buscarse un chunk que asegure filas completas en cada proceso para que luego fuucione el producto
//  Como el bucle a repatir entre procesos es el de i:0...fA eso es lo que debemos tener en cuenta
//  a la hora de calcular el chunk
/// Chunk: n*fA, con n: fA/__numprocs (+1 si no exacto)
// Debería asignar esa n filas a cada uno y el último lo que quedase

// #pragma omp cluster broad(fA, cA, fB, cB) scatter(matA[fA*cA]:chunk(n*cA)) broad(matB[fB*cB]) gather(matC[fA*cB]:chunk(n*cB))
#pragma omp cluster scatter(matA[fA][cA]:chunk(cA)) broad(matB[fB][cB]) gather(matC[fA][cB]:chunk(cB))
#pragma omp teams distribute dist_schedule(static,1)
#pragma omp parallel for private(r, j, k)
     for (i=0; i<fC; i++)
        for (k=0; k<cC; k++) {
          r = matA[i][k];
          for (j=0; j<cB; j++)
               matC[i][j] += r * matB[k][j];
        }
}

void Mult_ijk(int fA, int cA, int cB, float matA[fA][cA], float matB[cA][cB],
float matC[fA][cB]) {
    float result;
    int i,j,k;
    int cC = cB; // numero de columnas de C == columnas de B
    int fC = fA; // numero de filas de C == filas de A
    int fB = cA; 

// Ni OpenMP ni MPI se llevan bien como variables que no sean 1-D, contiguas
// Creo que no vale la pena plantearlo
// #pragma omp cluster broad(fA, cA, fB, cB) scatter(matA[fA][cA]:chunk(n*cA)) broad(matB[fB][cB]) gather(matC[fA][cB]:chunk(n*cB))
#pragma omp cluster scatter(matA[fA][cA]:chunk(cA)) broad(matB[fB][cB]) gather(matC[fA][cB]:chunk(cB))
#pragma omp teams distribute dist_schedule(static,1)
#pragma omp parallel for private(result, j, k)
    for (i=0; i<fC; i++) {
        for (j=0; j<cC; j++) {
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
		    printf("Error al leer las dimensiones de A\n");
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
#pragma omp cluster data broad(F1, C1, F2, C2)
    float (*matA)[C1] =     calloc(F1,C1*sizeof(float)); // M1 * N1
    float (*matB)[C2] =     calloc(F2,C2*sizeof(float)); // M2 * N2
    float (*matC)[C2] =     calloc(F1,C2*sizeof(float)); // M1 * N2
    float (*matC_ikj)[C2] = calloc(F1,C2*sizeof(float)); // M1 * N2
    inicializarMatriz (F1, C1, matA);
    inicializarMatriz (F2, C2, matB);

    gettimeofday(&t, NULL);
#pragma omp cluster
    Mult_ijk(F1, C1, C2, matA, matB, matC);
    gettimeofday(&t2, NULL);
    segundos = (((t2.tv_usec - t.tv_usec)/1000000.0f)  + (t2.tv_sec - t.tv_sec));
    printf("Total time using ijk was %f seconds\n", segundos);
    
    gettimeofday(&t, NULL);
#pragma omp cluster
    Mult_ikj(F1, C1, C2, matA, matB, matC_ikj);
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
	    printf("Test Failed in ikj!!!\n");
	else printf("Test Passed!!!\n");

    if (F1<10){ //Si las matrices son pequeñas, se muestran los valores por pantalla
      printf("\nValores de la matriz A\n");
      imprimeMat(C2, matA, 4, 5);
      printf("\nValores de la matriz B\n");
      imprimeMat(C2, matB, 5, 3);
      printf("\nValores de la matriz C\n");
      imprimeMat(C2, matC, 4, 3);
    }
 
    return(0);
}
