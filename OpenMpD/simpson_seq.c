#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>

#define PI 3.14159265358979323846

double get_time()
{
    struct timeval t;
    gettimeofday(&t, NULL);
    return t.tv_sec + t.tv_usec * 1e-6;
}

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

int main(int argc, char **argv)
{
    double eps = 1e-8;
    int depth = 20;

    if (argc > 1) {
        eps = atof(argv[1]);
    }

    if (argc > 2) {
        depth = atoi(argv[2]);
    }

    double a = 0.0;
    double b = PI;
    double m = (a + b) / 2.0;

    double fa = f(a);
    double fm = f(m);
    double fb = f(b);

    double whole = simpson(a, b, fa, fm, fb);

    double start_time = get_time();

    double result = simpson_seq(a, b, fa, fm, fb, whole, eps, depth);

    double elapsed_time = get_time() - start_time;

    printf("Simpson integral of sin(x) in [0, pi]\n");
    printf("eps = %.12e\n", eps);
    printf("depth = %d\n", depth);
    printf("result = %.15f\n", result);
    printf("expected = %.15f\n", 2.0);
    printf("error = %.12e\n", fabs(result - 2.0));
    printf("time: %f seconds\n", elapsed_time);

    return 0;
}