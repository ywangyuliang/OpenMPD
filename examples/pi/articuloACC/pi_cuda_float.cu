
#include <omp.h>
#include <stdio.h>
#include <sys/time.h>

#define NUM_THRS 1024

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


__global__ void reduction1(float *g_data, int n)
{
  int stride;
  // Define shared memory
  __shared__ float s_data[NUM_THRS/2];

  // Load the shared memory
  s_data[threadIdx.x] = g_data[threadIdx.x];
  if(threadIdx.x + blockDim.x < n)
    s_data[threadIdx.x] += g_data[threadIdx.x + blockDim.x];
  __syncthreads();

  // Do sum reduction on the shared memory
  for(stride = blockDim.x/2 ; stride >= 1; stride >>= 1)
  {
    if(threadIdx.x < stride)
      s_data[threadIdx.x] += s_data[threadIdx.x + stride];
    __syncthreads();
  }

 // Store results back to global memory
  if(threadIdx.x == 0)
    g_data[0] = s_data[0];

  return;
}



__global__ void pi_gpu(float *g_sum, float step, long num_steps) {

    int i = blockIdx.x*blockDim.x + threadIdx.x;
    float x;
    __shared__ float sum[NUM_THRS];

    sum[threadIdx.x] = 0;
    while (i < num_steps) {
	x = (i+0.5)*step;
        sum[threadIdx.x] += 4.0/(1.0+x*x);
	i += gridDim.x * blockDim.x;
    }
    __syncthreads();

    for( i= blockDim.x/2; i > 0; i >>= 1 )
    {
        if (threadIdx.x < i)
            sum[threadIdx.x] += sum[threadIdx.x + i];
        __syncthreads();
    }

 // Store results back to global memory
    if(threadIdx.x == 0)
        g_sum[blockIdx.x] = sum[0];

    return;
}


int main (int argc, char** argv) {
    static unsigned long int num_steps = (unsigned long int)((1<<30)-1);
    float step;

    float pi;
    float PI25DT = 3.141592653589793238462643;
    float sum=0.0;
    int factor=1;
    struct timeval t1, t2;
    double segundos;
    int blocks, thrs;
    float * d_sum;

HANDLE_ERROR(cudaMalloc((void**)&d_sum, sizeof(float)));

    if (argc == 1) {
        printf("num_steps %ld, ¿Factor de escala (1..4)?\n", num_steps);
        if (scanf("%d", &factor) <= 0)
            printf("scanf error, factor sin cambio %d\n", factor);
    }
    else factor = atoi(argv[1]);

    num_steps = num_steps * factor;

printf("%ld num_steps, %25.23f step\n", num_steps, (double)1.0/(double) num_steps);

gettimeofday(&t1, NULL);

step = 1.0/(float) num_steps;

if (num_steps > NUM_THRS * NUM_THRS) {
    blocks = NUM_THRS;
    thrs = NUM_THRS;
} else if (num_steps > NUM_THRS) {
    blocks = (num_steps%NUM_THRS)? (num_steps/NUM_THRS+1): (num_steps/NUM_THRS);
    thrs = NUM_THRS;
} else {
    blocks = 1;
    thrs = NUM_THRS;
}

HANDLE_ERROR(cudaMalloc((void**)&d_sum, blocks *sizeof(float)));
pi_gpu<<<blocks, thrs>>>(d_sum, step, num_steps);
cudaDeviceSynchronize();
CHECK_CUDA_ERROR("kernel invocation");
if (blocks > 1) {
      reduction1<<<1,blocks/2>>>(d_sum, blocks);
      cudaDeviceSynchronize();
      CHECK_CUDA_ERROR("kernel invocation");
}
HANDLE_ERROR(cudaMemcpy(&sum, d_sum, sizeof(float), cudaMemcpyDeviceToHost));
pi = step * sum;

gettimeofday(&t2, NULL);
segundos = (((t2.tv_usec - t1.tv_usec)/1000000.0f)  + (t2.tv_sec - t1.tv_sec));

printf("Pi es %25.23f, calculado con %ld pasos en %f segundos\n", pi,num_steps,segundos);
printf("Pi es %25.23f, Error relativo %10.8e\n", PI25DT, (double)100 * (pi - PI25DT)/PI25DT);
}
