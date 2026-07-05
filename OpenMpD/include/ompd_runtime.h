#ifndef OMPD_RUNTIME_H
#define OMPD_RUNTIME_H

/*
 * Runtime API compiled into generated tasking programs. It initializes MPI,
 * assigns scheduler/master/worker roles, registers task dependencies, submits
 * tasks, exchanges dependency values and shuts the runtime down.
 */

#include <stddef.h>

typedef int ompd_task_id_t;
typedef int ompd_task_kind_t;

typedef struct {
    ompd_task_kind_t generated_task_kind;
    void *task_input_data;
    size_t task_input_data_size;
} ompd_task_definition_t;

/* Returns whether the current MPI rank acts as scheduler */
int ompd_current_rank_is_scheduler();

/* Returns whether the current MPI rank acts as master */
int ompd_current_rank_is_master();

/* Returns whether the current MPI rank acts as worker */
int ompd_current_rank_is_worker();

/* Initializes the OMPD runtime and MPI environment */
void ompd_initialize_runtime(int *argc, char ***argv);

/* Validates that the active region uses the expected number of teams */
void ompd_check_region_num_teams(int num_teams);

/* Builds a runtime task from a generated task definition */
ompd_task_id_t ompd_build_task(const ompd_task_definition_t *task_definition);

/* Registers a scalar input dependency for a task */
void ompd_register_task_input_variable(ompd_task_id_t task_id, const char *name, size_t value_size);

/* Registers a scalar output dependency for a task */
void ompd_register_task_output_variable(ompd_task_id_t task_id, const char *name, size_t value_size);

/* Registers one array-element input dependency for a task */
void ompd_register_task_input_array_element(ompd_task_id_t task_id, const char *name, int element_index, size_t element_size);

/* Registers one array-element output dependency for a task */
void ompd_register_task_output_array_element(ompd_task_id_t task_id, const char *name, int element_index, size_t element_size);

/* Submits a prepared task to the runtime scheduler */
void ompd_submit_task(ompd_task_id_t task_id);

/* Executes generated task code for a task kind */
void ompd_execute_generated_task(ompd_task_kind_t task_kind, void *raw_task_input_data);

/* Waits until all child tasks of the current task finish */
void ompd_wait_for_child_tasks();

/* Receives an integer value from the runtime */
int ompd_runtime_recv_int_value(const char *name);

/* Receives a long value from the runtime */
long ompd_runtime_recv_long_value(const char *name);

/* Receives a double value from the runtime */
double ompd_runtime_recv_double_value(const char *name);

/* Sends an integer value through the runtime */
void ompd_runtime_send_int_value(const char *name, int value);

/* Sends a long value through the runtime */
void ompd_runtime_send_long_value(const char *name, long value);

/* Sends a double value through the runtime */
void ompd_runtime_send_double_value(const char *name, double value);

/* Finalizes the OMPD runtime and MPI environment */
void ompd_finalize_runtime();

#endif
