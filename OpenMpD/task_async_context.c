#include "task_async_context.h"
#include "hash_map.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef struct 
{
    char* chunk_key;
    char* mpi_type;
    int recv_from;

} read_dept_t;

typedef struct
{
    char* chunk_key;
    char* mpi_type;
} write_dept_t;

typedef struct
{
    int rank;

    read_dept_t* reads;
    int reads_len;
    int reads_cap;

    write_dept_t* writes;
    int writes_len;
    int writes_cap;

    char* body;

} task_info_t;

struct task_async_context
{
    int numTeams;

    char* for_stmt;

    task_info_t* tasks;
    int tasks_len;
    int tasks_cap;

    int current_task;

    hash_map_t* consumers;
    hash_map_t* last_producers;
};

typedef struct
{
    char* var; 
    char* base;
    char* len;
} chunk_parts_t;

/* UTILITIES ----------------------------------------------------------- */

static char* local_strdup(const char* s){
    
    size_t len;
    char* ss;

    len = strlen(s);
    ss = (char*)malloc(len + 1);
    if(ss == NULL){
        return NULL;
    }
    memcpy(ss, s, len + 1);
    return ss;
}

static int push_back_read_dept(task_info_t* task, const char* chunk_key, const char* mpi_type){
    read_dept_t* dept;
    read_dept_t* new_reads;
    int new_cap;

    if(task == NULL || chunk_key == NULL || mpi_type == NULL){
        return -1;
    }

    if(task->reads_cap <= task->reads_len){
        new_cap = (task->reads_cap == 0) ? 4 : (task->reads_cap * 2);
        
        new_reads = (read_dept_t*)realloc(task->reads, new_cap * sizeof(read_dept_t));
        if(new_reads == NULL){
            return -1;
        }

        memset(new_reads + task->reads_cap, 0, (size_t)(new_cap - task->reads_cap) * sizeof(read_dept_t));

        task->reads = new_reads;
        task->reads_cap = new_cap;
    }

    dept = &task->reads[task->reads_len];
    memset(dept, 0, sizeof(read_dept_t));

    dept->chunk_key = local_strdup(chunk_key);
    dept->mpi_type = local_strdup(mpi_type);
    dept->recv_from = -1;

    if(dept->chunk_key == NULL || dept->mpi_type == NULL){
        free(dept->chunk_key);
        free(dept->mpi_type);
        dept->chunk_key = NULL;
        dept->mpi_type = NULL;
        dept->recv_from = -1;
        return -1;
    }

    task->reads_len++;
    return 0;
}

static int push_back_write_dept(task_info_t* task, const char* chunk_key, const char* mpi_type){
    write_dept_t* dept;
    write_dept_t* new_writes;
    int new_cap;

    if(task == NULL || chunk_key == NULL || mpi_type == NULL){
        return -1;
    }

    if(task->writes_cap <= task->writes_len){
        new_cap = (task->writes_cap == 0) ? 4 : (task->writes_cap * 2);
        
        new_writes = (write_dept_t*)realloc(task->writes, new_cap * sizeof(write_dept_t));
        if(new_writes == NULL){
            return -1;
        }

        memset(new_writes + task->writes_cap, 0, (size_t)(new_cap - task->writes_cap) * sizeof(write_dept_t));

        task->writes = new_writes;
        task->writes_cap = new_cap;
    }

    dept = &task->writes[task->writes_len];
    memset(dept, 0, sizeof(write_dept_t));

    dept->chunk_key = local_strdup(chunk_key);
    dept->mpi_type = local_strdup(mpi_type);

    if(dept->chunk_key == NULL || dept->mpi_type == NULL){
        free(dept->chunk_key);
        free(dept->mpi_type);
        dept->chunk_key = NULL;
        dept->mpi_type = NULL;
        return -1;
    }

    task->writes_len++;
    return 0;
}

static void free_task(task_info_t* task){
    int i;

    if(task == NULL){
        return;
    }

    for(i = 0; i < task->reads_len; i++){
        free(task->reads[i].chunk_key);
        free(task->reads[i].mpi_type);
    }
    free(task->reads);

    for(i = 0; i < task->writes_len; i++){
        free(task->writes[i].chunk_key);
        free(task->writes[i].mpi_type);
    }
    free(task->writes);

    free(task->body);
}

static int add_consumer(hash_map_t* consumers, const char* key, int rank){
    int* values;
    size_t len, i;

    values = NULL;
    len = 0;

    if(consumers == NULL || key == NULL){
        return -1;
    }

    if(hash_map_get(consumers, key, &values, &len) == 0 && values != NULL){
        for(i = 0; i < len; i++){
            if(values[i] == rank){
                free(values);
                return 0;
            }
        }
        free(values);
    }
    return hash_map_add(consumers, key, rank);
}

