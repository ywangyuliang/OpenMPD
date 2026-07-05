/*
 * Source - https://stackoverflow.com/q/17002380
 * Posted by user2088790, modified by community. See post 'Timeline' for change history
 * Retrieved 2026-03-19, License - CC BY-SA 4.0
 */

    #include <stdio.h>
    #include <stdint.h>
    #include <stdlib.h>
    #include <omp.h>

    uint64_t fib_iterative(uint64_t n) {
        uint64_t fn0 = 0;
        uint64_t fn1 = 1;
        uint64_t fn2 = 0;
        if(n==0) return fn0;
        if(n==1) return fn1;

        for(int i=2; i<(n+1); i++) {
            fn2 = fn0 + fn1;
            fn0 = fn1;
            fn1 = fn2;
        }
        return fn2;
    }

    uint64_t fib_recursive(uint64_t n) {
        if ( n == 0 || n == 1 ) return(n);
        return(fib_recursive(n-1) + fib_recursive(n-2));
    }

    uint64_t fib_recursive_omp(uint64_t n) {
        uint64_t i, j;
        if (n<2)
        return n;
        else {
           #pragma omp task shared(i) firstprivate(n)
           i=fib_recursive_omp(n-1);

           #pragma omp task shared(j) firstprivate(n)
           j=fib_recursive_omp(n-2);

           #pragma omp taskwait
           return i+j;
        }
    }

    uint64_t fib_recursive_omp_if(uint64_t n) {
        uint64_t i, j;
        if (n<2)
        return n;
        else {
           #pragma omp task shared(i) firstprivate(n) if (n>20)
           i=fib_recursive_omp_if(n-1);

           #pragma omp task shared(j) firstprivate(n) if (n>20)
           j=fib_recursive_omp_if(n-2);

           #pragma omp taskwait
           return i+j;
        }
    }

    uint64_t fib_recursive_ompd_if(uint64_t n) {
        uint64_t i, j;
        if (n<2)
        return n;
        else {
           #pragma omp task_async shared(i) firstprivate(n) if (n>20) depend(out:i)
           i=fib_recursive_ompd_if(n-1);

           #pragma omp task_async shared(j) firstprivate(n) if (n>20) depend(out:j)
           j=fib_recursive_ompd_if(n-2);

           #pragma omp taskwait
           return i+j;
        }
    }

    uint64_t fib_recursive_omp_fix(uint64_t n) {
        uint64_t i, j;
        if (n<2)
        return n;
        else {
            if ( n < 20 )
            {
                return(fib_recursive_omp_fix(n-1)+fib_recursive_omp_fix(n-2));
            }
            else {
               #pragma omp task shared(i) firstprivate(n)
               i=fib_recursive_omp_fix(n-1);

               #pragma omp task shared(j) firstprivate(n)
               j=fib_recursive_omp_fix(n-2);

               #pragma omp taskwait
               return i+j;
            }
        }
    }

    int main(int argc, char** argv) {
        uint64_t n = atoi(argv[1]);
        uint64_t result;
        double dtime;

        dtime = omp_get_wtime();
        result = fib_iterative(n);
        dtime = omp_get_wtime() - dtime;
        printf("iterative time %f, results %lu\n", dtime, result);

        dtime = omp_get_wtime();
        result = fib_recursive(n);
        dtime = omp_get_wtime() - dtime;
        printf("recursive time %f, results %lu\n", dtime, result);

        dtime = omp_get_wtime();
	#pragma omp cluster
	{
		#pragma omp master
	result = fib_recursive_ompd_if(n);
	}
        dtime = omp_get_wtime() - dtime;
        printf("recursive omp if time %f, results %lu\n", dtime, result);

        dtime = omp_get_wtime();
	#pragma omp parallel
	{
		#pragma omp single
	result = fib_recursive_omp_fix(n);
	}
        dtime = omp_get_wtime() - dtime;
        printf("recursive omp fix time %f, results %lu\n", dtime, result);

    }

