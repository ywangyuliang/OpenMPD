
#include <stdio.h>
#include <stdlib.h>
#include <omp.h>


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
//	printf("tid %d: a[%d] = %d b[%d]=%d\n", omp_get_thread_num(), i, a[i], i, b[i]);
}
#pragma omp task depend(in:b[i:1]) depend(out:c[i:1])  firstprivate(i)
{
	c[i] = b[i] + 2;
//	printf("tid %d: c[%d]=%d b[%d]=%d\n", omp_get_thread_num(), i, c[i], i, b[i]);
}
#pragma omp task depend(in:c[i:1]) depend(out:d[i:1]) firstprivate(i)
{
	d[i] = c[i] + 3;
//	printf("tid %d: d[%d]=%d c[%d]=%d\n", omp_get_thread_num(), i, d[i], i, c[i]);
}
#pragma omp task depend(in:d[i:1]) firstprivate(i)
{
	e[i] = d[i] + 4;
//	printf("tid %d: e[%d]=%d d[%d]=%d\n", omp_get_thread_num(), i, e[i], i, d[i]);
}
}
}

for (i=0; i<N; i++)
    printf("a[%d]: %d b[%d]: %d c[%d]: %d d[%d]: %d e[%d]: %d\n", i, a[i], i, b[i], i, c[i], i, d[i], i, e[i]);
}
