#include "task_async_block.h"
#include <stdlib.h>
#include <string.h>

static char* sstrdup(const char* s){
    
    size_t len;
    char* copy;

    if(s == NULL){
        return NULL;
    }

    len = strlen(s);
    copy = (char*)malloc(len + 1);
    if(copy == NULL){
        return NULL;
    }
    memcpy(copy, s, len + 1);
    return copy;
}

void task_async_block_init(task_async_block_t *ta_aync_block){
    if(ta_aync_block == NULL){
        return;
    }

    ta_aync_block->task_id = -1;
    ta_aync_block->generated_task_kind = -1;
    ta_aync_block->body_text = NULL;
    ta_aync_block->emit_body_text = NULL;
    ta_aync_block->body_prepared = 0;

    task_depend_list_init(&ta_aync_block->depends);
    task_input_data_list_init(&ta_aync_block->input_datas);
}

void task_async_block_destroy(task_async_block_t *ta_aync_block){
    if(ta_aync_block == NULL){
        return;
    }

    ta_aync_block->task_id = -1;
    ta_aync_block->generated_task_kind = -1;

    free(ta_aync_block->body_text);
    ta_aync_block->body_text = NULL;

    free(ta_aync_block->emit_body_text);
    ta_aync_block->emit_body_text = NULL;

    ta_aync_block->body_prepared = 0;

    task_depend_list_destroy(&ta_aync_block->depends);
    task_input_data_list_destroy(&ta_aync_block->input_datas);
}

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
){
    if(ta_aync_block == NULL){
        return -1;
    }

    return task_depend_list_add(&ta_aync_block->depends, kind, obj_kind, raw_text, name, index, length, value_type, mpi_type);
}

int task_async_block_add_input_data(task_async_block_t *ta_aync_block, const char *field, const char *expr, const char *value_type){
    if(ta_aync_block == NULL){
        return -1;
    }

    return task_input_data_list_add(&ta_aync_block->input_datas, field, expr, value_type);
}

int task_async_block_set_body_text(task_async_block_t *ta_aync_block, const char* body_text) {
    char *copy;

    if(ta_aync_block == NULL || body_text == NULL){
        return -1;
    }

    copy = sstrdup(body_text);
    if(copy == NULL){
        return -1;
    }

    free(ta_aync_block->body_text);
    ta_aync_block->body_text = copy;
    return 0;
}

int task_async_block_set_emit_body_text(task_async_block_t *ta_aync_block, const char* emit_body_text){
    char *copy;

    if(ta_aync_block == NULL || emit_body_text == NULL){
        return -1;
    }

    copy = sstrdup(emit_body_text);
    if(copy == NULL){
        return -1;
    }

    free(ta_aync_block->emit_body_text);
    ta_aync_block->emit_body_text = copy;
    return 0;
}
