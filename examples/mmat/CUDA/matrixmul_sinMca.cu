/*
 * Matrix multiplication: P = M * N.
 * Host code
 */

/* includes, system */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>

#define HANDLE_ERROR( err ) (HandleError( err, __FILE__, __LINE__ ))

static void HandleError( cudaError_t err, const char *file, int line ) {
    if (err != cudaSuccess) {
        fprintf(stderr, "%s in %s at line %d\n", cudaGetErrorString( err ), file, line );
        exit( EXIT_FAILURE );
    }
}

#define CHECK_CUDA_ERROR( msg ) (checkCUDAError( msg, __FILE__, __LINE__ ))

static void checkCUDAError(const char *msg, const char *file, int line ) {
  cudaError_t err = cudaGetLastError();
  if (err != cudaSuccess) {
     fprintf(stderr, "Cuda error: %s: %s. In %s at line %d\n", msg,
                                        cudaGetErrorString(err), file, line );
     exit(EXIT_FAILURE);
  }
}

int InitMatrix ( float * matrix, const unsigned int M_WIDTH, const unsigned int M_HEIGHT)
{
    unsigned int i = 0, j = 0;

    for (i = 0; i < M_HEIGHT; i++) {
      for (j = 0; j < M_WIDTH; j++) {
        matrix[i*M_WIDTH + j] = floorf(100*(rand()/(float)RAND_MAX));
      }
    }
    return (1);
}

int GenMatrix ( float * matrix, const unsigned int M_WIDTH, const unsigned int M_HEIGHT)
{
    unsigned int i = 0, j = 0;

    for (i = 0; i < M_HEIGHT; i++) {
      for (j = 0; j < M_WIDTH; j++) {
        matrix[i*M_WIDTH + j] = i+j+1;
      }
    }
    return (1);
}

void CompareMatrix ( float * const m1, float * const m2, const unsigned int M_WIDTH, const unsigned int M_HEIGHT)
{
    unsigned int i = 0, j = 0, wrong = 0;
    int check_ok = 1;

    for (i = 0; i < M_HEIGHT && wrong < 15; i++) {
        for (j = 0; j < M_WIDTH && wrong < 15; j++) {
            if (m1[i*M_WIDTH+j] != m2[i*M_WIDTH+j]) {
                printf ("m1[%d][%d] != m2[%d][%d] : %f != %f\n",
                        i,j,i,j, m1[i*M_WIDTH+j], m2[i*M_WIDTH+j]);
                check_ok = 0; wrong++;
            }
        }
    }
    printf ("    Check ok? ");
    if (check_ok) printf ("Passed.\n");
    else printf ("Failed.\n");
}

float CheckSum(const float *matrix, const int width, const int height)
{
    int i, j;
    float s1, s2;

    for (i = 0, s1 = 0; i < height; i++) {
        for (j = 0, s2 = 0; j < width; j++) {
            s2 += matrix[i * width + j];
        }
        s1 += s2;
    }

    return s1;
}

/*
 * Compute reference data set
 * P = M * N
 * @param P          reference data, computed but preallocated
 * @param M          matrix M as provided to device
 * @param N          matrix N as provided to device
 * @param Mh         height of matrix M
 * @param Nw         width of matrix N
 */
void
computeGold(float* P, const float* M, const float* N, int Mh, int Mw, int Nw)
{
  int i, j, k;
  float sum, a, b;

  for (i = 0; i < Mh; i++)
    for (j = 0; j < Nw; j++)
      {
	    sum = 0;
	    for (k = 0; k < Mw; k++)
	    {
	        a = M[i * Mw + k];
	        b = N[k * Nw + j];
	        sum += a * b;
	    }
	    P[i * Nw + j] = (float)sum;
      }
}

/*
 * Matrix multiplication: P = M * N.
 * Device code
 */

/*
 * Simple test kernel for device functionality
 * @param g_idata  input data in global memory
 * @param g_odata  output data in global memory
 */

__global__ void matrixMul(float* R, const float* M, const float* N,
                          const int Mh, const int Mw, const int Nw,
                          const int block_size)
{
    const int bx = blockIdx.x;
    const int by = blockIdx.y;

    const int tx = threadIdx.x;
    const int ty = threadIdx.y;

    float Rvalue = 0;
    int i = 0, row = 0, col = 0;

    /* Index of the row of M loaded by this thread by the block */
    col = bx * blockDim.x + tx;

    /* Index of the column of N processed by the block */
    row = by * blockDim.y + ty;

    if ((row < Mh) && (col < Nw)) { 
        /* For each index from [0, Width of M] */
        for (i = 0; i < Mw; i++) {
            /*
             * Multiply the corresponding elements of M and N, and accumulate
             * into partial sum Psub
             */
            Rvalue += M[row*Mw + i] * N[i*Nw + col];
        }
        R[row*Nw+col] = Rvalue;
    }

}

/*
 * declaration, forward
 */
void runTest(int argc, char** argv);

/*
 * Program main
 */
int main(int argc, char** argv)
{
    runTest(argc, argv);
}

/*
 * Run a simple test for CUDA
 */
