#include "task_body_transform.h"
#include "task_async_block.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
	TBE_IDENTIFIER,
	TBE_CONSTANT,
	TBE_STRING_LITERAL,
	TBE_PAREN,
	TBE_UNARY,
	TBE_BINARY,
	TBE_ASSIGN,
	TBE_CALL,
	TBE_SUBSCRIPT,
	TBE_MEMBER,
	TBE_PTR_MEMBER,
	TBE_POST_INC,
	TBE_POST_DEC,
	TBE_PRE_INC,
	TBE_PRE_DEC,
	TBE_COMMA
} task_body_expr_kind_t;

typedef enum {
	TBS_EMPTY,
	TBS_EXPR,
	TBS_COMPOUND,
	TBS_DECL
} task_body_stmt_kind_t;

typedef struct {
	char *name;
	char *value_type;
	task_body_expr_t *init_expr;
} task_body_decl_item_t;

struct task_body_expr {
	task_body_expr_kind_t kind;
	char *op;
	char *raw_text;
	char *name;
	char *member;
	char *value_type;
	int is_function;

	task_body_expr_t *a;
	task_body_expr_t *b;

	task_body_expr_list_t *args;
};

struct task_body_stmt {
	task_body_stmt_kind_t kind;

	task_body_expr_t *expr;
	task_body_stmt_list_t *items;

	task_body_decl_item_t *decls;
	int decls_len;
	int decls_cap;
};

struct task_body_expr_list {
	task_body_expr_t **items;
	int len;
	int cap;
};

struct task_body_stmt_list {
	task_body_stmt_t **items;
	int len;
	int cap;
};

typedef struct {
	char *buf;
	int len;
	int cap;
} sb_t;

typedef struct {
	char **names;
	int len;
	int cap;
} local_scope_t;

typedef struct {
	char *expr_text;
	int index;
} task_input_data_item_t;

typedef struct {
	task_async_block_t *block;
	local_scope_t *scopes;
	int scopes_len;
	int scopes_cap;

	task_input_data_item_t *input_data_items;
	int input_data_items_len;
	int input_data_items_cap;

	int next_input_index;
} task_body_state_t;

static char *sstrdup(const char *s){
	size_t n;
	char *copy;

	if(s == NULL){
		return NULL;
	}

	n = strlen(s);
	copy = (char *)malloc(n + 1);
	if(copy == NULL){
		return NULL;
	}

	memcpy(copy, s, n + 1);
	return copy;
}

static int ensure_array_capacity(void **ptr, int *cap, int elem_size){
	int new_cap;
	void *new_mem;

	if(ptr == NULL || cap == NULL || elem_size <= 0){
		return -1;
	}

	new_cap = (*cap == 0) ? 4 : (*cap * 2);
	new_mem = realloc(*ptr, (size_t)new_cap * (size_t)elem_size);
	if(new_mem == NULL){
		return -1;
	}

	*ptr = new_mem;
	*cap = new_cap;
	return 0;
}

static int sb_init(sb_t *sb){
	if(sb == NULL){
		return -1;
	}

	sb->buf = NULL;
	sb->len = 0;
	sb->cap = 0;
	return 0;
}

static int sb_reserve(sb_t *sb, int extra){
	int need;
	char *new_buf;
	int new_cap;

	if(sb == NULL || extra < 0){
		return -1;
	}

	need = sb->len + extra + 1;
	if(need <= sb->cap){
		return 0;
	}

	new_cap = (sb->cap == 0) ? 64 : sb->cap;
	while(new_cap < need){
		new_cap *= 2;
	}

	new_buf = (char *)realloc(sb->buf, (size_t)new_cap);
	if(new_buf == NULL){
		return -1;
	}

	sb->buf = new_buf;
	sb->cap = new_cap;
	return 0;
}

static int sb_append(sb_t *sb, const char *text){
	int n;

	if(sb == NULL || text == NULL){
		return -1;
	}

	n = (int)strlen(text);
	if(sb_reserve(sb, n) != 0){
		return -1;
	}

	memcpy(sb->buf + sb->len, text, (size_t)n);
	sb->len += n;
	sb->buf[sb->len] = '\0';
	return 0;
}

static int sb_append_char(sb_t *sb, char c){
	if(sb == NULL){
		return -1;
	}

	if(sb_reserve(sb, 1) != 0){
		return -1;
	}

	sb->buf[sb->len++] = c;
	sb->buf[sb->len] = '\0';
	return 0;
}

static char *sb_take(sb_t *sb){
	char *out;

	if(sb == NULL){
		return NULL;
	}

	if(sb->buf == NULL){
		out = sstrdup("");
	} else {
		out = sb->buf;
	}

	sb->buf = NULL;
	sb->len = 0;
	sb->cap = 0;
	return out;
}

static task_body_expr_t *task_body_expr_create(task_body_expr_kind_t kind){
	task_body_expr_t *expr;

	expr = (task_body_expr_t *)calloc(1, sizeof(task_body_expr_t));
	if(expr == NULL){
		return NULL;
	}

	expr->kind = kind;
	return expr;
}

static task_body_stmt_t *task_body_stmt_create(task_body_stmt_kind_t kind){
	task_body_stmt_t *stmt;

	stmt = (task_body_stmt_t *)calloc(1, sizeof(task_body_stmt_t));
	if(stmt == NULL){
		return NULL;
	}

	stmt->kind = kind;
	return stmt;
}

