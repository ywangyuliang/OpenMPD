                    #include <assert.h>
#include <mpi.h>
#include <stdio.h>
#include "ompd_runtime.h"
                          
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
int main();
typedef enum {
    OMPD_TASK_KIND_GENERATED_1 = 1,
    OMPD_TASK_KIND_GENERATED_2 = 2,
    OMPD_TASK_KIND_GENERATED_3 = 3,
    OMPD_TASK_KIND_GENERATED_4 = 4,
    OMPD_TASK_KIND_GENERATED_5 = 5,
    OMPD_TASK_KIND_GENERATED_6 = 6,
} ompd_generated_task_kind_t;

typedef struct {
    int input_value_0;
} ompd_generated_task_input_data_1_t;

static void ompd_generated_task_body_1(void *raw_task_input_data)
{
    ompd_generated_task_input_data_1_t *task_input_data =
        (ompd_generated_task_input_data_1_t *) raw_task_input_data;

    int a_output_value;


    {
    a_output_value = 13 + task_input_data->input_value_0;
    }
    ompd_runtime_send_int_value("a", a_output_value);

}

static void ompd_generated_task_body_2(void *raw_task_input_data)
{
    int a_input_value;
    int b_output_value;

    a_input_value = ompd_runtime_recv_int_value("a");

    {
    b_output_value = a_input_value + 1;
    }
    ompd_runtime_send_int_value("b", b_output_value);

}

static void ompd_generated_task_body_3(void *raw_task_input_data)
{
    int b_input_value;
    int c_output_value;

    b_input_value = ompd_runtime_recv_int_value("b");

    {
    c_output_value = b_input_value + 2;
    }
    ompd_runtime_send_int_value("c", c_output_value);

}

static void ompd_generated_task_body_4(void *raw_task_input_data)
{
    int c_input_value;
    int d_output_value;

    c_input_value = ompd_runtime_recv_int_value("c");

    {
    d_output_value = c_input_value + 3;
    }
    ompd_runtime_send_int_value("d", d_output_value);

}

typedef struct {
    int input_value_0;
} ompd_generated_task_input_data_5_t;

static void ompd_generated_task_body_5(void *raw_task_input_data)
{
    ompd_generated_task_input_data_5_t *task_input_data =
        (ompd_generated_task_input_data_5_t *) raw_task_input_data;

    int d_input_value;
    int e_output_value;

    d_input_value = ompd_runtime_recv_int_value("d");

    {
    e_output_value = d_input_value + 4;
    printf("e[%d]: %d\n", task_input_data->input_value_0, e_output_value);
    }
    ompd_runtime_send_int_value("e", e_output_value);

}

typedef struct {
    int input_value_0;
} ompd_generated_task_input_data_6_t;

static void ompd_generated_task_body_6(void *raw_task_input_data)
{
    ompd_generated_task_input_data_6_t *task_input_data =
        (ompd_generated_task_input_data_6_t *) raw_task_input_data;

    int a_input_value;
    int b_input_value;
    int c_input_value;
    int d_input_value;
    int e_input_value;

    a_input_value = ompd_runtime_recv_int_value("a");
    b_input_value = ompd_runtime_recv_int_value("b");
    c_input_value = ompd_runtime_recv_int_value("c");
    d_input_value = ompd_runtime_recv_int_value("d");
    e_input_value = ompd_runtime_recv_int_value("e");

    {
    printf("a[%d]: %d b[%d]: %d c[%d]: %d d[%d]: %d e[%d]: %d\n", task_input_data->input_value_0, a_input_value, task_input_data->input_value_0, b_input_value, task_input_data->input_value_0, c_input_value, task_input_data->input_value_0, d_input_value, task_input_data->input_value_0, e_input_value);
    }

}

void ompd_execute_generated_task(int task_kind, void *raw_task_input_data)
{
    switch (task_kind) {
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
        case OMPD_TASK_KIND_GENERATED_5:
            ompd_generated_task_body_5(raw_task_input_data);
            break;
        case OMPD_TASK_KIND_GENERATED_6:
            ompd_generated_task_body_6(raw_task_input_data);
            break;
        default:
            fprintf(stderr, "Invalid generated task kind: %d\n", task_kind);
            MPI_Abort(MPI_COMM_WORLD, 1);
    }
}

                                                                    
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                

#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <assert.h>


void DeclareTypesMPI();

int __taskid = -1, __numprocs = -1;
MPI_Status __status;

