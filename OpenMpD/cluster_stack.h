#ifndef CLUSTER_STACK_H
#define CLUSTER_STACK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct tasking_region tasking_region_t;

typedef enum { CL_ONE_STMT, CL_BLOCK, CL_NOW } cluster_kind;
typedef enum { CL_OPEN, CL_CLOSE_PENDING } cluster_close_state;

typedef struct cluster_stack
{
    cluster_kind kind;
    int brace_depth;
    cluster_close_state close_state; 

    int enGather;
    int enAllGather;
    int enReduction;
    int enAllReduction;

    tasking_region_t *tasking_region;
    int has_task_pragma;
    int has_non_task_pragma;

    struct cluster_stack *next;
} cluster_stack;

extern cluster_stack *cluster_stack_top;

void cluster_stack_push(cluster_kind kind);
void cluster_stack_pop();
cluster_stack* cluster_stack_peek();
int cluster_stack_isEmpty();
void cluster_stack_close();

#endif
