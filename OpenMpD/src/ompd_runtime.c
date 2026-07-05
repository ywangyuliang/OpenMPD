#include "ompd_runtime.h"
#include "hash_map.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <mpi.h>

#define OMPD_MAX_TASKS 524288
#define OMPD_MAX_NAME_LEN 64
#define OMPD_MAX_TASK_INPUT_SIZE 1024
#define OMPD_MAX_DEPS_PER_TASK 64
#define OMPD_MAX_WAITERS_PER_KEY 64
#define OMPD_MAX_VALUE_SIZE 16
#define OMPD_MAX_DEPS (OMPD_MAX_TASKS * OMPD_MAX_DEPS_PER_TASK)
#define OMPD_DEP_INDEX_BUCKETS 1048576

#define OMPD_SCHEDULER_RANK 0
#define OMPD_MASTER_RANK 1
#define OMPD_FIRST_WORKER_RANK 2

#define OMPD_MPI_TAG_REQ         0
#define OMPD_MPI_TAG_WORK_REPLY  1
#define OMPD_MPI_TAG_TASK_INPUT  2
#define OMPD_MPI_TAG_WAIT_REPLY  3

typedef int ompd_dep_key_t;
typedef int ompd_rank_t;

/* Lifecycle states of a runtime task */
typedef enum {
    OMPD_TASK_STATE_CREATED = 1,  
    OMPD_TASK_STATE_BLOCKED,      
    OMPD_TASK_STATE_READY,        
    OMPD_TASK_STATE_RUNNING,      
    OMPD_TASK_STATE_WAITING,      
    OMPD_TASK_STATE_DONE        
} ompd_task_state_t;

/* Message kinds exchanged in the scheduler protocol */
typedef enum {
    OMPD_SCHED_BUILD_TASK = 1,
    OMPD_SCHED_REGISTER_DEP,
    OMPD_SCHED_SUBMIT_TASK,
    OMPD_SCHED_WAIT_CHILDREN,
    OMPD_SCHED_SEND_VALUE,
    OMPD_SCHED_RECV_VALUE,
    OMPD_SCHED_TASK_DONE,
    OMPD_SCHED_WORK_REQUEST,
    OMPD_SCHED_FINALIZE
} ompd_sched_kind_t;

typedef enum {
    OMPD_DEP_KIND_INPUT = 1,
    OMPD_DEP_KIND_OUTPUT
} ompd_dep_kind_t;

/* Reply the scheduler sends to a worker's work request */
typedef enum {
    OMPD_WORK_IDLE = 1,
    OMPD_WORK_RUN_TASK,
    OMPD_WORK_SHUTDOWN
} ompd_work_reply_kind_t;

/* One input/output dependency slot of a task */
typedef struct {
    int has_index;
    int element_index;
    size_t value_size;
    char name[OMPD_MAX_NAME_LEN];
} ompd_task_dep_t;

/* Scheduler-side record of one task and its state */
typedef struct {
    int active;
    ompd_task_id_t task_id;
    ompd_task_id_t parent_task_id;
    ompd_task_state_t state;

    ompd_task_kind_t generated_task_kind;
    size_t task_input_data_size;
    unsigned char *task_input_data;

    int input_dep_count;
    ompd_task_dep_t **input_deps;

    int output_dep_count;
    ompd_task_dep_t **output_deps;

    int pending_dep_count;
    int unfinished_child_count;
    ompd_rank_t waiting_rank;
} ompd_task_record_t;

/* Scheduler-side record of one dependency variable and its waiters */
typedef struct {
    int active;
    ompd_task_id_t scope_task_id;

    int has_index;
    int element_index;
    size_t value_size;
    char name[OMPD_MAX_NAME_LEN];

    ompd_task_id_t last_producer_task_id;

    int waiters_count;
    ompd_task_id_t *waiting_task_ids;

    int has_value;
    size_t stored_value_size;
    unsigned char *stored_value;
} ompd_dep_record_t;

/* Ring-buffer queue of task ids (used as the ready queue) */
typedef struct {
    ompd_task_id_t *values;
    int head;
    int tail;
} ompd_int_queue_t;

/* Scheduler request/reply message payloads (see ompd_sched_kind_t) */
typedef struct {
    int kind;
    ompd_task_id_t current_task_id;
    ompd_task_kind_t generated_task_kind;
    size_t task_input_data_size;
} ompd_sched_build_task_request_t;

typedef struct {
    ompd_task_id_t task_id;
} ompd_sched_build_task_reply_t;

typedef struct {
    int kind;
    ompd_task_id_t task_id;
    ompd_dep_kind_t dep_kind;
    int has_index;
    int element_index;
    size_t value_size;
    char name[OMPD_MAX_NAME_LEN];
} ompd_sched_register_dep_request_t;

typedef struct {
    int kind;
    ompd_task_id_t task_id;
} ompd_sched_submit_task_request_t;

typedef struct {
    int kind;
    ompd_task_id_t current_task_id;
} ompd_sched_wait_children_request_t;

typedef struct {
    int ok;
    ompd_task_id_t task_id;
} ompd_sched_wait_children_reply_t;

typedef struct {
    int kind;
    ompd_task_id_t current_task_id;
    size_t value_size;
    char name[OMPD_MAX_NAME_LEN];
} ompd_sched_send_value_request_t;

typedef struct {
    int kind;
    ompd_task_id_t current_task_id;
    char name[OMPD_MAX_NAME_LEN];
} ompd_sched_recv_value_request_t;

typedef struct {
    size_t value_size;
} ompd_sched_recv_value_reply_t;

typedef struct {
    int kind;
    ompd_task_id_t task_id;
} ompd_sched_task_done_request_t;

typedef struct {
    int kind;
    ompd_task_id_t task_id;
    ompd_task_kind_t generated_task_kind;
    size_t task_input_data_size;
} ompd_sched_work_reply_t;

typedef struct {
    int kind;
} ompd_sched_work_request_t;

static ompd_rank_t ompd_world_rank = -1;
static int ompd_world_size = 0;

static ompd_task_record_t **ompd_tasks = NULL;
static ompd_dep_record_t **ompd_deps = NULL;
static hash_map_t *ompd_dep_index = NULL;
static ompd_int_queue_t *ompd_ready_queue = NULL;
static unsigned char *ompd_completed_waits = NULL;

static ompd_task_id_t ompd_next_task_id = 1;
static ompd_dep_key_t ompd_next_dep_key = 0;
static ompd_task_id_t ompd_current_task_id = 0;
static int ompd_region_num_teams = 0;

static void ompd_runtime_abort(const char *message) {
    fprintf(stderr, "OMPD runtime error: %s\n", message);
    MPI_Abort(MPI_COMM_WORLD, 1);
}