task_body_expr_list_t *task_body_expr_list_create(void){
	return (task_body_expr_list_t *)calloc(1, sizeof(task_body_expr_list_t));
}

int task_body_expr_list_append(task_body_expr_list_t *list, task_body_expr_t *expr){
	if(list == NULL || expr == NULL){
		return -1;
	}

	if(list->len == list->cap){
		if(ensure_array_capacity((void **)&list->items, &list->cap, sizeof(task_body_expr_t *)) != 0){
			return -1;
		}
	}

	list->items[list->len++] = expr;
	return 0;
}

task_body_stmt_list_t *task_body_stmt_list_create(void){
	return (task_body_stmt_list_t *)calloc(1, sizeof(task_body_stmt_list_t));
}

int task_body_stmt_list_append(task_body_stmt_list_t *list, task_body_stmt_t *stmt){
	if(list == NULL || stmt == NULL){
		return -1;
	}

	if(list->len == list->cap){
		if(ensure_array_capacity((void **)&list->items, &list->cap, sizeof(task_body_stmt_t *)) != 0){
			return -1;
		}
	}

	list->items[list->len++] = stmt;
	return 0;
}

task_body_expr_t *task_body_expr_make_identifier(const char *name, const char *value_type, int is_function){
	task_body_expr_t *expr;

	expr = task_body_expr_create(TBE_IDENTIFIER);
	if(expr == NULL){
		return NULL;
	}

	expr->name = sstrdup(name != NULL ? name : "");
	expr->value_type = sstrdup(value_type != NULL ? value_type : "");
	expr->is_function = is_function;
	return expr;
}

task_body_expr_t *task_body_expr_make_constant(const char *raw_text, const char *value_type){
	task_body_expr_t *expr;

	expr = task_body_expr_create(TBE_CONSTANT);
	if(expr == NULL){
		return NULL;
	}

	expr->raw_text = sstrdup(raw_text != NULL ? raw_text : "");
	expr->value_type = sstrdup(value_type != NULL ? value_type : "");
	return expr;
}

task_body_expr_t *task_body_expr_make_string_literal(const char *raw_text){
	task_body_expr_t *expr;

	expr = task_body_expr_create(TBE_STRING_LITERAL);
	if(expr == NULL){
		return NULL;
	}

	expr->raw_text = sstrdup(raw_text != NULL ? raw_text : "");
	return expr;
}

task_body_expr_t *task_body_expr_make_paren(task_body_expr_t *inner){
	task_body_expr_t *expr;

	expr = task_body_expr_create(TBE_PAREN);
	if(expr == NULL){
		return NULL;
	}

	expr->a = inner;
	expr->value_type = sstrdup(inner != NULL && inner->value_type != NULL ? inner->value_type : "");
	return expr;
}

task_body_expr_t *task_body_expr_make_unary(const char *op, task_body_expr_t *operand){
	task_body_expr_t *expr;

	expr = task_body_expr_create(TBE_UNARY);
	if(expr == NULL){
		return NULL;
	}

	expr->op = sstrdup(op != NULL ? op : "");
	expr->a = operand;
	expr->value_type = sstrdup(operand != NULL && operand->value_type != NULL ? operand->value_type : "");
	return expr;
}

task_body_expr_t *task_body_expr_make_binary(const char *op, task_body_expr_t *lhs, task_body_expr_t *rhs, const char *value_type){
	task_body_expr_t *expr;

	expr = task_body_expr_create(TBE_BINARY);
	if(expr == NULL){
		return NULL;
	}

	expr->op = sstrdup(op != NULL ? op : "");
	expr->a = lhs;
	expr->b = rhs;
	expr->value_type = sstrdup(value_type != NULL ? value_type : "");
	return expr;
}

task_body_expr_t *task_body_expr_make_assignment(const char *op, task_body_expr_t *lhs, task_body_expr_t *rhs, const char *value_type){
	task_body_expr_t *expr;

	expr = task_body_expr_create(TBE_ASSIGN);
	if(expr == NULL){
		return NULL;
	}

	expr->op = sstrdup(op != NULL ? op : "");
	expr->a = lhs;
	expr->b = rhs;
	expr->value_type = sstrdup(value_type != NULL ? value_type : "");
	return expr;
}

task_body_expr_t *task_body_expr_make_call(task_body_expr_t *callee, task_body_expr_list_t *args, const char *value_type){
	task_body_expr_t *expr;

	expr = task_body_expr_create(TBE_CALL);
	if(expr == NULL){
		return NULL;
	}

	expr->a = callee;
	expr->args = args;
	expr->value_type = sstrdup(value_type != NULL ? value_type : "");
	return expr;
}

task_body_expr_t *task_body_expr_make_subscript(task_body_expr_t *base, task_body_expr_t *index, const char *value_type){
	task_body_expr_t *expr;

	expr = task_body_expr_create(TBE_SUBSCRIPT);
	if(expr == NULL){
		return NULL;
	}

	expr->a = base;
	expr->b = index;
	expr->value_type = sstrdup(value_type != NULL ? value_type : "");
	return expr;
}

task_body_expr_t *task_body_expr_make_member(task_body_expr_t *base, const char *member, const char *value_type){
	task_body_expr_t *expr;

	expr = task_body_expr_create(TBE_MEMBER);
	if(expr == NULL){
		return NULL;
	}

	expr->a = base;
	expr->member = sstrdup(member != NULL ? member : "");
	expr->value_type = sstrdup(value_type != NULL ? value_type : "");
	return expr;
}

