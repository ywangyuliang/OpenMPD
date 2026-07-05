#include "task_async_block.h"
#include "task_utils.h"
#include <stdlib.h>

void task_async_block_init(task_async_block_t *task_block){
    if(task_block == NULL){
        return;
    }

    task_block->task_id = -1;
    task_block->generated_task_kind = -1;
    task_block->body_text = NULL;
    task_block->emit_body_text = NULL;
    task_block->body_prepared = 0;

    task_depend_list_init(&task_block->depends);
    task_input_data_list_init(&task_block->input_datas);
}

void task_async_block_destroy(task_async_block_t *task_block){
    if(task_block == NULL){
        return;
    }

    task_block->task_id = -1;
    task_block->generated_task_kind = -1;

    free(task_block->body_text);
    task_block->body_text = NULL;

    free(task_block->emit_body_text);
    task_block->emit_body_text = NULL;

    task_block->body_prepared = 0;

    task_depend_list_destroy(&task_block->depends);
    task_input_data_list_destroy(&task_block->input_datas);
}

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
){
    if(task_block == NULL){
        return -1;
    }

    return task_depend_list_add(&task_block->depends, kind, obj_kind, raw_text, name, index, length, value_type, mpi_type);
}

int task_async_block_add_input_data(task_async_block_t *task_block, const char *field, const char *expr, const char *value_type){
    if(task_block == NULL){
        return -1;
    }

    return task_input_data_list_add(&task_block->input_datas, field, expr, value_type);
}

int task_async_block_set_body_text(task_async_block_t *task_block, const char* body_text) {
    char *copy;

    if(task_block == NULL || body_text == NULL){
        return -1;
    }

    copy = ompd_strdup(body_text);
    if(copy == NULL){
        return -1;
    }

    free(task_block->body_text);
    task_block->body_text = copy;
    return 0;
}

int task_async_block_set_emit_body_text(task_async_block_t *task_block, const char* emit_body_text){
    char *copy;

    if(task_block == NULL || emit_body_text == NULL){
        return -1;
    }

    copy = ompd_strdup(emit_body_text);
    if(copy == NULL){
        return -1;
    }

    free(task_block->emit_body_text);
    task_block->emit_body_text = copy;
    return 0;
}
