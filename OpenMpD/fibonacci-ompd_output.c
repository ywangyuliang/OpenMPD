                    #include <assert.h>
#include <mpi.h>
#include <stdio.h>
#include "ompd_runtime.h"
                          
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
static long fib_seq();
long fibonacci();
typedef enum {
    OMPD_TASK_KIND_GENERATED_1 = 1,
    OMPD_TASK_KIND_GENERATED_2 = 2,
} ompd_generated_task_kind_t;

typedef struct {
    int input_value_0;
    int input_value_1;
} ompd_generated_task_input_data_1_t;

static void ompd_generated_task_body_1(void *raw_task_input_data)
{
    ompd_generated_task_input_data_1_t *task_input_data =
        (ompd_generated_task_input_data_1_t *) raw_task_input_data;

    long n1_output_value;


    {
    n1_output_value = fibonacci(task_input_data->input_value_0 - 1, task_input_data->input_value_1);
    }
    ompd_runtime_send_long_value("n1", n1_output_value);

}

typedef struct {
    int input_value_0;
    int input_value_1;
} ompd_generated_task_input_data_2_t;

static void ompd_generated_task_body_2(void *raw_task_input_data)
{
    ompd_generated_task_input_data_2_t *task_input_data =
        (ompd_generated_task_input_data_2_t *) raw_task_input_data;

    long n2_output_value;


    {
    n2_output_value = fibonacci(task_input_data->input_value_0 - 2, task_input_data->input_value_1);
    }
    ompd_runtime_send_long_value("n2", n2_output_value);

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
        default:
            fprintf(stderr, "Invalid generated task kind: %d\n", task_kind);
            MPI_Abort(MPI_COMM_WORLD, 1);
    }
}

int main();
                  
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#ifdef _OPENMP
#include <omp.h>
void DeclareTypesMPI();

int __taskid = -1, __numprocs = -1;
MPI_Status __status;

        double start_time, elapsed_time;
#else
    struct timeval t1, t2;
    double elapsed_time;
#endif

static long fib_seq(int n){
    
    long n1, n2;

    if(n < 2){
        return n;
    }

    n1 = fib_seq(n-1);
    n2 = fib_seq(n-2);
    return n1 + n2; 
}

long fibonacci(int n, int thresh){
    
    long n1, n2;

    if(n < 2){
        return n;
    }

    if(n <= thresh){
        return fib_seq(n);
    }

	        {



{
    ompd_generated_task_input_data_1_t task_input_data_1;
    ompd_task_definition_t task_definition_1;
    ompd_task_id_t task_id_1;
    task_input_data_1.input_value_0 = n;
    task_input_data_1.input_value_1 = thresh;
    task_definition_1.generated_task_kind = OMPD_TASK_KIND_GENERATED_1;
    task_definition_1.task_input_data = &task_input_data_1;
    task_definition_1.task_input_data_size = sizeof(task_input_data_1);
    task_id_1 = ompd_build_task(&task_definition_1);
    ompd_register_task_output_variable(task_id_1, "n1", sizeof(long));
    ompd_submit_task(task_id_1);
}
{
    ompd_generated_task_input_data_2_t task_input_data_2;
    ompd_task_definition_t task_definition_2;
    ompd_task_id_t task_id_2;
    task_input_data_2.input_value_0 = n;
    task_input_data_2.input_value_1 = thresh;
    task_definition_2.generated_task_kind = OMPD_TASK_KIND_GENERATED_2;
    task_definition_2.task_input_data = &task_input_data_2;
    task_definition_2.task_input_data_size = sizeof(task_input_data_2);
    task_id_2 = ompd_build_task(&task_definition_2);
    ompd_register_task_output_variable(task_id_2, "n2", sizeof(long));
    ompd_submit_task(task_id_2);
}
ompd_wait_for_child_tasks();
n1 = ompd_runtime_recv_long_value("n1");
n2 = ompd_runtime_recv_long_value("n2");
	/* implicit task synchronization at end of cluster */
	ompd_wait_for_child_tasks();
	            }

	    return n1 + n2; 
	}

	int main(int argc, const char **argv)
	{
	    int n, thresh;
	    long res;
	        
	    if(argc < 3){
	        fprintf(stderr, "Invalid argument\n");
	        return 1;
	    }
	    if((n = atoi(argv[1])) < 0 || (thresh = atoi(argv[2])) < 0){
	        fprintf(stderr, "Invalid argument\n");
	        return 1;
	    }
	    
	ompd_initialize_runtime(&argc, (char ***)&argv);
if (ompd_current_rank_is_master()) {
#ifdef _OPENMP
	    start_time = omp_get_wtime();
#else
	    gettimeofday(&t1, NULL);
#endif

}
	        {
if (ompd_current_rank_is_master()) {
	                res = fibonacci(n, thresh);
}
if (ompd_current_rank_is_master()) {
	    }

	    fprintf(stdout, "fibonacci(%d) = %ld\n", n, res);

#ifdef _OPENMP
	    elapsed_time = omp_get_wtime() - start_time;
		printf("time: %f seconds\n", elapsed_time);
#else
		gettimeofday(&t2, NULL);
		elapsed_time = (((t2.tv_usec - t1.tv_usec)/1000000.0f)  + (t2.tv_sec - t1.tv_sec));
		fprintf(stdout, "time: %f seconds\n", elapsed_time);
#endif

}
	ompd_finalize_runtime();
    return 0;
}


void DeclareTypesMPI() {
}
