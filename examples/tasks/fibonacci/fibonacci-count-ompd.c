#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <sys/time.h>

static long fib_seq(int n)
{
    if (n < 2) {
        return n;
    }
    return fib_seq(n - 1) + fib_seq(n - 2);
}

long fibonacci(int n, int thresh, long *tasks)
{
    long n1, n2;
    long tasks1, tasks2;

    *tasks = 0;
    if (n < 2) {
        return n;
    }
    if (n <= thresh) {
        return fib_seq(n);
    }

    #pragma omp cluster
    {
        #pragma omp task_async depend(out:n1,tasks1)
        {
            n1 = fibonacci(n - 1, thresh, &tasks1);
            tasks1 = tasks1 + 1;
        }
        #pragma omp task_async depend(out:n2,tasks2)
        {
            n2 = fibonacci(n - 2, thresh, &tasks2);
            tasks2 = tasks2 + 1;
        }
        #pragma omp taskwait
    }

    *tasks = tasks1 + tasks2;
    return n1 + n2;
}

static int parse_argument(const char *text, int maximum, int *value)
{
    char *end;
    long parsed;

    errno = 0;
    parsed = strtol(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0'
        || parsed < 0 || parsed > maximum) {
        return 0;
    }
    *value = (int)parsed;
    return 1;
}

int main(int argc, const char **argv)
{
    int n, thresh;
    int max_n = LONG_MAX > 2147483647L ? 89 : 43;
    long res;
    long tasks;
    struct timeval start, end;
    double elapsed_time;

    if (argc != 3 || !parse_argument(argv[1], max_n, &n)
        || !parse_argument(argv[2], INT_MAX, &thresh)) {
        fprintf(stderr, "Usage: %s n (0..%d) thresh (0..%d)\n", argv[0], max_n, INT_MAX);
        return 1;
    }

    gettimeofday(&start, NULL);
    #pragma omp cluster
    {
        #pragma omp master
        res = fibonacci(n, thresh, &tasks);
    }
    gettimeofday(&end, NULL);
    elapsed_time = (end.tv_sec - start.tv_sec)
                   + (end.tv_usec - start.tv_usec) / 1000000.0;

    printf("fibonacci(%d) = %ld\n", n, res);
    printf("tasks executed = %ld\n", tasks);
    printf("OMPD_TASKS_EXECUTED=%ld\n", tasks);
    printf("time: %f seconds\n", elapsed_time);
    printf("OMPD_CALC_TIME_SECONDS=%.9f\n", elapsed_time);
    return 0;
}
