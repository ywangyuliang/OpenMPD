#include <stdio.h>

int main(void)
{
    int i;
    int stock = 10;
    int movements[] = {-4, -8, 5, -6};

    for (i = 0; i < 4; i++) {
        if (stock + movements[i] >= 0) {
            stock += movements[i];
        }
    }

    printf("final stock = %d\n", stock);
    return 0;
}
