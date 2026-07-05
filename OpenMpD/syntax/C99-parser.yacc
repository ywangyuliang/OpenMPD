%{
#include <bits/stdc++.h>
#include "symbol_table.h"
#include "cluster_stack.h"
#include "mpi_lifecycle.h"
#include "output_slots.h"
#include "tasking_region.h"
#include "task_body_transform.h"

void yyerror(char const *s);
extern int yylex (void);
extern ofstream output;

int error_count = 0;

int main_init = 0;
int main_end = 0;
int mpi_runtime_initialized = 0;
int mpi_initialization_pending_in_main = 0;
int main_scope_depth = 0;
int function_scope_depth = 0;
int function_call_seen = 0;
int declaration_handling_enabled = 0;
int sequential_declaration_pending = 0;
int sequential_region_active = 0;
int pointer_depth = 0;
int use_ompd_runtime_main = 0;

extern ofstream logFile;
extern ofstream errFile;

extern symbol_table table;

extern void flush_pending_clusters_at_eof();

extern int tasking_parse_should_begin_body(void);
extern int tasking_parse_is_in_body(void);
extern void tasking_parse_begin_body(void);
extern void tasking_parse_finish_body(void);
static int task_body_statement_depth = 0;
static const char *TASK_GLOBAL_DEFINITIONS_SLOT = "task_global_definitions";
static int task_global_definitions_slot_reserved = 0;

/* Wraps a task body expression in a parser symbol */
static symbol_info *task_make_expr_symbol(task_body_expr_t *expr, const std::string &value_type){
    symbol_info *symbol = new symbol_info("", "TASK_BODY_EXPR");
    symbol->set_variable_type(value_type);
    symbol->set_task_body_expr(expr);
    return symbol;
}

/* Returns the task body expression stored in a parser symbol */
static task_body_expr_t *task_expr_of(symbol_info *symbol){
    if(symbol == NULL){
        return NULL;
    }
    return symbol->get_task_body_expr();
}

/* Starts task body capture when the parser reaches the task statement */
static void task_body_begin_if_needed(){
    cluster_stack *c = cluster_stack_peek();

    if(tasking_parse_should_begin_body()){
        if(c == NULL || c->tasking_region == NULL){
            fprintf(stderr, "Error: task body without open tasking_region\n");
            exit(EXIT_FAILURE);
        }

        if(tasking_region_begin_task_async_block_body_parse(c->tasking_region) != 0){
            fprintf(stderr, "Error: tasking_region_begin_task_async_block_body_parse\n");
            exit(EXIT_FAILURE);
        }

        tasking_parse_begin_body();
        task_body_statement_depth = 0;
    }

    if(tasking_parse_is_in_body()){
        task_body_statement_depth++;
    }
}

/* Finishes task body capture when the root statement has been parsed */
static void task_body_finish_if_root(task_body_stmt_t *stmt){
    cluster_stack *c = cluster_stack_peek();

    if(!tasking_parse_is_in_body()){
        return;
    }

    task_body_statement_depth--;

    if(task_body_statement_depth == 0){
        if(c == NULL || c->tasking_region == NULL){
            fprintf(stderr, "Error: task body finished without tasking_region\n");
            exit(EXIT_FAILURE);
        }

        if(tasking_region_finish_task_async_block_body_ir(c->tasking_region, stmt) != 0){
            fprintf(stderr, "Error: tasking_region_finish_task_async_block_body_ir\n");
            exit(EXIT_FAILURE);
        }

        tasking_parse_finish_body();
    }
}

/* Converts stored declaration specifiers into C source text */
static std::string ompd_specs_to_c(const std::string &s){
    std::string out = s;
    for(char &c : out){
        if(c == '_') {
			c = ' ';
		} else {
			c = std::tolower((unsigned char)c);
		}
    }
    return out;
}

/* Emits a forward declaration for a function used by generated task code */
static void ompd_emit_forward_decl(symbol_info *func){
    if(func == nullptr) return;
    if(!use_ompd_runtime_main) return;

    std::string ret = ompd_specs_to_c(func->get_variable_type());
    std::string decl;

    if(!ret.empty()){
        decl += ret + " ";
    }

    if(func->is_pointer()){
        decl += "*";
    }

    decl += func->get_symbol_name();
    decl += "();\n";

    output_slot_append_replacement(TASK_GLOBAL_DEFINITIONS_SLOT, decl);
}

/* Reserves the deferred output slot used by task global definitions */
static void ompd_reserve_task_global_definitions_slot(){
    if(task_global_definitions_slot_reserved){
        return;
    }

    output << output_slot_marker(TASK_GLOBAL_DEFINITIONS_SLOT);
    output_slot_set_replacement(TASK_GLOBAL_DEFINITIONS_SLOT, "");
    task_global_definitions_slot_reserved = 1;
}
%}

%union{
	symbol_info * symbol;
	vector <symbol_info*> *symbol_list;
	void *parse_node;
}

%token SIZEOF
%token PTR_OP INC_OP DEC_OP LEFT_OP RIGHT_OP LE_OP GE_OP EQ_OP NE_OP
%token AND_OP OR_OP MUL_ASSIGN DIV_ASSIGN MOD_ASSIGN ADD_ASSIGN
%token SUB_ASSIGN LEFT_ASSIGN RIGHT_ASSIGN AND_ASSIGN
%token XOR_ASSIGN OR_ASSIGN

%token<symbol> TYPEDEF EXTERN STATIC AUTO REGISTER INLINE RESTRICT
%token<symbol> CHAR SHORT INT LONG SIGNED UNSIGNED FLOAT DOUBLE CONST VOLATILE VOID
%token<symbol> BOOL COMPLEX IMAGINARY USER_DEFINED
%token<symbol> STRUCT UNION ENUM ELLIPSIS

%token CASE DEFAULT IF ELSE SWITCH WHILE DO FOR GOTO CONTINUE BREAK RETURN

%token<symbol> CONSTANT IDENTIFIER STRING_LITERAL

%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE

%type<symbol> type_specifier struct_or_union_specifier enum_specifier struct_or_union specifier_qualifier_list
%type<symbol> storage_class_specifier direct_declarator declarator declaration_specifiers type_qualifier initializer_list
%type<symbol> init_declarator initializer parameter_declaration  struct_declarator type_name constant_expression

