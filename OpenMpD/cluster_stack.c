#include "cluster_stack.h"
#include "tasking_region.h"

extern void MPIGather();
extern void MPIAllGather();
extern void calcularReduceFinal(bool cluster);
extern void calcularAllReduceFinal(bool cluster);

cluster_stack *cluster_stack_top = NULL;

int cluster_stack_isEmpty(){
    return cluster_stack_top == NULL;
}

cluster_stack* cluster_stack_peek(){
    return cluster_stack_top;
}

void cluster_stack_push(cluster_kind kind){
    cluster_stack *c = (cluster_stack*)malloc(sizeof(cluster_stack));
    if(c == NULL){
        perror("malloc");
        exit(1);
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
    cluster_stack *aux;

    if(cluster_stack_top == NULL){
        perror("pop");
        exit(1);
    }

    aux = cluster_stack_top;
    cluster_stack_top = cluster_stack_top->next;

    if(aux->tasking_region != NULL){
        tasking_region_destroy(aux->tasking_region);
        aux->tasking_region = NULL;
    }

    free(aux);
}

void cluster_stack_close(){
    cluster_stack *c = cluster_stack_peek();
    if(c != NULL){
        if(c->enAllGather && c->kind == CL_NOW){
            MPIAllGather();
        } 
        if (c->enGather){
            MPIGather();
        }
        if (c->enAllReduction){
            calcularAllReduceFinal(true);
        } 
        if(c->enReduction){
            calcularReduceFinal(true);
        }
        cluster_stack_pop();
    }
}
