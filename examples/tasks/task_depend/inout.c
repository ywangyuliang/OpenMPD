#include <stdio.h>

int main(void)
{
    int i;
    int x = 10;
    int a[2] = {5, 7};
    int y;

    #pragma omp cluster
    {
        #pragma omp task_async depend(out:y)
        { y = 20; }
        #pragma omp task_async depend(inout:y)
        { y += 2; }
        for (i = 0; i < 4; i++) {
            #pragma omp task_async depend(inout:x)
            { x += 3; }
        }
        for (i = 0; i < 2; i++) {
            #pragma omp task_async depend(inout:a[i:1])
            { a[i] += 10; }
            #pragma omp task_async depend(inout:a[i:1])
            { a[i] += 2; }
        }
        #pragma omp task_async depend(in:x,y)
        { printf("inout x = %d, y = %d\n", x, y); }
        for (i = 0; i < 2; i++) {
            #pragma omp task_async depend(in:a[i:1])
            { printf("inout a[%d] = %d\n", i, a[i]); }
        }
        #pragma omp taskwait
    }
    printf("final x = %d, y = %d\n", x, y);
    return 0;
}