static ompd_task_record_t *ompd_create_task_record(ompd_task_id_t task_id) {
    ompd_task_record_t *task;

    if(task_id < 0 || task_id >= OMPD_MAX_TASKS) {
        ompd_runtime_abort("invalid task id while creating task record");
    }

    if(ompd_tasks[task_id] != NULL) {
        ompd_runtime_abort("task record already exists");
    }

    task = (ompd_task_record_t *) calloc(1, sizeof(*task));
    if(task == NULL) {
        ompd_runtime_abort("cannot allocate task record");
    }

    task->input_deps = (ompd_task_dep_t **) calloc(OMPD_MAX_DEPS_PER_TASK, sizeof(*task->input_deps));
    if(task->input_deps == NULL) {
        free(task);
        ompd_runtime_abort("cannot allocate input dependency pointer vector");
    }

    task->output_deps = (ompd_task_dep_t **) calloc(OMPD_MAX_DEPS_PER_TASK, sizeof(*task->output_deps));
    if(task->output_deps == NULL) {
        free(task->input_deps);
        free(task);
        ompd_runtime_abort("cannot allocate output dependency pointer vector");
    }

    ompd_tasks[task_id] = task;
    return task;
}

static void ompd_destroy_task_record(ompd_task_record_t *task) {
    int i;

    if(task == NULL) {
        return;
    }

    free(task->task_input_data);
    task->task_input_data = NULL;

    if(task->input_deps != NULL) {
        for(i = 0; i < task->input_dep_count; ++i) {
            free(task->input_deps[i]);
            task->input_deps[i] = NULL;
        }

        free(task->input_deps);
        task->input_deps = NULL;
    }

    if(task->output_deps != NULL) {
        for(i = 0; i < task->output_dep_count; ++i) {
            free(task->output_deps[i]);
            task->output_deps[i] = NULL;
        }

        free(task->output_deps);
        task->output_deps = NULL;
    }

    free(task);
}

static ompd_dep_record_t *ompd_create_dep_record(ompd_dep_key_t dep_key) {
    ompd_dep_record_t *dep;

    if(dep_key < 0 || dep_key >= OMPD_MAX_DEPS) {
        ompd_runtime_abort("invalid dependency key while creating dependency record");
    }

    if(ompd_deps[dep_key] != NULL) {
        ompd_runtime_abort("dependency record already exists");
    }

    dep = (ompd_dep_record_t *) calloc(1, sizeof(*dep));
    if(dep == NULL) {
        ompd_runtime_abort("cannot allocate dependency record");
    }

    dep->waiting_task_ids = (ompd_task_id_t *) calloc(OMPD_MAX_WAITERS_PER_KEY, sizeof(*dep->waiting_task_ids));
    if(dep->waiting_task_ids == NULL) {
        free(dep);
        ompd_runtime_abort("cannot allocate dependency waiters vector");
    }

    dep->stored_value = (unsigned char *) malloc(OMPD_MAX_VALUE_SIZE);
    if(dep->stored_value == NULL) {
        free(dep->waiting_task_ids);
        free(dep);
        ompd_runtime_abort("cannot allocate dependency stored value buffer");
    }

    ompd_deps[dep_key] = dep;
    return dep;
}

static void ompd_destroy_dep_record(ompd_dep_record_t *dep) {
    if(dep == NULL) {
        return;
    }

    free(dep->waiting_task_ids);
    dep->waiting_task_ids = NULL;

    free(dep->stored_value);
    dep->stored_value = NULL;

    free(dep);
}

static void ompd_allocate_runtime_memory(void) {
    if(ompd_completed_waits == NULL) {
        ompd_completed_waits = (unsigned char *) calloc(OMPD_MAX_TASKS, sizeof(*ompd_completed_waits));
        if(ompd_completed_waits == NULL) {
            ompd_runtime_abort("cannot allocate completed wait table");
        }
    }

    if(ompd_world_rank != OMPD_SCHEDULER_RANK) {
        return;
    }

    if(ompd_tasks == NULL) {
        ompd_tasks = (ompd_task_record_t **) calloc(OMPD_MAX_TASKS, sizeof(*ompd_tasks));
        if(ompd_tasks == NULL) {
            ompd_runtime_abort("cannot allocate task pointer vector");
        }
    }

    if(ompd_deps == NULL) {
        ompd_deps = (ompd_dep_record_t **) calloc(OMPD_MAX_DEPS, sizeof(*ompd_deps));
        if(ompd_deps == NULL) {
            ompd_runtime_abort("cannot allocate dependency pointer vector");
        }
    }

    if(ompd_dep_index == NULL) {
        ompd_dep_index = hash_map_create(OMPD_DEP_INDEX_BUCKETS);
        if(ompd_dep_index == NULL) {
            ompd_runtime_abort("cannot allocate dependency index");
        }
    }

    if(ompd_ready_queue == NULL) {
        ompd_ready_queue = (ompd_int_queue_t *) calloc(1, sizeof(*ompd_ready_queue));
        if(ompd_ready_queue == NULL) {
            ompd_runtime_abort("cannot allocate ready queue");
        }

        ompd_ready_queue->values = (ompd_task_id_t *) calloc(OMPD_MAX_TASKS, sizeof(*ompd_ready_queue->values));
        if(ompd_ready_queue->values == NULL) {
            free(ompd_ready_queue);
            ompd_ready_queue = NULL;
            ompd_runtime_abort("cannot allocate ready queue values");
        }
    }
}

static void ompd_free_runtime_memory(void) {
    int i;

    if(ompd_tasks != NULL) {
        for(i = 0; i < ompd_next_task_id; ++i) {
            ompd_destroy_task_record(ompd_tasks[i]);
            ompd_tasks[i] = NULL;
        }

        free(ompd_tasks);
        ompd_tasks = NULL;
    }

    if(ompd_dep_index != NULL) {
        hash_map_destroy(ompd_dep_index);
        ompd_dep_index = NULL;
    }

    if(ompd_deps != NULL) {
        for(i = 0; i < ompd_next_dep_key; ++i) {
            ompd_destroy_dep_record(ompd_deps[i]);
            ompd_deps[i] = NULL;
        }

        free(ompd_deps);
        ompd_deps = NULL;
    }

    if(ompd_ready_queue != NULL) {
        free(ompd_ready_queue->values);
        ompd_ready_queue->values = NULL;

        free(ompd_ready_queue);
        ompd_ready_queue = NULL;
    }

    free(ompd_completed_waits);
    ompd_completed_waits = NULL;
}

static void ompd_copy_name(char *dst, const char *src) {
    if(src == NULL) {
        dst[0] = '\0';
        return;
    }

    strncpy(dst, src, OMPD_MAX_NAME_LEN - 1);
    dst[OMPD_MAX_NAME_LEN - 1] = '\0';
}

static void ompd_make_dep_index_key(char *dst, size_t dst_size, ompd_task_id_t scope_task_id, const char *name, int has_index, int element_index) {
    if(dst == NULL || dst_size == 0) {
        ompd_runtime_abort("invalid dependency index key buffer");
    }

    if(name == NULL) {
        name = "";
    }

    snprintf(dst, dst_size, "%d|%s|%d|%d", scope_task_id, name, has_index, element_index);
}

static void ompd_queue_init(ompd_int_queue_t *queue) {
    if(queue == NULL || queue->values == NULL) {
        ompd_runtime_abort("ready queue not allocated");
    }

    queue->head = 0;
    queue->tail = 0;
}

static int ompd_queue_is_empty(const ompd_int_queue_t *queue) {
    return queue->head == queue->tail;
}

static void ompd_queue_push(ompd_int_queue_t *queue, ompd_task_id_t value) {
    int next_tail = (queue->tail + 1) % OMPD_MAX_TASKS;

    if(next_tail == queue->head) {
        ompd_runtime_abort("ready queue overflow");
    }

    queue->values[queue->tail] = value;
    queue->tail = next_tail;
}