%type<symbol> primary_expression postfix_expression unary_expression cast_expression
%type<symbol> multiplicative_expression additive_expression shift_expression
%type<symbol> relational_expression equality_expression and_expression expression
%type<symbol> exclusive_or_expression inclusive_or_expression logical_and_expression
%type<symbol> logical_or_expression conditional_expression assignment_expression function_specifier
%type<symbol_list> init_declarator_list parameter_type_list parameter_list struct_declaration_list struct_declarator_list struct_declaration
%type<symbol_list> declaration_list identifier_list declaration

%type<parse_node> statement compound_statement expression_statement
%type<parse_node> block_item block_item_list
%type<parse_node> argument_expression_list task_body_statement_head

%start translation_unit
%%

primary_expression
    : IDENTIFIER
	{
	if(tasking_parse_is_in_body()){
	$1->set_task_body_expr(task_body_expr_make_identifier($1->get_symbol_name().c_str(), $1->get_variable_type().c_str(), $1->is_function() ? 1 : 0));
	}
	$$ = $1;
	}
    | CONSTANT
	{
	if(tasking_parse_is_in_body()){
	$1->set_task_body_expr(task_body_expr_make_constant($1->get_symbol_name().c_str(), $1->get_variable_type().c_str()));
	}
	$$ = $1;
	}
    | STRING_LITERAL
	{
	if(tasking_parse_is_in_body()){
	$1->set_task_body_expr(task_body_expr_make_string_literal($1->get_symbol_name().c_str()));
	}
	$$ = $1;
	}
    | '(' expression ')'
	{
	if(tasking_parse_is_in_body()){
	$$ = task_make_expr_symbol(task_body_expr_make_paren(task_expr_of($2)), $2 != NULL ? $2->get_variable_type().c_str() : "");
	} else {
	$$ = $2;
	}
	}
    ;

postfix_expression
	: primary_expression
	{
	$$ = $1;
	}
	| postfix_expression '[' expression ']'
	{
	if(tasking_parse_is_in_body()){
	$$ = task_make_expr_symbol(
					task_body_expr_make_subscript(task_expr_of($1), task_expr_of($3), $1 != NULL ? $1->get_variable_type().c_str() : ""),
					$1 != NULL ? $1->get_variable_type().c_str() : ""
					);
	} else {
	$$ = $1;
	}
	}
	| postfix_expression '(' ')'
	{
	if(tasking_parse_is_in_body()){$$ = task_make_expr_symbol(
				task_body_expr_make_call(task_expr_of($1), NULL, $1 != NULL ? $1->get_variable_type().c_str() : ""),
				$1 != NULL ? $1->get_variable_type().c_str() : ""
				);
          } else {
              $$ = $1;
          }
      }
	| postfix_expression '(' argument_expression_list ')'
	{
	if(tasking_parse_is_in_body()){
	$$ = task_make_expr_symbol(
					task_body_expr_make_call(task_expr_of($1),(task_body_expr_list_t *)$3, $1 != NULL ? $1->get_variable_type().c_str() : ""),
					$1 != NULL ? $1->get_variable_type().c_str() : ""
					);
	} else {
	$$ = $1;
	}
	}
	| postfix_expression '.' IDENTIFIER
	{
	if(tasking_parse_is_in_body()){
	$$ = task_make_expr_symbol(
					task_body_expr_make_member(task_expr_of($1), $3->get_symbol_name().c_str(), $1 != NULL ? $1->get_variable_type().c_str() : ""),
					$1 != NULL ? $1->get_variable_type().c_str() : ""
					);
	} else {
	$$ = $1;
	}
	}
	| postfix_expression PTR_OP IDENTIFIER
	{
          if(tasking_parse_is_in_body()){
	$$ = task_make_expr_symbol(
                      task_body_expr_make_ptr_member(task_expr_of($1), $3->get_symbol_name().c_str(), $1 != NULL ? $1->get_variable_type().c_str() : ""),
					  $1 != NULL ? $1->get_variable_type().c_str() : "");
	} else {
	$$ = $1;
	}
	}
	| postfix_expression INC_OP
	{
	if(tasking_parse_is_in_body()){
	$$ = task_make_expr_symbol(
                      task_body_expr_make_post_inc(task_expr_of($1), $1 != NULL ? $1->get_variable_type().c_str() : ""),
					  $1 != NULL ? $1->get_variable_type().c_str() : ""
					  );
	} else {
	$$ = $1;
	}
	}
	| postfix_expression DEC_OP
        {
            if(tasking_parse_is_in_body()){
                $$ = task_make_expr_symbol(
                    task_body_expr_make_post_dec(task_expr_of($1), $1 != NULL ? $1->get_variable_type().c_str() : ""),
                    $1 != NULL ? $1->get_variable_type().c_str() : "");
            } else {
                $$ = $1;
            }
        }
	| '(' type_name ')' '{' initializer_list '}' { $$ = $5; }
	| '(' type_name ')' '{' initializer_list ',' '}' { $$ = $5; }
	;

argument_expression_list
	: assignment_expression
      {
          if(tasking_parse_is_in_body()){
              task_body_expr_list_t *list = task_body_expr_list_create();
              task_body_expr_list_append(list, task_expr_of($1));
              $$ = list;
          } else {
              $$ = NULL;
          }
      }
	| argument_expression_list ',' assignment_expression
      {
          if(tasking_parse_is_in_body()){
              task_body_expr_list_append((task_body_expr_list_t *)$1, task_expr_of($3));
              $$ = $1;
          } else {
              $$ = NULL;
          }
      }
	;

