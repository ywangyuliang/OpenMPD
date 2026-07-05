/* ***************************************************************
 *  Matrix multiplication program: Version ikj
 *
 * RUN:  ./executable n_rowsA n_colsA n_colsB
 ******************************************************************/
#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
#include <cuda.h>

#define BLOCK_SIZE 32
/* Initialize matrices with generic values */

void inicializarMatrizRandom(float *M, int m, int n)
{
    int i;
    for (i = 0; i < m * n; i++)
    {
        M[i] = rand() % 1000;
    }
}
/*  Test kernel for GPU warm-up */
__global__ void cuda_hello(){
    printf("Hello World from GPU!\n");
}

__global__ void matrixMul(float *A, float *B, float *C, int fA, int cA, int cB)
{
    int row = blockIdx.y * blockDim.y + threadIdx.y;
    int col = blockIdx.x * blockDim.x + threadIdx.x;

    if (row < fA && col < cB)
    {
        float sum = 0;
        for (int i = 0; i < cA; i++)
        {
            sum += A[row * cA + i] * B[i * cB + col];
        }
        C[row * cB + col] = sum;
    }
}

/** Print the first n elements from the first m rows of matrix M
 *  cM is the number of columns in the matrix **/

void imprimeMat(float *M, int m, int n, int cM)
{
    int i, j;
    for (i = 0; i < m; i++)
    {
        for (j = 0; j < n; j++)
        {
            printf(" %4f ", M[i * cM + j]);
        }
        printf("\n");
    }
}

int main(int argc, char **argv)
{
    if (argc < 4)
    {
        printf("Uso: Ejecutable n_filas(A) n_columnas(A) n_columnas(B)\n");
        return (0);
    }

    int F1 = atoi(argv[1]); /* Number of rows in A */
    int C1 = atoi(argv[2]); /* Number of columns in A */
    int C2 = atoi(argv[3]); /* Number of columns in B */
    int F2 = C1;            /* Number of rows in B == number of columns in A */

    printf("**** MatMult: Version i,k,j *********\n");

    float *h_A; /* Matrix A */
    float *h_B; /* Matrix B */
    float *h_C; /* Matrix C */

    /* Allocate memory and zero matrices A, B and C */

    cudaMallocHost((void **)&h_A, F1 * C1 * sizeof(float));
    cudaMallocHost((void **)&h_B, F2 * C2 * sizeof(float));
    cudaMallocHost((void **)&h_C, F1 * C2 * sizeof(float));

    /* Initialize matrices A and B with random values */

    inicializarMatrizRandom(h_A, F1, C1);
    inicializarMatrizRandom(h_B, F2, C2);

    float gpu_elapsed_time_ms;

    cudaEvent_t start, stop;
    cudaEventCreate(&start);
    cudaEventCreate(&stop);

    cudaEventRecord(start, 0);

    /* Allocate device memory */
    float *d_A, *d_B, *d_C;
    cudaMalloc((void **)&d_A, sizeof(float) * F1 * C1);
    cudaMalloc((void **)&d_B, sizeof(float) * F2 * C2);
    cudaMalloc((void **)&d_C, sizeof(float) * F1 * C2);

    /* Copy matrices A and B to the device */
    cudaMemcpy(d_A, h_A, sizeof(float) * F1 * C1, cudaMemcpyHostToDevice);
    cudaMemcpy(d_B, h_B, sizeof(float) * F2 * C2, cudaMemcpyHostToDevice);

    unsigned int grid_rows = (F1 + BLOCK_SIZE - 1) / BLOCK_SIZE;
    unsigned int grid_cols = (C2 + BLOCK_SIZE - 1) / BLOCK_SIZE;

    dim3 dimGrid(grid_cols, grid_rows);
    dim3 dimBlock(BLOCK_SIZE, BLOCK_SIZE);
    printf("***** grid_cols = %d, grid_rows = %d\n", grid_cols, grid_rows);

    matrixMul<<<dimGrid, dimBlock>>>(d_A, d_B, d_C, F1, C1, C2);

    /* Copy the result from the device to the host */
    cudaMemcpy(h_C, d_C, sizeof(float) * F1 * C2, cudaMemcpyDeviceToHost);

    cudaEventRecord(stop, 0);
    cudaEventSynchronize(stop);

    cudaEventElapsedTime(&gpu_elapsed_time_ms, start, stop);
    printf("Tiempo de ejecucion en GPU: %f ms\n", gpu_elapsed_time_ms);

    /* Print a few matrix values */
    printf("\nValores de la matriz A\n");
    imprimeMat(h_A, 4, 5, C1);
    printf("\nValores de la matriz B\n");
    imprimeMat(h_B, 5, 3, C2);
    printf("\nValores de la matriz Resultado\n");
    imprimeMat(h_C, 4, 3, C2);

    cudaFreeHost(h_A);
    cudaFreeHost(h_B);
    cudaFreeHost(h_C);

    cudaFree(d_A);
    cudaFree(d_B);
    cudaFree(d_C);

    return (0);
}