static ompd_task_id_t ompd_queue_pop(ompd_int_queue_t *queue) {
    ompd_task_id_t value;

    if(ompd_queue_is_empty(queue)) {
        ompd_runtime_abort("ready queue empty");
    }

    value = queue->values[queue->head];
    queue->head = (queue->head + 1) % OMPD_MAX_TASKS;
    return value;
}

static int ompd_scheduler_build_task(ompd_task_kind_t parent_task_id, ompd_task_kind_t generated_task_kind, const void *task_input_data, size_t task_input_data_size) {
    ompd_task_record_t *task;
    ompd_task_id_t task_id;

    if(ompd_next_task_id >= OMPD_MAX_TASKS) {
        ompd_runtime_abort("task table overflow");
    }

    if(task_input_data_size > OMPD_MAX_TASK_INPUT_SIZE) {
        ompd_runtime_abort("task input data too large");
    }

    task_id = ompd_next_task_id++;
    task = ompd_create_task_record(task_id);

    task->active = 1;
    task->task_id = task_id;
    task->parent_task_id = parent_task_id;
    task->state = OMPD_TASK_STATE_CREATED;
    task->generated_task_kind = generated_task_kind;
    task->task_input_data_size = task_input_data_size;
    task->waiting_rank = -1;

    if(task_input_data_size > 0 && task_input_data != NULL) {
        task->task_input_data = (unsigned char *) malloc(task_input_data_size);
        if(task->task_input_data == NULL) {
            ompd_runtime_abort("cannot allocate task input data");
        }

        memcpy(task->task_input_data, task_input_data, task_input_data_size);
    }

    return task_id;
}

static ompd_task_record_t *ompd_get_task(ompd_task_id_t task_id) {
    if(task_id < 0 || task_id >= OMPD_MAX_TASKS) {
        ompd_runtime_abort("invalid task id");
    }

    if(ompd_tasks == NULL || ompd_tasks[task_id] == NULL || !ompd_tasks[task_id]->active) {
        ompd_runtime_abort("task id not found");
    }

    return ompd_tasks[task_id];
}

static void ompd_add_task_dep(ompd_task_record_t *task, ompd_dep_kind_t dep_kind, const char *name, int has_index, int element_index, size_t value_size) {
    ompd_task_dep_t *dep;

    dep = (ompd_task_dep_t *) calloc(1, sizeof(*dep));
    if(dep == NULL) {
        ompd_runtime_abort("cannot allocate task dependency");
    }

    dep->has_index = has_index;
    dep->element_index = element_index;
    dep->value_size = value_size;
    ompd_copy_name(dep->name, name);

    if(dep_kind == OMPD_DEP_KIND_INPUT) {
        if(task->input_dep_count >= OMPD_MAX_DEPS_PER_TASK) {
            free(dep);
            ompd_runtime_abort("too many input dependencies on task");
        }

        task->input_deps[task->input_dep_count++] = dep;
    } else {
        if(task->output_dep_count >= OMPD_MAX_DEPS_PER_TASK) {
            free(dep);
            ompd_runtime_abort("too many output dependencies on task");
        }

        task->output_deps[task->output_dep_count++] = dep;
    }
}

static ompd_dep_key_t ompd_find_dep_key(ompd_task_id_t scope_task_id, const char *name, int has_index, int element_index) {
    char key[OMPD_MAX_NAME_LEN + 64];
    ompd_dep_key_t dep_key;

    if(ompd_dep_index == NULL) {
        ompd_runtime_abort("dependency index not allocated");
    }

    ompd_make_dep_index_key(key, sizeof(key), scope_task_id, name, has_index, element_index);

    if(hash_map_get_single(ompd_dep_index, key, &dep_key) != 0) {
        return -1;
    }

    if(dep_key < 0 || dep_key >= ompd_next_dep_key) {
        return -1;
    }

    if(ompd_deps[dep_key] == NULL || !ompd_deps[dep_key]->active) {
        return -1;
    }

    return dep_key;
}

static ompd_dep_key_t ompd_get_or_create_dep_key(ompd_task_id_t scope_task_id, const char *name, int has_index, int element_index, size_t value_size) {
    ompd_dep_key_t found;
    ompd_dep_key_t dep_key;
    char key[OMPD_MAX_NAME_LEN + 64];
    ompd_dep_record_t *dep;

    found = ompd_find_dep_key(scope_task_id, name, has_index, element_index);
    if(found >= 0) {
        return found;
    }

    if(ompd_next_dep_key >= OMPD_MAX_DEPS) {
        ompd_runtime_abort("dependency table overflow");
    }

    dep_key = ompd_next_dep_key++;
    dep = ompd_create_dep_record(dep_key);

    dep->active = 1;
    dep->scope_task_id = scope_task_id;
    dep->has_index = has_index;
    dep->element_index = element_index;
    dep->value_size = value_size;
    dep->last_producer_task_id = 0;
    ompd_copy_name(dep->name, name);

    ompd_make_dep_index_key(key, sizeof(key), scope_task_id, name, has_index, element_index);

    if(hash_map_set_single(ompd_dep_index, key, dep_key) != 0) {
        ompd_runtime_abort("cannot insert dependency into index");
    }

    return dep_key;
}

static int ompd_find_task_output_dep_by_name(const ompd_task_record_t *task, const char *name) {
    int i;

    for (i = 0; i < task->output_dep_count; i++) {
        if(task->output_deps[i] != NULL && strcmp(task->output_deps[i]->name, name) == 0) {
            return i;
        }
    }

    return -1;
}

static int ompd_scheduler_find_send_key(ompd_task_id_t current_task_id, const char *name) {
    ompd_task_record_t *task;
    int dep_index;

    if(current_task_id == 0) {
        return ompd_find_dep_key(0, name, 0, 0);
    }

    task = ompd_get_task(current_task_id);
    dep_index = ompd_find_task_output_dep_by_name(task, name);

    if(dep_index < 0) {
        ompd_runtime_abort("send value not registered as task output");
    }

    return ompd_find_dep_key(
        task->parent_task_id,
        task->output_deps[dep_index]->name,
        task->output_deps[dep_index]->has_index,
        task->output_deps[dep_index]->element_index);
}

static int ompd_find_task_input_dep_by_name(const ompd_task_record_t *task, const char *name) {
    int i;

    for (i = 0; i < task->input_dep_count; i++) {
        if(task->input_deps[i] != NULL && strcmp(task->input_deps[i]->name, name) == 0) {
            return i;
        }
    }

    return -1;
}

static int ompd_scheduler_find_recv_key(ompd_task_id_t current_task_id, const char *name) {
    ompd_task_record_t *task;
    int dep_index;
    ompd_dep_key_t dep_key;

    if(current_task_id == 0) {
        dep_key = ompd_find_dep_key(0, name, 0, 0);

        if(dep_key < 0) {
            ompd_runtime_abort("recv value not found in root scope");
        }

        return dep_key;
    }

    task = ompd_get_task(current_task_id);

    dep_index = ompd_find_task_input_dep_by_name(task, name);
    if(dep_index >= 0) {
        dep_key = ompd_find_dep_key(
            task->parent_task_id,
            task->input_deps[dep_index]->name,
            task->input_deps[dep_index]->has_index,
            task->input_deps[dep_index]->element_index);

        if(dep_key < 0) {
            ompd_runtime_abort("recv input dependency key not found");
        }

        return dep_key;
    }

    dep_key = ompd_find_dep_key(current_task_id, name, 0, 0);
    if(dep_key >= 0) {
        return dep_key;
    }

    ompd_runtime_abort("recv value not found as task input or child result");
    return -1;
}

