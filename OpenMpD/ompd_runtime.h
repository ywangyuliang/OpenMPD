#ifndef OMPD_RUNTIME_H
#define OMPD_RUNTIME_H

#include <stddef.h>

typedef int ompd_task_id_t;
typedef int ompd_task_kind_t;

typedef struct {
    ompd_task_kind_t generated_task_kind;
    void *task_input_data;
    size_t task_input_data_size;
} ompd_task_definition_t;

int ompd_current_rank_is_scheduler();
int ompd_current_rank_is_master();
int ompd_current_rank_is_worker();

void ompd_initialize_runtime(int *argc, char ***argv);

void ompd_check_region_num_teams(int num_teams);

ompd_task_id_t ompd_build_task(const ompd_task_definition_t *task_definition);

void ompd_register_task_input_variable(ompd_task_id_t task_id, const char *name, size_t value_size); 
void ompd_register_task_output_variable(ompd_task_id_t task_id, const char *name, size_t value_size); 
void ompd_register_task_input_array_element(ompd_task_id_t task_id, const char *name, int element_index, size_t element_size); 
void ompd_register_task_output_array_element(ompd_task_id_t task_id, const char *name, int element_index, size_t element_size); 

void ompd_submit_task(ompd_task_id_t task_id);

void ompd_execute_generated_task(ompd_task_kind_t task_kind, void *raw_task_input_data);

void ompd_wait_for_child_tasks();

int ompd_runtime_recv_int_value(const char *name);
long ompd_runtime_recv_long_value(const char *name);
double ompd_runtime_recv_double_value(const char *name);

void ompd_runtime_send_int_value(const char *name, int value);
void ompd_runtime_send_long_value(const char *name, long value);
void ompd_runtime_send_double_value(const char *name, double value);

void ompd_finalize_runtime();

#endif
