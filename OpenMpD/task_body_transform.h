#ifndef TASK_BODY_TRANSFORM_H
#define TASK_BODY_TRANSFORM_H

#include <stddef.h>

typedef struct task_async_block task_async_block_t;

typedef struct task_body_expr task_body_expr_t;
typedef struct task_body_stmt task_body_stmt_t;
typedef struct task_body_expr_list task_body_expr_list_t;
typedef struct task_body_stmt_list task_body_stmt_list_t;

task_body_expr_list_t *task_body_expr_list_create();
int task_body_expr_list_append(task_body_expr_list_t *list, task_body_expr_t *expr);
void task_body_expr_list_destroy(task_body_expr_list_t *list);

task_body_stmt_list_t *task_body_stmt_list_create();
int task_body_stmt_list_append(task_body_stmt_list_t *list, task_body_stmt_t *stmt);
void task_body_stmt_list_destroy(task_body_stmt_list_t *list);

task_body_expr_t *task_body_expr_make_identifier(const char *name, const char *value_type, int is_function);
task_body_expr_t *task_body_expr_make_constant(const char *raw_text, const char *value_type);
task_body_expr_t *task_body_expr_make_string_literal(const char *raw_text);
task_body_expr_t *task_body_expr_make_paren(task_body_expr_t *inner);
task_body_expr_t *task_body_expr_make_unary(const char *op, task_body_expr_t *operand);
task_body_expr_t *task_body_expr_make_binary(const char *op, task_body_expr_t *lhs, task_body_expr_t *rhs, const char *value_type);
task_body_expr_t *task_body_expr_make_assignment(const char *op, task_body_expr_t *lhs, task_body_expr_t *rhs, const char *value_type);
task_body_expr_t *task_body_expr_make_call(task_body_expr_t *callee, task_body_expr_list_t *args, const char *value_type);
task_body_expr_t *task_body_expr_make_subscript(task_body_expr_t *base, task_body_expr_t *index, const char *value_type);
task_body_expr_t *task_body_expr_make_member(task_body_expr_t *base, const char *member, const char *value_type);
task_body_expr_t *task_body_expr_make_ptr_member(task_body_expr_t *base, const char *member, const char *value_type);
task_body_expr_t *task_body_expr_make_post_inc(task_body_expr_t *expr, const char *value_type);
task_body_expr_t *task_body_expr_make_post_dec(task_body_expr_t *expr, const char *value_type);
task_body_expr_t *task_body_expr_make_pre_inc(task_body_expr_t *expr, const char *value_type);
task_body_expr_t *task_body_expr_make_pre_dec(task_body_expr_t *expr, const char *value_type);
task_body_expr_t *task_body_expr_make_comma(task_body_expr_t *lhs, task_body_expr_t *rhs, const char *value_type);

task_body_stmt_t *task_body_stmt_make_empty();
task_body_stmt_t *task_body_stmt_make_expr(task_body_expr_t *expr);
task_body_stmt_t *task_body_stmt_make_compound(task_body_stmt_list_t *items);

task_body_stmt_t *task_body_stmt_make_declaration();
int task_body_stmt_add_decl(task_body_stmt_t *stmt, const char *name, const char *value_type, task_body_expr_t *init_expr);

void task_body_stmt_destroy(task_body_stmt_t *stmt);

int task_body_prepare_task_async_block(task_async_block_t *task_async_block, task_body_stmt_t *body_root);

#endif
