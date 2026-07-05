#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <omp.h>

double get_time()
{
    struct timeval t;
    gettimeofday(&t, NULL);
    return t.tv_sec + t.tv_usec * 1e-6;
}

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

long binomial_omp(int n, int k, int thresh)
{
    long left = 0;
    long right = 0;

    if (k < 0 || k > n) {
        return 0;
    }

    if (k == 0 || k == n) {
        return 1;
    }

    if (n <= thresh) {
        return binomial_seq(n, k);
    }

    #pragma omp task shared(left) firstprivate(n, k, thresh)
    {
        left = binomial_omp(n - 1, k - 1, thresh);
    }

    #pragma omp task shared(right) firstprivate(n, k, thresh)
    {
        right = binomial_omp(n - 1, k, thresh);
    }

    #pragma omp taskwait

    return left + right;
}

long binomial_formula(int n, int k)
{
    long result = 1;

    if (k < 0 || k > n) {
        return 0;
    }

    if (k > n - k) {
        k = n - k;
    }

    for (int i = 1; i <= k; i++) {
        result = result * (n - k + i) / i;
    }

    return result;
}

int main(int argc, char **argv)
{
    int n = 35;
    int k = 17;
    int thresh = 20;

    if (argc > 1) {
        n = atoi(argv[1]);
    }

    if (argc > 2) {
        k = atoi(argv[2]);
    }

    if (argc > 3) {
        thresh = atoi(argv[3]);
    }

    if (n < 0 || k < 0 || k > n || thresh < 0) {
        fprintf(stderr, "Invalid argument\n");
        return 1;
    }

    long result = 0;

    double start_time = get_time();

    #pragma omp parallel
    {
        #pragma omp single
        {
            result = binomial_omp(n, k, thresh);
        }
    }

    double elapsed_time = get_time() - start_time;

    long expected = binomial_formula(n, k);

    printf("binomial(%d, %d) = %ld\n", n, k, result);
    printf("threshold = %d\n", thresh);
    printf("expected = %ld\n", expected);
    printf("correct = %s\n", result == expected ? "yes" : "no");
    printf("time: %f seconds\n", elapsed_time);

    return 0;
}