task_body_expr_t *task_body_expr_make_ptr_member(task_body_expr_t *base, const char *member, const char *value_type){
	task_body_expr_t *expr;

	expr = task_body_expr_create(TBE_PTR_MEMBER);
	if(expr == NULL){
		return NULL;
	}

	expr->a = base;
	expr->member = sstrdup(member != NULL ? member : "");
	expr->value_type = sstrdup(value_type != NULL ? value_type : "");
	return expr;
}

task_body_expr_t *task_body_expr_make_post_inc(task_body_expr_t *expr0, const char *value_type){
	task_body_expr_t *expr;

	expr = task_body_expr_create(TBE_POST_INC);
	if(expr == NULL){
		return NULL;
	}

	expr->a = expr0;
	expr->value_type = sstrdup(value_type != NULL ? value_type : "");
	return expr;
}

task_body_expr_t *task_body_expr_make_post_dec(task_body_expr_t *expr0, const char *value_type){
	task_body_expr_t *expr;

	expr = task_body_expr_create(TBE_POST_DEC);
	if(expr == NULL){
		return NULL;
	}

	expr->a = expr0;
	expr->value_type = sstrdup(value_type != NULL ? value_type : "");
	return expr;
}

task_body_expr_t *task_body_expr_make_pre_inc(task_body_expr_t *expr0, const char *value_type){
	task_body_expr_t *expr;

	expr = task_body_expr_create(TBE_PRE_INC);
	if(expr == NULL){
		return NULL;
	}

	expr->a = expr0;
	expr->value_type = sstrdup(value_type != NULL ? value_type : "");
	return expr;
}

task_body_expr_t *task_body_expr_make_pre_dec(task_body_expr_t *expr0, const char *value_type){
	task_body_expr_t *expr;

	expr = task_body_expr_create(TBE_PRE_DEC);
	if(expr == NULL){
		return NULL;
	}

	expr->a = expr0;
	expr->value_type = sstrdup(value_type != NULL ? value_type : "");
	return expr;
}

task_body_expr_t *task_body_expr_make_comma(task_body_expr_t *lhs, task_body_expr_t *rhs, const char *value_type){
	task_body_expr_t *expr;

	expr = task_body_expr_create(TBE_COMMA);
	if(expr == NULL){
		return NULL;
	}

	expr->a = lhs;
	expr->b = rhs;
	expr->value_type = sstrdup(value_type != NULL ? value_type : "");
	return expr;
}

task_body_stmt_t *task_body_stmt_make_empty(void){
	return task_body_stmt_create(TBS_EMPTY);
}

task_body_stmt_t *task_body_stmt_make_expr(task_body_expr_t *expr){
	task_body_stmt_t *stmt;

	stmt = task_body_stmt_create(TBS_EXPR);
	if(stmt == NULL){
		return NULL;
	}

	stmt->expr = expr;
	return stmt;
}

task_body_stmt_t *task_body_stmt_make_compound(task_body_stmt_list_t *items){
	task_body_stmt_t *stmt;

	stmt = task_body_stmt_create(TBS_COMPOUND);
	if(stmt == NULL){
		return NULL;
	}

	stmt->items = items;
	return stmt;
}

task_body_stmt_t *task_body_stmt_make_declaration(void){
	return task_body_stmt_create(TBS_DECL);
}

static const char *normalize_c_type_name(const char *value_type){
    if(value_type == NULL || value_type[0] == '\0'){
        fprintf(stderr, "Error: missing value_type in normalize_c_type_name\n");
        exit(901);
    }

    if(strcmp(value_type, "INT") == 0) return "int";
    if(strcmp(value_type, "LONG") == 0) return "long";
    if(strcmp(value_type, "DOUBLE") == 0) return "double";
    if(strcmp(value_type, "FLOAT") == 0) return "float";
    if(strcmp(value_type, "CHAR") == 0) return "char";
    if(strcmp(value_type, "SHORT") == 0) return "short";
    if(strcmp(value_type, "VOID") == 0) return "void";
    if(strcmp(value_type, "SIGNED") == 0) return "signed";
    if(strcmp(value_type, "UNSIGNED") == 0) return "unsigned";

    return value_type;
}

int task_body_stmt_add_decl(task_body_stmt_t *stmt, const char *name, const char *value_type, task_body_expr_t *init_expr){
	task_body_decl_item_t *item;

	if(stmt == NULL || stmt->kind != TBS_DECL || name == NULL || value_type == NULL){
		return -1;
	}

	if(stmt->decls_len == stmt->decls_cap){
		if(ensure_array_capacity((void **)&stmt->decls, &stmt->decls_cap, sizeof(task_body_decl_item_t)) != 0){
			return -1;
		}
	}

	item = &stmt->decls[stmt->decls_len++];
	item->name = sstrdup(name);
	item->value_type = sstrdup(normalize_c_type_name(value_type));
	item->init_expr = init_expr;
	return 0;
}

static void task_body_expr_destroy(task_body_expr_t *expr){
	if(expr == NULL){
		return;
	}

	task_body_expr_destroy(expr->a);
	task_body_expr_destroy(expr->b);

	if(expr->args != NULL){
		task_body_expr_list_destroy(expr->args);
		expr->args = NULL;
	}

	free(expr->op);
	free(expr->raw_text);
	free(expr->name);
	free(expr->member);
	free(expr->value_type);
	free(expr);
}

