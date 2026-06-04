#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <mpi.h>
#include "ompd_runtime.h"

#ifdef _OPENMP
    #include <omp.h>
    double start_time, elapsed_time;
#else
    struct timeval t1, t2;
    double elapsed_time;
#endif

static long fib_seq();
long fibonacci();

static long fib_seq(int n){
    long n1, n2;

    if(n < 2){
        return n;
    }

    n1 = fib_seq(n-1);
    n2 = fib_seq(n-2);
    return n1 + n2;
}

typedef struct {
    int input_value_0;
    int input_value_1;
} ompd_generated_task_input_data_0_t;

typedef struct {
    int input_value_0;
    int input_value_1;
} ompd_generated_task_input_data_1_t;

typedef enum {
    OMPD_TASK_KIND_GENERATED_0 = 1,
    OMPD_TASK_KIND_GENERATED_1
} ompd_generated_task_kind_t;

static void ompd_generated_task_body_0(void *raw_task_input_data)
{
    long n1_output_value;
    ompd_generated_task_input_data_0_t *task_input_data;

    task_input_data = (ompd_generated_task_input_data_0_t *) raw_task_input_data;

    n1_output_value = fibonacci(
        task_input_data->input_value_0,
        task_input_data->input_value_1);

    ompd_runtime_send_long_value("n1", n1_output_value);
}

static void ompd_generated_task_body_1(void *raw_task_input_data)
{
    long n2_output_value;
    ompd_generated_task_input_data_1_t *task_input_data;

    task_input_data = (ompd_generated_task_input_data_1_t *) raw_task_input_data;

    n2_output_value = fibonacci(
        task_input_data->input_value_0,
        task_input_data->input_value_1);

    ompd_runtime_send_long_value("n2", n2_output_value);
}

void ompd_execute_generated_task(int task_kind, void *raw_task_input_data)
{
    switch (task_kind) {
        case OMPD_TASK_KIND_GENERATED_0:
            ompd_generated_task_body_0(raw_task_input_data);
            break;

        case OMPD_TASK_KIND_GENERATED_1:
            ompd_generated_task_body_1(raw_task_input_data);
            break;

        default:
            fprintf(stderr, "Invalid generated task kind: %d\n", task_kind);
            MPI_Abort(MPI_COMM_WORLD, 1);
    }
}

long fibonacci(int n, int thresh){
    long n1, n2;

    ompd_generated_task_input_data_0_t task_input_data_0;
    ompd_generated_task_input_data_1_t task_input_data_1;

    ompd_task_definition_t task_definition_0;
    ompd_task_definition_t task_definition_1;

    ompd_task_id_t task_id_0;
    ompd_task_id_t task_id_1;

    if(n < 2){
        return n;
    }

    if(n <= thresh){
        return fib_seq(n);
    }

    // #pragma omp cluster
    {
        task_input_data_0.input_value_0 = n - 1;
        task_input_data_0.input_value_1 = thresh;

        task_definition_0.generated_task_kind = OMPD_TASK_KIND_GENERATED_0;
        task_definition_0.task_input_data = &task_input_data_0;
        task_definition_0.task_input_data_size = sizeof(task_input_data_0);
        
        // #pragma omp task_async depend(out:n1)
        task_id_0 = ompd_build_task(&task_definition_0);
        ompd_register_task_output_variable(task_id_0, "n1", sizeof(long));
        ompd_submit_task(task_id_0);

        task_input_data_1.input_value_0 = n - 2;
        task_input_data_1.input_value_1 = thresh;

        task_definition_1.generated_task_kind = OMPD_TASK_KIND_GENERATED_1;
        task_definition_1.task_input_data = &task_input_data_1;
        task_definition_1.task_input_data_size = sizeof(task_input_data_1);

        // #pragma omp task_async depend(out:n2)
        task_id_1 = ompd_build_task(&task_definition_1);
        ompd_register_task_output_variable(task_id_1, "n2", sizeof(long));
        ompd_submit_task(task_id_1);

        // #pragma omp taskwait
        ompd_wait_for_child_tasks();

        n1 = ompd_runtime_recv_long_value("n1");
        n2 = ompd_runtime_recv_long_value("n2");
    }

    return n1 + n2;
}

int main(int argc, char **argv)
{
    int n, thresh;
    long res = 0;

    if (argc < 3) {
        fprintf(stderr, "Invalid argument\n");
        return 1;
    }

    if ((n = atoi(argv[1])) < 0 || (thresh = atoi(argv[2])) < 0) {
        fprintf(stderr, "Invalid argument\n");
        return 1;
    }

    ompd_initialize_runtime(&argc, &argv);

    if(ompd_current_rank_is_master()) {

#ifdef _OPENMP
        start_time = omp_get_wtime();
#else
        gettimeofday(&t1, NULL);
#endif

        res = fibonacci(n, thresh);
        fprintf(stdout, "fibonacci(%d) = %ld\n", n, res);

#ifdef _OPENMP
        elapsed_time = omp_get_wtime() - start_time;
        printf("time: %f seconds\n", elapsed_time);
#else
        gettimeofday(&t2, NULL);
        elapsed_time = (((t2.tv_usec - t1.tv_usec) / 1000000.0f) + (t2.tv_sec - t1.tv_sec));
        fprintf(stdout, "time: %f seconds\n", elapsed_time);
#endif
    }

    ompd_finalize_runtime();
    return 0;
}
