#include <stdio.h>
#include "ompd_runtime.h"

void ompd_execute_generated_task(ompd_task_kind_t kind, void *input)
{
    (void)kind;
    (void)input;
}

int main(int argc, char **argv)
{
    unsigned char input[1025] = {0};
    ompd_initialize_runtime(&argc, &argv);

    if (ompd_current_rank_is_master()) {
        ompd_task_definition_t definition = {0, input, sizeof(input)};
        ompd_task_id_t task = ompd_build_task(&definition);
        ompd_submit_task(task);
        ompd_wait_for_child_tasks();
        printf("Accepted %zu input bytes\n", sizeof(input));
    }

    ompd_finalize_runtime();
    return 0;
}
