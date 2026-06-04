#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include "ompd_runtime.h"

typedef struct {
    int input_value_0;
} ompd_generated_task_input_data_0_t;

typedef struct {
    int input_value_0;
} ompd_generated_task_input_data_1_t;

typedef struct {
    int input_value_0;
} ompd_generated_task_input_data_2_t;

typedef struct {
    int input_value_0;
} ompd_generated_task_input_data_3_t;

typedef struct {
    int input_value_0;
} ompd_generated_task_input_data_4_t;

typedef enum {
    OMPD_TASK_KIND_GENERATED_0 = 1,
    OMPD_TASK_KIND_GENERATED_1,
    OMPD_TASK_KIND_GENERATED_2,
    OMPD_TASK_KIND_GENERATED_3,
    OMPD_TASK_KIND_GENERATED_4
} ompd_generated_task_kind_t;

static void ompd_generated_task_body_0(void *raw_task_input_data)
{
    ompd_generated_task_input_data_0_t *task_input_data;
    int a_output_value;

    task_input_data = (ompd_generated_task_input_data_0_t *) raw_task_input_data;

    a_output_value = 13 + task_input_data->input_value_0;

    ompd_runtime_send_int_value("a", a_output_value);
}

static void ompd_generated_task_body_1(void *raw_task_input_data)
{
    int a_input_value;
    int b_output_value;

    a_input_value = ompd_runtime_recv_int_value("a");

    b_output_value = a_input_value + 1;

    ompd_runtime_send_int_value("b", b_output_value);
}

static void ompd_generated_task_body_2(void *raw_task_input_data)
{
    int a_input_value;
    int c_output_value;

    a_input_value = ompd_runtime_recv_int_value("a");

    c_output_value = a_input_value + 2;

    ompd_runtime_send_int_value("c", c_output_value);
}

static void ompd_generated_task_body_3(void *raw_task_input_data)
{
    int c_input_value;
    int d_output_value;

    c_input_value = ompd_runtime_recv_int_value("c");

    d_output_value = c_input_value + 3;

    ompd_runtime_send_int_value("d", d_output_value);
}

static void ompd_generated_task_body_4(void *raw_task_input_data)
{
    ompd_generated_task_input_data_4_t *task_input_data;
    int c_input_value;
    int d_input_value;
    int e_output_value;

    task_input_data = (ompd_generated_task_input_data_4_t *) raw_task_input_data;

    c_input_value = ompd_runtime_recv_int_value("c");
    d_input_value = ompd_runtime_recv_int_value("d");

    e_output_value = c_input_value + 4;

    printf("e[%d]: %d\n", task_input_data->input_value_0, e_output_value);
    ompd_runtime_send_int_value("e", e_output_value);
}

void ompd_execute_generated_task(int task_kind, void *raw_task_input_data)
{
    switch(task_kind) {
        case OMPD_TASK_KIND_GENERATED_0:
            ompd_generated_task_body_0(raw_task_input_data);
            break;

        case OMPD_TASK_KIND_GENERATED_1:
            ompd_generated_task_body_1(raw_task_input_data);
            break;

        case OMPD_TASK_KIND_GENERATED_2:
            ompd_generated_task_body_2(raw_task_input_data);
            break;

        case OMPD_TASK_KIND_GENERATED_3:
            ompd_generated_task_body_3(raw_task_input_data);
            break;

        case OMPD_TASK_KIND_GENERATED_4:
            ompd_generated_task_body_4(raw_task_input_data);
            break;

        default:
            fprintf(stderr, "Invalid generated task kind: %d\n", task_kind);
            MPI_Abort(MPI_COMM_WORLD, 1);
    }
}