int main(int argc, char **argv)
{
int i, N=10;
int a[N], b[N], c[N], d[N], e[N];

	ompd_initialize_runtime(&argc, (char ***)&argv);
	{
ompd_check_region_num_teams(5);
		for (i=0; i<N; i++) {











{
    ompd_generated_task_input_data_1_t task_input_data_1;
    ompd_task_definition_t task_definition_1;
    ompd_task_id_t task_id_1;
    task_input_data_1.input_value_0 = i;
    task_definition_1.generated_task_kind = OMPD_TASK_KIND_GENERATED_1;
    task_definition_1.task_input_data = &task_input_data_1;
    task_definition_1.task_input_data_size = sizeof(task_input_data_1);
    task_id_1 = ompd_build_task(&task_definition_1);
    ompd_register_task_output_array_element(task_id_1, "a", i, sizeof(int));
    ompd_submit_task(task_id_1);
}
{
    ompd_task_definition_t task_definition_2;
    ompd_task_id_t task_id_2;
    task_definition_2.generated_task_kind = OMPD_TASK_KIND_GENERATED_2;
    task_definition_2.task_input_data = NULL;
    task_definition_2.task_input_data_size = 0;
    task_id_2 = ompd_build_task(&task_definition_2);
    ompd_register_task_input_array_element(task_id_2, "a", i, sizeof(int));
    ompd_register_task_output_array_element(task_id_2, "b", i, sizeof(int));
    ompd_submit_task(task_id_2);
}
{
    ompd_task_definition_t task_definition_3;
    ompd_task_id_t task_id_3;
    task_definition_3.generated_task_kind = OMPD_TASK_KIND_GENERATED_3;
    task_definition_3.task_input_data = NULL;
    task_definition_3.task_input_data_size = 0;
    task_id_3 = ompd_build_task(&task_definition_3);
    ompd_register_task_input_array_element(task_id_3, "b", i, sizeof(int));
    ompd_register_task_output_array_element(task_id_3, "c", i, sizeof(int));
    ompd_submit_task(task_id_3);
}
{
    ompd_task_definition_t task_definition_4;
    ompd_task_id_t task_id_4;
    task_definition_4.generated_task_kind = OMPD_TASK_KIND_GENERATED_4;
    task_definition_4.task_input_data = NULL;
    task_definition_4.task_input_data_size = 0;
    task_id_4 = ompd_build_task(&task_definition_4);
    ompd_register_task_input_array_element(task_id_4, "c", i, sizeof(int));
    ompd_register_task_output_array_element(task_id_4, "d", i, sizeof(int));
    ompd_submit_task(task_id_4);
}
{
    ompd_generated_task_input_data_5_t task_input_data_5;
    ompd_task_definition_t task_definition_5;
    ompd_task_id_t task_id_5;
    task_input_data_5.input_value_0 = i;
    task_definition_5.generated_task_kind = OMPD_TASK_KIND_GENERATED_5;
    task_definition_5.task_input_data = &task_input_data_5;
    task_definition_5.task_input_data_size = sizeof(task_input_data_5);
    task_id_5 = ompd_build_task(&task_definition_5);
    ompd_register_task_input_array_element(task_id_5, "d", i, sizeof(int));
    ompd_register_task_output_array_element(task_id_5, "e", i, sizeof(int));
    ompd_submit_task(task_id_5);
}
	} 
	for (i=0; i<N; i++) {
{
    ompd_generated_task_input_data_6_t task_input_data_6;
    ompd_task_definition_t task_definition_6;
    ompd_task_id_t task_id_6;
    task_input_data_6.input_value_0 = i;
    task_definition_6.generated_task_kind = OMPD_TASK_KIND_GENERATED_6;
    task_definition_6.task_input_data = &task_input_data_6;
    task_definition_6.task_input_data_size = sizeof(task_input_data_6);
    task_id_6 = ompd_build_task(&task_definition_6);
    ompd_register_task_input_array_element(task_id_6, "a", i, sizeof(int));
    ompd_register_task_input_array_element(task_id_6, "b", i, sizeof(int));
    ompd_register_task_input_array_element(task_id_6, "c", i, sizeof(int));
    ompd_register_task_input_array_element(task_id_6, "d", i, sizeof(int));
    ompd_register_task_input_array_element(task_id_6, "e", i, sizeof(int));
    ompd_submit_task(task_id_6);
}
	}
	/* implicit task synchronization at end of cluster */
	ompd_wait_for_child_tasks();
if (ompd_current_rank_is_master()) {
	} 
}
	ompd_finalize_runtime();
} 


void DeclareTypesMPI() {
}
