#ifndef CLUSTER_STACK_H
#define CLUSTER_STACK_H

/*
 * Tracks nested OpenMPD cluster regions while the translator scans the input.
 * Each stack entry stores the region shape, brace depth, pending close state,
 * active collective clauses and tasking metadata needed when the region ends.
 */

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

    int gather_clause_active;
    int allgather_clause_active;
    int reduction_clause_active;
    int allreduction_clause_active;

    tasking_region_t *tasking_region;
    int has_task_pragma;
    int has_non_task_pragma;

    struct cluster_stack *next;
} cluster_stack;

extern cluster_stack *cluster_stack_top;

/* Pushes a new cluster scope onto the stack */
void cluster_stack_push(cluster_kind kind);

/* Removes the current cluster scope from the stack */
void cluster_stack_pop();

/* Returns the current cluster scope without removing it */
cluster_stack* cluster_stack_peek();

/* Returns whether the cluster stack is empty */
int cluster_stack_is_empty();

/* Marks the current cluster scope as closing */
void cluster_stack_close();

#endif