void task_body_expr_list_destroy(task_body_expr_list_t *list){
	int i;

	if(list == NULL){
		return;
	}

	for(i = 0; i < list->len; i++){
		task_body_expr_destroy(list->items[i]);
	}

	free(list->items);
	free(list);
}

void task_body_stmt_destroy(task_body_stmt_t *stmt){
	int i;

	if(stmt == NULL){
		return;
	}

	task_body_expr_destroy(stmt->expr);

	if(stmt->items != NULL){
		task_body_stmt_list_destroy(stmt->items);
		stmt->items = NULL;
	}

	for(i = 0; i < stmt->decls_len; i++){
		free(stmt->decls[i].name);
		free(stmt->decls[i].value_type);
		task_body_expr_destroy(stmt->decls[i].init_expr);
		stmt->decls[i].init_expr = NULL;
	}

	free(stmt->decls);
	free(stmt);
}

void task_body_stmt_list_destroy(task_body_stmt_list_t *list){
	int i;

	if(list == NULL){
		return;
	}

	for(i = 0; i < list->len; i++){
		task_body_stmt_destroy(list->items[i]);
	}

	free(list->items);
	free(list);
}

static char *emit_original_expr(task_body_expr_t *expr){
	sb_t sb;
	int i;

	if(expr == NULL){
		return sstrdup("");
	}

	sb_init(&sb);

	switch(expr->kind){
		case TBE_IDENTIFIER: {
			sb_append(&sb, expr->name != NULL ? expr->name : "");
			break;
		}

		case TBE_CONSTANT: {
			sb_append(&sb, expr->raw_text != NULL ? expr->raw_text : "");
			break;
		}

		case TBE_STRING_LITERAL: {
			sb_append(&sb, expr->raw_text != NULL ? expr->raw_text : "");
			break;
		}

		case TBE_PAREN: {
			char *inner = emit_original_expr(expr->a);
			sb_append_char(&sb, '(');
			sb_append(&sb, inner);
			sb_append_char(&sb, ')');
			free(inner);
			break;
		}

		case TBE_UNARY: {
			char *inner = emit_original_expr(expr->a);
			sb_append(&sb, expr->op != NULL ? expr->op : "");
			sb_append(&sb, inner);
			free(inner);
			break;
		}

		case TBE_BINARY: {
			char *lhs = emit_original_expr(expr->a);
			char *rhs = emit_original_expr(expr->b);
			sb_append(&sb, lhs);
			sb_append_char(&sb, ' ');
			sb_append(&sb, expr->op != NULL ? expr->op : "");
			sb_append_char(&sb, ' ');
			sb_append(&sb, rhs);
			free(lhs);
			free(rhs);
			break;
		}

		case TBE_ASSIGN: {
			char *lhs = emit_original_expr(expr->a);
			char *rhs = emit_original_expr(expr->b);
			sb_append(&sb, lhs);
			sb_append_char(&sb, ' ');
			sb_append(&sb, expr->op != NULL ? expr->op : "");
			sb_append_char(&sb, ' ');
			sb_append(&sb, rhs);
			free(lhs);
			free(rhs);
			break;
		}

		case TBE_CALL: {
			char *callee = emit_original_expr(expr->a);
			sb_append(&sb, callee);
			sb_append_char(&sb, '(');
			free(callee);

			if(expr->args != NULL){
				for(i = 0; i < expr->args->len; i++){
					char *arg = emit_original_expr(expr->args->items[i]);
					if(i > 0){
						sb_append(&sb, ", ");
					}
					sb_append(&sb, arg);
					free(arg);
				}
			}

			sb_append_char(&sb, ')');
			break;
		}

		case TBE_SUBSCRIPT: {
			char *base = emit_original_expr(expr->a);
			char *index = emit_original_expr(expr->b);
			sb_append(&sb, base);
			sb_append_char(&sb, '[');
			sb_append(&sb, index);
			sb_append_char(&sb, ']');
			free(base);
			free(index);
			break;
		}

		case TBE_MEMBER: {
			char *base = emit_original_expr(expr->a);
			sb_append(&sb, base);
			sb_append_char(&sb, '.');
			sb_append(&sb, expr->member != NULL ? expr->member : "");
			free(base);
			break;
		}

		case TBE_PTR_MEMBER: {
			char *base = emit_original_expr(expr->a);
			sb_append(&sb, base);
			sb_append(&sb, "->");
			sb_append(&sb, expr->member != NULL ? expr->member : "");
			free(base);
			break;
		}

		case TBE_POST_INC: {
			char *inner = emit_original_expr(expr->a);
			sb_append(&sb, inner);
			sb_append(&sb, "++");
			free(inner);
			break;
		}

		case TBE_POST_DEC: {
			char *inner = emit_original_expr(expr->a);
			sb_append(&sb, inner);
			sb_append(&sb, "--");
			free(inner);
			break;
		}

		case TBE_PRE_INC: {
			char *inner = emit_original_expr(expr->a);
			sb_append(&sb, "++");
			sb_append(&sb, inner);
			free(inner);
			break;
		}

		case TBE_PRE_DEC: {	
			char *inner = emit_original_expr(expr->a);
			sb_append(&sb, "--");
			sb_append(&sb, inner);
			free(inner);
			break;
		}

		case TBE_COMMA: {	
			char *lhs = emit_original_expr(expr->a);
			char *rhs = emit_original_expr(expr->b);
			sb_append(&sb, lhs);
			sb_append(&sb, ", ");
			sb_append(&sb, rhs);
			free(lhs);
			free(rhs);
			break;
		}
	}

	return sb_take(&sb);
}

