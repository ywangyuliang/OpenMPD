#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#ifdef _OPENMP
    #include <omp.h>
    double start_time, elapsed_time;
#else
    struct timeval t1, t2;
    double elapsed_time;
#endif

long binomial_seq(int n, int k)
{
    if (k < 0 || k > n) {
        return 0;
    }

    if (k == 0 || k == n) {
        return 1;
    }

    return binomial_seq(n - 1, k - 1) + binomial_seq(n - 1, k);
}

long binomial(int n, int k, int thresh)
{
    long left, right;

    if (k < 0 || k > n) {
        return 0;
    }

    if (k == 0 || k == n) {
        return 1;
    }

    if (n <= thresh) {
        return binomial_seq(n, k);
    }

    #pragma omp cluster
    {
        #pragma omp task_async depend(out:left)
        {
            left = binomial(n - 1, k - 1, thresh);
        }

        #pragma omp task_async depend(out:right)
        {
            right = binomial(n - 1, k, thresh);
        }

        #pragma omp taskwait
    }

    return left + right;
}

int main(int argc, const char **argv)
{
    int n, k, thresh;
    long res;
    long expected;

    if (argc < 4) {
        fprintf(stderr, "Uso: %s n k thresh\n", argv[0]);
        return 1;
    }

    n = atoi(argv[1]);
    k = atoi(argv[2]);
    thresh = atoi(argv[3]);

    if (n < 0 || k < 0 || thresh < 0 || k > n) {
        fprintf(stderr, "Invalid argument\n");
        return 1;
    }

#ifdef _OPENMP
    start_time = omp_get_wtime();
#else
    gettimeofday(&t1, NULL);
#endif

    #pragma omp cluster
    {
        #pragma omp master
        res = binomial(n, k, thresh);
    }

#ifdef _OPENMP
    elapsed_time = omp_get_wtime() - start_time;
#else
    gettimeofday(&t2, NULL);
    elapsed_time = (((t2.tv_usec - t1.tv_usec) / 1000000.0f)
                    + (t2.tv_sec - t1.tv_sec));
#endif

    expected = binomial_seq(n, k);

    fprintf(stdout, "binomial(%d, %d) = %ld\n", n, k, res);
    fprintf(stdout, "expected = %ld\n", expected);
    fprintf(stdout, "correct = %s\n", res == expected ? "yes" : "no");
    fprintf(stdout, "time: %f seconds\n", elapsed_time);

    return 0;
}