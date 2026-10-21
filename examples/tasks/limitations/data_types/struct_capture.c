#include <stdio.h>

struct Pair {
    int x;
    int y;
};

int sum(struct Pair value)
{
    return value.x + value.y;
}

int main(void)
{
    struct Pair pair = {3, 4};
    int result = 0;

    #pragma omp cluster
    {
        #pragma omp task_async depend(out:result)
        { result = sum(pair); }
        #pragma omp taskwait
    }

    printf("result = %d (expected 7)\n", result);
    return 0;
}