static void ompd_scheduler_register_waiter(ompd_dep_key_t dep_key, ompd_task_id_t task_id) {
    ompd_dep_record_t *dep = ompd_deps[dep_key];

    if(dep == NULL || !dep->active) {
        ompd_runtime_abort("register waiter on invalid dependency key");
    }

    if(dep->waiters_count >= OMPD_MAX_WAITERS_PER_KEY) {
        ompd_runtime_abort("too many waiters on dependency key");
    }

    dep->waiting_task_ids[dep->waiters_count++] = task_id;
}

static void ompd_scheduler_make_ready_or_blocked(ompd_task_record_t *task) {
    if(task->pending_dep_count == 0) {
        task->state = OMPD_TASK_STATE_READY;
        ompd_queue_push(ompd_ready_queue, task->task_id);
    } else {
        task->state = OMPD_TASK_STATE_BLOCKED;
    }
}

static void ompd_scheduler_submit_task(ompd_task_id_t task_id) {
    int i;
    ompd_dep_key_t dep_key;
    ompd_task_record_t *task;
    ompd_task_record_t *producer;

    task = ompd_get_task(task_id);

    if(task->state != OMPD_TASK_STATE_CREATED) {
        ompd_runtime_abort("submit task not in CREATED state");
    }

    task->pending_dep_count = 0;
    ompd_get_task(task->parent_task_id)->unfinished_child_count++;

    for(i = 0; i < task->input_dep_count; ++i) {
        if(task->input_deps[i] == NULL) {
            ompd_runtime_abort("null input dependency on task");
        }

        dep_key = ompd_get_or_create_dep_key(
            task->parent_task_id,
            task->input_deps[i]->name,
            task->input_deps[i]->has_index,
            task->input_deps[i]->element_index,
            task->input_deps[i]->value_size);

        if(ompd_deps[dep_key]->last_producer_task_id != 0) {
            producer = ompd_get_task(ompd_deps[dep_key]->last_producer_task_id);

            if(producer->state != OMPD_TASK_STATE_DONE) {
                task->pending_dep_count++;
                ompd_scheduler_register_waiter(dep_key, task_id);
            }
        }
    }

    for(i = 0; i < task->output_dep_count; ++i) {
        if(task->output_deps[i] == NULL) {
            ompd_runtime_abort("null output dependency on task");
        }

        dep_key = ompd_get_or_create_dep_key(
            task->parent_task_id,
            task->output_deps[i]->name,
            task->output_deps[i]->has_index,
            task->output_deps[i]->element_index,
            task->output_deps[i]->value_size);

        if(ompd_deps[dep_key]->last_producer_task_id != 0) {
            producer = ompd_get_task(ompd_deps[dep_key]->last_producer_task_id);

            if(producer->state != OMPD_TASK_STATE_DONE) {
                task->pending_dep_count++;
                ompd_scheduler_register_waiter(dep_key, task_id);
            }
        }

        ompd_deps[dep_key]->last_producer_task_id = task_id;
        ompd_deps[dep_key]->has_value = 0;
        ompd_deps[dep_key]->stored_value_size = 0;
    }

    ompd_scheduler_make_ready_or_blocked(task);
}

static void ompd_scheduler_store_value(ompd_task_id_t current_task_id, const char *name, const void *value, size_t value_size) {
    ompd_dep_key_t dep_key = ompd_scheduler_find_send_key(current_task_id, name);

    if(dep_key < 0) {
        ompd_runtime_abort("send value on unknown dependency");
    }

    if(value_size > OMPD_MAX_VALUE_SIZE) {
        ompd_runtime_abort("value too large for runtime storage");
    }

    if(ompd_deps[dep_key] == NULL || ompd_deps[dep_key]->stored_value == NULL) {
        ompd_runtime_abort("send value on invalid dependency storage");
    }

    memcpy(ompd_deps[dep_key]->stored_value, value, value_size);
    ompd_deps[dep_key]->stored_value_size = value_size;
    ompd_deps[dep_key]->has_value = 1;
}

static void ompd_scheduler_wake_tasks_waiting_on_dep(ompd_dep_key_t dep_key) {
    
    int j;
    ompd_dep_record_t *dep;
    ompd_task_record_t *task;

    if(dep_key < 0 || dep_key >= ompd_next_dep_key) {
        return;
    }

    dep = ompd_deps[dep_key];

    if(dep == NULL || !dep->active) {
        return;
    }

    for (j = 0; j < dep->waiters_count; j++) {
        task = ompd_get_task(dep->waiting_task_ids[j]);

        if(task->pending_dep_count > 0) {
            task->pending_dep_count--;
        }

        if(task->pending_dep_count == 0 && task->state == OMPD_TASK_STATE_BLOCKED) {
            task->state = OMPD_TASK_STATE_READY;
            ompd_queue_push(ompd_ready_queue, task->task_id);
        }
    }

    dep->waiters_count = 0;
}

static void ompd_scheduler_wake_tasks_waiting_on(ompd_task_id_t finished_task_id) {
    
    int i;
    ompd_dep_key_t dep_key;
    ompd_task_record_t *finished_task;

    finished_task = ompd_get_task(finished_task_id);

    for(i = 0; i < finished_task->output_dep_count; i++) {
        if(finished_task->output_deps[i] == NULL) {
            continue;
        }

        dep_key = ompd_find_dep_key(
            finished_task->parent_task_id,
            finished_task->output_deps[i]->name,
            finished_task->output_deps[i]->has_index,
            finished_task->output_deps[i]->element_index);

        if(dep_key >= 0 && ompd_deps[dep_key] != NULL && ompd_deps[dep_key]->last_producer_task_id == finished_task_id) {
            ompd_scheduler_wake_tasks_waiting_on_dep(dep_key);
        }
    }
}

static void ompd_mark_wait_completed(ompd_task_id_t task_id) {
    if(task_id < 0 || task_id >= OMPD_MAX_TASKS) {
        ompd_runtime_abort("invalid task id in wait completion");
    }
    ompd_completed_waits[task_id] = 1;
}

static int ompd_consume_wait_completed(ompd_task_id_t task_id) {
    if(task_id < 0 || task_id >= OMPD_MAX_TASKS) {
        ompd_runtime_abort("invalid task id while consuming wait completion");
    }

    if(!ompd_completed_waits[task_id]) {
        return 0;
    }

    ompd_completed_waits[task_id] = 0;
    return 1;
}

