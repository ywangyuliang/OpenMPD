#include "cluster_stack.h"
#include "tasking_region.h"
#include "reduction_transform.h"
#include "scatter_gather_transform.h"

#include <stdlib.h>

cluster_stack *cluster_stack_top = NULL;

int cluster_stack_is_empty(){
    return cluster_stack_top == NULL;
}

cluster_stack* cluster_stack_peek(){
    return cluster_stack_top;
}

void cluster_stack_push(cluster_kind kind){
    cluster_stack *c = (cluster_stack*)malloc(sizeof(cluster_stack));
    if(c == NULL){
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    memset(c, 0, sizeof(cluster_stack));

    c->kind = kind;
    c->brace_depth = 0;
    c->close_state = CL_OPEN;

    c->tasking_region = NULL;
    c->has_non_task_pragma = 0;
    c->has_task_pragma = 0;

    c->next = cluster_stack_top;
    cluster_stack_top = c;
}

void cluster_stack_pop(){
    cluster_stack *removed_cluster;

    if(cluster_stack_top == NULL){
        perror("pop");
        exit(EXIT_FAILURE);
    }

    removed_cluster = cluster_stack_top;
    cluster_stack_top = cluster_stack_top->next;

    if(removed_cluster->tasking_region != NULL){
        tasking_region_destroy(removed_cluster->tasking_region);
        removed_cluster->tasking_region = NULL;
    }

    free(removed_cluster);
}

void cluster_stack_close(){
    cluster_stack *c = cluster_stack_peek();
    if(c != NULL){
        if(c->allgather_clause_active && c->kind == CL_NOW){
            allgather_emit_all();
        } 
        if (c->gather_clause_active){
            gather_emit_all();
        }
        if (c->allreduction_clause_active){
            allreduction_emit_final(true);
        } 
        if(c->reduction_clause_active){
            reduction_emit_final(true);
        }
        cluster_stack_pop();
    }
}