static int compute_recv_from(task_async_context_t* ta_ctx){

    int i, j, recv_from_rank;
    task_info_t* task;
    int* producers;
    size_t producers_len;
    
    if(ta_ctx == NULL){
        return -1;
    }

    for(i = 0; i < ta_ctx->tasks_len; i++){
        task = &ta_ctx->tasks[i];

        /* IN */
        for(j = 0; j < task->reads_len; j++){
            if(hash_map_get(ta_ctx->last_producers, task->reads[j].chunk_key, &producers, &producers_len) != 0 || producers == NULL || producers_len == 0){
                if(producers != NULL){
                    free(producers);
                }
                return -1;    
            }

            recv_from_rank = producers[producers_len - 1];
            free(producers);

            task->reads[j].recv_from = recv_from_rank;
        }

        /* OUT */
        for(j = 0; j < task->writes_len; j++){
            if(hash_map_add(ta_ctx->last_producers, task->writes[j].chunk_key, task->rank) != 0){
                return -1;
            }
        }
    }
    
    return 0;
}

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

static void chunk_parts_init(chunk_parts_t* chunk_parts){
    if(chunk_parts == NULL){
        return;
    }
    chunk_parts->var = NULL;
    chunk_parts->base = NULL;
    chunk_parts->len = NULL;
}

static void chunk_parts_free(chunk_parts_t* chunk_parts){
    if(chunk_parts == NULL){
        return;
    }
    free(chunk_parts->var);
    free(chunk_parts->base);
    free(chunk_parts->len);
}

/* a[i:1] -> var="a", base="i", len="1" */
static int parse_chunk_key(const char* key, chunk_parts_t* out_chunk_parts){
    
    const char* lb;
    const char* rb;
    const char* cl;
    size_t size;

    if(key == NULL || out_chunk_parts == NULL){
        return -1;
    }

    lb = strchr(key, '[');
    rb = strchr(key, ']');
    cl = strchr(key, ':');

    if(lb == NULL || rb == NULL || cl == NULL){
        return -1;
    }
    if(!(lb < cl && cl < rb)){
        return -1;
    }

    /* var */
    size = (size_t)(lb - key); 
    out_chunk_parts->var = (char*)realloc(out_chunk_parts->var, size + 1);
    if(out_chunk_parts->var == NULL){
        return -1;
    }
    memcpy(out_chunk_parts->var, key, size);
    out_chunk_parts->var[size] = '\0';
    trim(out_chunk_parts->var);

    /* base */
    size = (size_t)(cl - (lb + 1)); 
    out_chunk_parts->base = (char*)realloc(out_chunk_parts->base, size + 1);
    if(out_chunk_parts->base == NULL){
        return -1;
    }
    memcpy(out_chunk_parts->base, lb + 1, size);
    out_chunk_parts->base[size] = '\0';
    trim(out_chunk_parts->base);

    /* len */
    size = (size_t)(rb - (cl + 1)); 
    out_chunk_parts->len = (char*)realloc(out_chunk_parts->len, size + 1);
    if(out_chunk_parts->len == NULL){
        return -1;
    }
    memcpy(out_chunk_parts->len, cl + 1, size);
    out_chunk_parts->len[size] = '\0';
    trim(out_chunk_parts->len);

    return 0;
}

/* APIs ------------------------------------------------------------------ */

task_async_context_t* task_async_context_create(int numTeams) {
    
    task_async_context_t* ta_ctx;

    ta_ctx = (task_async_context_t*)calloc(1, sizeof(task_async_context_t));
    if(ta_ctx == NULL){
        return NULL;
    }

    ta_ctx->numTeams = numTeams;
    ta_ctx->current_task = -1;
    ta_ctx->consumers = hash_map_create(64);
    ta_ctx->last_producers = hash_map_create(64);

    if(ta_ctx->consumers == NULL || ta_ctx->last_producers == NULL){
        task_async_context_destroy(ta_ctx);
        return NULL;
    }

    return ta_ctx;
}

void task_async_context_destroy(task_async_context_t* ta_ctx) {
    
    int i;
    
    if(ta_ctx == NULL){
        return;
    }

    free(ta_ctx->for_stmt);

    for(i = 0; i < ta_ctx->tasks_len; i++){
        free_task(&ta_ctx->tasks[i]);
    }
    free(ta_ctx->tasks);
    
    hash_map_destroy(ta_ctx->consumers);
    hash_map_destroy(ta_ctx->last_producers);

    free(ta_ctx);
}

int task_async_context_begin_for(task_async_context_t* ta_ctx, const char* for_stmt){
    
    if(ta_ctx == NULL || for_stmt == NULL){
        return -1;
    }

    ta_ctx->for_stmt = local_strdup(for_stmt);
    if(ta_ctx->for_stmt == NULL){
        return -1;
    }

    return 0;
}

