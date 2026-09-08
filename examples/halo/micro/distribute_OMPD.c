#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <omp.h>

#define M 8
#define N 4

int main(int argc, char **argv) {

    int i, j;
    int img[M][N];
    int filtered[M][N];
    int u[M][N];
    int sum = 0;
    int halo_rows = 1;
    struct timeval calculation_start, calculation_end;
    double calculation_seconds;

    for (i = 0; i < M; i++) {
        for (j = 0; j < N; j++) {
            img[i][j] = i;
            filtered[i][j] = -1;
        }
    }

    printf("Matriz inicial img antes del cluster:\n");

    for (i = 0; i < M; i++) {
        printf("Fila %d: ", i);

        for (j = 0; j < N; j++) {
            printf("%d ", img[i][j]);
        }

        printf("\n");
    }

    printf("\n");

    gettimeofday(&calculation_start, NULL);

    #pragma omp cluster broad(u[M][N]) halo(u[M][N]:halo_rows*N)
    {
        #pragma omp cluster distribute update halo(u[M][N]:halo_rows*N)
        #pragma omp parallel for
        for (i = 0; i < M; i++) {
            for (j = 0; j < N; j++) {
                /* Local block computation */
            }
        }
    }

    gettimeofday(&calculation_end, NULL);
    calculation_seconds =
        (calculation_end.tv_sec - calculation_start.tv_sec)
        + (calculation_end.tv_usec - calculation_start.tv_usec) / 1000000.0;
    printf("OMPD_CALC_TIME_SECONDS=%.9f\n", calculation_seconds);

    printf("\nMatriz final filtered tras gather:\n");

    for (i = 0; i < M; i++) {
        printf("Fila %d: ", i);

        for (j = 0; j < N; j++) {
            printf("%d ", filtered[i][j]);
        }

        printf("\n");
    }

    printf("\nSuma total = %d\n", sum);

    return 0;
}
