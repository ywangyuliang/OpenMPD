                    #include <assert.h>
#include <mpi.h>
#include <stdio.h>
#include "ompd_runtime.h"
                          
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
                                                                                                                                
double f();
double simpson();
double simpson_seq();
double simpson_ompd();
typedef enum {
    OMPD_TASK_KIND_GENERATED_1 = 1,
    OMPD_TASK_KIND_GENERATED_2 = 2,
} ompd_generated_task_kind_t;

typedef struct {
    double input_value_0;
    double input_value_1;
    double input_value_2;
    double input_value_3;
    double input_value_4;
    double input_value_5;
    double input_value_6;
    int input_value_7;
    int input_value_8;
} ompd_generated_task_input_data_1_t;

static void ompd_generated_task_body_1(void *raw_task_input_data)
{
    ompd_generated_task_input_data_1_t *task_input_data =
        (ompd_generated_task_input_data_1_t *) raw_task_input_data;

    double left_result_output_value;


    {
    left_result_output_value = simpson_ompd(task_input_data->input_value_0, task_input_data->input_value_1, task_input_data->input_value_2, task_input_data->input_value_3, task_input_data->input_value_4, task_input_data->input_value_5, task_input_data->input_value_6, task_input_data->input_value_7, task_input_data->input_value_8);
    }
    ompd_runtime_send_double_value("left_result", left_result_output_value);

}

typedef struct {
    double input_value_0;
    double input_value_1;
    double input_value_2;
    double input_value_3;
    double input_value_4;
    double input_value_5;
    double input_value_6;
    int input_value_7;
    int input_value_8;
} ompd_generated_task_input_data_2_t;

