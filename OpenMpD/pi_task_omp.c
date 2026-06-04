#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <omp.h>

static double get_time()
{
    struct timeval t;
    gettimeofday(&t, NULL);
    return t.tv_sec + t.tv_usec * 1e-6;
}

static double compute_pi_part(long start, long end, long num_steps)
{
    double step = 1.0 / (double) num_steps;
    double sum = 0.0;

    for (long i = start; i < end; i++) {
        double x = (i + 0.5) * step;
        sum += 4.0 / (1.0 + x * x);
    }

    return sum;
}

int main(int argc, char **argv)
{
    long num_steps = 100000000;

    if (argc > 1) {
        num_steps = atol(argv[1]);
    }

    long chunk = num_steps / 4;
    double step = 1.0 / (double) num_steps;

    double s0 = 0.0;
    double s1 = 0.0;
    double s2 = 0.0;
    double s3 = 0.0;

    double start_time = get_time();

    #pragma omp parallel
    {
        #pragma omp single
        {
            #pragma omp task shared(s0)
            {
                s0 = compute_pi_part(0, chunk, num_steps);
            }

            #pragma omp task shared(s1)
            {
                s1 = compute_pi_part(chunk, 2 * chunk, num_steps);
            }

            #pragma omp task shared(s2)
            {
                s2 = compute_pi_part(2 * chunk, 3 * chunk, num_steps);
            }

            #pragma omp task shared(s3)
            {
                s3 = compute_pi_part(3 * chunk, num_steps, num_steps);
            }

            #pragma omp taskwait
        }
    }

    double pi = step * (s0 + s1 + s2 + s3);
    double elapsed_time = get_time() - start_time;

    printf("num_steps = %ld\n", num_steps);
    printf("pi = %.15f\n", pi);
    printf("time = %.6f seconds\n", elapsed_time);

    return 0;
}