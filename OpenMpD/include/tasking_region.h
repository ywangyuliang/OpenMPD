#ifndef TASKING_REGION_H
#define TASKING_REGION_H

/*
 * Models a translated tasking region. It stores normal blocks, taskwaits,
 * async task blocks, dependencies and captured input data, then emits the
 * global definitions and executable body required by the tasking runtime.
 */

#include <stdio.h>
#include <stddef.h>

typedef struct tasking_region tasking_region_t;
typedef struct task_async_block task_async_block_t;
typedef struct task_body_stmt task_body_stmt_t;

/* Monotonic id assigned to each created tasking region */
extern int tasking_region_id_counter;

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
    int count;
    int capacity;
} task_depend_list_t;

typedef struct {
    char *field;        /* input_value_0 */
    char *expr;         /* n - 1 */
    char *value_type;   /* int */
} task_input_data_t;

typedef struct {
    task_input_data_t *items;
    int count;
    int capacity;
} task_input_data_list_t;

typedef enum {
    TB_NORMAL,
    TB_TASK_ASYNC,
    TB_TASKWAIT
} tasking_block_kind_t;

/* Initializes a task dependency list */
void task_depend_list_init(task_depend_list_t *list);

/* Releases all memory owned by a task dependency list */
void task_depend_list_destroy(task_depend_list_t *list);

/* Adds one dependency entry to a dependency list */
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

/* Initializes a captured input-data list */
void task_input_data_list_init(task_input_data_list_t *list);

/* Releases all memory owned by a captured input-data list */
void task_input_data_list_destroy(task_input_data_list_t *list);

/* Adds one captured input-data entry */
int task_input_data_list_add(task_input_data_list_t *list, const char *field, const char *expr, const char *value_type);

/* Creates a tasking region with a stable region id */
tasking_region_t *tasking_region_create(int region_id);

/* Releases all memory owned by a tasking region */
void tasking_region_destroy(tasking_region_t *tasking_region);

/* Returns the stable id of a tasking region */
int tasking_region_get_id(const tasking_region_t *tasking_region);

/* Returns the number of generated blocks in a tasking region */
int tasking_region_get_num_blocks(const tasking_region_t *tasking_region);

/* Starts a normal source block inside a tasking region */
int tasking_region_begin_normal_block(tasking_region_t *tasking_region, const char *text);

/* Starts a taskwait block inside a tasking region */
int tasking_region_begin_taskwait_block(tasking_region_t *tasking_region, const char *raw_text);

/* Starts an async task block inside a tasking region */
int tasking_region_begin_task_async_block(tasking_region_t *tasking_region);

/* Marks the current async task block as ready for body parsing */
int tasking_region_begin_task_async_block_body_parse(tasking_region_t *tasking_region);

/* Stores the parsed body IR for the current async task block */
int tasking_region_finish_task_async_block_body_ir(tasking_region_t *tasking_region, task_body_stmt_t *body_root);

/* Adds one dependency to the current async task block */
int tasking_region_add_task_async_block_depend(
    tasking_region_t *tasking_region,
    task_depend_kind_t kind,
    const char *raw_text,
    const char *value_type,
    const char *mpi_type
);

/* Adds one captured input value to the current async task block */
int tasking_region_add_task_async_block_input_data(
    tasking_region_t *tasking_region,
    const char *field,
    const char *expr,
    const char *value_type
);

/* Stores original body text for the current async task block */
int tasking_region_set_task_async_block_body_text(tasking_region_t *tasking_region, const char* body);

/* Emits global definitions needed by a tasking region */
int tasking_region_emit_global_defs(tasking_region_t *tasking_region, FILE *file);

/* Emits executable body code for a tasking region */
int tasking_region_emit_body(tasking_region_t *tasking_region, FILE *file);

#endif