static void ompd_poll_wait_replies(void) {
    int flag;
    MPI_Status status;
    ompd_sched_wait_children_reply_t wait_children_reply;

    while(1) {
        flag = 0;

        MPI_Iprobe(OMPD_SCHEDULER_RANK, OMPD_MPI_TAG_WAIT_REPLY, MPI_COMM_WORLD, &flag, &status);

        if(!flag) {
            break;
        }

        MPI_Recv(&wait_children_reply, sizeof(wait_children_reply), MPI_BYTE, OMPD_SCHEDULER_RANK, OMPD_MPI_TAG_WAIT_REPLY, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        if(wait_children_reply.ok) {
            ompd_mark_wait_completed(wait_children_reply.task_id);
        }
    }
}

static void ompd_scheduler_complete_task(ompd_task_id_t task_id) {
    ompd_task_record_t *task;
    ompd_task_record_t *parent;
    ompd_sched_wait_children_reply_t wait_children_reply;

    task = ompd_get_task(task_id);
    parent = ompd_get_task(task->parent_task_id);

    task->state = OMPD_TASK_STATE_DONE;

    ompd_scheduler_wake_tasks_waiting_on(task_id);

    if(parent->unfinished_child_count > 0) {
        parent->unfinished_child_count--;
    }

    if(parent->unfinished_child_count == 0 && parent->state == OMPD_TASK_STATE_WAITING && parent->waiting_rank >= 0) {
        wait_children_reply.ok = 1;
        wait_children_reply.task_id = parent->task_id;

        MPI_Send(&wait_children_reply, sizeof(wait_children_reply), MPI_BYTE, parent->waiting_rank, OMPD_MPI_TAG_WAIT_REPLY, MPI_COMM_WORLD);

        parent->state = OMPD_TASK_STATE_RUNNING;
        parent->waiting_rank = -1;
    }
}

static void ompd_scheduler_send_shutdown_to_worker(ompd_rank_t worker_rank) {
    ompd_sched_work_reply_t work_reply;

    memset(&work_reply, 0, sizeof(work_reply));
    work_reply.kind = OMPD_WORK_SHUTDOWN;

    MPI_Send(&work_reply, sizeof(work_reply), MPI_BYTE, worker_rank, OMPD_MPI_TAG_WORK_REPLY, MPI_COMM_WORLD);
}

static void ompd_scheduler_reply_to_work_request(ompd_rank_t worker_rank) {
    ompd_sched_work_reply_t work_reply;
    ompd_task_record_t *task;
    ompd_task_id_t task_id;

    memset(&work_reply, 0, sizeof(work_reply));

    if(ompd_queue_is_empty(ompd_ready_queue)) {
        work_reply.kind = OMPD_WORK_IDLE;
        MPI_Send(&work_reply, sizeof(work_reply), MPI_BYTE, worker_rank, OMPD_MPI_TAG_WORK_REPLY, MPI_COMM_WORLD);
        return;
    }

    task_id = ompd_queue_pop(ompd_ready_queue);
    task = ompd_get_task(task_id);

    task->state = OMPD_TASK_STATE_RUNNING;

    work_reply.kind = OMPD_WORK_RUN_TASK;
    work_reply.task_id = task_id;
    work_reply.generated_task_kind = task->generated_task_kind;
    work_reply.task_input_data_size = task->task_input_data_size;

    if(work_reply.task_input_data_size < 0) {
        ompd_runtime_abort("invalid task input data size in work reply");
    }

    MPI_Send(&work_reply, sizeof(work_reply), MPI_BYTE, worker_rank, OMPD_MPI_TAG_WORK_REPLY, MPI_COMM_WORLD);

    if(work_reply.task_input_data_size > 0) {
        MPI_Send(task->task_input_data,
                (int) work_reply.task_input_data_size,
                MPI_BYTE,
                worker_rank,
                OMPD_MPI_TAG_TASK_INPUT,
                MPI_COMM_WORLD);
    } else {
        unsigned char dummy = 0;
        MPI_Send(&dummy, 0, MPI_BYTE, worker_rank, OMPD_MPI_TAG_TASK_INPUT, MPI_COMM_WORLD);
    }
}

static void ompd_run_task_from_work_reply(const ompd_sched_work_reply_t *work_reply) {
    ompd_sched_task_done_request_t task_done_request;
    unsigned char *input_data_buffer;
    ompd_task_id_t previous_task_id;

    if(work_reply->kind != OMPD_WORK_RUN_TASK) {
        ompd_runtime_abort("worker received invalid work response");
    }

    if(work_reply->task_input_data_size > OMPD_MAX_TASK_INPUT_SIZE) {
        ompd_runtime_abort("worker received oversized task input data");
    }

    if(work_reply->task_input_data_size < 0) {
        ompd_runtime_abort("worker received invalid task input data size");
    }

    input_data_buffer = (unsigned char *) malloc(OMPD_MAX_TASK_INPUT_SIZE);
    if(input_data_buffer == NULL) {
        ompd_runtime_abort("cannot allocate worker task input buffer");
    }

    MPI_Recv(input_data_buffer,
            (int) work_reply->task_input_data_size,
            MPI_BYTE,
            OMPD_SCHEDULER_RANK,
            OMPD_MPI_TAG_TASK_INPUT,
            MPI_COMM_WORLD,
            MPI_STATUS_IGNORE);

    previous_task_id = ompd_current_task_id;
    ompd_current_task_id = work_reply->task_id;

    ompd_execute_generated_task(work_reply->generated_task_kind, input_data_buffer);

    ompd_current_task_id = previous_task_id;

    free(input_data_buffer);
    input_data_buffer = NULL;

    task_done_request.kind = OMPD_SCHED_TASK_DONE;
    task_done_request.task_id = work_reply->task_id;

    MPI_Send(&task_done_request.kind, 1, MPI_INT, OMPD_SCHEDULER_RANK, OMPD_MPI_TAG_REQ, MPI_COMM_WORLD);

    MPI_Send(((unsigned char *) &task_done_request) + sizeof(int),
            sizeof(task_done_request) - sizeof(int),
            MPI_BYTE,
            OMPD_SCHEDULER_RANK,
            OMPD_MPI_TAG_REQ,
            MPI_COMM_WORLD);
}

static void ompd_scheduler_loop() {
    
    int shutdown_requested;
    int shutdown_reply_count;
    int active_worker_count;
    int sched_kind;
    MPI_Status status;
    ompd_dep_key_t dep_key;
    int ok;
    int next_kind;
    MPI_Status worker_status;

    unsigned char input_data_buffer[OMPD_MAX_TASK_INPUT_SIZE];
    unsigned char value_buffer[OMPD_MAX_VALUE_SIZE];
    ompd_task_record_t *task;

    ompd_sched_build_task_request_t build_task_request;
    ompd_sched_build_task_reply_t build_task_reply;
    ompd_sched_register_dep_request_t register_dep_request;
    ompd_sched_submit_task_request_t submit_task_request;
    ompd_sched_wait_children_request_t wait_children_request;
    ompd_sched_wait_children_reply_t wait_children_reply;
    ompd_sched_send_value_request_t send_value_request;
    ompd_sched_recv_value_request_t recv_value_request;
    ompd_sched_recv_value_reply_t recv_value_reply;
    ompd_sched_task_done_request_t task_done_request;

    shutdown_requested = 0;
    shutdown_reply_count = 0;

    active_worker_count = ompd_world_size - OMPD_FIRST_WORKER_RANK;
    if(active_worker_count < 0) {
        active_worker_count = 0;
    }
    
    while(1){
        sched_kind = 0;

        MPI_Recv(&sched_kind, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);

        switch (sched_kind) {
            case OMPD_SCHED_BUILD_TASK:
                memset(&build_task_request, 0, sizeof(build_task_request));
                build_task_request.kind = sched_kind;

                MPI_Recv(((unsigned char *) &build_task_request) + sizeof(int),
                        sizeof(build_task_request) - sizeof(int),
                        MPI_BYTE,
                        status.MPI_SOURCE,
                        0,
                        MPI_COMM_WORLD,
                        MPI_STATUS_IGNORE);

                if(build_task_request.task_input_data_size > OMPD_MAX_TASK_INPUT_SIZE) {
                    ompd_runtime_abort("task input data too large in build task request");
                }

                if(build_task_request.task_input_data_size < 0) {
                    ompd_runtime_abort("invalid task input data size in build task request");
                }

                MPI_Recv(input_data_buffer,
                        (int) build_task_request.task_input_data_size,
                        MPI_BYTE,
                        status.MPI_SOURCE,
                        0,
                        MPI_COMM_WORLD,
                        MPI_STATUS_IGNORE);

                build_task_reply.task_id = ompd_scheduler_build_task(
                    build_task_request.current_task_id,
                    build_task_request.generated_task_kind,
                    input_data_buffer,
                    build_task_request.task_input_data_size);

                MPI_Send(&build_task_reply, sizeof(build_task_reply), MPI_BYTE, status.MPI_SOURCE, 0, MPI_COMM_WORLD);
                
                break;
            
            case OMPD_SCHED_REGISTER_DEP:
                memset(&register_dep_request, 0, sizeof(register_dep_request));
                register_dep_request.kind = sched_kind;

                MPI_Recv(((unsigned char *) &register_dep_request) + sizeof(int),
                        sizeof(register_dep_request) - sizeof(int),
                        MPI_BYTE,
                        status.MPI_SOURCE,
                        0,
                        MPI_COMM_WORLD,
                        MPI_STATUS_IGNORE);

                task = ompd_get_task(register_dep_request.task_id);

                ompd_add_task_dep(
                    task,
                    register_dep_request.dep_kind,
                    register_dep_request.name,
                    register_dep_request.has_index,
                    register_dep_request.element_index,
                    register_dep_request.value_size);
                break;
            
            case OMPD_SCHED_SUBMIT_TASK:
                memset(&submit_task_request, 0, sizeof(submit_task_request));
                submit_task_request.kind = sched_kind;

                MPI_Recv(((unsigned char *) &submit_task_request) + sizeof(int),
                        sizeof(submit_task_request) - sizeof(int),
                        MPI_BYTE,
                        status.MPI_SOURCE,
                        0,
                        MPI_COMM_WORLD,
                        MPI_STATUS_IGNORE);

                ompd_scheduler_submit_task(submit_task_request.task_id);
                
                break;

            case OMPD_SCHED_WAIT_CHILDREN:
                memset(&wait_children_request, 0, sizeof(wait_children_request));
                wait_children_request.kind = sched_kind;
                
                MPI_Recv(((unsigned char *) &wait_children_request) + sizeof(int),
                        sizeof(wait_children_request) - sizeof(int),
                        MPI_BYTE,
                        status.MPI_SOURCE,
                        0,
                        MPI_COMM_WORLD,
                        MPI_STATUS_IGNORE);

                task = ompd_get_task(wait_children_request.current_task_id);

                if(task->unfinished_child_count == 0) {
                    wait_children_reply.ok = 1;
                    wait_children_reply.task_id = task->task_id;
                    MPI_Send(&wait_children_reply, sizeof(wait_children_reply), MPI_BYTE, status.MPI_SOURCE, OMPD_MPI_TAG_WAIT_REPLY, MPI_COMM_WORLD);
                } else {
                    task->state = OMPD_TASK_STATE_WAITING;
                    task->waiting_rank = status.MPI_SOURCE;
                }

                break;
            
            case OMPD_SCHED_SEND_VALUE:
                memset(&send_value_request, 0, sizeof(send_value_request));
                send_value_request.kind = sched_kind;

                MPI_Recv(((unsigned char *) &send_value_request) + sizeof(int),
                        sizeof(send_value_request) - sizeof(int),
                        MPI_BYTE,
                        status.MPI_SOURCE,
                        0,
                        MPI_COMM_WORLD,
                        MPI_STATUS_IGNORE);

                if(send_value_request.value_size > OMPD_MAX_VALUE_SIZE) {
                    ompd_runtime_abort("value too large in send request");
                }

                if(send_value_request.value_size <= 0) {
                    ompd_runtime_abort("invalid value size in send request");
                }

                MPI_Recv(value_buffer,
                        (int) send_value_request.value_size,
                        MPI_BYTE,
                        status.MPI_SOURCE,
                        0,
                        MPI_COMM_WORLD,
                        MPI_STATUS_IGNORE);

                ompd_scheduler_store_value(
                    send_value_request.current_task_id,
                    send_value_request.name,
                    value_buffer,
                    send_value_request.value_size);

                break;

            case OMPD_SCHED_RECV_VALUE:
                memset(&recv_value_request, 0, sizeof(recv_value_request));
                recv_value_request.kind = sched_kind;

                MPI_Recv(((unsigned char *) &recv_value_request) + sizeof(int),
                        sizeof(recv_value_request) - sizeof(int),
                        MPI_BYTE,
                        status.MPI_SOURCE,
                        0,
                        MPI_COMM_WORLD,
                        MPI_STATUS_IGNORE);

                dep_key = ompd_scheduler_find_recv_key(recv_value_request.current_task_id, recv_value_request.name);

                if(dep_key < 0 || ompd_deps[dep_key] == NULL || !ompd_deps[dep_key]->has_value) {
                    ompd_runtime_abort("recv request for unavailable value");
                }

                recv_value_reply.value_size = ompd_deps[dep_key]->stored_value_size;

                MPI_Send(&recv_value_reply, sizeof(recv_value_reply), MPI_BYTE, status.MPI_SOURCE, 0, MPI_COMM_WORLD);

                MPI_Send(ompd_deps[dep_key]->stored_value,
                        (int) recv_value_reply.value_size,
                        MPI_BYTE,
                        status.MPI_SOURCE,
                        0,
                        MPI_COMM_WORLD);
                break;

            case OMPD_SCHED_TASK_DONE:
                memset(&task_done_request, 0, sizeof(task_done_request));
                task_done_request.kind = sched_kind;

                MPI_Recv(((unsigned char *) &task_done_request) + sizeof(int),
                        sizeof(task_done_request) - sizeof(int),
                        MPI_BYTE,
                        status.MPI_SOURCE,
                        0,
                        MPI_COMM_WORLD,
                        MPI_STATUS_IGNORE);

                ompd_scheduler_complete_task(task_done_request.task_id);

                break;
            
            case OMPD_SCHED_WORK_REQUEST:
                if(shutdown_requested) {
                    ompd_scheduler_send_shutdown_to_worker(status.MPI_SOURCE);
                    shutdown_reply_count++;
                } else {
                    ompd_scheduler_reply_to_work_request(status.MPI_SOURCE);
                }

                break;
            
            case OMPD_SCHED_FINALIZE: 
                shutdown_requested = 1;
                
                if(active_worker_count == 0) {
                    ok = 1;
                    MPI_Send(&ok, 1, MPI_INT, status.MPI_SOURCE, 0, MPI_COMM_WORLD);
                    ompd_free_runtime_memory();
                    MPI_Finalize();
                    exit(0);
                }

                while (shutdown_reply_count < active_worker_count) {
                    next_kind = 0;
                    MPI_Recv(&next_kind, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &worker_status);

                    if(next_kind == OMPD_SCHED_WORK_REQUEST) {
                        ompd_scheduler_send_shutdown_to_worker(worker_status.MPI_SOURCE);
                        shutdown_reply_count++;
                        continue;
                    }

                    if(next_kind == OMPD_SCHED_TASK_DONE) {
                        memset(&task_done_request, 0, sizeof(task_done_request));
                        task_done_request.kind = next_kind;

                        MPI_Recv(((unsigned char *) &task_done_request) + sizeof(int),
                                sizeof(task_done_request) - sizeof(int),
                                MPI_BYTE,
                                worker_status.MPI_SOURCE,
                                0,
                                MPI_COMM_WORLD,
                                MPI_STATUS_IGNORE);

                        ompd_scheduler_complete_task(task_done_request.task_id);
                        continue;
                    }

                    ompd_runtime_abort("unexpected sched during runtime shutdown");
                }

                ok = 1;
                MPI_Send(&ok, 1, MPI_INT, status.MPI_SOURCE, 0, MPI_COMM_WORLD);

                ompd_free_runtime_memory();
                MPI_Finalize();
                exit(0);
                break;
            
            default:
                ompd_runtime_abort("unknown runtime sched");
        }
    }
}

static void ompd_worker_loop() {

    ompd_sched_work_request_t work_request;
    ompd_sched_work_reply_t work_reply;

    while (1) {
        work_request.kind = OMPD_SCHED_WORK_REQUEST;

        MPI_Send(&work_request.kind, 1, MPI_INT, OMPD_SCHEDULER_RANK, OMPD_MPI_TAG_REQ, MPI_COMM_WORLD);
        MPI_Recv(&work_reply, sizeof(work_reply), MPI_BYTE, OMPD_SCHEDULER_RANK, OMPD_MPI_TAG_WORK_REPLY, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        if(work_reply.kind == OMPD_WORK_SHUTDOWN) {
            ompd_free_runtime_memory();
            MPI_Finalize();
            exit(0);
        }

        if(work_reply.kind == OMPD_WORK_IDLE) {
            continue;
        }

        ompd_run_task_from_work_reply(&work_reply);
    }
}

int ompd_current_rank_is_scheduler() {
    return ompd_world_rank == OMPD_SCHEDULER_RANK;
}

int ompd_current_rank_is_master() {
    return ompd_world_rank == OMPD_MASTER_RANK;
}

int ompd_current_rank_is_worker() {
    return ompd_world_rank >= OMPD_FIRST_WORKER_RANK;
}

void ompd_initialize_runtime(int *argc, char ***argv) {
    MPI_Init(argc, argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &ompd_world_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &ompd_world_size);

    if(ompd_world_size < 2) {
        ompd_runtime_abort("runtime needs at least 2 ranks: scheduler + master");
    }

    ompd_allocate_runtime_memory();

    ompd_next_task_id = 1;
    ompd_next_dep_key = 0;
    ompd_current_task_id = 0;
    ompd_region_num_teams = 0;

    if(ompd_current_rank_is_scheduler()) {
        ompd_task_record_t *root_task;

        ompd_queue_init(ompd_ready_queue);

        root_task = ompd_create_task_record(0);
        root_task->active = 1;
        root_task->task_id = 0;
        root_task->parent_task_id = 0;
        root_task->state = OMPD_TASK_STATE_RUNNING;
        root_task->waiting_rank = -1;

        ompd_scheduler_loop();
        return;
    }

    if(ompd_current_rank_is_worker()) {
        ompd_worker_loop();
        return;
    }

}

void ompd_check_region_num_teams(int num_teams) {
    if(!ompd_current_rank_is_master()) {
        return;
    }

    if(num_teams <= 0) {
        ompd_runtime_abort("num teams must be greater than zero");
    }

    if(num_teams != ompd_world_size){
        ompd_runtime_abort("num teams does not match MPI process count");
    }
}

ompd_task_id_t ompd_build_task(const ompd_task_definition_t *task_definition) {
    ompd_sched_build_task_request_t build_task_request;
    ompd_sched_build_task_reply_t build_task_reply;

    if(task_definition == NULL) {
        ompd_runtime_abort("build task with null definition");
    }

    build_task_request.kind = OMPD_SCHED_BUILD_TASK;
    build_task_request.current_task_id = ompd_current_task_id;
    build_task_request.generated_task_kind = task_definition->generated_task_kind;
    build_task_request.task_input_data_size = task_definition->task_input_data_size;
    
    if(build_task_request.task_input_data_size > OMPD_MAX_TASK_INPUT_SIZE) {
        ompd_runtime_abort("worker received oversized task input data");
    }

    if(build_task_request.task_input_data_size < 0) {
        ompd_runtime_abort("worker received invalid task input data size");
    }

    MPI_Send(&build_task_request.kind, 1, MPI_INT, OMPD_SCHEDULER_RANK, 0, MPI_COMM_WORLD);

    MPI_Send(((unsigned char *) &build_task_request) + sizeof(int),
            sizeof(build_task_request) - sizeof(int),
            MPI_BYTE,
            OMPD_SCHEDULER_RANK,
            0,
            MPI_COMM_WORLD);

    MPI_Send(task_definition->task_input_data,
            (int) build_task_request.task_input_data_size,
            MPI_BYTE,
            OMPD_SCHEDULER_RANK,
            0,
            MPI_COMM_WORLD);
    
    MPI_Recv(&build_task_reply, sizeof(build_task_reply), MPI_BYTE, OMPD_SCHEDULER_RANK, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    return build_task_reply.task_id;
}

static void ompd_register_dep_common(ompd_task_id_t task_id, ompd_dep_kind_t dep_kind, const char *name, int has_index, int element_index, size_t value_size) {
    ompd_sched_register_dep_request_t register_dep_request;

    register_dep_request.kind = OMPD_SCHED_REGISTER_DEP;
    register_dep_request.task_id = task_id;
    register_dep_request.dep_kind = dep_kind;
    register_dep_request.has_index = has_index;
    register_dep_request.element_index = element_index;
    register_dep_request.value_size = value_size;
    ompd_copy_name(register_dep_request.name, name);

    MPI_Send(&register_dep_request.kind, 1, MPI_INT, OMPD_SCHEDULER_RANK, 0, MPI_COMM_WORLD);

    MPI_Send(((unsigned char *) &register_dep_request) + sizeof(int),
            sizeof(register_dep_request) - sizeof(int),
            MPI_BYTE,
            OMPD_SCHEDULER_RANK,
            0,
            MPI_COMM_WORLD);
}

void ompd_register_task_input_variable(ompd_task_id_t task_id, const char *name, size_t value_size) {
    ompd_register_dep_common(task_id, OMPD_DEP_KIND_INPUT, name, 0, 0, value_size);
}

void ompd_register_task_output_variable(ompd_task_id_t task_id, const char *name, size_t value_size) {
    ompd_register_dep_common(task_id, OMPD_DEP_KIND_OUTPUT, name, 0, 0, value_size);
}

void ompd_register_task_input_array_element(ompd_task_id_t task_id, const char *name, int element_index, size_t element_size) {
    ompd_register_dep_common(task_id, OMPD_DEP_KIND_INPUT, name, 1, element_index, element_size);
}

void ompd_register_task_output_array_element(ompd_task_id_t task_id, const char *name, int element_index, size_t element_size) {
    ompd_register_dep_common(task_id, OMPD_DEP_KIND_OUTPUT, name, 1, element_index, element_size);
}

void ompd_submit_task(ompd_task_id_t task_id) {
    ompd_sched_submit_task_request_t submit_task_request;

    submit_task_request.kind = OMPD_SCHED_SUBMIT_TASK;
    submit_task_request.task_id = task_id;

    MPI_Send(&submit_task_request.kind, 1, MPI_INT, OMPD_SCHEDULER_RANK, 0, MPI_COMM_WORLD);

    MPI_Send(((unsigned char *) &submit_task_request) + sizeof(int),
            sizeof(submit_task_request) - sizeof(int),
            MPI_BYTE,
            OMPD_SCHEDULER_RANK,
            0,
            MPI_COMM_WORLD);
}

void ompd_wait_for_child_tasks() {
    ompd_sched_wait_children_request_t wait_children_request;
    ompd_sched_work_request_t work_request;
    ompd_sched_work_reply_t work_reply;
    ompd_task_id_t waiting_task_id;

    waiting_task_id = ompd_current_task_id;

    wait_children_request.kind = OMPD_SCHED_WAIT_CHILDREN;
    wait_children_request.current_task_id = waiting_task_id;

    MPI_Send(&wait_children_request.kind, 1, MPI_INT, OMPD_SCHEDULER_RANK, OMPD_MPI_TAG_REQ, MPI_COMM_WORLD);

    MPI_Send(((unsigned char *) &wait_children_request) + sizeof(int),
            sizeof(wait_children_request) - sizeof(int),
            MPI_BYTE,
            OMPD_SCHEDULER_RANK,
            OMPD_MPI_TAG_REQ,
            MPI_COMM_WORLD);

    while(1){
        ompd_poll_wait_replies();

        if(ompd_consume_wait_completed(waiting_task_id)) {
            return;
        }

        work_request.kind = OMPD_SCHED_WORK_REQUEST;

        MPI_Send(&work_request.kind, 1, MPI_INT,OMPD_SCHEDULER_RANK, OMPD_MPI_TAG_REQ, MPI_COMM_WORLD);

        MPI_Recv(&work_reply,
                sizeof(work_reply),
                MPI_BYTE,
                OMPD_SCHEDULER_RANK,
                OMPD_MPI_TAG_WORK_REPLY,
                MPI_COMM_WORLD,
                MPI_STATUS_IGNORE);

        if(work_reply.kind == OMPD_WORK_IDLE) {
            continue;
        }

        if(work_reply.kind == OMPD_WORK_SHUTDOWN) {
            ompd_runtime_abort("unexpected shutdown while waiting for child tasks");
        }

        ompd_run_task_from_work_reply(&work_reply);
    }
}

static void ompd_send_value_common(const char *name, const void *value, size_t value_size) {
    ompd_sched_send_value_request_t send_value_request;

    if(value_size > OMPD_MAX_VALUE_SIZE) {
        ompd_runtime_abort("value too large to send");
    }

    if(value_size <= 0) {
        ompd_runtime_abort("invalid value size to send");
    }

    send_value_request.kind = OMPD_SCHED_SEND_VALUE;
    send_value_request.current_task_id = ompd_current_task_id;
    send_value_request.value_size = value_size;
    ompd_copy_name(send_value_request.name, name);

    MPI_Send(&send_value_request.kind, 1, MPI_INT, OMPD_SCHEDULER_RANK, 0, MPI_COMM_WORLD);

    MPI_Send(((unsigned char *) &send_value_request) + sizeof(int),
            sizeof(send_value_request) - sizeof(int),
            MPI_BYTE,
            OMPD_SCHEDULER_RANK,
            0,
            MPI_COMM_WORLD);

    MPI_Send((void *) value, (int) value_size, MPI_BYTE, OMPD_SCHEDULER_RANK, 0, MPI_COMM_WORLD);
}

void ompd_runtime_send_int_value(const char *name, int value) {
    ompd_send_value_common(name, &value, sizeof(value));
}

void ompd_runtime_send_long_value(const char *name, long value) {
    ompd_send_value_common(name, &value, sizeof(value));
}

void ompd_runtime_send_double_value(const char *name, double value) {
    ompd_send_value_common(name, &value, sizeof(value));
}

static void ompd_recv_value_common(const char *name, void *value_out, size_t expected_size){
    ompd_sched_recv_value_request_t recv_value_request;
    ompd_sched_recv_value_reply_t recv_value_reply;

    recv_value_request.kind = OMPD_SCHED_RECV_VALUE;
    recv_value_request.current_task_id = ompd_current_task_id;
    
    ompd_copy_name(recv_value_request.name, name);

    MPI_Send(&recv_value_request.kind, 1, MPI_INT, OMPD_SCHEDULER_RANK, 0, MPI_COMM_WORLD);

    MPI_Send(((unsigned char *) &recv_value_request) + sizeof(int),
            sizeof(recv_value_request) - sizeof(int),
            MPI_BYTE,
            OMPD_SCHEDULER_RANK,
            0,
            MPI_COMM_WORLD);

    MPI_Recv(&recv_value_reply, sizeof(recv_value_reply), MPI_BYTE, OMPD_SCHEDULER_RANK, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    if(recv_value_reply.value_size > OMPD_MAX_VALUE_SIZE) {
        ompd_runtime_abort("recv value too large");
    }

    if(expected_size != 0 && recv_value_reply.value_size != expected_size) {
        ompd_runtime_abort("recv value size mismatch");
    }

    MPI_Recv(value_out,
            (int) recv_value_reply.value_size,
            MPI_BYTE,
            OMPD_SCHEDULER_RANK,
            0,
            MPI_COMM_WORLD,
            MPI_STATUS_IGNORE);
}

int ompd_runtime_recv_int_value(const char *name) {
    ompd_task_id_t value;

    value = 0;
    ompd_recv_value_common(name, &value, sizeof(value));
    return value;
}

long ompd_runtime_recv_long_value(const char *name) {
    long value;

    value = 0;
    ompd_recv_value_common(name, &value, sizeof(value));
    return value;
}

double ompd_runtime_recv_double_value(const char *name) {
    double value;

    value = 0.0;
    ompd_recv_value_common(name, &value, sizeof(value));
    return value;
}

void ompd_finalize_runtime() {
    int sched_kind;
    int ok;

    sched_kind = OMPD_SCHED_FINALIZE;
    ok = 0;

    if(!ompd_current_rank_is_master()) {
        return;
    }

    MPI_Send(&sched_kind, 1, MPI_INT, OMPD_SCHEDULER_RANK, 0, MPI_COMM_WORLD);
    MPI_Recv(&ok, 1, MPI_INT, OMPD_SCHEDULER_RANK, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    ompd_free_runtime_memory();
    MPI_Finalize();
}
