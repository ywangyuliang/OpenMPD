#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

#define N 8

int main(int argc, char **argv) {

    int i;
    int a[N];

    for (i = 0; i < N; i++) {
        a[i] = -1;
    }

    #pragma omp cluster gather(a[N])
    {
        #pragma omp cluster distribute
        for (i = 0; i < N; i++) {
            a[i] = __taskid;
            printf("Proceso %d de %d calcula a[%d]\n",
                __taskid, __numprocs, i);
        }
    }

    for (i = 0; i < N; i++) {
        printf("a[%d] = %d\n", i, a[i]);
    }

    return 0;
}