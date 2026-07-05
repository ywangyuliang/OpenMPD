#include <math.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#define M 64
#define N 64

int main(int argc, char *argv[])
{
    double epsilon;
    double diff;
    double aux_diff;
    double mean;
    double u[M][N];
    double w[M][N];
    int i;
    int iterations;
    int iterations_print;
    int j;
    FILE *output;
    const char *output_filename;

#ifdef _OPENMP
    double start_time;
    double run_time;
#else
    struct timeval tv_start;
    struct timeval tv_end;
    double run_time;
#endif

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <epsilon> <output-file>\n", argv[0]);
        return 1;
    }

    epsilon = atof(argv[1]);
    output_filename = argv[2];
    diff = epsilon;

    mean = 0.0;
    for (i = 1; i < M - 1; i++) {
        w[i][0] = 100.0;
        w[i][N - 1] = 100.0;
        mean += w[i][0] + w[i][N - 1];
    }

    for (j = 0; j < N; j++) {
        w[M - 1][j] = 100.0;
        w[0][j] = 0.0;
        mean += w[M - 1][j] + w[0][j];
    }

    mean = mean / (double)(2 * M + 2 * N - 4);

    for (i = 1; i < M - 1; i++) {
        for (j = 1; j < N - 1; j++) {
            w[i][j] = mean;
        }
    }

    iterations = 0;
    iterations_print = 1;

#ifdef _OPENMP
    start_time = omp_get_wtime();
#else
    gettimeofday(&tv_start, NULL);
#endif

#pragma omp cluster broad(epsilon, diff) broad(w[M][N]) gather(w[M][N]) halo(u[M][N]:1*N)
    {
        while (epsilon <= diff) {
#pragma omp cluster distribute update halo(u[M][N]:1*N)
#pragma omp parallel for private(j)
            for (i = 0; i < M; i++)
                for (j = 0; j < N; j++)
                    u[i][j] = w[i][j];

            diff = 0.0;
#pragma omp cluster distribute allreduction(max:diff)
#pragma omp parallel for private(aux_diff, j)
            for (i = 0; i < M; i++) {
                for (j = 0; j < N; j++) {
                    if (i > 0 && i < M - 1 && j > 0 && j < N - 1) {
                        w[i][j] = (u[i - 1][j] + u[i + 1][j] + u[i][j - 1] + u[i][j + 1]) * 0.25;
                        aux_diff = fabs(w[i][j] - u[i][j]);
                        diff = diff < aux_diff ? aux_diff : diff;
                    }
                }
            }

            iterations++;

#pragma omp cluster teams master
            if (iterations == iterations_print) {
                printf("%8d  %lg\n", iterations, diff);
                iterations_print = 2 * iterations_print;
            }
        }
    }

#ifdef _OPENMP
    run_time = omp_get_wtime() - start_time;
#else
    gettimeofday(&tv_end, NULL);
    run_time = (tv_end.tv_sec - tv_start.tv_sec) * 1000000 + (tv_end.tv_usec - tv_start.tv_usec);
    run_time = run_time / 1000000;
#endif

    output = fopen(output_filename, "wt");
    if (output == NULL) {
        fprintf(stderr, "Cannot open output file %s\n", output_filename);
        return 1;
    }

    fprintf(output, "%d\n", M);
    fprintf(output, "%d\n", N);
    for (i = 0; i < M; i++) {
        for (j = 0; j < N; j++) {
            fprintf(output, "%lg ", w[i][j]);
        }
        fprintf(output, "\n");
    }
    fclose(output);

    printf("%8d  %lg\n", iterations, diff);
    printf("Error tolerance achieved.\n");
    printf("Elapsed time = %lg s\n", run_time);
    printf("Solution written to %s\n", output_filename);
    printf("INPUT_HALO: Normal end of execution.\n");

    return 0;
}
