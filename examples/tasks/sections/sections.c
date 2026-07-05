
#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

int main(int argc, char **argv)
{
int a = 13;

#pragma omp parallel num_threads(4) shared(a)
#pragma omp sections 
{
#pragma omp section
#pragma omp atomic
	a += 1;
#pragma omp section
#pragma omp atomic
	a += 2;
#pragma omp section
#pragma omp atomic
	a += 3;
#pragma omp section
#pragma omp atomic
	a += 4;
}

printf("a: %d\n", a);
}
