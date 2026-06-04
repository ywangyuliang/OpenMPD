#ifndef TASK_ASYNC_CONTEXT_H
#define TASK_ASYNC_CONTEXT_H

#include <stdio.h>

typedef enum {DEPEND_IN, DEPEND_OUT} depend_kind;

typedef struct task_async_context task_async_context_t;

task_async_context_t* task_async_context_create(int numTeams);
void task_async_context_destroy(task_async_context_t* ta_ctx);
int task_async_context_begin_for(task_async_context_t* ta_ctx, const char* for_stmt);
int task_async_context_add_task(task_async_context_t* ta_ctx, int rank);
int task_async_context_add_depend(task_async_context_t* ta_ctx, depend_kind kind, const char* chunk_key, const char* mpi_type);
int task_async_context_add_body(task_async_context_t* ta_ctx, const char* body_text);
int task_async_context_finalize(task_async_context_t* ta_ctx, FILE* file, const char* tag, const char* rank, const char* status);

#endif
