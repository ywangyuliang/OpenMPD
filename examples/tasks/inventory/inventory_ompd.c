#include <stdio.h>

int main(void)
{
    int i;
    int stock = 10;
    int movements[] = {-4, -8, 5, -6};

    #pragma omp cluster teams num_teams(4)
    {
        for (i = 0; i < 4; i++) {
            #pragma omp task_async depend(inout: stock)
            {
                if (stock + movements[i] >= 0) {
                    stock += movements[i];
                }
            }
        }

        #pragma omp taskwait
    }

    printf("final stock = %d\n", stock);
    return 0;
}
