#include <stdio.h>
#include <stdlib.h>

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

int main(int argc, char** argv)
{
  int n;

    if (argc != 2) {
	printf("Use fib number\n");
	return(0);
    }
    n = atoi(argv[1]);

    printf("Fibonacci of %d: %ld\n", n, fib(n));

printf("alternativa\n");
int n_aux = n/2;
int n1, n2;
    n1 = fib(n_aux);
    n2 = fib(n_aux-1);
    printf("Fibonacci of %d: %ld\n", n, n1*fib(n-(n_aux-1))+n2*fib(n-n_aux));

    return(0);
}