void runTest(int argc, char** argv)
{
    float * deviceM = NULL, * deviceN = NULL, * deviceP = NULL;
    float * hostM = NULL, * hostN = NULL, * hostP = NULL;
    int Mw = 0, Mh = 0, Nw = 0, Nh = 0, Pw = 0, Ph = 0;
    int block_size = 16; /* thread number = block_size^2 */
    int check=0;
    struct timeval t1, t2;
    double seconds;
    float gpu_elapsed_time_ms;
    cudaEvent_t start, stop;

    if ((argc != 4) && (argc != 5)) {
        fprintf(stderr, "Error: Wrong input parameter numbers.\n");
        fprintf(stderr, "Usage:\n"
                        "$> ./matrixmul RowsA ColsA ColsB [check]\n"
                        "Examples:\n"
                        "      $> ./matrixmul 1000 200 100 check\n"
                        );
        exit(1);
    }
    /* Initializing the GPU */
    HANDLE_ERROR(cudaMalloc((void**) &deviceP, sizeof(float)));

    cudaEventCreate(&start);
    cudaEventCreate(&stop);

    /* Note: Matrix width and height must be multiples of block size */
    Mh = Ph = atoi(argv[1]);
    Mw = Nh = atoi(argv[2]);
    Nw = Pw = atoi(argv[3]);

    if (argc == 5)
        if (strcmp("check", argv[4]) == 0)
            check = 1; /* True */
        else check = 0;

    if (check)
        printf("Setup host side environment and launch kernel:\n");

    /* allocate host memory for matrices M and N */
    if (check) {
        printf("  Allocate host memory for matrices M and N.\n");
        printf("    M: %d x %d\n", Mh, Mw);
        printf("    N: %d x %d\n", Nh, Nw);
    }
    unsigned int size_M = Mh * Mw;
    unsigned int mem_size_M = sizeof(float) * size_M;
    cudaMallocHost((void**) &hostM, mem_size_M);
    unsigned int size_N = Nh * Nw;
    unsigned int mem_size_N = sizeof(float) * size_N;
    cudaMallocHost((void**) &hostN, mem_size_N);

    /* allocate memory for the result on host side */
    if (check)
        printf("  Allocate memory for the result on host side.\n");
    unsigned int size_P = Ph * Pw;
    unsigned int mem_size_P = sizeof(float) * size_P;
    cudaMallocHost((void**) &hostP, mem_size_P);

    /* Initialize the input matrices */
    if (check)
        printf("  Initialize the input matrices.\n");
    InitMatrix(hostM, Mw, Mh);
    InitMatrix(hostN, Nw, Nh);

    cudaDeviceSynchronize();
    gettimeofday(&t1, NULL);
    cudaEventRecord(start, 0);

    if (check)
        printf("  Allocate device memory.\n");
    cudaMalloc((void**) &deviceM, mem_size_M);
    cudaMalloc((void**) &deviceN, mem_size_N);

    if (check)
        printf("  Copy host memory data to device.\n");
    cudaMemcpy(deviceM, hostM, mem_size_M, cudaMemcpyHostToDevice);
    cudaMemcpy(deviceN, hostN, mem_size_N, cudaMemcpyHostToDevice);

    if (check)
        printf("  Allocate device memory for results and clean it.\n");
    cudaMalloc((void**) &deviceP, mem_size_P);
    cudaMemset(deviceP, 0, mem_size_P);

    if (check)
        printf("  Setup kernel execution parameters.\n");

    #if 1
    dim3 block;
    dim3 grid;
    grid.x = (Ph+block_size-1)/block_size;  /* Rows */
    grid.y = (Pw+block_size-1)/block_size;  /* Cols */
    block.x = block_size;
    block.y = block_size;
    #else
    dim3 block(block_size, block_size);
    dim3 grid(Ph/block.x, Pw/block.y);
    #endif

    if (check)
        printf("  # of threads in a block: %d x %d (%d)\n",
            block.x, block.y, block.x * block.y);
    if (check)
        printf("  # of blocks in a grid  : %d x %d (%d)\n",
            grid.x, grid.y, grid.x * grid.y);

    if (check)
        printf("  Executing the kernel...\n");

    matrixMul<<<grid, block>>>(deviceP, deviceM, deviceN, Mh, Mw, Nw, block_size);

    if (check) {
        cudaDeviceSynchronize();
        CHECK_CUDA_ERROR("Error in kernel");
    }

    if (check)
        printf("  Copy result from device to host.\n");
    cudaMemcpy(hostP, deviceP, mem_size_P, cudaMemcpyDeviceToHost);

    cudaEventRecord(stop, 0);
    cudaEventSynchronize(stop);

    cudaEventElapsedTime(&gpu_elapsed_time_ms, start, stop);

    gettimeofday(&t2, NULL);

    if (check) {
        float* reference = (float*) malloc(mem_size_P);

        printf("\nCheck results with those computed by CPU.\n");
        printf ("  Computing reference solution.\n");

        computeGold(reference, hostM, hostN, Mh, Mw, Nw);

        printf("  CPU checksum: %g\n", CheckSum(reference, Pw, Ph));
        printf("  GPU checksum: %g\n", CheckSum(hostP, Pw, Ph));

        CompareMatrix(hostP, reference, Pw, Ph);

        free(reference);
    }

    seconds = (((t2.tv_usec - t1.tv_usec)/1000000.0f)  + (t2.tv_sec - t1.tv_sec))
;
    printf("\n *******> Timne used: %f seconds (GPU %f ms)\n\n", seconds, gpu_elapsed_time_ms);

    cudaFree(hostM); cudaFree(hostN); cudaFree(hostP);

    cudaFree(deviceM);
    cudaFree(deviceN);
    cudaFree(deviceP);
}