unary_expression
	: postfix_expression
		{
			$$ = $1;
		}
	| INC_OP unary_expression
		{
			if(tasking_parse_is_in_body()){
				$$ = task_make_expr_symbol(
					task_body_expr_make_pre_inc(
						task_expr_of($2),
						$2 != NULL ? $2->get_variable_type().c_str() : ""
					),
					$2 != NULL ? $2->get_variable_type().c_str() : ""
				);
			} else {
				$$ = $2;
			}
		}
	| DEC_OP unary_expression
		{
			if(tasking_parse_is_in_body()){
				$$ = task_make_expr_symbol(
					task_body_expr_make_pre_dec(
						task_expr_of($2),
						$2 != NULL ? $2->get_variable_type().c_str() : ""
					),
					$2 != NULL ? $2->get_variable_type().c_str() : ""
				);
			} else {
				$$ = $2;
			}
		}
	| '&' cast_expression
		{
			if(tasking_parse_is_in_body()){
				$$ = task_make_expr_symbol(
					task_body_expr_make_unary("&", task_expr_of($2)),
					$2 != NULL ? $2->get_variable_type().c_str() : ""
				);
			} else {
				$$ = $2;
			}
		}
	| '*' cast_expression
		{
			if(tasking_parse_is_in_body()){
				$$ = task_make_expr_symbol(
					task_body_expr_make_unary("*", task_expr_of($2)),
					$2 != NULL ? $2->get_variable_type().c_str() : ""
				);
			} else {
				$$ = $2;
			}
		}
	| '+' cast_expression
		{
			if(tasking_parse_is_in_body()){
				$$ = task_make_expr_symbol(
					task_body_expr_make_unary("+", task_expr_of($2)),
					$2 != NULL ? $2->get_variable_type().c_str() : ""
				);
			} else {
				$$ = $2;
			}
		}
	| '-' cast_expression
		{
			if(tasking_parse_is_in_body()){
				$$ = task_make_expr_symbol(
					task_body_expr_make_unary("-", task_expr_of($2)),
					$2 != NULL ? $2->get_variable_type().c_str() : ""
				);
			} else {
				$$ = $2;
			}
		}
	| '~' cast_expression
		{
			if(tasking_parse_is_in_body()){
				$$ = task_make_expr_symbol(
					task_body_expr_make_unary("~", task_expr_of($2)),
					$2 != NULL ? $2->get_variable_type().c_str() : ""
				);
			} else {
				$$ = $2;
			}
		}
	| '!' cast_expression
		{
			if(tasking_parse_is_in_body()){
				$$ = task_make_expr_symbol(
					task_body_expr_make_unary("!", task_expr_of($2)),
					$2 != NULL ? $2->get_variable_type().c_str() : ""
				);
			} else {
				$$ = $2;
			}
		}
	| SIZEOF unary_expression
		{
			$$ = $2;
		}
	| SIZEOF '(' type_name ')'
		{
			$$ = $3;
		}
	;

cast_expression
	: unary_expression {$$ = $1;}
	| '(' type_name ')' cast_expression {$$ = $4;}
	;

multiplicative_expression
	: cast_expression
		{
			$$ = $1;
		}
	| multiplicative_expression '*' cast_expression
		{
			if(tasking_parse_is_in_body()){
				$$ = task_make_expr_symbol(
					task_body_expr_make_binary("*", task_expr_of($1), task_expr_of($3), $1 != NULL ? $1->get_variable_type().c_str() : ""),
					$1 != NULL ? $1->get_variable_type().c_str() : ""
				);
			} else {
				$$ = $1;
			}
		}
	| multiplicative_expression '/' cast_expression
		{
			if(tasking_parse_is_in_body()){
				$$ = task_make_expr_symbol(
					task_body_expr_make_binary("/", task_expr_of($1), task_expr_of($3), $1 != NULL ? $1->get_variable_type().c_str() : ""),
					$1 != NULL ? $1->get_variable_type().c_str() : ""
				);
			} else {
				$$ = $1;
			}
		}
	| multiplicative_expression '%' cast_expression
		{
			if(tasking_parse_is_in_body()){
				$$ = task_make_expr_symbol(
					task_body_expr_make_binary("%", task_expr_of($1), task_expr_of($3), $1 != NULL ? $1->get_variable_type().c_str() : ""),
					$1 != NULL ? $1->get_variable_type().c_str() : ""
				);
			} else {
				$$ = $1;
			}
		}
	;

additive_expression
	: multiplicative_expression
		{
			$$ = $1;
		}
	| additive_expression '+' multiplicative_expression
		{
			if(tasking_parse_is_in_body()){
				$$ = task_make_expr_symbol(
					task_body_expr_make_binary("+", task_expr_of($1), task_expr_of($3), $1 != NULL ? $1->get_variable_type().c_str() : ""),
					$1 != NULL ? $1->get_variable_type().c_str() : ""
				);
			} else {
				$$ = $1;
			}
		}
	| additive_expression '-' multiplicative_expression
		{
			if(tasking_parse_is_in_body()){
				$$ = task_make_expr_symbol(
					task_body_expr_make_binary("-", task_expr_of($1), task_expr_of($3), $1 != NULL ? $1->get_variable_type().c_str() : ""),
					$1 != NULL ? $1->get_variable_type().c_str() : ""
				);
			} else {
				$$ = $1;
			}
		}
	;

shift_expression
	: additive_expression {$$ = $1;}
	| shift_expression LEFT_OP additive_expression
	| shift_expression RIGHT_OP additive_expression
	;

relational_expression
	: shift_expression {$$ = $1;}
	| relational_expression '<' shift_expression
	| relational_expression '>' shift_expression
	| relational_expression LE_OP shift_expression
	| relational_expression GE_OP shift_expression
	;

equality_expression
	: relational_expression {$$ = $1;}
	| equality_expression EQ_OP relational_expression
	| equality_expression NE_OP relational_expression
	;

and_expression
	: equality_expression {$$ = $1;}
	| and_expression '&' equality_expression
	;

exclusive_or_expression
	: and_expression {$$ = $1;}
	| exclusive_or_expression '^' and_expression
	;

inclusive_or_expression
	: exclusive_or_expression {$$ = $1;}
	| inclusive_or_expression '|' exclusive_or_expression
	;

logical_and_expression
	: inclusive_or_expression {$$ = $1;}
	| logical_and_expression AND_OP inclusive_or_expression
	;

logical_or_expression
	: logical_and_expression {$$ = $1;}
	| logical_or_expression OR_OP logical_and_expression
	;

conditional_expression
	: logical_or_expression {$$ = $1;}
	| logical_or_expression '?' expression ':' conditional_expression
	;

