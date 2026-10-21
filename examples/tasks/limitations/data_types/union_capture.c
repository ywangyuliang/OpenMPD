#include <stdio.h>

union Value {
    int integer;
    double real;
};

int read_value(union Value value)
{
    return value.integer;
}

int main(void)
{
    union Value value = {7};
    int result = 0;

    #pragma omp cluster
    {
        #pragma omp task_async depend(out:result)
        { result = read_value(value); }
        #pragma omp taskwait
    }

    printf("result = %d (expected 7)\n", result);
    return 0;
}
