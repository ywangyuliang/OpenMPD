#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

#define MAX_THREADS 64

static long same_parent_thread = 0;
static long different_parent_thread = 0;
static long tasks_per_thread[MAX_THREADS] = {0};

static void small_work(int iterations)
{
    volatile long x = 0;

    for (int i = 0; i < iterations; i++) {
        x += i % 7;
    }
}

static void task_tree(int level, int max_level, int work, int parent_thread)
{
    int thread_id = omp_get_thread_num();

    #pragma omp atomic
    tasks_per_thread[thread_id]++;

    if (parent_thread != -1) {
        if (thread_id == parent_thread) {
            #pragma omp atomic
            same_parent_thread++;
        } else {
            #pragma omp atomic
            different_parent_thread++;
        }
    }

    small_work(work);

    if (level < max_level) {
        #pragma omp task firstprivate(level, max_level, work, thread_id)
        task_tree(level + 1, max_level, work, thread_id);

        #pragma omp task firstprivate(level, max_level, work, thread_id)
        task_tree(level + 1, max_level, work, thread_id);

        #pragma omp taskwait
    }
}

static long expected_tasks(int depth)
{
    long total = 0;
    long level_tasks = 1;

    for (int i = 0; i <= depth; i++) {
        total += level_tasks;
        level_tasks *= 2;
    }

    return total;
}

int main(int argc, char **argv)
{
    int depth = 12;
    int work = 1000;

    if (argc >= 2) {
        depth = atoi(argv[1]);
    }

    if (argc >= 3) {
        work = atoi(argv[2]);
    }

    double start = omp_get_wtime();

    #pragma omp parallel
    {
        #pragma omp single
        {
            printf("OpenMP threads = %d\n", omp_get_num_threads());
            task_tree(0, depth, work, -1);
        }
    }

    double end = omp_get_wtime();

    long same = same_parent_thread;
    long different = different_parent_thread;
    long total_children = same + different;

    printf("\n--- Simple OpenMP task experiment ---\n");
    printf("tree depth              = %d\n", depth);
    printf("expected total tasks    = %ld\n", expected_tasks(depth));
    printf("measured child tasks    = %ld\n", total_children);
    printf("execution time          = %f seconds\n", end - start);

    printf("\nParent/executor test:\n");
    printf("same parent thread      = %ld\n", same);
    printf("different parent thread = %ld\n", different);

    if (total_children > 0) {
        printf("same parent percentage  = %.2f%%\n",
               100.0 * (double)same / (double)total_children);
        printf("different percentage    = %.2f%%\n",
               100.0 * (double)different / (double)total_children);
    }

    printf("\nTasks executed per thread:\n");
    printf("thread | tasks\n");
    printf("---------------\n");

    for (int i = 0; i < omp_get_max_threads(); i++) {
        if (tasks_per_thread[i] > 0) {
            printf("%6d | %ld\n", i, tasks_per_thread[i]);
        }
    }

    return 0;
}