assignment_expression
	: conditional_expression
		{
			$$ = $1;
		}
	| unary_expression '=' assignment_expression
		{
			if(tasking_parse_is_in_body()){
				$$ = task_make_expr_symbol(
					task_body_expr_make_assignment(
						"=",
						task_expr_of($1),
						task_expr_of($3),
						$1 != NULL ? $1->get_variable_type().c_str() : ""
					),
					$1 != NULL ? $1->get_variable_type().c_str() : ""
				);
			} else {
				$$ = $1;
			}
		}
	| unary_expression MUL_ASSIGN assignment_expression
		{
			if(tasking_parse_is_in_body()){
				$$ = task_make_expr_symbol(
					task_body_expr_make_assignment(
						"*=",
						task_expr_of($1),
						task_expr_of($3),
						$1 != NULL ? $1->get_variable_type().c_str() : ""
					),
					$1 != NULL ? $1->get_variable_type().c_str() : ""
				);
			} else {
				$$ = $1;
			}
		}
	| unary_expression DIV_ASSIGN assignment_expression
		{
			if(tasking_parse_is_in_body()){
				$$ = task_make_expr_symbol(
					task_body_expr_make_assignment(
						"/=",
						task_expr_of($1),
						task_expr_of($3),
						$1 != NULL ? $1->get_variable_type().c_str() : ""
					),
					$1 != NULL ? $1->get_variable_type().c_str() : ""
				);
			} else {
				$$ = $1;
			}
		}
	| unary_expression MOD_ASSIGN assignment_expression
		{
			if(tasking_parse_is_in_body()){
				$$ = task_make_expr_symbol(
					task_body_expr_make_assignment(
						"%=",
						task_expr_of($1),
						task_expr_of($3),
						$1 != NULL ? $1->get_variable_type().c_str() : ""
					),
					$1 != NULL ? $1->get_variable_type().c_str() : ""
				);
			} else {
				$$ = $1;
			}
		}
	| unary_expression ADD_ASSIGN assignment_expression
		{
			if(tasking_parse_is_in_body()){
				$$ = task_make_expr_symbol(
					task_body_expr_make_assignment(
						"+=",
						task_expr_of($1),
						task_expr_of($3),
						$1 != NULL ? $1->get_variable_type().c_str() : ""
					),
					$1 != NULL ? $1->get_variable_type().c_str() : ""
				);
			} else {
				$$ = $1;
			}
		}
	| unary_expression SUB_ASSIGN assignment_expression
		{
			if(tasking_parse_is_in_body()){
				$$ = task_make_expr_symbol(
					task_body_expr_make_assignment(
						"-=",
						task_expr_of($1),
						task_expr_of($3),
						$1 != NULL ? $1->get_variable_type().c_str() : ""
					),
					$1 != NULL ? $1->get_variable_type().c_str() : ""
				);
			} else {
				$$ = $1;
			}
		}
	| unary_expression LEFT_ASSIGN assignment_expression
		{
			if(tasking_parse_is_in_body()){
				$$ = task_make_expr_symbol(
					task_body_expr_make_assignment(
						"<<=",
						task_expr_of($1),
						task_expr_of($3),
						$1 != NULL ? $1->get_variable_type().c_str() : ""
					),
					$1 != NULL ? $1->get_variable_type().c_str() : ""
				);
			} else {
				$$ = $1;
			}
		}
	| unary_expression RIGHT_ASSIGN assignment_expression
		{
			if(tasking_parse_is_in_body()){
				$$ = task_make_expr_symbol(
					task_body_expr_make_assignment(
						">>=",
						task_expr_of($1),
						task_expr_of($3),
						$1 != NULL ? $1->get_variable_type().c_str() : ""
					),
					$1 != NULL ? $1->get_variable_type().c_str() : ""
				);
			} else {
				$$ = $1;
			}
		}
	| unary_expression AND_ASSIGN assignment_expression
		{
			if(tasking_parse_is_in_body()){
				$$ = task_make_expr_symbol(
					task_body_expr_make_assignment(
						"&=",
						task_expr_of($1),
						task_expr_of($3),
						$1 != NULL ? $1->get_variable_type().c_str() : ""
					),
					$1 != NULL ? $1->get_variable_type().c_str() : ""
				);
			} else {
				$$ = $1;
			}
		}
	| unary_expression XOR_ASSIGN assignment_expression
		{
			if(tasking_parse_is_in_body()){
				$$ = task_make_expr_symbol(
					task_body_expr_make_assignment(
						"^=",
						task_expr_of($1),
						task_expr_of($3),
						$1 != NULL ? $1->get_variable_type().c_str() : ""
					),
					$1 != NULL ? $1->get_variable_type().c_str() : ""
				);
			} else {
				$$ = $1;
			}
		}
	| unary_expression OR_ASSIGN assignment_expression
		{
			if(tasking_parse_is_in_body()){
				$$ = task_make_expr_symbol(
					task_body_expr_make_assignment(
						"|=",
						task_expr_of($1),
						task_expr_of($3),
						$1 != NULL ? $1->get_variable_type().c_str() : ""
					),
					$1 != NULL ? $1->get_variable_type().c_str() : ""
				);
			} else {
				$$ = $1;
			}
		}
	;

expression
	: assignment_expression
		{
			$$ = $1;
		}
	| expression ',' assignment_expression
		{
			if(tasking_parse_is_in_body()){
				$$ = task_make_expr_symbol(
					task_body_expr_make_comma(task_expr_of($1), task_expr_of($3), $3 != NULL ? $3->get_variable_type().c_str() : ""),
					$3 != NULL ? $3->get_variable_type().c_str() : ""
				);
			} else {
				$$ = $1;
			}
		}
	;

constant_expression
	: conditional_expression
	;

