#ifndef TRANSLATOR_STATE_H
#define TRANSLATOR_STATE_H

/*
 * Central translator context shared across parser and transform modules. It
 * groups tasking, distribute, master-region, halo and num_teams state so each
 * feature can track progress while the source is scanned incrementally.
 */

#include <string>
#include <vector>

/*
 * Sentinel for translator_distribute_state_t.brace_count meaning the
 * distributed loop body has finished and the region is ready to close.
 */
#define DISTRIBUTE_BRACE_CLOSED (-100)

typedef struct {
    int expect_task_body;
    int in_task_body_parse;
} translator_tasking_state_t;

typedef struct {
    int active;
    int extended_mode;
    int for_state;
    int brace_count;
    int insert_for_braces;
    std::string pending_lines;
    std::string loop_variable;
    std::string schedule_expr;
} translator_distribute_state_t;

typedef struct {
    int active;
    int brace_count;
    int closes_by_braces;
} translator_master_state_t;

typedef struct {
    int pending_distribute;
    std::string last_distribute_start;
    std::string last_distribute_end;
    std::string pending_var;
    std::string pending_elems;
    std::string pending_rows;
    std::string pending_type;
    std::vector<std::string> declared_vars;
} translator_halo_state_t;

typedef struct {
    int active;
    std::string expr;
} translator_num_teams_state_t;

typedef struct {
    translator_tasking_state_t tasking;
    translator_distribute_state_t distribute;
    translator_master_state_t master;
    translator_halo_state_t halo;
    translator_num_teams_state_t num_teams;
} translator_state_t;

typedef translator_state_t translator_context_t;

/* Returns the global translator state */
translator_state_t *translator_state_get(void);

/* Returns the global translator context alias */
translator_context_t *translator_context_get(void);

/* Resets the full translator state */
void translator_state_reset(translator_state_t *state);

/* Resets tasking parser state */
void translator_tasking_state_reset(translator_tasking_state_t *state);

/* Resets distribute transformation state */
void translator_distribute_state_reset(translator_distribute_state_t *state);

/* Resets master-region tracking state */
void translator_master_state_reset(translator_master_state_t *state);

/* Resets halo transformation state */
void translator_halo_state_reset(translator_halo_state_t *state);

/* Resets num_teams clause state */
void translator_num_teams_state_reset(translator_num_teams_state_t *state);

/* Returns whether task body capture is active */
int tasking_parse_is_capture_active(void);

/* Returns whether the next statement should start task body capture */
int tasking_parse_should_begin_body(void);

/* Returns whether the parser is inside a captured task body */
int tasking_parse_is_in_body(void);

/* Marks that the next parsed statement is a task body */
void tasking_parse_expect_body(void);

/* Enters task body parsing mode */
void tasking_parse_begin_body(void);

/* Leaves task body parsing mode */
void tasking_parse_finish_body(void);

/* Returns whether a num_teams clause is pending */
int num_teams_is_active(void);

/* Returns the pending num_teams expression */
const std::string &num_teams_expr(void);

/* Stores a pending num_teams expression */
void num_teams_set_expr(const char *expr);

/* Clears the pending num_teams expression */
void num_teams_clear(void);

/* Starts tracking a master region */
void master_region_begin(void);

/* Returns whether a master region is being tracked */
int master_region_is_active(void);

/* Consumes a source line and returns whether it ends the master region */
int master_region_consume_line(const char *line);

#endif
