                    #include <assert.h>
#include <mpi.h>
#include <stdio.h>
#include "ompd_runtime.h"
                          
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
static double get_time();
static double compute_pi_part();
int main();
typedef enum {
    OMPD_TASK_KIND_GENERATED_1 = 1,
    OMPD_TASK_KIND_GENERATED_2 = 2,
    OMPD_TASK_KIND_GENERATED_3 = 3,
    OMPD_TASK_KIND_GENERATED_4 = 4,
} ompd_generated_task_kind_t;

typedef struct {
    long input_value_0;
    long input_value_1;
} ompd_generated_task_input_data_1_t;

static void ompd_generated_task_body_1(void *raw_task_input_data)
{
    ompd_generated_task_input_data_1_t *task_input_data =
        (ompd_generated_task_input_data_1_t *) raw_task_input_data;

    double s0_output_value;


    {
    s0_output_value = compute_pi_part(0, task_input_data->input_value_0, task_input_data->input_value_1);
    }
    ompd_runtime_send_double_value("s0", s0_output_value);

}

typedef struct {
    long input_value_0;
    long input_value_1;
} ompd_generated_task_input_data_2_t;

static void ompd_generated_task_body_2(void *raw_task_input_data)
{
    ompd_generated_task_input_data_2_t *task_input_data =
        (ompd_generated_task_input_data_2_t *) raw_task_input_data;

    double s1_output_value;


    {
    s1_output_value = compute_pi_part(task_input_data->input_value_0, 2 * task_input_data->input_value_0, task_input_data->input_value_1);
    }
    ompd_runtime_send_double_value("s1", s1_output_value);

}

typedef struct {
    long input_value_0;
    long input_value_1;
} ompd_generated_task_input_data_3_t;

static void ompd_generated_task_body_3(void *raw_task_input_data)
{
    ompd_generated_task_input_data_3_t *task_input_data =
        (ompd_generated_task_input_data_3_t *) raw_task_input_data;

    double s2_output_value;


    {
    s2_output_value = compute_pi_part(2 * task_input_data->input_value_0, 3 * task_input_data->input_value_0, task_input_data->input_value_1);
    }
    ompd_runtime_send_double_value("s2", s2_output_value);

}

typedef struct {
    long input_value_0;
    long input_value_1;
} ompd_generated_task_input_data_4_t;

static void ompd_generated_task_body_4(void *raw_task_input_data)
{
    ompd_generated_task_input_data_4_t *task_input_data =
        (ompd_generated_task_input_data_4_t *) raw_task_input_data;

    double s3_output_value;


    {
    s3_output_value = compute_pi_part(3 * task_input_data->input_value_0, task_input_data->input_value_1, task_input_data->input_value_1);
    }
    ompd_runtime_send_double_value("s3", s3_output_value);

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
        default:
            fprintf(stderr, "Invalid generated task kind: %d\n", task_kind);
            MPI_Abort(MPI_COMM_WORLD, 1);
    }
}

                                                                                         
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

void DeclareTypesMPI();

int __taskid = -1, __numprocs = -1;
MPI_Status __status;

static double get_time()
{
    struct timeval t;
    gettimeofday(&t, NULL);
    return t.tv_sec + t.tv_usec * 1e-6;
}

static double compute_pi_part(long start, long end, long num_steps)
{
    double step = 1.0 / (double) num_steps;
    double sum = 0.0;

    for (long i = start; i < end; i++) {
        double x = (i + 0.5) * step;
        sum += 4.0 / (1.0 + x * x);
    }

    return sum;
}

int main(int argc, char **argv)
{
    long num_steps = 100000000;

    if (argc > 1) {
        num_steps = atol(argv[1]);
    }

    long chunk = num_steps / 4;
    double step = 1.0 / (double) num_steps;

    double s0 = 0.0;
    double s1 = 0.0;
    double s2 = 0.0;
    double s3 = 0.0;

    double start_time = get_time();

	ompd_initialize_runtime(&argc, (char ***)&argv);
	        {








{
    ompd_generated_task_input_data_1_t task_input_data_1;
    ompd_task_definition_t task_definition_1;
    ompd_task_id_t task_id_1;
    task_input_data_1.input_value_0 = chunk;
    task_input_data_1.input_value_1 = num_steps;
    task_definition_1.generated_task_kind = OMPD_TASK_KIND_GENERATED_1;
    task_definition_1.task_input_data = &task_input_data_1;
    task_definition_1.task_input_data_size = sizeof(task_input_data_1);
    task_id_1 = ompd_build_task(&task_definition_1);
    ompd_register_task_output_variable(task_id_1, "s0", sizeof(double));
    ompd_submit_task(task_id_1);
}
{
    ompd_generated_task_input_data_2_t task_input_data_2;
    ompd_task_definition_t task_definition_2;
    ompd_task_id_t task_id_2;
    task_input_data_2.input_value_0 = chunk;
    task_input_data_2.input_value_1 = num_steps;
    task_definition_2.generated_task_kind = OMPD_TASK_KIND_GENERATED_2;
    task_definition_2.task_input_data = &task_input_data_2;
    task_definition_2.task_input_data_size = sizeof(task_input_data_2);
    task_id_2 = ompd_build_task(&task_definition_2);
    ompd_register_task_output_variable(task_id_2, "s1", sizeof(double));
    ompd_submit_task(task_id_2);
}
{
    ompd_generated_task_input_data_3_t task_input_data_3;
    ompd_task_definition_t task_definition_3;
    ompd_task_id_t task_id_3;
    task_input_data_3.input_value_0 = chunk;
    task_input_data_3.input_value_1 = num_steps;
    task_definition_3.generated_task_kind = OMPD_TASK_KIND_GENERATED_3;
    task_definition_3.task_input_data = &task_input_data_3;
    task_definition_3.task_input_data_size = sizeof(task_input_data_3);
    task_id_3 = ompd_build_task(&task_definition_3);
    ompd_register_task_output_variable(task_id_3, "s2", sizeof(double));
    ompd_submit_task(task_id_3);
}
{
    ompd_generated_task_input_data_4_t task_input_data_4;
    ompd_task_definition_t task_definition_4;
    ompd_task_id_t task_id_4;
    task_input_data_4.input_value_0 = chunk;
    task_input_data_4.input_value_1 = num_steps;
    task_definition_4.generated_task_kind = OMPD_TASK_KIND_GENERATED_4;
    task_definition_4.task_input_data = &task_input_data_4;
    task_definition_4.task_input_data_size = sizeof(task_input_data_4);
    task_id_4 = ompd_build_task(&task_definition_4);
    ompd_register_task_output_variable(task_id_4, "s3", sizeof(double));
    ompd_submit_task(task_id_4);
}
ompd_wait_for_child_tasks();
s0 = ompd_runtime_recv_double_value("s0");
s1 = ompd_runtime_recv_double_value("s1");
s2 = ompd_runtime_recv_double_value("s2");
s3 = ompd_runtime_recv_double_value("s3");
	/* implicit task synchronization at end of cluster */
	ompd_wait_for_child_tasks();
if (ompd_current_rank_is_master()) {
	            }

}
	    double pi = step * (s0 + s1 + s2 + s3);
	    double elapsed_time = get_time() - start_time;

if (ompd_current_rank_is_master()) {
	    printf("num_steps = %ld\n", num_steps);
	    printf("pi = %.15f\n", pi);
	    printf("time = %.6f seconds\n", elapsed_time);

}
	ompd_finalize_runtime();
    return 0;
}

void DeclareTypesMPI() {
}
