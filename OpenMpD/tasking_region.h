#ifndef TASKING_REGION_H
#define TASKING_REGION_H

#include <stdio.h>
#include <stddef.h>

typedef struct tasking_region tasking_region_t;
typedef struct task_async_block task_async_block_t;
typedef struct task_body_stmt task_body_stmt_t;

typedef enum {
    TD_DEPEND_IN,
    TD_DEPEND_OUT,
    TD_DEPEND_INOUT
} task_depend_kind_t;

typedef enum {
    TDO_VARIABLE,
    TDO_ARRAY_ELEMENT
} task_depend_obj_kind_t;

typedef struct {
    task_depend_kind_t kind;
    task_depend_obj_kind_t obj_kind;
    char *raw_text; 
    char *name;
    char *index;    /* only array */
    char *length;   /* only array */
    char *value_type;
    char *mpi_type;
} task_depend_t;

typedef struct {
    task_depend_t *items;
    int len;
    int cap;
} task_depend_list_t;

typedef struct {
    char *field;        /* input_value_0 */
    char *expr;         /* n - 1 */
    char *value_type;   /* int */
} task_input_data_t;

typedef struct {
    task_input_data_t *items;
    int len;
    int cap;
} task_input_data_list_t;

typedef enum {
    TB_NORMAL,
    TB_TASK_ASYNC,
    TB_TASKWAIT
} tasking_block_kind_t;

void task_depend_list_init(task_depend_list_t *list);
void task_depend_list_destroy(task_depend_list_t *list);
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
);

void task_input_data_list_init(task_input_data_list_t *list);
void task_input_data_list_destroy(task_input_data_list_t *list);
int task_input_data_list_add(task_input_data_list_t *list, const char *field, const char *expr, const char *value_type);

tasking_region_t *tasking_region_create(int region_id /*, int owner_depth*/);
void tasking_region_destroy(tasking_region_t *tasking_region);

int tasking_region_get_id(const tasking_region_t *tasking_region);
// int tasking_region_get_owner_depth(const tasking_region_t *tasking_region);
int tasking_region_get_num_blocks(const tasking_region_t *tasking_region);

int tasking_region_begin_normal_block(tasking_region_t *tasking_region, const char *text);

int tasking_region_begin_taskwait_block(tasking_region_t *tasking_region, const char *raw_text);

int tasking_region_begin_task_async_block(tasking_region_t *tasking_region);
int tasking_region_begin_task_async_block_body_parse(tasking_region_t *tasking_region);
int tasking_region_finish_task_async_block_body_ir(tasking_region_t *tasking_region, task_body_stmt_t *body_root);

int tasking_region_add_task_async_block_depend(
    tasking_region_t *tasking_region,
    task_depend_kind_t kind,
    const char *raw_text,
    const char *value_type,
    const char *mpi_type
);
int tasking_region_add_task_async_block_input_data(
    tasking_region_t *tasking_region,
    const char *field,
    const char *expr,
    const char *value_type
);
int tasking_region_set_task_async_block_body_text(tasking_region_t *tasking_region, const char* body);

int tasking_region_emit_global_defs(tasking_region_t *tasking_region, FILE *file);
int tasking_region_emit_body(tasking_region_t *tasking_region, FILE *file);

#endif