static char *emit_original_stmt(task_body_stmt_t *stmt){
	sb_t sb;
	int i;

	if(stmt == NULL){
		return sstrdup("");
	}

	sb_init(&sb);

	switch(stmt->kind){
		case TBS_EMPTY: {
			sb_append(&sb, ";");
			break;
		}

		case TBS_EXPR: {
			char *expr_text = emit_original_expr(stmt->expr);
			sb_append(&sb, expr_text);
			sb_append(&sb, ";");
			free(expr_text);
			break;
		}

		case TBS_DECL: {
			for(i = 0; i < stmt->decls_len; i++){
				if(i > 0){
					sb_append(&sb, " ");
				}
				sb_append(&sb, stmt->decls[i].value_type != NULL ? stmt->decls[i].value_type : "");
				sb_append_char(&sb, ' ');
				sb_append(&sb, stmt->decls[i].name != NULL ? stmt->decls[i].name : "");
				if(stmt->decls[i].init_expr != NULL){
					char *init_text = emit_original_expr(stmt->decls[i].init_expr);
					sb_append(&sb, " = ");
					sb_append(&sb, init_text);
					free(init_text);
				}
				sb_append(&sb, ";");
			}
			break;
		}

		case TBS_COMPOUND: {
			sb_append(&sb, "{\n");
			if(stmt->items != NULL){
				for(i = 0; i < stmt->items->len; i++){
					char *item_text = emit_original_stmt(stmt->items->items[i]);
					sb_append(&sb, item_text);
					sb_append(&sb, "\n");
					free(item_text);
				}
			}
			sb_append(&sb, "}");
			break;
		}
	}

	return sb_take(&sb);
}

static void local_scope_destroy(local_scope_t *scope){
	int i;

	if(scope == NULL){
		return;
	}

	for(i = 0; i < scope->len; i++){
		free(scope->names[i]);
	}

	free(scope->names);
	scope->names = NULL;
	scope->len = 0;
	scope->cap = 0;
}

static int local_scope_add(local_scope_t *scope, const char *name){
	if(scope == NULL || name == NULL){
		return -1;
	}

	if(scope->len == scope->cap){
		if(ensure_array_capacity((void **)&scope->names, &scope->cap, sizeof(char *)) != 0){
			return -1;
		}
	}

	scope->names[scope->len++] = sstrdup(name);
	return 0;
}

static int task_body_push_scope(task_body_state_t *state){
	local_scope_t *scope;

	if(state == NULL){
		return -1;
	}

	if(state->scopes_len == state->scopes_cap){
		if(ensure_array_capacity((void **)&state->scopes, &state->scopes_cap, sizeof(local_scope_t)) != 0){
			return -1;
		}
	}

	scope = &state->scopes[state->scopes_len++];
	scope->names = NULL;
	scope->len = 0;
	scope->cap = 0;
	return 0;
}

static void task_body_pop_scope(task_body_state_t *state){
	if(state == NULL || state->scopes_len <= 0){
		return;
	}

	state->scopes_len--;
	local_scope_destroy(&state->scopes[state->scopes_len]);
}

static int is_local_name(const task_body_state_t *state, const char *name){
	int i;
	int j;

	if(state == NULL || name == NULL){
		return 0;
	}

	for(i = state->scopes_len - 1; i >= 0; i--){
		for(j = 0; j < state->scopes[i].len; j++){
			if(strcmp(state->scopes[i].names[j], name) == 0){
				return 1;
			}
		}
	}

	return 0;
}

static const task_depend_t *find_depend_by_raw_text(const task_async_block_t *block, const char *raw_text){
	int i;

	if(block == NULL || raw_text == NULL){
		return NULL;
	}

	for(i = 0; i < block->depends.len; i++){
		if(block->depends.items[i].raw_text != NULL && strcmp(block->depends.items[i].raw_text, raw_text) == 0){
			return &block->depends.items[i];
		}
	}

	return NULL;
}

static char *normalize_depend_key(task_body_expr_t *expr){
	sb_t sb;
	char *index_text;
	char *out;

	if(expr == NULL){
		return sstrdup("");
	}

	if(expr->kind == TBE_IDENTIFIER){
		return sstrdup(expr->name != NULL ? expr->name : "");
	}

	if(expr->kind == TBE_SUBSCRIPT && expr->a != NULL && expr->a->kind == TBE_IDENTIFIER){
		index_text = emit_original_expr(expr->b);

		sb_init(&sb);
		sb_append(&sb, expr->a->name != NULL ? expr->a->name : "");
		sb_append_char(&sb, '[');
		sb_append(&sb, index_text);
		sb_append(&sb, ":1]");
		free(index_text);

		out = sb_take(&sb);
		return out;
	}

	return sstrdup("");
}

