#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>


long int fib (long int n)
{
   if (n > 2)
       return fib(n-1) + fib(n-2);
   else if (n == 2)
 	return 1;
   else if (n == 1)
 	return 1;
   else if (n == 0)
 	return 0;
}

long int fibL (long int n)
{
int n_aux = n/2;

    if (n < 20)
        return(fib(n));
    return(fibL(n_aux) * fibL(n-(n_aux-1))+fibL(n_aux-1) * fibL(n-n_aux));
}

int main(int argc, char** argv)
{
  int n;
  long int fibN;
  struct timeval t1, t2;
  double segundos;

    if (argc != 2) {
	printf("Use fib number\n");
	return(0);
    }
    n = atoi(argv[1]);

    gettimeofday(&t1, NULL);
    fibN = fib(n);
    gettimeofday(&t2, NULL);
    segundos = (((t2.tv_usec - t1.tv_usec)/1000000.0f)  + (t2.tv_sec - t1.tv_sec));
    printf("Fibonacci of %d: %ld in %f sec.\n", n, fibN, segundos);

    gettimeofday(&t1, NULL);
    fibN = fibL(n);
    gettimeofday(&t2, NULL);
    segundos = (((t2.tv_usec - t1.tv_usec)/1000000.0f)  + (t2.tv_sec - t1.tv_sec));
    printf("Fibonacci of %d: %ld in %f sec\n", n, fibN, segundos);


    return(0);
}
