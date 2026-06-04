#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

long fibonacci(int n){
    
    long n1, n2;

    if(n < 2){
        return n;
    }

    n1 = fibonacci(n-1);
    n2 = fibonacci(n-2);

    return n1 + n2;
}

int main(int argc, const char **argv)
{
    int n;
    long res;
    struct timeval t1, t2;
    double elapsed_time;
    
    if(argc < 2){
        fprintf(stderr, "Invalid argument\n");
        return 1;
    }
    if((n = atoi(argv[1])) < 0){
        fprintf(stderr, "Invalid argument\n");
        return 1;
    }

    gettimeofday(&t1, NULL);
    res = fibonacci(n);
    gettimeofday(&t2, NULL);
    
    elapsed_time = (((t2.tv_usec - t1.tv_usec)/1000000.0f)  + (t2.tv_sec - t1.tv_sec));
    fprintf(stdout, "fibonacci(%d) = %ld\n", n, res);
    fprintf(stdout, "time: %f seconds\n", elapsed_time);

    return 0;
}
