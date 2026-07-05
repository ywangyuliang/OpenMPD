#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#ifdef _OPENMP
    #include <omp.h>
    double start_time, elapsed_time;
#else
    struct timeval t1, t2;
    double elapsed_time;
#endif

static long fib_seq(int n){
    
    long n1, n2;

    if(n < 2){
        return n;
    }

    n1 = fib_seq(n-1);
    n2 = fib_seq(n-2);
    return n1 + n2; 
}

long fibonacci(int n, int thresh){
    
    long n1, n2;

    if(n < 2){
        return n;
    }

    if(n <= thresh){
        return fib_seq(n);
    }

    #pragma omp cluster
    {
        #pragma omp task_async depend(out:n1)
        {
            n1 = fibonacci(n-1, thresh);
        }
        #pragma omp task_async depend(out:n2)
        {
            n2 = fibonacci(n-2, thresh);
        }

        #pragma omp taskwait
    }

    return n1 + n2; 
}

int main(int argc, const char **argv)
{
    int n, thresh;
    long res;
        
    if(argc < 3){
        fprintf(stderr, "Invalid argument\n");
        return 1;
    }
    if((n = atoi(argv[1])) < 0 || (thresh = atoi(argv[2])) < 0){
        fprintf(stderr, "Invalid argument\n");
        return 1;
    }
    
#ifdef _OPENMP
    start_time = omp_get_wtime();
#else
    gettimeofday(&t1, NULL);
#endif

    #pragma omp cluster
    {
        #pragma omp master
        res = fibonacci(n, thresh);
    }

    fprintf(stdout, "fibonacci(%d) = %ld\n", n, res);

#ifdef _OPENMP
    elapsed_time = omp_get_wtime() - start_time;
	printf("time: %f seconds\n", elapsed_time);
#else
	gettimeofday(&t2, NULL);
	elapsed_time = (((t2.tv_usec - t1.tv_usec)/1000000.0f)  + (t2.tv_sec - t1.tv_sec));
	fprintf(stdout, "time: %f seconds\n", elapsed_time);
#endif

    return 0;
}
