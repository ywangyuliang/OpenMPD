#include <stdio.h>

int sum(int x, int y)
{
    struct Pair {
        int x;
        int y;
    };
    struct Pair pair = {x, y};
    return pair.x + pair.y;
}

int main(void)
{
    int result = 0;

    #pragma omp cluster
    {
        #pragma omp task_async depend(out:result)
        { result = sum(3, 4); }
        #pragma omp taskwait
    }

    printf("result = %d (expected 7)\n", result);
    return 0;
}