static void ompd_generated_task_body_2(void *raw_task_input_data)
{
    ompd_generated_task_input_data_2_t *task_input_data =
        (ompd_generated_task_input_data_2_t *) raw_task_input_data;

    double right_result_output_value;


    {
    right_result_output_value = simpson_ompd(task_input_data->input_value_0, task_input_data->input_value_1, task_input_data->input_value_2, task_input_data->input_value_3, task_input_data->input_value_4, task_input_data->input_value_5, task_input_data->input_value_6, task_input_data->input_value_7, task_input_data->input_value_8);
    }
    ompd_runtime_send_double_value("right_result", right_result_output_value);

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
#include <math.h>
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

#define PI 3.14159265358979323846

double f(double x)
{
    return sin(x);
}

double simpson(double a, double b, double fa, double fm, double fb)
{
    return (b - a) * (fa + 4.0 * fm + fb) / 6.0;
}

double simpson_seq(double a, double b, double fa, double fm, double fb, double whole, double eps, int depth)
{
    double m = (a + b) / 2.0;
    double lm = (a + m) / 2.0;
    double rm = (m + b) / 2.0;

    double flm = f(lm);
    double frm = f(rm);

    double left = simpson(a, m, fa, flm, fm);
    double right = simpson(m, b, fm, frm, fb);

    double error = fabs(left + right - whole);

    if (depth <= 0 || error <= 15.0 * eps) {
        return left + right + (left + right - whole) / 15.0;
    }

    double eps2 = eps * 0.5;
    int next_depth = depth - 1;

    return simpson_seq(a, m, fa, flm, fm, left, eps2, next_depth) + simpson_seq(m, b, fm, frm, fb, right, eps2, next_depth);
}

double simpson_ompd(double a, double b, double fa, double fm, double fb, double whole, double eps, int depth, int thresh)
{
    double m = (a + b) / 2.0;
    double lm = (a + m) / 2.0;
    double rm = (m + b) / 2.0;

    double flm = f(lm);
    double frm = f(rm);

    double left = simpson(a, m, fa, flm, fm);
    double right = simpson(m, b, fm, frm, fb);

    double error = fabs(left + right - whole);

    if (depth <= 0 || error <= 15.0 * eps) {
        return left + right + (left + right - whole) / 15.0;
    }

    double eps2 = eps * 0.5;
    int next_depth = depth - 1;

    if (depth <= thresh) {
        return simpson_seq(a, m, fa, flm, fm, left, eps2, next_depth) + simpson_seq(m, b, fm, frm, fb, right, eps2, next_depth);
    }

    double left_result = 0.0;
    double right_result = 0.0;

	        {




{
    ompd_generated_task_input_data_1_t task_input_data_1;
    ompd_task_definition_t task_definition_1;
    ompd_task_id_t task_id_1;
    task_input_data_1.input_value_0 = a;
    task_input_data_1.input_value_1 = m;
    task_input_data_1.input_value_2 = fa;
    task_input_data_1.input_value_3 = flm;
    task_input_data_1.input_value_4 = fm;
    task_input_data_1.input_value_5 = left;
    task_input_data_1.input_value_6 = eps2;
    task_input_data_1.input_value_7 = next_depth;
    task_input_data_1.input_value_8 = thresh;
    task_definition_1.generated_task_kind = OMPD_TASK_KIND_GENERATED_1;
    task_definition_1.task_input_data = &task_input_data_1;
    task_definition_1.task_input_data_size = sizeof(task_input_data_1);
    task_id_1 = ompd_build_task(&task_definition_1);
    ompd_register_task_output_variable(task_id_1, "left_result", sizeof(double));
    ompd_submit_task(task_id_1);
}
{
    ompd_generated_task_input_data_2_t task_input_data_2;
    ompd_task_definition_t task_definition_2;
    ompd_task_id_t task_id_2;
    task_input_data_2.input_value_0 = m;
    task_input_data_2.input_value_1 = b;
    task_input_data_2.input_value_2 = fm;
    task_input_data_2.input_value_3 = frm;
    task_input_data_2.input_value_4 = fb;
    task_input_data_2.input_value_5 = right;
    task_input_data_2.input_value_6 = eps2;
    task_input_data_2.input_value_7 = next_depth;
    task_input_data_2.input_value_8 = thresh;
    task_definition_2.generated_task_kind = OMPD_TASK_KIND_GENERATED_2;
    task_definition_2.task_input_data = &task_input_data_2;
    task_definition_2.task_input_data_size = sizeof(task_input_data_2);
    task_id_2 = ompd_build_task(&task_definition_2);
    ompd_register_task_output_variable(task_id_2, "right_result", sizeof(double));
    ompd_submit_task(task_id_2);
}
ompd_wait_for_child_tasks();
left_result = ompd_runtime_recv_double_value("left_result");
right_result = ompd_runtime_recv_double_value("right_result");
	/* implicit task synchronization at end of cluster */
	ompd_wait_for_child_tasks();
	            }

	    return left_result + right_result;
	}

	int main(int argc, const char **argv)
	{
	    double eps = 1e-8;
	    int depth = 20;
	    int thresh = 10;

	    if (argc > 1) {
	        eps = atof(argv[1]);
	    }

	    if (argc > 2) {
	        depth = atoi(argv[2]);
	    }

	    if (argc > 3) {
	        thresh = atoi(argv[3]);
	    }

	    double a = 0.0;
	    double b = PI;
	    double m = (a + b) / 2.0;

	    double fa = f(a);
	    double fm = f(m);
	    double fb = f(b);

	    double whole = simpson(a, b, fa, fm, fb);

	    double result = 0.0;

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
	                result = simpson_ompd(a, b, fa, fm, fb, whole, eps, depth, thresh);
}
if (ompd_current_rank_is_master()) {
	    }

#ifdef _OPENMP
	    elapsed_time = omp_get_wtime() - start_time;
#else
	    gettimeofday(&t2, NULL);
	    elapsed_time = (((t2.tv_usec - t1.tv_usec) / 1000000.0f)
	                    + (t2.tv_sec - t1.tv_sec));
#endif

	    printf("Simpson integral of sin(x) in [0, pi]\n");
	    printf("eps = %.12e\n", eps);
	    printf("depth = %d\n", depth);
	    printf("thresh = %d\n", thresh);
	    printf("result = %.15f\n", result);
	    printf("expected = %.15f\n", 2.0);
	    printf("error = %.12e\n", fabs(result - 2.0));
	    printf("time: %f seconds\n", elapsed_time);

}
	ompd_finalize_runtime();
    return 0;
}

void DeclareTypesMPI() {
}
