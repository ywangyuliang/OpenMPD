
#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

/*
 * Small pipeline example for checking dependency ordering.  With N=10 and
 * print-heavy output, it is useful for validation but not for performance
 * comparisons.
 */

int main(int argc, char **argv)
{
int i, N=10;
int a[N], b[N], c[N], d[N], e[N];

for (i=0; i<N; i++){
	a[i] = 13+i;
	b[i] = a[i] + 1;
	c[i] = b[i] + 2;
	d[i] = c[i] + 3;
	e[i] = d[i] + 4;
}

for (i=0; i<N; i++)
    printf("a[%d]: %d b[%d]: %d c[%d]: %d d[%d]: %d e[%d]: %d\n", i, a[i], i, b[i], i, c[i], i, d[i], i, e[i]);
}
