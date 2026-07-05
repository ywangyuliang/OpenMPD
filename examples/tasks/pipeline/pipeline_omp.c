
#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

/*
 * Small pipeline example for checking task dependency ordering.  With N=10,
 * tiny task bodies and print-heavy output, it is useful for validation but not
 * for performance comparisons.
 */

int main(int argc, char **argv)
{
int i, N=10;
int a[N], b[N], c[N], d[N], e[N];

#pragma omp parallel num_threads(5) shared(a,b,c,d)
{
#pragma omp single
for (i=0; i<N; i++){
#pragma omp task depend(out:a[i:1])  firstprivate(i)
{
	a[i] = 13+ i;
}
#pragma omp task depend(in:a[i:1]) depend(out:b[i:1])  firstprivate(i)
{
	b[i] = a[i] + 1;
}
#pragma omp task depend(in:b[i:1]) depend(out:c[i:1])  firstprivate(i)
{
	c[i] = b[i] + 2;
}
#pragma omp task depend(in:c[i:1]) depend(out:d[i:1]) firstprivate(i)
{
	d[i] = c[i] + 3;
}
#pragma omp task depend(in:d[i:1]) firstprivate(i)
{
	e[i] = d[i] + 4;
}
}
}

for (i=0; i<N; i++)
    printf("a[%d]: %d b[%d]: %d c[%d]: %d d[%d]: %d e[%d]: %d\n", i, a[i], i, b[i], i, c[i], i, d[i], i, e[i]);
}