int main(int argc, char **argv)
{
    int i, N = 10;
    int a[N], b[N], c[N], d[N], e[N];

    ompd_initialize_runtime(&argc, &argv);

    //#pragma omp cluster teams num_teams(5)
    {
        ompd_set_region_num_teams(5);

        for (i = 0; i < N; i++) {
            ompd_generated_task_input_data_0_t task_input_data_0;
            ompd_generated_task_input_data_1_t task_input_data_1;
            ompd_generated_task_input_data_2_t task_input_data_2;
            ompd_generated_task_input_data_3_t task_input_data_3;
            ompd_generated_task_input_data_4_t task_input_data_4;

            ompd_task_definition_t task_definition_0;
            ompd_task_definition_t task_definition_1;
            ompd_task_definition_t task_definition_2;
            ompd_task_definition_t task_definition_3;
            ompd_task_definition_t task_definition_4;

            ompd_task_id_t task_id_0;
            ompd_task_id_t task_id_1;
            ompd_task_id_t task_id_2;
            ompd_task_id_t task_id_3;
            ompd_task_id_t task_id_4;

            // #pragma omp task_async depend(out:a[i:1])  
            task_input_data_0.input_value_0 = i;
            task_definition_0.generated_task_kind = OMPD_TASK_KIND_GENERATED_0;
            task_definition_0.task_input_data = &task_input_data_0;
            task_definition_0.task_input_data_size = sizeof(task_input_data_0);

            task_id_0 = ompd_build_task(&task_definition_0);
            ompd_register_task_output_array_element(task_id_0, "a", i, sizeof(int));
            ompd_submit_task(task_id_0);

            // #pragma omp task_async depend(in:a[i:1]) depend(out:b[i:1])  
            task_input_data_1.input_value_0 = i;
            task_definition_1.generated_task_kind = OMPD_TASK_KIND_GENERATED_1;
            task_definition_1.task_input_data = &task_input_data_1;
            task_definition_1.task_input_data_size = sizeof(task_input_data_1);

            task_id_1 = ompd_build_task(&task_definition_1);
            ompd_register_task_input_array_element(task_id_1, "a", i, sizeof(int));
            ompd_register_task_output_array_element(task_id_1, "b", i, sizeof(int));
            ompd_submit_task(task_id_1);

            // #pragma omp task_async depend(in:a[i:1]) depend(out:c[i:1])  
            task_input_data_2.input_value_0 = i;
            task_definition_2.generated_task_kind = OMPD_TASK_KIND_GENERATED_2;
            task_definition_2.task_input_data = &task_input_data_2;
            task_definition_2.task_input_data_size = sizeof(task_input_data_2);

            task_id_2 = ompd_build_task(&task_definition_2);
            ompd_register_task_input_array_element(task_id_2, "a", i, sizeof(int));
            ompd_register_task_output_array_element(task_id_2, "c", i, sizeof(int));
            ompd_submit_task(task_id_2);

            // #pragma omp task_async depend(in:c[i:1]) depend(out:d[i:1]) 
            task_input_data_3.input_value_0 = i;
            task_definition_3.generated_task_kind = OMPD_TASK_KIND_GENERATED_3;
            task_definition_3.task_input_data = &task_input_data_3;
            task_definition_3.task_input_data_size = sizeof(task_input_data_3);

            task_id_3 = ompd_build_task(&task_definition_3);
            ompd_register_task_input_array_element(task_id_3, "c", i, sizeof(int));
            ompd_register_task_output_array_element(task_id_3, "d", i, sizeof(int));
            ompd_submit_task(task_id_3);

            // #pragma omp task_async depend(in:c[i:1], d[i:1]) 
            task_input_data_4.input_value_0 = i;
            task_definition_4.generated_task_kind = OMPD_TASK_KIND_GENERATED_4;
            task_definition_4.task_input_data = &task_input_data_4;
            task_definition_4.task_input_data_size = sizeof(task_input_data_4);

            task_id_4 = ompd_build_task(&task_definition_4);
            ompd_register_task_input_array_element(task_id_4, "c", i, sizeof(int));
            ompd_register_task_input_array_element(task_id_4, "d", i, sizeof(int));
            ompd_register_task_output_array_element(task_id_4, "e", i, sizeof(int));
            ompd_submit_task(task_id_4);
        }
    }

    ompd_finalize_runtime();
    return 0;
}
