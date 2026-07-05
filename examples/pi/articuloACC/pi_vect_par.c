
#include <omp.h>
#include <stdio.h>

#define NUM_THREADS 8

    static long num_steps = 1<<27;
    double step;
void main () {
    int i;
    double pi;
    double x;
    double sum=0.0;

step = 1.0/(double) num_steps;

#pragma omp parallel for simd private(x) reduction(+:sum) num_threads(NUM_THREADS)
for (i=0;i< num_steps; i++) {
    x = (i+0.5)*step;
    sum += 4.0/(1.0+x*x);
}
pi = step * sum;
printf("Pi es %12.10f, calculado con %ld pasos y %d threads\n", pi,num_steps,NUM_THREADS);
}
