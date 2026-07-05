#ifndef TASK_ASYNC_BLOCK_H
#define TASK_ASYNC_BLOCK_H

/*
 * Represents one task_async block inside a tasking region. It stores the task
 * id, generated task kind, dependency lists, captured input values and both
 * the original and generated task body text.
 */

#include "tasking_region.h"

typedef struct task_async_block {
    int task_id;
    int generated_task_kind;
    task_depend_list_t depends;
    task_input_data_list_t input_datas;
    char* body_text;
    char* emit_body_text;
    int body_prepared;
} task_async_block_t;

/* Initializes an async task block */
void task_async_block_init(task_async_block_t *task_block);

/* Releases all memory owned by an async task block */
void task_async_block_destroy(task_async_block_t *task_block);

/* Adds one dependency to an async task block */
int task_async_block_add_depend(
    task_async_block_t *task_block,
    task_depend_kind_t kind,
    task_depend_obj_kind_t obj_kind,
    const char *raw_text,
    const char *name,
    const char *index,
    const char *length,
    const char *value_type,
    const char *mpi_type
);

/* Adds one captured input value to an async task block */
int task_async_block_add_input_data(
    task_async_block_t *task_block,
    const char *field,
    const char *expr,
    const char *value_type
);

/* Stores the original task body text */
int task_async_block_set_body_text(task_async_block_t *task_block, const char* body);

/* Stores the generated task body text */
int task_async_block_set_emit_body_text(task_async_block_t *task_block, const char* body);

#endif
