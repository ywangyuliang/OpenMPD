

#include "tasking_region.h"
#include "task_async_block.h"
#include "task_body_transform.h"
#include "hash_map.h"
#include "task_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

int tasking_region_id_counter = 0;

/* Parsed pieces of a dependency key: name and optional array index/length */
typedef struct {
    int is_array_element;
    char *name;
    char *index;    /* only array */
    char *length;   /* only array */
} depend_parts_t;

typedef struct {
    char *raw_text;
} normal_block_t;

typedef struct {
    char *raw_text;
} taskwait_block_t;

/* One block in a tasking region: a tagged union over the block kinds */
typedef struct {
    tasking_block_kind_t kind;

    union {
        normal_block_t *normal_block;
        task_async_block_t *task_async_block;
        taskwait_block_t *taskwait_block;
    } block;
} tasking_block_t;

/* Dependency analysis: last producer and consumers per dependency key */
typedef struct {
    hash_map_t *last_producers;
    hash_map_t *consumers;
} tasking_region_analytics_t;

struct tasking_region {
    int region_id;
    tasking_block_t *blocks;
    int block_count;
    int block_capacity;

    int current_open_block;

    int next_task_id;
    int next_generated_task_kind;

    tasking_region_analytics_t analysis;
};

static void trim(char* s){
    char* left;
    char* right;
    size_t new_len;

    if(s == NULL){
        return;
    }

    left = s;
    while(*left != '\0' && isspace((unsigned char)*left)){
        left++;
    }

    if (*left == '\0'){
        *s = '\0';
        return;
    }
    
    right = left + strlen(left);
    while(right > left && isspace((unsigned char)*(right - 1))){
        right--;
    }

    new_len = (size_t)(right - left);
    memmove(s, left, new_len);
    s[new_len] = '\0';
}

static void depend_parts_init(depend_parts_t* depend_parts) {
    if(depend_parts == NULL){
        return;
    }

    depend_parts->is_array_element = 0;
    depend_parts->name = NULL;
    depend_parts->index = NULL;
    depend_parts->length = NULL;
}

static void depend_parts_free(depend_parts_t* depend_parts){
    if(depend_parts == NULL){
        return;
    }

    free(depend_parts->name);
    free(depend_parts->index);
    free(depend_parts->length);
}

/* Parses a dependency key string into its name and array index/length parts */
static int parse_depend_key(const char* key, depend_parts_t* out_depend_parts){
    const char* left_bracket;
    const char* right_bracket;
    const char* colon;
    char* resized_buffer;
    size_t text_length;

    if(key == NULL || out_depend_parts == NULL){
        return -1;
    }

    left_bracket = strchr(key, '[');
    right_bracket = strchr(key, ']');
    colon = strchr(key, ':');

    if(left_bracket == NULL){
        text_length = strlen(key);
        resized_buffer = (char*)realloc(out_depend_parts->name, text_length + 1);
        if(resized_buffer == NULL){
            return -1;
        }

        out_depend_parts->name = resized_buffer;
        memcpy(out_depend_parts->name, key, text_length + 1);
        trim(out_depend_parts->name);
        out_depend_parts->is_array_element = 0;
        return 0;
    }

    if(right_bracket == NULL || colon == NULL || !(left_bracket < colon && colon < right_bracket)){
        return -1;
    }

    out_depend_parts->is_array_element = 1;

    /* name */
    text_length = (size_t)(left_bracket - key);
    resized_buffer = (char*)realloc(out_depend_parts->name, text_length + 1);
    if(resized_buffer == NULL){
        return -1;
    }
    out_depend_parts->name = resized_buffer;
    memcpy(out_depend_parts->name, key, text_length);
    out_depend_parts->name[text_length] = '\0';
    trim(out_depend_parts->name);

    /* index */
    text_length = (size_t)(colon - (left_bracket + 1));
    resized_buffer = (char*)realloc(out_depend_parts->index, text_length + 1);
    if(resized_buffer == NULL){
        return -1;
    }
    out_depend_parts->index = resized_buffer;
    memcpy(out_depend_parts->index, left_bracket + 1, text_length);
    out_depend_parts->index[text_length] = '\0';
    trim(out_depend_parts->index);

    /* length */
    text_length = (size_t)(right_bracket - (colon + 1));
    resized_buffer = (char*)realloc(out_depend_parts->length, text_length + 1);
    if(resized_buffer == NULL){
        return -1;
    }
    out_depend_parts->length = resized_buffer;
    memcpy(out_depend_parts->length, colon + 1, text_length);
    out_depend_parts->length[text_length] = '\0';
    trim(out_depend_parts->length);

    return 0;
}

