#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>

#ifdef _OPENMP
    #include <omp.h>
    double start_time, elapsed_time;
#else
    struct timeval t1, t2;
    double elapsed_time;
#endif

#define PI 3.14159265358979323846

/*
 * Small tasking example for checking correctness.  This integral converges very
 * quickly, so the measured time mostly reflects MPI/runtime overhead rather
 * than useful numerical work.
 */

double f(double x)
{
    return sin(x);
}

double simpson(double a, double b, double fa, double fm, double fb)
{
    return (b - a) * (fa + 4.0 * fm + fb) / 6.0;
}

double simpson_seq(double a, double b, double fa, double fm, double fb, double whole, double eps, int depth)
{
    double m = (a + b) / 2.0;
    double lm = (a + m) / 2.0;
    double rm = (m + b) / 2.0;

    double flm = f(lm);
    double frm = f(rm);

    double left = simpson(a, m, fa, flm, fm);
    double right = simpson(m, b, fm, frm, fb);

    double error = fabs(left + right - whole);

    if (depth <= 0 || error <= 15.0 * eps) {
        return left + right + (left + right - whole) / 15.0;
    }

    double eps2 = eps * 0.5;
    int next_depth = depth - 1;

    return simpson_seq(a, m, fa, flm, fm, left, eps2, next_depth) + simpson_seq(m, b, fm, frm, fb, right, eps2, next_depth);
}

double simpson_ompd(double a, double b, double fa, double fm, double fb, double whole, double eps, int depth, int thresh)
{
    double m = (a + b) / 2.0;
    double lm = (a + m) / 2.0;
    double rm = (m + b) / 2.0;

    double flm = f(lm);
    double frm = f(rm);

    double left = simpson(a, m, fa, flm, fm);
    double right = simpson(m, b, fm, frm, fb);

    double error = fabs(left + right - whole);

    if (depth <= 0 || error <= 15.0 * eps) {
        return left + right + (left + right - whole) / 15.0;
    }

    double eps2 = eps * 0.5;
    int next_depth = depth - 1;

    if (depth <= thresh) {
        return simpson_seq(a, m, fa, flm, fm, left, eps2, next_depth) + simpson_seq(m, b, fm, frm, fb, right, eps2, next_depth);
    }

    double left_result = 0.0;
    double right_result = 0.0;

    #pragma omp cluster
    {
        #pragma omp task_async depend(out:left_result)
        {
            left_result = simpson_ompd(a, m, fa, flm, fm, left, eps2, next_depth, thresh);
        }

        #pragma omp task_async depend(out:right_result)
        {
            right_result = simpson_ompd(m, b, fm, frm, fb, right, eps2, next_depth, thresh);
        }

        #pragma omp taskwait
    }

    return left_result + right_result;
}

int main(int argc, const char **argv)
{
    double eps = 1e-8;
    int depth = 20;
    int thresh = 10;

    if (argc > 1) {
        eps = atof(argv[1]);
    }

    if (argc > 2) {
        depth = atoi(argv[2]);
    }

    if (argc > 3) {
        thresh = atoi(argv[3]);
    }

    double a = 0.0;
    double b = PI;
    double m = (a + b) / 2.0;

    double fa = f(a);
    double fm = f(m);
    double fb = f(b);

    double whole = simpson(a, b, fa, fm, fb);

    double result = 0.0;

#ifdef _OPENMP
    start_time = omp_get_wtime();
#else
    gettimeofday(&t1, NULL);
#endif

    #pragma omp cluster
    {
        #pragma omp master
        result = simpson_ompd(a, b, fa, fm, fb, whole, eps, depth, thresh);
    }

#ifdef _OPENMP
    elapsed_time = omp_get_wtime() - start_time;
#else
    gettimeofday(&t2, NULL);
    elapsed_time = (((t2.tv_usec - t1.tv_usec) / 1000000.0f)
                    + (t2.tv_sec - t1.tv_sec));
#endif

    printf("Simpson integral of sin(x) in [0, pi]\n");
    printf("eps = %.12e\n", eps);
    printf("depth = %d\n", depth);
    printf("thresh = %d\n", thresh);
    printf("result = %.15f\n", result);
    printf("expected = %.15f\n", 2.0);
    printf("error = %.12e\n", fabs(result - 2.0));
    printf("time: %f seconds\n", elapsed_time);

    return 0;
}
