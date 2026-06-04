#ifndef TASK_ASYNC_BLOCK_H
#define TASK_ASYNC_BLOCK_H

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

void task_async_block_init(task_async_block_t *ta_aync_block);
void task_async_block_destroy(task_async_block_t *ta_aync_block);

int task_async_block_add_depend(
    task_async_block_t *ta_aync_block,
    task_depend_kind_t kind,
    task_depend_obj_kind_t obj_kind,
    const char *raw_text,
    const char *name,
    const char *index,
    const char *length,
    const char *value_type,
    const char *mpi_type
);

int task_async_block_add_input_data(
    task_async_block_t *ta_aync_block,
    const char *field,
    const char *expr,
    const char *value_type
);

int task_async_block_set_body_text(task_async_block_t *ta_aync_block, const char* body);
int task_async_block_set_emit_body_text(task_async_block_t *ta_aync_block, const char* body);

#endif