static int is_input_data_expr(task_body_state_t *state, task_body_expr_t *expr){
	char *dep_key;
	const task_depend_t *depend;
	int result;

	if(expr == NULL){
		return 0;
	}

	switch(expr->kind){
		case TBE_IDENTIFIER:
			if(expr->is_function){
				return 0;
			}
			if(is_local_name(state, expr->name)){
				return 0;
			}
			dep_key = normalize_depend_key(expr);
			depend = find_depend_by_raw_text(state->block, dep_key);
			free(dep_key);
			return depend == NULL;

		case TBE_CONSTANT:
			return 0;

		case TBE_STRING_LITERAL:
			return 0;

		case TBE_PAREN:
			return is_input_data_expr(state, expr->a);

		case TBE_UNARY:
			return is_input_data_expr(state, expr->a);

		case TBE_BINARY:
			return is_input_data_expr(state, expr->a) && is_input_data_expr(state, expr->b);

		case TBE_SUBSCRIPT:
			dep_key = normalize_depend_key(expr);
			depend = find_depend_by_raw_text(state->block, dep_key);
			free(dep_key);
			if(depend != NULL){
				return 0;
			}
			result = is_input_data_expr(state, expr->a) && is_input_data_expr(state, expr->b);
			return result;

		default:
			return 0;
	}
}

static char *add_input_data_if_needed(task_body_state_t *state, task_body_expr_t *expr){
	char *expr_text;
	char field_name[64];
	const char *value_type;
	int i;
	char *result;
	sb_t sb;

	if(state == NULL || expr == NULL){
		return sstrdup("");
	}

	expr_text = emit_original_expr(expr);

	for(i = 0; i < state->input_data_items_len; i++){
		if(strcmp(state->input_data_items[i].expr_text, expr_text) == 0){
			snprintf(field_name, sizeof(field_name), "input_value_%d", state->input_data_items[i].index);
			free(expr_text);

			sb_init(&sb);
			sb_append(&sb, "task_input_data->");
			sb_append(&sb, field_name);
			result = sb_take(&sb);
			return result;
		}
	}

	value_type = (expr->value_type != NULL && expr->value_type[0] != '\0') ? expr->value_type : "int";
	snprintf(field_name, sizeof(field_name), "input_value_%d", state->next_input_index);

	if(task_async_block_add_input_data(state->block, field_name, expr_text, value_type) != 0){
		free(expr_text);
		return sstrdup("");
	}

	if(state->input_data_items_len == state->input_data_items_cap){
		if(ensure_array_capacity((void **)&state->input_data_items, &state->input_data_items_cap, sizeof(task_input_data_item_t)) != 0){
			free(expr_text);
			return sstrdup("");
		}
	}

	state->input_data_items[state->input_data_items_len].expr_text = expr_text;
	state->input_data_items[state->input_data_items_len].index = state->next_input_index;
	state->input_data_items_len++;
	state->next_input_index++;

	sb_init(&sb);
	sb_append(&sb, "task_input_data->");
	sb_append(&sb, field_name);
	result = sb_take(&sb);
	return result;
}

static char *emit_read_expr(task_body_state_t *state, task_body_expr_t *expr);
static char *emit_write_expr(task_body_state_t *state, task_body_expr_t *expr);

static char *emit_read_expr(task_body_state_t *state, task_body_expr_t *expr){
	char *dep_key;
	const task_depend_t *depend;
	char *tmp;
	sb_t sb;
	int i;

	if(expr == NULL){
		return sstrdup("");
	}

	dep_key = normalize_depend_key(expr);
	depend = find_depend_by_raw_text(state->block, dep_key);
	free(dep_key);

	if(depend != NULL){
		sb_init(&sb);
		sb_append(&sb, depend->name != NULL ? depend->name : "");
		if(depend->kind == TD_DEPEND_IN){
			sb_append(&sb, "_input_value");
		} else if(depend->kind == TD_DEPEND_OUT){
			sb_append(&sb, "_output_value");
		} else {
			sb_append(&sb, "_inout_value");
		}
		return sb_take(&sb);
	}

	if(is_input_data_expr(state, expr)){
		return add_input_data_if_needed(state, expr);
	}

	sb_init(&sb);

	switch(expr->kind){
		case TBE_IDENTIFIER: {
			sb_append(&sb, expr->name != NULL ? expr->name : "");
			break;
		}

		case TBE_CONSTANT: {
			sb_append(&sb, expr->raw_text != NULL ? expr->raw_text : "");
			break;
		}
		
		case TBE_STRING_LITERAL: {
			sb_append(&sb, expr->raw_text != NULL ? expr->raw_text : "");
			break;
		}

		case TBE_PAREN: {
			tmp = emit_read_expr(state, expr->a);
			sb_append_char(&sb, '(');
			sb_append(&sb, tmp);
			sb_append_char(&sb, ')');
			free(tmp);
			break;
		}

		case TBE_UNARY: {
			tmp = emit_read_expr(state, expr->a);
			sb_append(&sb, expr->op != NULL ? expr->op : "");
			sb_append(&sb, tmp);
			free(tmp);
			break;
		}

		case TBE_BINARY: {
			char *lhs = emit_read_expr(state, expr->a);
			char *rhs = emit_read_expr(state, expr->b);
			sb_append(&sb, lhs);
			sb_append_char(&sb, ' ');
			sb_append(&sb, expr->op != NULL ? expr->op : "");
			sb_append_char(&sb, ' ');
			sb_append(&sb, rhs);
			free(lhs);
			free(rhs);
			break;
		}

		case TBE_ASSIGN: {
			char *lhs = emit_write_expr(state, expr->a);
			char *rhs = emit_read_expr(state, expr->b);
			sb_append(&sb, lhs);
			sb_append_char(&sb, ' ');
			sb_append(&sb, expr->op != NULL ? expr->op : "");
			sb_append_char(&sb, ' ');
			sb_append(&sb, rhs);
			free(lhs);
			free(rhs);
			break;
		}

		case TBE_CALL: {
			char *callee = emit_original_expr(expr->a);
			sb_append(&sb, callee);
			sb_append_char(&sb, '(');
			free(callee);

			if(expr->args != NULL){
				for(i = 0; i < expr->args->len; i++){
					char *arg = emit_read_expr(state, expr->args->items[i]);
					if(i > 0){
						sb_append(&sb, ", ");
					}
					sb_append(&sb, arg);
					free(arg);
				}
			}

			sb_append_char(&sb, ')');
			break;
		}

		case TBE_SUBSCRIPT: {
			char *base = emit_read_expr(state, expr->a);
			char *index = emit_read_expr(state, expr->b);
			sb_append(&sb, base);
			sb_append_char(&sb, '[');
			sb_append(&sb, index);
			sb_append_char(&sb, ']');
			free(base);
			free(index);
			break;
		}

		case TBE_MEMBER: {
			char *base = emit_read_expr(state, expr->a);
			sb_append(&sb, base);
			sb_append_char(&sb, '.');
			sb_append(&sb, expr->member != NULL ? expr->member : "");
			free(base);
			break;
		}

		case TBE_PTR_MEMBER: {
			char *base = emit_read_expr(state, expr->a);
			sb_append(&sb, base);
			sb_append(&sb, "->");
			sb_append(&sb, expr->member != NULL ? expr->member : "");
			free(base);
			break;
		}

		case TBE_POST_INC: {
			char *inner = emit_read_expr(state, expr->a);
			sb_append(&sb, inner);
			sb_append(&sb, "++");
			free(inner);
			break;
		}

		case TBE_POST_DEC: {
			char *inner = emit_read_expr(state, expr->a);
			sb_append(&sb, inner);
			sb_append(&sb, "--");
			free(inner);
			break;
		}

		case TBE_PRE_INC: {
			char *inner = emit_read_expr(state, expr->a);
			sb_append(&sb, "++");
			sb_append(&sb, inner);
			free(inner);
			break;
		}

		case TBE_PRE_DEC: {
			char *inner = emit_read_expr(state, expr->a);
			sb_append(&sb, "--");
			sb_append(&sb, inner);
			free(inner);
			break;
		}

		case TBE_COMMA: {
			char *lhs = emit_read_expr(state, expr->a);
			char *rhs = emit_read_expr(state, expr->b);
			sb_append(&sb, lhs);
			sb_append(&sb, ", ");
			sb_append(&sb, rhs);
			free(lhs);
			free(rhs);
			break;
		}
	}

	return sb_take(&sb);
}

