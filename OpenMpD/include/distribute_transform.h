#ifndef DISTRIBUTE_TRANSFORM_H
#define DISTRIBUTE_TRANSFORM_H

/*
 * Handles OpenMPD distribute directives. It tracks the active distribute scope,
 * forwards compatible OpenMP clauses, rewrites loop headers with MPI
 * worksharing bounds and emits the generated distribution code.
 */

#include <string>

/* Set while forwarding OpenMP clause text into an extended distribute scope */
extern int clause_forwarding_active;

/* Starts a basic distribute translation scope */
void distribute_begin(void);

/* Starts an extended distribute scope and stores forwarded pragma text */
void distribute_begin_extended(int extended_mode, const char *pending_pragma);

/* Ends the current extended distribute scope */
void distribute_end_extended(void);

/* Returns whether a distribute scope is active */
int distribute_is_active(void);

/* Returns the active extended distribute mode */
int distribute_extended_mode(void);

/* Returns whether an extended distribute scope is active */
int distribute_extended_is_active(void);

/* Returns whether the active mode accepts SIMD clauses */
int distribute_extended_accepts_simd_clauses(void);

/* Returns whether the active mode accepts parallel clauses */
int distribute_extended_accepts_parallel_clauses(void);

/* Returns whether the active mode is parallel without SIMD */
int distribute_extended_is_parallel_without_simd(void);

/* Returns whether the active mode is parallel SIMD */
int distribute_extended_is_parallel_simd(void);

/* Returns whether the active mode is parallel for */
int distribute_extended_is_parallel_for(void);

/* Returns whether source lines should be buffered for rewriting */
int distribute_should_buffer_line(void);

/* Returns whether the next loop header should be rewritten */
int distribute_should_rewrite_loop(void);

/* Returns whether the distribute scope should close now */
int distribute_should_close(void);

/* Returns whether the transformer inserted a loop brace */
int distribute_inserted_loop_brace(void);

/* Starts tracking the body owned by a distribute directive */
void distribute_start_body_tracking(void);

/* Records an opening brace inside the tracked body */
void distribute_note_open_brace(void);

/* Records a closing brace inside the tracked body */
void distribute_note_close_brace(void);

/* Records a token while searching for the loop variable */
void distribute_note_token(const char *token);

/* Rewrites a token when it matches the distributed loop variable */
void distribute_rewrite_loop_variable_token(char **token);

/* Finishes a single-line distribute body when needed */
void distribute_finish_single_line_if_needed(const char *line);

/* Marks that the next loop header should be rewritten */
void distribute_prepare_loop_rewrite(void);

/* Rewrites a loop header into MPI worksharing bounds */
void distribute_rewrite_loop_header(std::string *start_expr, std::string *end_expr, char **line);

/* Marks the active loop header as already rewritten */
void distribute_mark_loop_rewritten(void);

/* Returns the mutable buffer for forwarded OpenMP clauses */
std::string &distribute_pending_lines(void);

/* Returns and clears buffered forwarded OpenMP clauses */
std::string distribute_take_pending_lines(void);

/* Returns whether a dist_schedule expression was captured */
int distribute_has_schedule(void);

/* Returns the captured dist_schedule expression */
const std::string &distribute_schedule_expr(void);

/* Stores the captured dist_schedule expression */
void distribute_set_schedule_expr(const char *expr);

/* Clears the captured dist_schedule expression */
void distribute_clear_schedule(void);

/* Resets distribute state after its generated block closes */
void distribute_reset_after_close(void);

/* Emits MPI worksharing code for the distributed loop range */
void distribute_emit_mpi_worksharing(std::string start_expr, std::string end_expr);

#endif