int task_async_context_add_task(task_async_context_t* ta_ctx, int rank){
    
    int new_cap;
    task_info_t* new_tasks;
    task_info_t* task;

    if(ta_ctx == NULL){
        return -1;
    }

    if(ta_ctx->tasks_cap <= ta_ctx->tasks_len){
        new_cap = (ta_ctx->tasks_cap == 0) ? 4 : (ta_ctx->tasks_cap * 2);
        
        new_tasks = (task_info_t*)realloc(ta_ctx->tasks, new_cap * sizeof(task_info_t));
        if(new_tasks == NULL){
            return -1;
        }

        memset(new_tasks + ta_ctx->tasks_cap, 0, (size_t)(new_cap - ta_ctx->tasks_cap) * sizeof(task_info_t));

        ta_ctx->tasks = new_tasks;
        ta_ctx->tasks_cap = new_cap;
    }

    task = &ta_ctx->tasks[ta_ctx->tasks_len];
    memset(task, 0, sizeof(task_info_t));
    task->rank = rank;

    ta_ctx->current_task = ta_ctx->tasks_len;
    ta_ctx->tasks_len++;

    return 0;
}

int task_async_context_add_depend(task_async_context_t* ta_ctx, depend_kind kind, const char* chunk_key, const char* mpi_type){
    
    task_info_t* task;

    if(ta_ctx == NULL || chunk_key == NULL || mpi_type == NULL){    
        return -1;
    }

    if(ta_ctx->current_task < 0 || ta_ctx->current_task >= ta_ctx->tasks_len){
        return -1;
    }

    task = &ta_ctx->tasks[ta_ctx->current_task];

    if(kind == DEPEND_IN){
        if(push_back_read_dept(task, chunk_key, mpi_type) != 0){
            return -1;
        }
        if(add_consumer(ta_ctx->consumers, chunk_key, task->rank) != 0){
            return -1;
        }
    } else { /* DEPEND_OUT */
        if(push_back_write_dept(task, chunk_key, mpi_type) != 0){
            return -1;
        }
    }
    
    return 0;
}

int task_async_context_add_body(task_async_context_t* ta_ctx, const char* body_text){
    
    task_info_t* task;

    if(ta_ctx == NULL || body_text == NULL){
        return -1;
    }
    if(ta_ctx->current_task < 0 || ta_ctx->current_task >= ta_ctx->tasks_len){
        return -1;
    }

    task = &ta_ctx->tasks[ta_ctx->current_task];
    task->body = local_strdup(body_text);
    return (task->body == NULL) ? -1 : 0;
}

int task_async_context_finalize(task_async_context_t* ta_ctx, FILE* file, const char* tag, const char* rank, const char* status) {
    
    int i, j;
    size_t k;
    task_info_t* task;
    chunk_parts_t chunk_parts;
    const char* key;
    const char* mpi_type;
    int recv_from_rank;
    int* cons;
    size_t cons_len;
    int dest;

    chunk_parts_init(&chunk_parts);
    
    if(ta_ctx == NULL || file == NULL || tag == NULL || rank == NULL || status == NULL){
        return -1;
    }

    if(ta_ctx->for_stmt == NULL){
        return -1;
    }
    
    if(compute_recv_from(ta_ctx) != 0){
        return -1;
    }

    fprintf(file, "%s\n", ta_ctx->for_stmt);

    for(i = 0; i < ta_ctx->tasks_len; i++){
        task = &ta_ctx->tasks[i];

        fprintf(file, "if(%s == %d){\n", rank, task->rank);

        for(j = 0; j < task->reads_len; j++){
            key = task->reads[j].chunk_key;
            mpi_type = task->reads[j].mpi_type;
            recv_from_rank = task->reads[j].recv_from;

            if(recv_from_rank == task->rank){
                continue;
            }
            
            if(parse_chunk_key(key, &chunk_parts) == 0){
                fprintf(file, "MPI_Recv(&%s[%s], %s, %s, %d, %s, MPI_COMM_WORLD, &%s);\n", 
                    chunk_parts.var, chunk_parts.base, chunk_parts.len, mpi_type, recv_from_rank, tag, status);
            } else {
                chunk_parts_free(&chunk_parts);
                return -1;
            }
        }

        if(task->body != NULL && task->body[0] != '\0'){
            fprintf(file, "%s\n", task->body);
        } else {
            fprintf(file, "\n");
        }

        for(j = 0; j < task->writes_len; j++){
            key = task->writes[j].chunk_key;
            mpi_type = task->writes[j].mpi_type;

            if(hash_map_get(ta_ctx->consumers, key, &cons, &cons_len) != 0 || cons == NULL || cons_len == 0){
                if(cons != NULL){
                    free(cons);
                    continue;
                }
            }

            for(k = 0; k < cons_len; k++){
                dest = cons[k];
                if(dest == task->rank){
                    continue;
                }

                if(parse_chunk_key(key, &chunk_parts) == 0){
                    fprintf(file, "MPI_Send(&%s[%s], %s, %s, %d, %s, MPI_COMM_WORLD);\n", 
                        chunk_parts.var, chunk_parts.base, chunk_parts.len, mpi_type, dest, tag);
                } else {
                    chunk_parts_free(&chunk_parts);
                    return -1;
                }
            }

            free(cons);
        }

        fprintf(file, "}\n");
    }
    
    chunk_parts_free(&chunk_parts);
    return 0;
}