static char *emit_write_expr(task_body_state_t *state, task_body_expr_t *expr){
	char *dep_key;
	const task_depend_t *depend;
	sb_t sb;
	char *out;

	if(expr == NULL){
		return sstrdup("");
	}

	dep_key = normalize_depend_key(expr);
	depend = find_depend_by_raw_text(state->block, dep_key);
	free(dep_key);

	if(depend != NULL){
		sb_init(&sb);
		sb_append(&sb, depend->name != NULL ? depend->name : "");
		if(depend->kind == TD_DEPEND_OUT){
			sb_append(&sb, "_output_value");
		} else if(depend->kind == TD_DEPEND_INOUT){
			sb_append(&sb, "_inout_value");
		} else {
			sb_append(&sb, "_input_value");
		}
		out = sb_take(&sb);
		return out;
	}

	return emit_original_expr(expr);
}

static const char *runtime_suffix_for_value_type(const char *value_type){
    const char *t = normalize_c_type_name(value_type);

    if(strcmp(t, "int") == 0){
        return "int";
    }
    if(strcmp(t, "long") == 0){
        return "long";
    }
    if(strcmp(t, "double") == 0){
        return "double";
    }
    return "";
}

static void emit_depend_prologue(task_async_block_t *block, sb_t *out){
	int i;
	const char *suffix;

	if(block == NULL || out == NULL){
		return;
	}

	for(i = 0; i < block->depends.len; i++){
		task_depend_t *depend = &block->depends.items[i];

		sb_append(out, "    ");
		sb_append(out, depend->value_type != NULL ? depend->value_type : "int");
		sb_append_char(out, ' ');
		sb_append(out, depend->name != NULL ? depend->name : "");

		if(depend->kind == TD_DEPEND_IN){
			sb_append(out, "_input_value;\n");
		} else if(depend->kind == TD_DEPEND_OUT){
			sb_append(out, "_output_value;\n");
		} else {
			sb_append(out, "_inout_value;\n");
		}
	}

	if(block->depends.len > 0){
		sb_append(out, "\n");
	}

	for(i = 0; i < block->depends.len; i++){
		task_depend_t *depend = &block->depends.items[i];
		suffix = runtime_suffix_for_value_type(depend->value_type);

		if(suffix[0] == '\0'){
			continue;
		}

		if(depend->kind == TD_DEPEND_IN){
			sb_append(out, "    ");
			sb_append(out, depend->name);
			sb_append(out, "_input_value = ompd_runtime_recv_");
			sb_append(out, suffix);
			sb_append(out, "_value(\"");
			sb_append(out, depend->name);
			sb_append(out, "\");\n");
		} else if(depend->kind == TD_DEPEND_INOUT){
			sb_append(out, "    ");
			sb_append(out, depend->name);
			sb_append(out, "_inout_value = ompd_runtime_recv_");
			sb_append(out, suffix);
			sb_append(out, "_value(\"");
			sb_append(out, depend->name);
			sb_append(out, "\");\n");
		}
	}

	if(block->depends.len > 0){
		sb_append(out, "\n");
	}
}