declaration
	: declaration_specifiers ';'
	{
		if($1->is_struct()){
			$$ = new vector<symbol_info*>();
			symbol_info* symbol = new symbol_info(*$1);
			$$->push_back(symbol);
			table.insert($1);
		}
	}

	| declaration_specifiers init_declarator_list ';' {
		$$ = new vector<symbol_info*>();
		bool hasTypedef = (strstr($1->get_symbol_type().c_str(), "TYPEDEF") != NULL);
		if($1->is_struct()){
			if($1->get_parameter_list() != nullptr){
				for(std::vector<symbol_info*>::size_type i = 0; i < $2->size(); i++){
					logFile << "Debug: " << $1->get_symbol_type() << " Debug: " << $2->at(i)->get_symbol_name() << " Debug: " << $2->at(i)->get_variable_type() << endl;
					if(hasTypedef) $2->at(i)->set_is_type_symbol(true);
					$2->at(i)->set_variable_type($1->get_symbol_type());
					$2->at(i)->set_is_struct(true);
					$2->at(i)->set_parameter_list($1->get_parameter_list());
					symbol_info* symbol = new symbol_info(*$2->at(i));
					$$->push_back(symbol);
					table.insert($2->at(i));
				}
			}
			else {
				$1->set_parameter_list($2);
				$$->push_back($1);
				for(std::vector<symbol_info*>::size_type i = 0; i < $2->size(); i++){
					$2->at(i)->set_variable_type($1->get_symbol_name());
					table.insert($2->at(i));
				}
			}
		}
		else{
			for(std::vector<symbol_info*>::size_type i = 0; i < $2->size(); i++){
				logFile << "Debug: " << $1->get_symbol_type() << " Debug: " << $2->at(i)->get_symbol_name() << " Debug: " << $2->at(i)->get_variable_type() << endl;
				$2->at(i)->set_variable_type($1->get_symbol_type());
				if(hasTypedef) $2->at(i)->set_is_type_symbol(true);
				if(!$2->at(i)->is_function()){
					symbol_info* symbol = new symbol_info(*$2->at(i));
					$$->push_back(symbol);
					table.insert($2->at(i));
				}
			}
		}
	}
	;

declaration_specifiers
	: storage_class_specifier { $$ = $1; }
	| storage_class_specifier declaration_specifiers
		{
			std::ostringstream oss;
			oss << $1->get_symbol_type() << "_" << $2->get_symbol_type();
			$2->set_symbol_type(oss.str());
			$$ = $2;
		}
	| type_specifier { $$ = $1; }
	| type_specifier declaration_specifiers
		{
			std::ostringstream oss;
			oss << $1->get_symbol_type() << "_" << $2->get_symbol_type();
			$2->set_symbol_type(oss.str());
			$$ = $2;
		}
	| type_qualifier { $$ = $1; }
	| type_qualifier declaration_specifiers
		{
			std::ostringstream oss;
			oss << $1->get_symbol_type() << "_" << $2->get_symbol_type();
			$2->set_symbol_type(oss.str());
			$$ = $2;
		}
	| function_specifier { $$ = $1; }
	| function_specifier declaration_specifiers
		{
			std::ostringstream oss;
			oss << $1->get_symbol_type() << "_" << $2->get_symbol_type();
			$2->set_symbol_type(oss.str());
			$$ = $2;
		}
	;

init_declarator_list
	: init_declarator	{ $$ = new vector<symbol_info*>(); $$->push_back($1); }
	| init_declarator_list ',' init_declarator	{ $1->push_back($3); $$ = $1; }
	;

init_declarator
	: declarator
		{
			$1->set_task_init_expr(NULL);
			$$ = $1;
		}
	| declarator '=' initializer
		{
			$1->set_is_defined(true);
			$1->set_task_init_expr($3 != NULL ? $3->get_task_body_expr() : NULL);
			$$ = $1;
		}
	;

storage_class_specifier
	: { mpi_global_declarations_reserve_slot(); } TYPEDEF { $$ = new symbol_info("typedef", "TYPEDEF"); }
	| { mpi_global_declarations_reserve_slot(); } EXTERN { $$ = new symbol_info("extern", "EXTERN"); }
	| { mpi_global_declarations_reserve_slot(); } STATIC		{ $$ = new symbol_info("static", "STATIC"); }
	| { mpi_global_declarations_reserve_slot(); } AUTO { $$ = new symbol_info("auto", "AUTO"); }
	| { mpi_global_declarations_reserve_slot(); } REGISTER { $$ = new symbol_info("register", "REGISTER"); }
	;

type_specifier
    : { mpi_global_declarations_reserve_slot(); } VOID          { $$ = new symbol_info("void", "VOID"); }
    | { mpi_global_declarations_reserve_slot(); } CHAR          { $$ = new symbol_info("char", "CHAR"); }
    | { mpi_global_declarations_reserve_slot(); } SHORT         { $$ = new symbol_info("short", "SHORT"); }
    | { mpi_global_declarations_reserve_slot(); } INT           { $$ = new symbol_info("int", "INT"); }
    | { mpi_global_declarations_reserve_slot(); } LONG          { $$ = new symbol_info("long", "LONG"); }
    | { mpi_global_declarations_reserve_slot(); } FLOAT         { $$ = new symbol_info("float", "FLOAT"); }
    | { mpi_global_declarations_reserve_slot(); } DOUBLE        { $$ = new symbol_info("double", "DOUBLE"); }
    | { mpi_global_declarations_reserve_slot(); } SIGNED        { $$ = new symbol_info("signed", "SIGNED"); }
    | { mpi_global_declarations_reserve_slot(); } UNSIGNED      { $$ = new symbol_info("unsigned", "UNSIGNED"); }
    | { mpi_global_declarations_reserve_slot(); } BOOL          { $$ = new symbol_info("bool", "BOOL"); }
    | { mpi_global_declarations_reserve_slot(); } COMPLEX       { $$ = new symbol_info("complex", "COMPLEX"); }
    | { mpi_global_declarations_reserve_slot(); } IMAGINARY     { $$ = new symbol_info("imaginary", "IMAGINARY"); }
	| USER_DEFINED  { $$ = $1;}
    | struct_or_union_specifier  { $$ = $1;}
    | enum_specifier             { $$ = $1;}
    ;

