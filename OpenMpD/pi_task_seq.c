#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

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

    double step = 1.0 / (double) num_steps;

    double start_time = get_time();

    double sum = compute_pi_part(0, num_steps, num_steps);
    double pi = step * sum;

    double elapsed_time = get_time() - start_time;

    printf("num_steps = %ld\n", num_steps);
    printf("pi = %.15f\n", pi);
    printf("time = %.6f seconds\n", elapsed_time);

    return 0;
}