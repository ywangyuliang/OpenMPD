#ifndef TASK_BODY_TRANSFORM_H
#define TASK_BODY_TRANSFORM_H

/*
 * Builds and transforms a lightweight IR for captured task bodies. It creates
 * expression and statement nodes from parsed C code and prepares generated
 * task bodies that can run through the OMPD tasking runtime.
 */

#include <stddef.h>

typedef struct task_async_block task_async_block_t;

typedef struct task_body_expr task_body_expr_t;
typedef struct task_body_stmt task_body_stmt_t;
typedef struct task_body_expr_list task_body_expr_list_t;
typedef struct task_body_stmt_list task_body_stmt_list_t;

/* Creates an empty expression list */
task_body_expr_list_t *task_body_expr_list_create();

/* Appends an expression to an expression list */
int task_body_expr_list_append(task_body_expr_list_t *list, task_body_expr_t *expr);

/* Releases an expression list and its items */
void task_body_expr_list_destroy(task_body_expr_list_t *list);

/* Creates an empty statement list */
task_body_stmt_list_t *task_body_stmt_list_create();

/* Appends a statement to a statement list */
int task_body_stmt_list_append(task_body_stmt_list_t *list, task_body_stmt_t *stmt);

/* Releases a statement list and its items */
void task_body_stmt_list_destroy(task_body_stmt_list_t *list);

/* Creates an identifier expression */
task_body_expr_t *task_body_expr_make_identifier(const char *name, const char *value_type, int is_function);

/* Creates a numeric or typed constant expression */
task_body_expr_t *task_body_expr_make_constant(const char *raw_text, const char *value_type);

/* Creates a string literal expression */
task_body_expr_t *task_body_expr_make_string_literal(const char *raw_text);

/* Creates a parenthesized expression */
task_body_expr_t *task_body_expr_make_paren(task_body_expr_t *inner);

/* Creates a unary operator expression */
task_body_expr_t *task_body_expr_make_unary(const char *op, task_body_expr_t *operand);

/* Creates a binary operator expression */
task_body_expr_t *task_body_expr_make_binary(const char *op, task_body_expr_t *lhs, task_body_expr_t *rhs, const char *value_type);

/* Creates an assignment expression */
task_body_expr_t *task_body_expr_make_assignment(const char *op, task_body_expr_t *lhs, task_body_expr_t *rhs, const char *value_type);

/* Creates a function call expression */
task_body_expr_t *task_body_expr_make_call(task_body_expr_t *callee, task_body_expr_list_t *arguments, const char *value_type);

/* Creates an array subscript expression */
task_body_expr_t *task_body_expr_make_subscript(task_body_expr_t *base, task_body_expr_t *index, const char *value_type);

/* Creates a struct member access expression */
task_body_expr_t *task_body_expr_make_member(task_body_expr_t *base, const char *member, const char *value_type);

/* Creates a pointer member access expression */
task_body_expr_t *task_body_expr_make_ptr_member(task_body_expr_t *base, const char *member, const char *value_type);

/* Creates a post-increment expression */
task_body_expr_t *task_body_expr_make_post_inc(task_body_expr_t *expr, const char *value_type);

/* Creates a post-decrement expression */
task_body_expr_t *task_body_expr_make_post_dec(task_body_expr_t *expr, const char *value_type);

/* Creates a pre-increment expression */
task_body_expr_t *task_body_expr_make_pre_inc(task_body_expr_t *expr, const char *value_type);

/* Creates a pre-decrement expression */
task_body_expr_t *task_body_expr_make_pre_dec(task_body_expr_t *expr, const char *value_type);

/* Creates a comma expression */
task_body_expr_t *task_body_expr_make_comma(task_body_expr_t *lhs, task_body_expr_t *rhs, const char *value_type);

/* Creates an empty statement */
task_body_stmt_t *task_body_stmt_make_empty();

/* Creates an expression statement */
task_body_stmt_t *task_body_stmt_make_expr(task_body_expr_t *expr);

/* Creates a compound statement from a statement list */
task_body_stmt_t *task_body_stmt_make_compound(task_body_stmt_list_t *items);

/* Creates an empty declaration statement */
task_body_stmt_t *task_body_stmt_make_declaration();

/* Adds one declarator to a declaration statement */
int task_body_stmt_add_decl(task_body_stmt_t *stmt, const char *name, const char *value_type, task_body_expr_t *init_expr);

/* Releases a statement tree */
void task_body_stmt_destroy(task_body_stmt_t *stmt);

/* Prepares generated code for an async task block from its parsed body */
int task_body_prepare_task_async_block(task_async_block_t *task_async_block, task_body_stmt_t *body_root);

#endif