struct_or_union_specifier
	: struct_or_union IDENTIFIER '{' struct_declaration_list '}'
	{
		std::string tag = $1->get_symbol_type() + "_" + $2->get_symbol_name();
		if(table.get_symbol_info(tag) == NULL){
			$2->set_is_struct(true);
			$2->set_variable_type(tag);
			$2->set_symbol_name(tag);
			if (table.insert($2)) {
				logFile << "Inserted: " << $2->get_symbol_name() << " in scope " << table.current_scope_id() << endl;
			}else {
				logFile << "Error: " << $2->get_symbol_name() << " already exists in scope " << endl;
				errFile << "Error: " << $2->get_symbol_name() << " already exists in scope " << endl;
				error_count++;
			}
		}

		$2->set_parameter_list($4);
		for(std::vector<symbol_info*>::size_type i = 0; i < $4->size(); i++){
			logFile << "Struct item 2: " << $4->at(i)->get_symbol_name() << endl;
		}

		$$ = $2;
	}
	| struct_or_union '{' struct_declaration_list '}'
	| struct_or_union IDENTIFIER
	{
		std::string tag = $1->get_symbol_type() + "_" + $2->get_symbol_name();
		if(table.get_symbol_info(tag) == NULL){
			$2->set_is_struct(true);
			$2->set_variable_type(tag);
			$2->set_symbol_name(tag);

			if (table.insert($2)) {
				logFile << "Inserted: " << $2->get_symbol_name() << " in scope " << table.current_scope_id() << endl;
			}else {
				logFile << "Error: " << $2->get_symbol_name() << " already exists in scope " << endl;
				errFile << "Error: " << $2->get_symbol_name() << " already exists in scope " << endl;
				error_count++;
			}
		}

		$$ = $2;
	}
	;

struct_or_union
	: STRUCT		{ $$ = new symbol_info("struct", "STRUCT"); }
	| UNION			{ $$ = new symbol_info("union", "UNION"); }
	;

struct_declaration_list
	: struct_declaration { $$ = $1;
	for(std::vector<symbol_info*>::size_type i = 0; i < $1->size(); i++){
			logFile << "Struct item: " << $1->at(i)->get_symbol_name() << endl;
		}
	}

	| struct_declaration_list struct_declaration
	{
		for(std::vector<symbol_info*>::size_type i = 0; i < $2->size(); i++){
			$1->push_back($2->at(i));
			logFile << "Struct item: " << $2->at(i)->get_symbol_name() << endl;
		}
		$$ = $1;
	}
	;

struct_declaration
	: specifier_qualifier_list struct_declarator_list ';'
	{
		for(std::vector<symbol_info*>::size_type i = 0; i < $2->size(); i++){
			$2->at(i)->set_variable_type($1->get_symbol_type());
		}
		$$ = $2;
	}
	;

specifier_qualifier_list
	: type_qualifier
	| type_qualifier specifier_qualifier_list
	| type_specifier
	| type_specifier specifier_qualifier_list
	;

struct_declarator_list
	: struct_declarator { $$ = new vector<symbol_info*>(); $$->push_back($1); }
	| struct_declarator_list ',' struct_declarator { $1->push_back($3); $$ = $1; }
	;

struct_declarator
	: declarator { $$ = $1; }
	| ':' constant_expression { $$ = $2; }
	| declarator ':' constant_expression { $$ = $1; }
	;

enum_specifier
	: ENUM '{' enumerator_list '}'
	| ENUM IDENTIFIER '{' enumerator_list '}'
	| ENUM '{' enumerator_list ',' '}'
	| ENUM IDENTIFIER '{' enumerator_list ',' '}'
	| ENUM IDENTIFIER
	;

enumerator_list
	: enumerator
	| enumerator_list ',' enumerator
	;

enumerator
	: IDENTIFIER
	| IDENTIFIER '=' constant_expression
	;

type_qualifier
	: CONST
	| RESTRICT
	| VOLATILE
	;

function_specifier
	: INLINE
	;

declarator
	: pointer direct_declarator { $2->set_is_pointer(true);
		for(int addSize = 0; addSize < pointer_depth; addSize++) {
			$2->add_array_dimension("0");
		}
		$$ = $2;
		pointer_depth = 0;}
	| direct_declarator	{ $$ = $1;}
	;


direct_declarator
	: IDENTIFIER		{ $$ = $1; }
	| '(' declarator ')' { $$ = $2; }
	| direct_declarator '[' type_qualifier_list assignment_expression ']'{
		if(!$1->is_array()){
			$1->set_is_array(true);
		}
		$1->add_array_dimension(($4->get_symbol_name()));
		$$ = $1;
	}
	| direct_declarator '[' type_qualifier_list ']'{
		$1->set_is_array(true);
		$$ = $1;
	}
	| direct_declarator '[' assignment_expression ']' {
		if(!$1->is_array()){
			$1->set_is_array(true);
		}
		$1->add_array_dimension(($3->get_symbol_name()));
		$$ = $1;
	}
	| direct_declarator '[' STATIC type_qualifier_list assignment_expression ']'{
		if(!$1->is_array()){
			$1->set_is_array(true);
		}
		$1->add_array_dimension(($5->get_symbol_name()));
		$$ = $1;
	}
	| direct_declarator '[' type_qualifier_list STATIC assignment_expression ']'{
		if(!$1->is_array()){
			$1->set_is_array(true);
		}
		$1->add_array_dimension(($5->get_symbol_name()));
		$$ = $1;
	}
	| direct_declarator '[' type_qualifier_list '*' ']'{
		$1->set_is_array(true);
		$$ = $1;
	}
	| direct_declarator '[' '*' ']'{
		$1->set_is_array(true);
		$$ = $1;
	}
	| direct_declarator '[' ']' {
		$1->set_is_array(true);
		$$ = $1;
	}
	| direct_declarator '(' parameter_type_list ')' {
		$1->set_parameter_list($3);
		$$ = $1;
	}
	| direct_declarator '(' identifier_list ')' { $$ = $1;}
	| direct_declarator '(' ')' { $$ = $1; }
	;

pointer
	: '*' {pointer_depth++;}
	| '*' type_qualifier_list
	| '*' {pointer_depth++;} pointer
	| '*' type_qualifier_list pointer
	;

type_qualifier_list
	: type_qualifier
	| type_qualifier_list type_qualifier
	;


parameter_type_list
	: parameter_list { $$ = $1; }
	| parameter_list ',' ELLIPSIS
	;

parameter_list
	: parameter_declaration { $$ = new vector<symbol_info*>(); $$->push_back($1); }
	| parameter_list ',' parameter_declaration { $1->push_back($3); $$ = $1; }
	;