void task_depend_list_init(task_depend_list_t *list){
    if(list == NULL){
        return;
    }

    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

void task_depend_list_destroy(task_depend_list_t *list){
    int i;

    if(list == NULL){
        return;
    }
    
    for(i = 0; i < list->count; i++){
        free(list->items[i].raw_text);
        free(list->items[i].name);
        free(list->items[i].index);
        free(list->items[i].length);
        free(list->items[i].value_type);
        free(list->items[i].mpi_type);
    }

    free(list->items);
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

int task_depend_list_add(
    task_depend_list_t *list,
    task_depend_kind_t kind,
    task_depend_obj_kind_t obj_kind,
    const char *raw_text,
    const char *name,
    const char *index,
    const char *length,
    const char *value_type,
    const char *mpi_type
){
    task_depend_t *new_items;
    task_depend_t *new_dependency;
    int new_capacity;

    if(list == NULL || raw_text == NULL || name == NULL || value_type == NULL || mpi_type == NULL){
        return -1;
    }

    if(list->count >= list->capacity){
        new_capacity = (list->capacity == 0) ? 4 : (list->capacity * 2);
        new_items = (task_depend_t *)realloc(list->items, (size_t)new_capacity * sizeof(task_depend_t));
        if(new_items == NULL){
            return -1;
        }
        memset(new_items + list->capacity, 0, (size_t)(new_capacity - list->capacity) * sizeof(task_depend_t));
        list->items = new_items;
        list->capacity = new_capacity;
    }

    new_dependency = &list->items[list->count];
    new_dependency->kind = kind;
    new_dependency->obj_kind = obj_kind;
    new_dependency->raw_text = ompd_strdup(raw_text);
    new_dependency->name = ompd_strdup(name);
    new_dependency->index = index != NULL ? ompd_strdup(index) : NULL;
    new_dependency->length = length != NULL ? ompd_strdup(length) : NULL;
    new_dependency->value_type = ompd_strdup(ompd_normalize_c_type_name(value_type));
    new_dependency->mpi_type = ompd_strdup(mpi_type);

    if(new_dependency->raw_text == NULL || new_dependency->name == NULL || new_dependency->value_type == NULL || new_dependency->mpi_type == NULL){
        free(new_dependency->raw_text);
        free(new_dependency->name);
        free(new_dependency->index);
        free(new_dependency->length);
        free(new_dependency->value_type);
        free(new_dependency->mpi_type);
        new_dependency->raw_text = NULL;
        new_dependency->name = NULL;
        new_dependency->index = NULL;
        new_dependency->length = NULL;
        new_dependency->value_type = NULL;
        new_dependency->mpi_type = NULL;
        return -1;
    }

    list->count++;
    return 0;
}

void task_input_data_list_init(task_input_data_list_t *list){
    if(list == NULL){
        return;
    }

    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

void task_input_data_list_destroy(task_input_data_list_t *list){
    int i;

    if(list == NULL){
        return;
    }

    for(i = 0; i < list->count; i++){
        free(list->items[i].field);
        free(list->items[i].expr);
        free(list->items[i].value_type);
    }

    free(list->items);
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

int task_input_data_list_add(task_input_data_list_t *list, const char *field, const char *expr, const char *value_type){
    task_input_data_t *new_items;
    task_input_data_t *new_input;
    int new_capacity;

    if(list == NULL || field == NULL || expr == NULL || value_type == NULL){
        return -1;
    }

    if(list->count >= list->capacity){
        new_capacity = (list->capacity == 0) ? 4 : (list->capacity * 2);
        new_items = (task_input_data_t *)realloc(list->items, (size_t)new_capacity * sizeof(task_input_data_t));
        if(new_items == NULL){
            return -1;
        }
        memset(new_items + list->capacity, 0, (size_t)(new_capacity - list->capacity) * sizeof(task_input_data_t));
        list->items = new_items;
        list->capacity = new_capacity;
    }

    new_input = &list->items[list->count];
    new_input->field = ompd_strdup(field);
    new_input->expr = ompd_strdup(expr);
    new_input->value_type = ompd_strdup(ompd_normalize_c_type_name(value_type));

    if(new_input->field == NULL || new_input->expr == NULL || new_input->value_type == NULL){
        free(new_input->field);
        free(new_input->expr);
        free(new_input->value_type);
        return -1;
    }

    list->count++;
    return 0;
}

static void tasking_block_destroy(tasking_block_t *tasking_block){
    if(tasking_block == NULL){
        return;
    }

    switch(tasking_block->kind){
        case TB_NORMAL:
            if(tasking_block->block.normal_block != NULL){
                free(tasking_block->block.normal_block->raw_text);
                tasking_block->block.normal_block->raw_text = NULL;
                free(tasking_block->block.normal_block);
                tasking_block->block.normal_block = NULL;
            }
            break;
        case TB_TASK_ASYNC:
            if(tasking_block->block.task_async_block != NULL){
                task_async_block_destroy(tasking_block->block.task_async_block);
                free(tasking_block->block.task_async_block);
                tasking_block->block.task_async_block = NULL;
            }
            break;

        case TB_TASKWAIT:
            if(tasking_block->block.taskwait_block != NULL){
                free(tasking_block->block.taskwait_block->raw_text);
                tasking_block->block.taskwait_block->raw_text = NULL;
                free(tasking_block->block.taskwait_block);
                tasking_block->block.taskwait_block = NULL;
            }
            break;
        
        default:
            break;
    }
}

static int tasking_region_ensure_block_capacity(tasking_region_t *tasking_region){
    tasking_block_t *new_blocks;
    int new_capacity;

    if(tasking_region == NULL){
        return -1;
    }

    if(tasking_region->block_count < tasking_region->block_capacity){
        return 0;
    }

    new_capacity = (tasking_region->block_capacity == 0) ? 4 : (tasking_region->block_capacity * 2);
    new_blocks = (tasking_block_t*)realloc(tasking_region->blocks, (size_t)new_capacity * sizeof(tasking_block_t));
    if(new_blocks == NULL){
        return -1;
    }

    memset(new_blocks + tasking_region->block_capacity, 0, (size_t)(new_capacity - tasking_region->block_capacity) * sizeof(tasking_block_t));
    tasking_region->blocks = new_blocks;
    tasking_region->block_capacity = new_capacity;
    return 0;
}

static int tasking_region_add_consumer(tasking_region_t *tasking_region, const char* depend_key, int consumer_task_id){

    if(tasking_region == NULL || depend_key == NULL){
        return -1;
    }

    return hash_map_add_unique(tasking_region->analysis.consumers, depend_key, consumer_task_id);
}

static int tasking_region_add_last_producer(tasking_region_t *tasking_region, const char* depend_key, int producer_task_id){

    if(tasking_region == NULL || depend_key == NULL){
        return -1;
    }
    return hash_map_add(tasking_region->analysis.last_producers, depend_key, producer_task_id);
}

/* Updates the producer/consumer analysis for one dependency of a task */
static int tasking_region_update_depend_analysis(tasking_region_t *tasking_region, task_depend_kind_t kind, const char* depend_key, int task_id){
    if(tasking_region == NULL || depend_key == NULL){
        return -1;
    }

    switch (kind)
    {
        case TD_DEPEND_IN:
            return tasking_region_add_consumer(tasking_region, depend_key, task_id);
        
        case TD_DEPEND_OUT:
            return tasking_region_add_last_producer(tasking_region, depend_key, task_id);
        
        case TD_DEPEND_INOUT:
            if(tasking_region_add_consumer(tasking_region, depend_key, task_id) != 0){
                return -1;
            }
            return tasking_region_add_last_producer(tasking_region, depend_key, task_id);

        default:
            return -1;
    }
}

tasking_region_t *tasking_region_create(int region_id){
    tasking_region_t *tasking_region;

    tasking_region = (tasking_region_t*)calloc(1, sizeof(tasking_region_t));
    if(tasking_region == NULL){
        return NULL;
    }

    tasking_region->region_id = region_id;
    tasking_region->current_open_block = -1;
    tasking_region->next_task_id = 0;
    tasking_region->next_generated_task_kind = 1;
    tasking_region->analysis.last_producers = hash_map_create(64);
    tasking_region->analysis.consumers = hash_map_create(64);

    if(tasking_region->analysis.last_producers == NULL || tasking_region->analysis.consumers == NULL){
        if(tasking_region->analysis.last_producers != NULL){
            hash_map_destroy(tasking_region->analysis.last_producers);
        }
        if(tasking_region->analysis.consumers != NULL){
            hash_map_destroy(tasking_region->analysis.consumers);
        }
        free(tasking_region);
        return NULL;
    }

    return tasking_region;
}

void tasking_region_destroy(tasking_region_t *tasking_region){
    int i;

    if(tasking_region == NULL){
        return;
    }

    for(i = 0; i < tasking_region->block_count; i++){
        tasking_block_destroy(&tasking_region->blocks[i]);
    }

    free(tasking_region->blocks);

    if(tasking_region->analysis.last_producers != NULL){
        hash_map_destroy(tasking_region->analysis.last_producers);
    }
    if(tasking_region->analysis.consumers != NULL){
        hash_map_destroy(tasking_region->analysis.consumers);
    }

    free(tasking_region);
}

int tasking_region_get_id(const tasking_region_t *tasking_region){
    if(tasking_region == NULL){
        return -1;
    }

    return tasking_region->region_id;
}

int tasking_region_get_num_blocks(const tasking_region_t *tasking_region){
    if(tasking_region == NULL){
        return -1;
    }

    return tasking_region->block_count;
}

int tasking_region_begin_normal_block(tasking_region_t *tasking_region, const char *text){
    tasking_block_t *tasking_block;

    if(tasking_region == NULL || text == NULL){
        return -1;
    }
    if(tasking_region->current_open_block != -1){
        return -1;
    }

    if(tasking_region_ensure_block_capacity(tasking_region) != 0){
        return -1;
    }

    tasking_block = &tasking_region->blocks[tasking_region->block_count];
    memset(tasking_block, 0, sizeof(tasking_block_t));

    tasking_block->kind = TB_NORMAL;
    
    tasking_block->block.normal_block = (normal_block_t*)malloc(sizeof(normal_block_t));
    if(tasking_block->block.normal_block == NULL){
        return -1;
    }

    tasking_block->block.normal_block->raw_text = ompd_strdup(text);
    if(tasking_block->block.normal_block->raw_text == NULL){
        free(tasking_block->block.normal_block);
        tasking_block->block.normal_block = NULL;
        return -1;
    }

    tasking_region->block_count++;
    return 0;
}

int tasking_region_begin_task_async_block(tasking_region_t *tasking_region){
    tasking_block_t *tasking_block;
    task_async_block_t *task_async_block;
    
    if(tasking_region == NULL){
        return -1;
    }
    if(tasking_region->current_open_block != -1){
        return -1;
    }

    if(tasking_region_ensure_block_capacity(tasking_region) != 0){
        return -1;
    }

    task_async_block = (task_async_block_t*)malloc(sizeof(task_async_block_t));
    if(task_async_block == NULL){
        return -1;
    }

    task_async_block_init(task_async_block);
    task_async_block->task_id = tasking_region->next_task_id++;
    task_async_block->generated_task_kind = tasking_region->next_generated_task_kind++;

    tasking_block = &tasking_region->blocks[tasking_region->block_count];
    memset(tasking_block, 0, sizeof(tasking_block_t));

    tasking_block->kind = TB_TASK_ASYNC;
    tasking_block->block.task_async_block = task_async_block;

    tasking_region->current_open_block = tasking_region->block_count;
    tasking_region->block_count++;

    return 0;
}

int tasking_region_begin_taskwait_block(tasking_region_t *tasking_region, const char *raw_text){
    tasking_block_t *tasking_block;

    if(tasking_region == NULL || raw_text == NULL){
        return -1;
    }
    
    if(tasking_region->current_open_block != -1){
        return -1;
    }

    if(tasking_region_ensure_block_capacity(tasking_region) != 0){
        return -1;
    }

    tasking_block = &tasking_region->blocks[tasking_region->block_count];
    memset(tasking_block, 0, sizeof(tasking_block_t));

    tasking_block->kind = TB_TASKWAIT;
    tasking_block->block.taskwait_block = (taskwait_block_t*)calloc(1, sizeof(taskwait_block_t));
    if(tasking_block->block.taskwait_block == NULL){
        return -1;
    }

    tasking_block->block.taskwait_block->raw_text = ompd_strdup(raw_text);
    if(tasking_block->block.taskwait_block->raw_text == NULL){
        free(tasking_block->block.taskwait_block);
        tasking_block->block.taskwait_block = NULL;
        return -1;
    }

    tasking_region->block_count++;
    return 0;
}

int tasking_region_add_task_async_block_depend(tasking_region_t *tasking_region, task_depend_kind_t kind, const char *raw_text, const char *value_type, const char *mpi_type){
    tasking_block_t *tasking_block;
    task_async_block_t *task_async_block;
    depend_parts_t parts;
    task_depend_obj_kind_t obj_kind;

    if(tasking_region == NULL || raw_text == NULL || value_type == NULL || mpi_type == NULL){
        return -1;
    }
    if(tasking_region->current_open_block < 0 || tasking_region->current_open_block >= tasking_region->block_count){
        return -1;
    }

    tasking_block = &tasking_region->blocks[tasking_region->current_open_block];
    if(tasking_block->block.task_async_block == NULL || tasking_block->kind != TB_TASK_ASYNC){
        return -1;
    }

    depend_parts_init(&parts);

    if(parse_depend_key(raw_text, &parts) != 0){
        depend_parts_free(&parts);
        return -1;
    }

    obj_kind = parts.is_array_element ? TDO_ARRAY_ELEMENT : TDO_VARIABLE;
    task_async_block = tasking_block->block.task_async_block;

    if(task_async_block_add_depend(task_async_block, kind, obj_kind, raw_text, parts.name, parts.index, parts.length, value_type, mpi_type) != 0){
        depend_parts_free(&parts);
        return -1;
    }

    if(tasking_region_update_depend_analysis(tasking_region, kind, raw_text, task_async_block->task_id) != 0){
        depend_parts_free(&parts);
        return -1;
    }

    depend_parts_free(&parts);
    return 0;
}

int tasking_region_add_task_async_block_input_data(tasking_region_t *tasking_region, const char *field, const char *expr, const char *value_type){
	tasking_block_t *tasking_block;

	if(tasking_region == NULL || field == NULL || expr == NULL || value_type == NULL){
		return -1;
	}

	if(tasking_region->current_open_block < 0 || tasking_region->current_open_block >= tasking_region->block_count){
		return -1;
	}

	tasking_block = &tasking_region->blocks[tasking_region->current_open_block];
	if(tasking_block->kind != TB_TASK_ASYNC || tasking_block->block.task_async_block == NULL){
		return -1;
	}

	return task_async_block_add_input_data(tasking_block->block.task_async_block, field, expr, value_type);
}

int tasking_region_set_task_async_block_body_text(tasking_region_t *tasking_region, const char* body){
    tasking_block_t *tasking_block;

    if(tasking_region == NULL || body == NULL){
        return -1;
    }
    if(tasking_region->current_open_block < 0 || tasking_region->current_open_block >= tasking_region->block_count){
        return -1;
    }

    tasking_block = &tasking_region->blocks[tasking_region->current_open_block];
    if(tasking_block->kind != TB_TASK_ASYNC || tasking_block->block.task_async_block == NULL){
        return -1;
    }

    return task_async_block_set_body_text(tasking_block->block.task_async_block, body);
}

int tasking_region_begin_task_async_block_body_parse(tasking_region_t *tasking_region){
    tasking_block_t *tasking_block;

    if(tasking_region == NULL){
        return -1;
    }

    if(tasking_region->current_open_block < 0 || tasking_region->current_open_block >= tasking_region->block_count){
        return -1;
    }

    tasking_block = &tasking_region->blocks[tasking_region->current_open_block];
    if(tasking_block->kind != TB_TASK_ASYNC || tasking_block->block.task_async_block == NULL){
        return -1;
    }

    return 0;
}

int tasking_region_finish_task_async_block_body_ir(tasking_region_t *tasking_region, task_body_stmt_t *body_root){
    tasking_block_t *tasking_block;
    task_async_block_t *task_async_block;

    if(tasking_region == NULL || body_root == NULL){
        return -1;
    }

    if(tasking_region->current_open_block < 0 || tasking_region->current_open_block >= tasking_region->block_count){
        return -1;
    }

    tasking_block = &tasking_region->blocks[tasking_region->current_open_block];
    if(tasking_block->kind != TB_TASK_ASYNC || tasking_block->block.task_async_block == NULL){
        return -1;
    }

    task_async_block = tasking_block->block.task_async_block;

    if(task_body_prepare_task_async_block(task_async_block, body_root) != 0){
        task_body_stmt_destroy(body_root);
        return -1;
    }

    tasking_region->current_open_block = -1;
    return 0;
}

static int tasking_region_validate_emit_ready(tasking_region_t *tasking_region){
    int i;
    tasking_block_t *tasking_block;

    if(tasking_region == NULL){
        return -1;
    }
    if(tasking_region->current_open_block != -1){
        return -1;
    }

    for(i = 0; i < tasking_region->block_count; i++){
        tasking_block = &tasking_region->blocks[i];

        if(tasking_block->kind == TB_TASK_ASYNC && tasking_block->block.task_async_block != NULL){
            if(tasking_block->block.task_async_block->body_prepared == 0){
                fprintf(stderr, "Error: task_async body not prepared before emit\n");
                return -1;
            }
        }
    }

    return 0;
}

/* Emits receives for task outputs still unconsumed at a taskwait */
static void emit_taskwait_receives_for_visible_outputs(tasking_region_t *tasking_region, int wait_index, FILE *file){
    int k, j, m, n, shadowed;
    tasking_block_t *producer_block;
    tasking_block_t *later_block;
    task_async_block_t *producer_task;
    task_async_block_t *later_task;
    const char *suffix;

    if(tasking_region == NULL || file == NULL){
        return;
    }

    for(k = 0; k < wait_index; k++){
        producer_block = &tasking_region->blocks[k];

        if(producer_block->kind != TB_TASK_ASYNC || producer_block->block.task_async_block == NULL){
            continue;
        }

        producer_task = producer_block->block.task_async_block;

        for(j = 0; j < producer_task->depends.count; j++){
            task_depend_t *depend = &producer_task->depends.items[j];

            if(depend->obj_kind != TDO_VARIABLE){
                continue;
            }

            if(depend->kind != TD_DEPEND_OUT && depend->kind != TD_DEPEND_INOUT){
                continue;
            }

            shadowed = 0;
            for(m = k + 1; m < wait_index && !shadowed; m++){
                later_block = &tasking_region->blocks[m];

                if(later_block->kind != TB_TASK_ASYNC || later_block->block.task_async_block == NULL){
                    continue;
                }

                later_task = later_block->block.task_async_block;

                for(n = 0; n < later_task->depends.count; n++){
                    task_depend_t *later_depend = &later_task->depends.items[n];

                    if(later_depend->obj_kind != TDO_VARIABLE){
                        continue;
                    }

                    if(later_depend->kind != TD_DEPEND_OUT && later_depend->kind != TD_DEPEND_INOUT){
                        continue;
                    }

                    if(strcmp(later_depend->name, depend->name) == 0){
                        shadowed = 1;
                        break;
                    }
                }
            }

            if(shadowed){
                continue;
            }

            suffix = ompd_runtime_suffix_for_value_type(depend->value_type);
            if(suffix[0] == '\0'){
                continue;
            }

            fprintf(file, "%s = ompd_runtime_recv_%s_value(\"%s\");\n",
                depend->name,
                suffix,
                depend->name);
        }
    }
}

int tasking_region_emit_global_defs(tasking_region_t *tasking_region, FILE *file){
    int i, j;
    tasking_block_t *tasking_block;
    task_async_block_t *task_async_block;

    if(tasking_region == NULL || file == NULL){
        return -1;
    }

    if(tasking_region_validate_emit_ready(tasking_region) != 0){
        return -1;
    }

    fprintf(file, "typedef enum {\n");
    for(i = 0; i < tasking_region->block_count; i++){
        tasking_block = &tasking_region->blocks[i];
        if(tasking_block->kind == TB_TASK_ASYNC && tasking_block->block.task_async_block != NULL){
            task_async_block = tasking_block->block.task_async_block;
            fprintf(file, "    OMPD_TASK_KIND_GENERATED_%d = %d,\n",
                task_async_block->generated_task_kind,
                task_async_block->generated_task_kind);
        }
    }
    fprintf(file, "} ompd_generated_task_kind_t;\n\n");

    for(i = 0; i < tasking_region->block_count; i++){
        tasking_block = &tasking_region->blocks[i];

        if(tasking_block->kind == TB_TASK_ASYNC && tasking_block->block.task_async_block != NULL){
            task_async_block = tasking_block->block.task_async_block;

            if(task_async_block->input_datas.count > 0){
                fprintf(file, "typedef struct {\n");
                for(j = 0; j < task_async_block->input_datas.count; j++){
                    fprintf(file, "    %s %s;\n",
                        task_async_block->input_datas.items[j].value_type,
                        task_async_block->input_datas.items[j].field);
                }
                fprintf(file, "} ompd_generated_task_input_data_%d_t;\n\n",
                    task_async_block->generated_task_kind);
            }

            fprintf(file, "static void ompd_generated_task_body_%d(void *raw_task_input_data)\n",
                task_async_block->generated_task_kind);
            fprintf(file, "{\n");

            if(task_async_block->input_datas.count > 0){
                fprintf(file, "    ompd_generated_task_input_data_%d_t *task_input_data =\n",
                    task_async_block->generated_task_kind);
                fprintf(file, "        (ompd_generated_task_input_data_%d_t *) raw_task_input_data;\n\n",
                    task_async_block->generated_task_kind);
            }

            if(task_async_block->emit_body_text != NULL && task_async_block->emit_body_text[0] != '\0'){
                fprintf(file, "%s\n", task_async_block->emit_body_text);
            }

            fprintf(file, "}\n\n");
        }
    }

    fprintf(file, "void ompd_execute_generated_task(int task_kind, void *raw_task_input_data)\n");
    fprintf(file, "{\n");
    fprintf(file, "    switch (task_kind) {\n");

    for(i = 0; i < tasking_region->block_count; i++){
        tasking_block = &tasking_region->blocks[i];
        if(tasking_block->kind == TB_TASK_ASYNC && tasking_block->block.task_async_block != NULL){
            task_async_block = tasking_block->block.task_async_block;
            fprintf(file, "        case OMPD_TASK_KIND_GENERATED_%d:\n",
                task_async_block->generated_task_kind);
            fprintf(file, "            ompd_generated_task_body_%d(raw_task_input_data);\n",
                task_async_block->generated_task_kind);
            fprintf(file, "            break;\n");
        }
    }

    fprintf(file, "        default:\n");
    fprintf(file, "            fprintf(stderr, \"Invalid generated task kind: %%d\\n\", task_kind);\n");
    fprintf(file, "            MPI_Abort(MPI_COMM_WORLD, 1);\n");
    fprintf(file, "    }\n");
    fprintf(file, "}\n\n");

    return 0;
}

int tasking_region_emit_body(tasking_region_t *tasking_region, FILE *file){
    int i, j;
    tasking_block_t *tasking_block;
    task_async_block_t *task_async_block;

    if(tasking_region == NULL || file == NULL){
        return -1;
    }

    if(tasking_region_validate_emit_ready(tasking_region) != 0){
        return -1;
    }

    for(i = 0; i < tasking_region->block_count; i++){
        tasking_block = &tasking_region->blocks[i];

        if(tasking_block->kind == TB_NORMAL){
            fprintf(file, "%s\n",
                tasking_block->block.normal_block->raw_text != NULL ?
                tasking_block->block.normal_block->raw_text : "");
            continue;
        }

        if(tasking_block->kind == TB_TASKWAIT){
            fprintf(file, "ompd_wait_for_child_tasks();\n");
            emit_taskwait_receives_for_visible_outputs(tasking_region, i, file);
            continue;
        }

        if(tasking_block->kind == TB_TASK_ASYNC && tasking_block->block.task_async_block != NULL){
            task_async_block = tasking_block->block.task_async_block;

            fprintf(file, "{\n");

            if(task_async_block->input_datas.count > 0){
                fprintf(file, "    ompd_generated_task_input_data_%d_t task_input_data_%d;\n",
                    task_async_block->generated_task_kind,
                    task_async_block->generated_task_kind);
            }

            fprintf(file, "    ompd_task_definition_t task_definition_%d;\n",
                task_async_block->generated_task_kind);
            fprintf(file, "    ompd_task_id_t task_id_%d;\n",
                task_async_block->generated_task_kind);

            for(j = 0; j < task_async_block->input_datas.count; j++){
                fprintf(file, "    task_input_data_%d.%s = %s;\n",
                    task_async_block->generated_task_kind,
                    task_async_block->input_datas.items[j].field,
                    task_async_block->input_datas.items[j].expr);
            }

            fprintf(file, "    task_definition_%d.generated_task_kind = OMPD_TASK_KIND_GENERATED_%d;\n",
                task_async_block->generated_task_kind,
                task_async_block->generated_task_kind);

            if(task_async_block->input_datas.count > 0){
                fprintf(file, "    task_definition_%d.task_input_data = &task_input_data_%d;\n",
                    task_async_block->generated_task_kind,
                    task_async_block->generated_task_kind);
                fprintf(file, "    task_definition_%d.task_input_data_size = sizeof(task_input_data_%d);\n",
                    task_async_block->generated_task_kind,
                    task_async_block->generated_task_kind);
            } else {
                fprintf(file, "    task_definition_%d.task_input_data = NULL;\n",
                    task_async_block->generated_task_kind);
                fprintf(file, "    task_definition_%d.task_input_data_size = 0;\n",
                    task_async_block->generated_task_kind);
            }

            fprintf(file, "    task_id_%d = ompd_build_task(&task_definition_%d);\n",
                task_async_block->generated_task_kind,
                task_async_block->generated_task_kind);

            for(j = 0; j < task_async_block->depends.count; j++){
                task_depend_t *depend = &task_async_block->depends.items[j];

                if(depend->obj_kind == TDO_VARIABLE){
                    if(depend->kind == TD_DEPEND_IN || depend->kind == TD_DEPEND_INOUT){
                        fprintf(file, "    ompd_register_task_input_variable(task_id_%d, \"%s\", sizeof(%s));\n",
                            task_async_block->generated_task_kind,
                            depend->name,
                            depend->value_type);
                    }
                    if(depend->kind == TD_DEPEND_OUT || depend->kind == TD_DEPEND_INOUT){
                        fprintf(file, "    ompd_register_task_output_variable(task_id_%d, \"%s\", sizeof(%s));\n",
                            task_async_block->generated_task_kind,
                            depend->name,
                            depend->value_type);
                    }
                } else {
                    if(depend->kind == TD_DEPEND_IN || depend->kind == TD_DEPEND_INOUT){
                        fprintf(file, "    ompd_register_task_input_array_element(task_id_%d, \"%s\", %s, sizeof(%s));\n",
                            task_async_block->generated_task_kind,
                            depend->name,
                            depend->index,
                            depend->value_type);
                    }
                    if(depend->kind == TD_DEPEND_OUT || depend->kind == TD_DEPEND_INOUT){
                        fprintf(file, "    ompd_register_task_output_array_element(task_id_%d, \"%s\", %s, sizeof(%s));\n",
                            task_async_block->generated_task_kind,
                            depend->name,
                            depend->index,
                            depend->value_type);
                    }
                }
            }

            fprintf(file, "    ompd_submit_task(task_id_%d);\n",
                task_async_block->generated_task_kind);
            fprintf(file, "}\n");
        }
    }

    return 0;
}