static void emit_depend_epilogue(task_async_block_t *block, sb_t *out){
	int i;
	const char *suffix;

	if(block == NULL || out == NULL){
		return;
	}

	for(i = 0; i < block->depends.len; i++){
		task_depend_t *depend = &block->depends.items[i];
		suffix = runtime_suffix_for_value_type(depend->value_type);

		if(suffix[0] == '\0'){
			continue;
		}

		if(depend->kind == TD_DEPEND_OUT){
			sb_append(out, "    ompd_runtime_send_");
			sb_append(out, suffix);
			sb_append(out, "_value(\"");
			sb_append(out, depend->name);
			sb_append(out, "\", ");
			sb_append(out, depend->name);
			sb_append(out, "_output_value);\n");
		} else if(depend->kind == TD_DEPEND_INOUT){
			sb_append(out, "    ompd_runtime_send_");
			sb_append(out, suffix);
			sb_append(out, "_value(\"");
			sb_append(out, depend->name);
			sb_append(out, "\", ");
			sb_append(out, depend->name);
			sb_append(out, "_inout_value);\n");
		}
	}
}

static void emit_stmt(task_body_state_t *state, task_body_stmt_t *stmt, sb_t *out){
	int i;

	if(state == NULL || stmt == NULL || out == NULL){
		return;
	}

	switch(stmt->kind){
		case TBS_EMPTY:
			sb_append(out, "    ;\n");
			break;

		case TBS_EXPR:
			if(stmt->expr != NULL && stmt->expr->kind == TBE_ASSIGN){
				char *lhs = emit_write_expr(state, stmt->expr->a);
				char *rhs = emit_read_expr(state, stmt->expr->b);

				sb_append(out, "    ");
				sb_append(out, lhs);
				sb_append_char(out, ' ');
				sb_append(out, stmt->expr->op != NULL ? stmt->expr->op : "");
				sb_append_char(out, ' ');
				sb_append(out, rhs);
				sb_append(out, ";\n");

				free(lhs);
				free(rhs);
			} else {
				char *expr_text = emit_read_expr(state, stmt->expr);
				sb_append(out, "    ");
				sb_append(out, expr_text);
				sb_append(out, ";\n");
				free(expr_text);
			}
			break;

		case TBS_DECL:
			for(i = 0; i < stmt->decls_len; i++){
				char *init_text = NULL;

				if(state->scopes_len > 0){
					local_scope_add(&state->scopes[state->scopes_len - 1], stmt->decls[i].name);
				}

				sb_append(out, "    ");
				sb_append(out, stmt->decls[i].value_type != NULL ? stmt->decls[i].value_type : "int");
				sb_append_char(out, ' ');
				sb_append(out, stmt->decls[i].name != NULL ? stmt->decls[i].name : "");

				if(stmt->decls[i].init_expr != NULL){
					init_text = emit_read_expr(state, stmt->decls[i].init_expr);
					sb_append(out, " = ");
					sb_append(out, init_text);
					free(init_text);
				}

				sb_append(out, ";\n");
			}
			break;

		case TBS_COMPOUND:
			sb_append(out, "    {\n");
			task_body_push_scope(state);

			if(stmt->items != NULL){
				for(i = 0; i < stmt->items->len; i++){
					emit_stmt(state, stmt->items->items[i], out);
				}
			}

			task_body_pop_scope(state);
			sb_append(out, "    }\n");
			break;
	}
}

int task_body_prepare_task_async_block(task_async_block_t *task_async_block, task_body_stmt_t *body_root){
	task_body_state_t state;
	sb_t emit_body;
	char *original_body;
	int i;

	if(task_async_block == NULL || body_root == NULL){
		return -1;
	}

	memset(&state, 0, sizeof(state));
	state.block = task_async_block;
	state.next_input_index = 0;

	if(task_body_push_scope(&state) != 0){
		return -1;
	}

	original_body = emit_original_stmt(body_root);
	if(original_body == NULL){
		task_body_pop_scope(&state);
		free(state.scopes);
		return -1;
	}

	if(task_async_block_set_body_text(task_async_block, original_body) != 0){
		free(original_body);
		task_body_pop_scope(&state);
		free(state.scopes);
		return -1;
	}
	free(original_body);

	sb_init(&emit_body);
	emit_depend_prologue(task_async_block, &emit_body);
	emit_stmt(&state, body_root, &emit_body);
	emit_depend_epilogue(task_async_block, &emit_body);

	{
		char *emit_text = sb_take(&emit_body);
		if(emit_text == NULL){
			task_body_pop_scope(&state);
			free(state.scopes);
			for(i = 0; i < state.input_data_items_len; i++){
				free(state.input_data_items[i].expr_text);
			}
			free(state.input_data_items);
			return -1;
		}

		if(task_async_block_set_emit_body_text(task_async_block, emit_text) != 0){
			free(emit_text);
			task_body_pop_scope(&state);
			free(state.scopes);
			for(i = 0; i < state.input_data_items_len; i++){
				free(state.input_data_items[i].expr_text);
			}
			free(state.input_data_items);
			return -1;
		}

		free(emit_text);
	}

	task_async_block->body_prepared = 1;

	task_body_pop_scope(&state);
	free(state.scopes);

	for(i = 0; i < state.input_data_items_len; i++){
		free(state.input_data_items[i].expr_text);
	}
	free(state.input_data_items);

	return 0;
}