parameter_declaration
	: declaration_specifiers declarator {
		$2->set_variable_type($1->get_symbol_type());
		$$ = $2;
	}
	| declaration_specifiers abstract_declarator
	| declaration_specifiers
	;

identifier_list
	: IDENTIFIER { $$ = new vector<symbol_info*>(); $$->push_back($1); }
	| identifier_list ',' IDENTIFIER { $1->push_back($3); $$ = $1; }
	;

type_name
	: specifier_qualifier_list
	| specifier_qualifier_list abstract_declarator
	;

abstract_declarator
	: pointer {pointer_depth = 0;}
	| direct_abstract_declarator
	| pointer direct_abstract_declarator {pointer_depth = 0;}
	;

direct_abstract_declarator
	: '(' abstract_declarator ')'
	| '[' ']'
	| '[' assignment_expression ']'
	| direct_abstract_declarator '[' ']'
	| direct_abstract_declarator '[' assignment_expression ']'
	| '[' '*' ']'
	| direct_abstract_declarator '[' '*' ']'
	| '(' ')'
	| '(' parameter_type_list ')'
	| direct_abstract_declarator '(' ')'
	| direct_abstract_declarator '(' parameter_type_list ')'
	;

initializer
	: assignment_expression
	| '{' initializer_list '}' {$$ = $2;}
	| '{' initializer_list ',' '}' {$$ = $2;}
	;

initializer_list
	: initializer
	| designation initializer {$$ = $2;}
	| initializer_list ',' initializer
	| initializer_list ',' designation initializer
	;

designation
	: designator_list '='
	;

designator_list
	: designator
	| designator_list designator
	;

designator
	: '[' constant_expression ']'
	| '.' IDENTIFIER
	;

task_body_statement_head
    : {
        task_body_begin_if_needed();
        $$ = NULL;
      }
    ;

statement
	: task_body_statement_head labeled_statement
		{
			if(tasking_parse_is_in_body()){
				fprintf(stderr, "Error: labeled_statement not supported yet in task body\n");
				exit(EXIT_FAILURE);
			}
			$$ = NULL;
			task_body_finish_if_root((task_body_stmt_t *)$$);
		}
	| task_body_statement_head compound_statement
		{
			$$ = $2;
			task_body_finish_if_root((task_body_stmt_t *)$$);
		}
	| task_body_statement_head expression_statement
		{
			$$ = $2;
			task_body_finish_if_root((task_body_stmt_t *)$$);
		}
	| task_body_statement_head { table.enter_scope(); } selection_statement { table.exit_scope(); }
		{
			if(tasking_parse_is_in_body()){
				fprintf(stderr, "Error: selection_statement not supported yet in task body\n");
				exit(EXIT_FAILURE);
			}
			$$ = NULL;
			task_body_finish_if_root((task_body_stmt_t *)$$);
		}
	| task_body_statement_head { table.enter_scope(); } iteration_statement { table.exit_scope(); }
		{
			if(tasking_parse_is_in_body()){
				fprintf(stderr, "Error: iteration_statement not supported yet in task body\n");
				exit(EXIT_FAILURE);
			}
			$$ = NULL;
			task_body_finish_if_root((task_body_stmt_t *)$$);
		}
	| task_body_statement_head jump_statement
		{
			if(tasking_parse_is_in_body()){
				fprintf(stderr, "Error: jump_statement not supported yet in task body\n");
				exit(EXIT_FAILURE);
			}
			$$ = NULL;
			task_body_finish_if_root((task_body_stmt_t *)$$);
		}
	;

labeled_statement
	: IDENTIFIER ':' statement
	| CASE constant_expression ':' statement
	| DEFAULT ':' statement
	;

compound_statement
	: '{' '}'
	{
	if(tasking_parse_is_in_body()){
	$$ = task_body_stmt_make_compound(task_body_stmt_list_create());
	} else {
	$$ = NULL;
	}
	}
	| '{' {
			if (main_scope_depth > 0) {
				main_scope_depth++;
			}
			else {
				function_scope_depth++;
			}
		}
	block_item_list
	'}' {
			if (main_scope_depth == 2 && mpi_runtime_initialized == 1 && main_end == 1) {
				flush_pending_clusters_at_eof();
				mpi_runtime_emit_finalize();
				main_end = 0;
			}
			else if (main_scope_depth > 0) {
				main_scope_depth--;
			}

			if (!main_scope_depth){/*
				if (function_scope_depth == 2 && sequential_region_active) {
					output << "}" << endl;
					function_scope_depth = 0;
				}
				else*/ if (function_scope_depth > 0) {
					function_scope_depth--;
				}
			}

            if(tasking_parse_is_in_body()){
                $$ = task_body_stmt_make_compound((task_body_stmt_list_t *)$3);
            } else {
                $$ = NULL;
            }
		}
	;

block_item_list
	: block_item
	{
	if(tasking_parse_is_in_body()){
	task_body_stmt_list_t *list = task_body_stmt_list_create();
	if($1 != NULL){
	task_body_stmt_list_append(list, (task_body_stmt_t *)$1);
	}
	$$ = list;
	} else {
	$$ = NULL;
	}
	}
	| block_item_list block_item
	{
	if(tasking_parse_is_in_body()){
	if($2 != NULL){
	task_body_stmt_list_append((task_body_stmt_list_t *)$1, (task_body_stmt_t *)$2);
	}
	$$ = $1;
	} else {
	$$ = NULL;
	}
	}
	;

block_item
	: {if((/*function_scope_depth == 2 ||*/ main_scope_depth == 2) && declaration_handling_enabled && sequential_region_active){output << "}" << endl; sequential_region_active = 0;}} declaration {if ((function_scope_depth == 2 || main_scope_depth == 2) && declaration_handling_enabled) {sequential_declaration_pending = 1;}}
		{
			if(tasking_parse_is_in_body()){
				task_body_stmt_t *decl_stmt;
				size_t i;

				decl_stmt = task_body_stmt_make_declaration();
				if(decl_stmt == NULL){
					fprintf(stderr, "Error: task_body_stmt_make_declaration\n");
					exit(EXIT_FAILURE);
				}

				for(i = 0; i < $2->size(); i++){
					symbol_info *symbol = $2->at(i);

					if(symbol == NULL){
						continue;
					}

					if(task_body_stmt_add_decl(decl_stmt, symbol->get_symbol_name().c_str(), symbol->get_variable_type().c_str(), symbol->get_task_init_expr()) != 0){
						fprintf(stderr, "Error: task_body_stmt_add_decl\n");
						exit(EXIT_FAILURE);
					}
				}

				$$ = decl_stmt;
			} else {
				$$ = NULL;
			}
		}
	|
	{
		mpi_runtime_prepare_statement_emission(1);
	}
	statement
		{
			$$ = $2;
		}
	;

expression_statement
	: ';'
	{
	if(tasking_parse_is_in_body()){
		$$ = task_body_stmt_make_empty();
	} else {
	$$ = NULL;
	}
	}
	| expression ';'
	{
	if(tasking_parse_is_in_body()){
	$$ = task_body_stmt_make_expr(task_expr_of($1));
	} else {
	$$ = NULL;
	}
	}
	;

selection_statement
	: IF '(' expression ')' statement %prec LOWER_THAN_ELSE
	| IF '(' expression ')' statement ELSE { table.exit_scope(); table.enter_scope(); } statement
	| SWITCH '(' expression ')' statement
	;

iteration_statement
	: WHILE '(' expression ')' statement
	| DO statement WHILE '(' expression ')' ';'
	| FOR '(' expression_statement expression_statement ')' statement
	| FOR '(' expression_statement expression_statement expression ')' statement
	| FOR '(' declaration expression_statement ')' statement
	| FOR '(' declaration expression_statement expression ')' statement
	;

jump_statement
	: GOTO IDENTIFIER ';'
	| CONTINUE ';'
	| BREAK ';'
	|
	RETURN
	{
		if(main_end == 1 && mpi_runtime_initialized == 1 && main_scope_depth == 2){
			flush_pending_clusters_at_eof();
			mpi_runtime_emit_finalize();
			main_end = 0;
		}
	}  ';'
	|
	RETURN
	{
		if(main_end == 1 && mpi_runtime_initialized == 1 && main_scope_depth == 2){
			flush_pending_clusters_at_eof();
			mpi_runtime_emit_finalize();
			main_end = 0;
		}
	}
	expression ';'

translation_unit
    : {
        ompd_reserve_task_global_definitions_slot();
      } external_declaration
    | translation_unit external_declaration
    ;

external_declaration
	: function_definition
	| declaration
	;

function_definition
	: declaration_specifiers declarator {
		$2->set_is_function(true);
		$2->set_variable_type($1->get_symbol_type());
		ompd_emit_forward_decl($2);
		if (table.insert($2)) {
			logFile << "Inserted Function: " << $2->get_symbol_name() << " in scope " << table.current_scope_id() << endl;
			if($2->get_symbol_name() == "main"){
				main_init = 1;
			}
		}
		else {
			logFile << "Error: " << $2->get_symbol_name() << " already exists in scope " << endl;
			errFile << "Error: " << $2->get_symbol_name() << " already exists in scope " << endl;
			error_count++;
		}
		table.enter_scope();
	} declaration_list {
		table.get_symbol_info($2->get_symbol_name())->set_parameter_list($4);
		if($2->get_symbol_name() == "main"){
			main_scope_depth = 1;
			function_scope_depth = 0;
		}
		else{
			function_scope_depth = 1;
			main_scope_depth = 0;
		}

		declaration_handling_enabled = 0;
		sequential_region_active = 0;
	} compound_statement {
		$2->set_is_defined(true);
		table.exit_scope();
		if($2->get_symbol_name() == "main"){
			main_scope_depth = 0;
		}

		function_scope_depth = 0;
		declaration_handling_enabled = 0;
	}
	| declaration_specifiers declarator {
		$2->set_is_function(true);
		$2->set_variable_type($1->get_symbol_type());
		ompd_emit_forward_decl($2);
		if (table.insert($2)) {
			logFile << "Inserted Function: " << $2->get_symbol_name() << " in scope " << table.current_scope_id() << endl;
			if($2->get_symbol_name() == "main"){
				main_init = 1;
			}
		}
		else {
			logFile << "Error: " << $2->get_symbol_name() << " already exists in scope " << endl;
			errFile << "Error: " << $2->get_symbol_name() << " already exists in scope " << endl;
			error_count++;
		}
		table.enter_scope();
		if ($2->get_parameter_list() != nullptr) {
			for(std::vector<symbol_info*>::size_type i = 0; i < $2->get_parameter_list()->size(); i++){
				symbol_info* symbol = new symbol_info(*$2->get_parameter_list()->at(i));
				if (table.insert(symbol)) {
					logFile << "Inserted Parameter: " << symbol->get_symbol_name() << " in scope " << table.current_scope_id() << endl;
				}
				else {
					logFile << "Error: " << symbol->get_symbol_name() << " already exists in scope " << endl;
					errFile << "Error: " << symbol->get_symbol_name() << " already exists in scope " << endl;
					error_count++;
				}
			}
		}
		if($2->get_symbol_name() == "main"){
			main_scope_depth = 1;
			function_scope_depth = 0;
		}
		else{
			function_scope_depth = 1;
			main_scope_depth = 0;
		}
		declaration_handling_enabled = 0;
		sequential_region_active = 0;
	} compound_statement {
		$2->set_is_defined(true);
		table.exit_scope();
		if($2->get_symbol_name() == "main"){
			main_scope_depth = 0;
		}

		function_scope_depth = 0;
		declaration_handling_enabled = 0;
	}
	;

declaration_list
    : declaration {
        $$ = new vector<symbol_info*>();
        $$->insert($$->end(), $1->begin(), $1->end());
    }
    | declaration_list declaration {
        $1->insert($1->end(), $2->begin(), $2->end());
        $$ = $1;
    }
    ;


%%
#include <stdio.h>

extern char yytext[];
extern int column;
extern int line_count;

void yyerror(char const *s)
{
	logFile << "Error at line " << line_count << " column: " << column << ": syntax error" << endl << endl;
	errFile << "Error at line " << line_count << " column: " << column << ": syntax error" << endl << endl;
	error_count++;

	/*
	 * fflush(stdout);
	 * printf("\n%*s\n%*s\n", line_count, "^", column, s);
	 */
}

