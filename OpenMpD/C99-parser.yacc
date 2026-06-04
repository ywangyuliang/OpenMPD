%{
#include <bits/stdc++.h>
#include "SymbolTable.h"
#include "templates.h"
#include "cluster_stack.h"
#include "tasking_region.h"
#include "task_body_transform.h"

void yyerror(char const *s);
extern int yylex (void);

int error_count = 0;

int main_init = 0;
int main_end = 0;
int MPIInitDone = 0;
int MPIInitMainDone = 0;
long posInit = -100;
long posVarsInit = -100;
long posTaskGlobalDefs = -100;
long posTaskGlobalDefsCursor = -100;
int enMain = 0;
int enFuncion = 0;
int llamadaFuncion = 0;
int activarDeclaracion = 0;
int escribirSeq = 0;
int enSecuencial = 0;
int n_point = 0;
int use_ompd_runtime_main = 0;

extern ofstream logFile;
extern ofstream errFile;

extern SymbolTable table;

extern void flush_pending_clusters_at_eof();

extern int ta_expect_task_body;
extern int ta_in_task_body_parse;
static int ta_task_body_statement_depth = 0;

static SymbolInfo *task_make_expr_symbol(task_body_expr_t *expr, const std::string &value_type){
    SymbolInfo *sym = new SymbolInfo("", "TASK_BODY_EXPR");
    sym->setVariableType(value_type);
    sym->setTaskBodyExpr(expr);
    return sym;
}

static task_body_expr_t *task_expr_of(SymbolInfo *sym){
    if(sym == NULL){
        return NULL;
    }
    return sym->getTaskBodyExpr();
}

static void task_body_begin_if_needed(){
    cluster_stack *c = cluster_stack_peek();

    if(ta_expect_task_body){
        if(c == NULL || c->tasking_region == NULL){
            fprintf(stderr, "Error: task body without open tasking_region\n");
            exit(901);
        }

        if(tasking_region_begin_task_async_block_body_parse(c->tasking_region) != 0){
            fprintf(stderr, "Error: tasking_region_begin_task_async_block_body_parse\n");
            exit(901);
        }

        ta_expect_task_body = 0;
        ta_in_task_body_parse = 1;
        ta_task_body_statement_depth = 0;
    }

    if(ta_in_task_body_parse){
        ta_task_body_statement_depth++;
    }
}

static void task_body_finish_if_root(task_body_stmt_t *stmt){
    cluster_stack *c = cluster_stack_peek();

    if(!ta_in_task_body_parse){
        return;
    }

    ta_task_body_statement_depth--;

    if(ta_task_body_statement_depth == 0){
        if(c == NULL || c->tasking_region == NULL){
            fprintf(stderr, "Error: task body finished without tasking_region\n");
            exit(901);
        }

        if(tasking_region_finish_task_async_block_body_ir(c->tasking_region, stmt) != 0){
            fprintf(stderr, "Error: tasking_region_finish_task_async_block_body_ir\n");
            exit(901);
        }

        ta_in_task_body_parse = 0;
    }
}

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

static void ompd_emit_forward_decl(SymbolInfo *func){
    if(func == nullptr) return;
    if(posTaskGlobalDefsCursor == -100) return;

    std::string ret = ompd_specs_to_c(func->getVariableType());
    std::string decl;

    if(!ret.empty()){
        decl += ret + " ";
    }

    if(func->isPointer()){
        decl += "*";
    }

    decl += func->getSymbolName();
    decl += "();\n";

    std::streampos cur = output.tellp();
    output.seekp(posTaskGlobalDefsCursor);
    output << decl;
    posTaskGlobalDefsCursor = output.tellp();
    output.seekp(cur);
}
%}

%union{
	SymbolInfo * sym;
	vector <SymbolInfo*> *symList;
	void *ptype;
}

%token SIZEOF
%token PTR_OP INC_OP DEC_OP LEFT_OP RIGHT_OP LE_OP GE_OP EQ_OP NE_OP
%token AND_OP OR_OP MUL_ASSIGN DIV_ASSIGN MOD_ASSIGN ADD_ASSIGN
%token SUB_ASSIGN LEFT_ASSIGN RIGHT_ASSIGN AND_ASSIGN
%token XOR_ASSIGN OR_ASSIGN

%token<sym> TYPEDEF EXTERN STATIC AUTO REGISTER INLINE RESTRICT
%token<sym> CHAR SHORT INT LONG SIGNED UNSIGNED FLOAT DOUBLE CONST VOLATILE VOID
%token<sym> BOOL COMPLEX IMAGINARY USER_DEFINED
%token<sym> STRUCT UNION ENUM ELLIPSIS

%token CASE DEFAULT IF ELSE SWITCH WHILE DO FOR GOTO CONTINUE BREAK RETURN

%token<sym> CONSTANT IDENTIFIER STRING_LITERAL

%type<sym> type_specifier struct_or_union_specifier enum_specifier struct_or_union specifier_qualifier_list   
%type<sym> storage_class_specifier direct_declarator declarator declaration_specifiers type_qualifier initializer_list
%type<sym> init_declarator initializer parameter_declaration  struct_declarator type_name constant_expression

%type<sym> primary_expression postfix_expression unary_expression cast_expression
%type<sym> multiplicative_expression additive_expression shift_expression
%type<sym> relational_expression equality_expression and_expression expression
%type<sym> exclusive_or_expression inclusive_or_expression logical_and_expression
%type<sym> logical_or_expression conditional_expression assignment_expression function_specifier 
%type<symList> init_declarator_list parameter_type_list parameter_list struct_declaration_list struct_declarator_list struct_declaration
%type<symList> declaration_list identifier_list declaration

%type<ptype> statement compound_statement expression_statement
%type<ptype> block_item block_item_list
%type<ptype> argument_expression_list task_body_statement_head

%start translation_unit
%%

primary_expression
    : IDENTIFIER
      	{
          	if(ta_in_task_body_parse){
              	$1->setTaskBodyExpr(task_body_expr_make_identifier($1->getSymbolName().c_str(), $1->getVariableType().c_str(), $1->isFunction() ? 1 : 0));
          	}
          	$$ = $1;
      	}
    | CONSTANT
      	{
          	if(ta_in_task_body_parse){
              	$1->setTaskBodyExpr(task_body_expr_make_constant($1->getSymbolName().c_str(), $1->getVariableType().c_str()));
          	}
          	$$ = $1;
      	}
    | STRING_LITERAL
      	{
          	if(ta_in_task_body_parse){
              	$1->setTaskBodyExpr(task_body_expr_make_string_literal($1->getSymbolName().c_str()));
          	}
          	$$ = $1;
      	}
    | '(' expression ')'
      	{
          	if(ta_in_task_body_parse){
              	$$ = task_make_expr_symbol(task_body_expr_make_paren(task_expr_of($2)), $2 != NULL ? $2->getVariableType().c_str() : "");
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
          	if(ta_in_task_body_parse){
              	$$ = task_make_expr_symbol(
					task_body_expr_make_subscript(task_expr_of($1), task_expr_of($3), $1 != NULL ? $1->getVariableType().c_str() : ""), 
					$1 != NULL ? $1->getVariableType().c_str() : ""
					);
          	} else {
              	$$ = $1;
          	}
      	}
	| postfix_expression '(' ')'
      	{
          	if(ta_in_task_body_parse){$$ = task_make_expr_symbol(
				task_body_expr_make_call(task_expr_of($1), NULL, $1 != NULL ? $1->getVariableType().c_str() : ""), 
				$1 != NULL ? $1->getVariableType().c_str() : ""
				);
          } else {
              $$ = $1;
          }
      }
	| postfix_expression '(' argument_expression_list ')'
      	{
          	if(ta_in_task_body_parse){
              	$$ = task_make_expr_symbol(
					task_body_expr_make_call(task_expr_of($1),(task_body_expr_list_t *)$3, $1 != NULL ? $1->getVariableType().c_str() : ""),
					$1 != NULL ? $1->getVariableType().c_str() : ""
					);
          	} else {
              	$$ = $1;
          	}
      	}
	| postfix_expression '.' IDENTIFIER
      	{
          	if(ta_in_task_body_parse){
              	$$ = task_make_expr_symbol(
					task_body_expr_make_member(task_expr_of($1), $3->getSymbolName().c_str(), $1 != NULL ? $1->getVariableType().c_str() : ""),
					$1 != NULL ? $1->getVariableType().c_str() : ""
					);
          	} else {
              	$$ = $1;
          	}
      	}
	| postfix_expression PTR_OP IDENTIFIER
      	{
          if(ta_in_task_body_parse){
              	$$ = task_make_expr_symbol(
                      task_body_expr_make_ptr_member(task_expr_of($1), $3->getSymbolName().c_str(), $1 != NULL ? $1->getVariableType().c_str() : ""),
					  $1 != NULL ? $1->getVariableType().c_str() : "");
          	} else {
              	$$ = $1;
          	}
      	}
	| postfix_expression INC_OP
      	{
          	if(ta_in_task_body_parse){
              	$$ = task_make_expr_symbol(
                      task_body_expr_make_post_inc(task_expr_of($1), $1 != NULL ? $1->getVariableType().c_str() : ""),
					  $1 != NULL ? $1->getVariableType().c_str() : ""
					  );
          	} else {
              	$$ = $1;
          	}
      	}
	| postfix_expression DEC_OP
      	{
          	if(ta_in_task_body_parse){
              	$$ = task_make_expr_symbol(
                      task_body_expr_make_post_dec(task_expr_of($1), $1 != NULL ? $1->getVariableType().c_str() : ""),
					  $1 != NULL ? $1->getVariableType().c_str() : "");
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
          if(ta_in_task_body_parse){
              task_body_expr_list_t *list = task_body_expr_list_create();
              task_body_expr_list_append(list, task_expr_of($1));
              $$ = list;
          } else {
              $$ = NULL;
          }
      }
	| argument_expression_list ',' assignment_expression
      {
          if(ta_in_task_body_parse){
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
			if(ta_in_task_body_parse){
				$$ = task_make_expr_symbol(
					task_body_expr_make_pre_inc(
						task_expr_of($2),
						$2 != NULL ? $2->getVariableType().c_str() : ""
					),
					$2 != NULL ? $2->getVariableType().c_str() : ""
				);
			} else {
				$$ = $2;
			}
		}
	| DEC_OP unary_expression
		{
			if(ta_in_task_body_parse){
				$$ = task_make_expr_symbol(
					task_body_expr_make_pre_dec(
						task_expr_of($2),
						$2 != NULL ? $2->getVariableType().c_str() : ""
					),
					$2 != NULL ? $2->getVariableType().c_str() : ""
				);
			} else {
				$$ = $2;
			}
		}
	| '&' cast_expression
		{
			if(ta_in_task_body_parse){
				$$ = task_make_expr_symbol(
					task_body_expr_make_unary("&", task_expr_of($2)),
					$2 != NULL ? $2->getVariableType().c_str() : ""
				);
			} else {
				$$ = $2;
			}
		}
	| '*' cast_expression
		{
			if(ta_in_task_body_parse){
				$$ = task_make_expr_symbol(
					task_body_expr_make_unary("*", task_expr_of($2)),
					$2 != NULL ? $2->getVariableType().c_str() : ""
				);
			} else {
				$$ = $2;
			}
		}
	| '+' cast_expression
		{
			if(ta_in_task_body_parse){
				$$ = task_make_expr_symbol(
					task_body_expr_make_unary("+", task_expr_of($2)),
					$2 != NULL ? $2->getVariableType().c_str() : ""
				);
			} else {
				$$ = $2;
			}
		}
	| '-' cast_expression
		{
			if(ta_in_task_body_parse){
				$$ = task_make_expr_symbol(
					task_body_expr_make_unary("-", task_expr_of($2)),
					$2 != NULL ? $2->getVariableType().c_str() : ""
				);
			} else {
				$$ = $2;
			}
		}
	| '~' cast_expression
		{
			if(ta_in_task_body_parse){
				$$ = task_make_expr_symbol(
					task_body_expr_make_unary("~", task_expr_of($2)),
					$2 != NULL ? $2->getVariableType().c_str() : ""
				);
			} else {
				$$ = $2;
			}
		}
	| '!' cast_expression
		{
			if(ta_in_task_body_parse){
				$$ = task_make_expr_symbol(
					task_body_expr_make_unary("!", task_expr_of($2)),
					$2 != NULL ? $2->getVariableType().c_str() : ""
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
			if(ta_in_task_body_parse){
				$$ = task_make_expr_symbol(
					task_body_expr_make_binary("*", task_expr_of($1), task_expr_of($3), $1 != NULL ? $1->getVariableType().c_str() : ""),
					$1 != NULL ? $1->getVariableType().c_str() : ""
				);
			} else {
				$$ = $1;
			}
		}
	| multiplicative_expression '/' cast_expression
		{
			if(ta_in_task_body_parse){
				$$ = task_make_expr_symbol(
					task_body_expr_make_binary("/", task_expr_of($1), task_expr_of($3), $1 != NULL ? $1->getVariableType().c_str() : ""),
					$1 != NULL ? $1->getVariableType().c_str() : ""
				);
			} else {
				$$ = $1;
			}
		}
	| multiplicative_expression '%' cast_expression
		{
			if(ta_in_task_body_parse){
				$$ = task_make_expr_symbol(
					task_body_expr_make_binary("%", task_expr_of($1), task_expr_of($3), $1 != NULL ? $1->getVariableType().c_str() : ""),
					$1 != NULL ? $1->getVariableType().c_str() : ""
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
			if(ta_in_task_body_parse){
				$$ = task_make_expr_symbol(
					task_body_expr_make_binary("+", task_expr_of($1), task_expr_of($3), $1 != NULL ? $1->getVariableType().c_str() : ""),
					$1 != NULL ? $1->getVariableType().c_str() : ""
				);
			} else {
				$$ = $1;
			}
		}
	| additive_expression '-' multiplicative_expression
		{
			if(ta_in_task_body_parse){
				$$ = task_make_expr_symbol(
					task_body_expr_make_binary("-", task_expr_of($1), task_expr_of($3), $1 != NULL ? $1->getVariableType().c_str() : ""),
					$1 != NULL ? $1->getVariableType().c_str() : ""
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
			if(ta_in_task_body_parse){
				$$ = task_make_expr_symbol(
					task_body_expr_make_assignment(
						"=",
						task_expr_of($1),
						task_expr_of($3),
						$1 != NULL ? $1->getVariableType().c_str() : ""
					),
					$1 != NULL ? $1->getVariableType().c_str() : ""
				);
			} else {
				$$ = $1;
			}
		}
	| unary_expression MUL_ASSIGN assignment_expression
		{
			if(ta_in_task_body_parse){
				$$ = task_make_expr_symbol(
					task_body_expr_make_assignment(
						"*=",
						task_expr_of($1),
						task_expr_of($3),
						$1 != NULL ? $1->getVariableType().c_str() : ""
					),
					$1 != NULL ? $1->getVariableType().c_str() : ""
				);
			} else {
				$$ = $1;
			}
		}
	| unary_expression DIV_ASSIGN assignment_expression
		{
			if(ta_in_task_body_parse){
				$$ = task_make_expr_symbol(
					task_body_expr_make_assignment(
						"/=",
						task_expr_of($1),
						task_expr_of($3),
						$1 != NULL ? $1->getVariableType().c_str() : ""
					),
					$1 != NULL ? $1->getVariableType().c_str() : ""
				);
			} else {
				$$ = $1;
			}
		}
	| unary_expression MOD_ASSIGN assignment_expression
		{
			if(ta_in_task_body_parse){
				$$ = task_make_expr_symbol(
					task_body_expr_make_assignment(
						"%=",
						task_expr_of($1),
						task_expr_of($3),
						$1 != NULL ? $1->getVariableType().c_str() : ""
					),
					$1 != NULL ? $1->getVariableType().c_str() : ""
				);
			} else {
				$$ = $1;
			}
		}
	| unary_expression ADD_ASSIGN assignment_expression
		{
			if(ta_in_task_body_parse){
				$$ = task_make_expr_symbol(
					task_body_expr_make_assignment(
						"+=",
						task_expr_of($1),
						task_expr_of($3),
						$1 != NULL ? $1->getVariableType().c_str() : ""
					),
					$1 != NULL ? $1->getVariableType().c_str() : ""
				);
			} else {
				$$ = $1;
			}
		}
	| unary_expression SUB_ASSIGN assignment_expression
		{
			if(ta_in_task_body_parse){
				$$ = task_make_expr_symbol(
					task_body_expr_make_assignment(
						"-=",
						task_expr_of($1),
						task_expr_of($3),
						$1 != NULL ? $1->getVariableType().c_str() : ""
					),
					$1 != NULL ? $1->getVariableType().c_str() : ""
				);
			} else {
				$$ = $1;
			}
		}
	| unary_expression LEFT_ASSIGN assignment_expression
		{
			if(ta_in_task_body_parse){
				$$ = task_make_expr_symbol(
					task_body_expr_make_assignment(
						"<<=",
						task_expr_of($1),
						task_expr_of($3),
						$1 != NULL ? $1->getVariableType().c_str() : ""
					),
					$1 != NULL ? $1->getVariableType().c_str() : ""
				);
			} else {
				$$ = $1;
			}
		}
	| unary_expression RIGHT_ASSIGN assignment_expression
		{
			if(ta_in_task_body_parse){
				$$ = task_make_expr_symbol(
					task_body_expr_make_assignment(
						">>=",
						task_expr_of($1),
						task_expr_of($3),
						$1 != NULL ? $1->getVariableType().c_str() : ""
					),
					$1 != NULL ? $1->getVariableType().c_str() : ""
				);
			} else {
				$$ = $1;
			}
		}
	| unary_expression AND_ASSIGN assignment_expression
		{
			if(ta_in_task_body_parse){
				$$ = task_make_expr_symbol(
					task_body_expr_make_assignment(
						"&=",
						task_expr_of($1),
						task_expr_of($3),
						$1 != NULL ? $1->getVariableType().c_str() : ""
					),
					$1 != NULL ? $1->getVariableType().c_str() : ""
				);
			} else {
				$$ = $1;
			}
		}
	| unary_expression XOR_ASSIGN assignment_expression
		{
			if(ta_in_task_body_parse){
				$$ = task_make_expr_symbol(
					task_body_expr_make_assignment(
						"^=",
						task_expr_of($1),
						task_expr_of($3),
						$1 != NULL ? $1->getVariableType().c_str() : ""
					),
					$1 != NULL ? $1->getVariableType().c_str() : ""
				);
			} else {
				$$ = $1;
			}
		}
	| unary_expression OR_ASSIGN assignment_expression
		{
			if(ta_in_task_body_parse){
				$$ = task_make_expr_symbol(
					task_body_expr_make_assignment(
						"|=",
						task_expr_of($1),
						task_expr_of($3),
						$1 != NULL ? $1->getVariableType().c_str() : ""
					),
					$1 != NULL ? $1->getVariableType().c_str() : ""
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
			if(ta_in_task_body_parse){
				$$ = task_make_expr_symbol(
					task_body_expr_make_comma(task_expr_of($1), task_expr_of($3), $3 != NULL ? $3->getVariableType().c_str() : ""),
					$3 != NULL ? $3->getVariableType().c_str() : ""
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
		if($1->isStruct()){
			$$ = new vector<SymbolInfo*>();
			SymbolInfo* symbol = new SymbolInfo(*$1);
			$$->push_back(symbol);
			table.insert($1);
		}
	}

	| declaration_specifiers init_declarator_list ';' {
		$$ = new vector<SymbolInfo*>();
		bool hasTypedef = (strstr($1->getSymbolType().c_str(), "TYPEDEF") != NULL);
		if($1->isStruct()){
			if($1->getParamList() != nullptr){
				for(std::vector<SymbolInfo*>::size_type i = 0; i < $2->size(); i++){
					logFile << "Debug: " << $1->getSymbolType() << " Debug: " << $2->at(i)->getSymbolName() << " Debug: " << $2->at(i)->getVariableType() << endl;
					if(hasTypedef) $2->at(i)->setSymIsType(true);
					$2->at(i)->setVariableType($1->getSymbolType());				
					$2->at(i)->setIsStruct(true);
					$2->at(i)->setParamList($1->getParamList());
					SymbolInfo* symbol = new SymbolInfo(*$2->at(i));
					$$->push_back(symbol);
					table.insert($2->at(i));
				}
			}
			else {
				$1->setParamList($2);
				$$->push_back($1);
				for(std::vector<SymbolInfo*>::size_type i = 0; i < $2->size(); i++){
					$2->at(i)->setVariableType($1->getSymbolName());
					table.insert($2->at(i));
				}
			}
		}
		else{
			for(std::vector<SymbolInfo*>::size_type i = 0; i < $2->size(); i++){
				logFile << "Debug: " << $1->getSymbolType() << " Debug: " << $2->at(i)->getSymbolName() << " Debug: " << $2->at(i)->getVariableType() << endl;
				$2->at(i)->setVariableType($1->getSymbolType());
				if(hasTypedef) $2->at(i)->setSymIsType(true);
				if(!$2->at(i)->isFunction()){
					SymbolInfo* symbol = new SymbolInfo(*$2->at(i));
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
			oss << $1->getSymbolType() << "_" << $2->getSymbolType();
			$2->setSymbolType(oss.str());
			$$ = $2;
		}
	| type_specifier { $$ = $1; }
	| type_specifier declaration_specifiers 
		{ 
			std::ostringstream oss;
			oss << $1->getSymbolType() << "_" << $2->getSymbolType();
			$2->setSymbolType(oss.str());
			$$ = $2;
		}
	| type_qualifier { $$ = $1; }
	| type_qualifier declaration_specifiers 
		{ 
			std::ostringstream oss;
			oss << $1->getSymbolType() << "_" << $2->getSymbolType();
			$2->setSymbolType(oss.str());
			$$ = $2;
		}
	| function_specifier { $$ = $1; }
	| function_specifier declaration_specifiers 
		{ 
			std::ostringstream oss;
			oss << $1->getSymbolType() << "_" << $2->getSymbolType();
			$2->setSymbolType(oss.str());
			$$ = $2;
		}
	;

init_declarator_list
	: init_declarator	{ $$ = new vector<SymbolInfo*>(); $$->push_back($1); }
	| init_declarator_list ',' init_declarator	{ $1->push_back($3); $$ = $1; }
	;

init_declarator
	: declarator
		{
			$1->setTaskInitExpr(NULL);
			$$ = $1;
		}
	| declarator '=' initializer
		{
			$1->setIsDefined(true);
			$1->setTaskInitExpr($3 != NULL ? $3->getTaskBodyExpr() : NULL);
			$$ = $1;
		}
	;

storage_class_specifier
	: { if (posVarsInit == -100 && !MPIInitDone) {
			posVarsInit = output.tellp();
			output.write("                                                            \n", 83);
		} } TYPEDEF { $$ = new SymbolInfo("typedef", "TYPEDEF"); }
	| { if (posVarsInit == -100 && !MPIInitDone) {
			posVarsInit = output.tellp();
			output.write("                                                            \n", 83);
		} } EXTERN { $$ = new SymbolInfo("extern", "EXTERN"); }
	| { if (posVarsInit == -100 && !MPIInitDone) {
			posVarsInit = output.tellp();
			output.write("                                                            \n", 83);
		} } STATIC		{ $$ = new SymbolInfo("static", "STATIC"); }
	| { if (posVarsInit == -100 && !MPIInitDone) {
			posVarsInit = output.tellp();
			output.write("                                                            \n", 83);
		} } AUTO { $$ = new SymbolInfo("auto", "AUTO"); }
	| { if (posVarsInit == -100 && !MPIInitDone) {
			posVarsInit = output.tellp();
			output.write("                                                            \n", 83);
		} } REGISTER { $$ = new SymbolInfo("register", "REGISTER"); }
	;

type_specifier
    : { if (posVarsInit == -100 && !MPIInitDone) {
			posVarsInit = output.tellp();
			output.write("                                                            \n", 83);
		}} VOID          { $$ = new SymbolInfo("void", "VOID"); }
    | { if (posVarsInit == -100 && !MPIInitDone) {
			posVarsInit = output.tellp();
			output.write("                                                            \n", 83);
		}} CHAR          { $$ = new SymbolInfo("char", "CHAR"); }
    | { if (posVarsInit == -100 && !MPIInitDone) {
			posVarsInit = output.tellp();
			output.write("                                                            \n", 83);
		}} SHORT         { $$ = new SymbolInfo("short", "SHORT"); }
    | { if (posVarsInit == -100 && !MPIInitDone) {
			posVarsInit = output.tellp();
			output.write("                                                            \n", 83);
		}} INT           { $$ = new SymbolInfo("int", "INT"); }
    | { if (posVarsInit == -100 && !MPIInitDone) {
			posVarsInit = output.tellp();
			output.write("                                                            \n", 83);
		}} LONG          { $$ = new SymbolInfo("long", "LONG"); }
    | { if (posVarsInit == -100 && !MPIInitDone) {
			posVarsInit = output.tellp();
			output.write("                                                            \n", 83);
		}} FLOAT         { $$ = new SymbolInfo("float", "FLOAT"); }
    | { if (posVarsInit == -100 && !MPIInitDone) {
			posVarsInit = output.tellp();
			output.write("                                                            \n", 83);
		}} DOUBLE        { $$ = new SymbolInfo("double", "DOUBLE"); }
    | { if (posVarsInit == -100 && !MPIInitDone) {
			posVarsInit = output.tellp();
			output.write("                                                            \n", 83);
		}} SIGNED        { $$ = new SymbolInfo("signed", "SIGNED"); }
    | { if (posVarsInit == -100 && !MPIInitDone) {
			posVarsInit = output.tellp();
			output.write("                                                            \n", 83);
		}} UNSIGNED      { $$ = new SymbolInfo("unsigned", "UNSIGNED"); }
    | { if (posVarsInit == -100 && !MPIInitDone) {
			posVarsInit = output.tellp();
			output.write("                                                            \n", 83);
		}} BOOL          { $$ = new SymbolInfo("bool", "BOOL"); }
    | { if (posVarsInit == -100 && !MPIInitDone) {
			posVarsInit = output.tellp();
			output.write("                                                            \n", 83);
		}} COMPLEX       { $$ = new SymbolInfo("complex", "COMPLEX"); }
    | { if (posVarsInit == -100 && !MPIInitDone) {
			posVarsInit = output.tellp();
			output.write("                                                            \n", 83);
		}} IMAGINARY     { $$ = new SymbolInfo("imaginary", "IMAGINARY"); }
	| USER_DEFINED  { $$ = $1;}
    | struct_or_union_specifier  { $$ = $1;}
    | enum_specifier             { $$ = $1;}
    ;

struct_or_union_specifier
	: struct_or_union IDENTIFIER '{' struct_declaration_list '}'    
	{ 
		std::string tag = $1->getSymbolType() + "_" + $2->getSymbolName();
		if(table.getSymbolInfo(tag) == NULL){
			$2->setIsStruct(true);
			$2->setVariableType(tag);
			$2->setSymbolName(tag);
			if (table.insert($2)) {
				logFile << "Inserted: " << $2->getSymbolName() << " in scope " << table.printScopeId() << endl;
			}else {
				logFile << "Error: " << $2->getSymbolName() << " already exists in scope " << endl;
				errFile << "Error: " << $2->getSymbolName() << " already exists in scope " << endl;
				error_count++;
			}
		}
	
		$2->setParamList($4);
		for(std::vector<SymbolInfo*>::size_type i = 0; i < $4->size(); i++){
			logFile << "Struct item 2: " << $4->at(i)->getSymbolName() << endl;
		}

		$$ = $2;
	}
	| struct_or_union '{' struct_declaration_list '}'
	| struct_or_union IDENTIFIER
	{ 
		std::string tag = $1->getSymbolType() + "_" + $2->getSymbolName();
		if(table.getSymbolInfo(tag) == NULL){
			$2->setIsStruct(true);
			$2->setVariableType(tag);
			$2->setSymbolName(tag);

			if (table.insert($2)) {
				logFile << "Inserted: " << $2->getSymbolName() << " in scope " << table.printScopeId() << endl;
			}else {
				logFile << "Error: " << $2->getSymbolName() << " already exists in scope " << endl;
				errFile << "Error: " << $2->getSymbolName() << " already exists in scope " << endl;
				error_count++;
			}
		}
		
		$$ = $2;
	}
	;

struct_or_union
	: STRUCT		{ $$ = new SymbolInfo("struct", "STRUCT"); }
	| UNION			{ $$ = new SymbolInfo("union", "UNION"); }
	;

struct_declaration_list
	: struct_declaration { $$ = $1; 
	for(std::vector<SymbolInfo*>::size_type i = 0; i < $1->size(); i++){
			logFile << "Struct item: " << $1->at(i)->getSymbolName() << endl;
		} 
	}
	
	| struct_declaration_list struct_declaration 
	{ 
		for(std::vector<SymbolInfo*>::size_type i = 0; i < $2->size(); i++){
			$1->push_back($2->at(i));
			logFile << "Struct item: " << $2->at(i)->getSymbolName() << endl;
		} 
		$$ = $1; 
	}
	;

struct_declaration
	: specifier_qualifier_list struct_declarator_list ';'
	{
		for(std::vector<SymbolInfo*>::size_type i = 0; i < $2->size(); i++){
			$2->at(i)->setVariableType($1->getSymbolType());
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
	: struct_declarator { $$ = new vector<SymbolInfo*>(); $$->push_back($1); }
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
	: pointer direct_declarator { $2->setIsPointer(true); 
		for(int addSize = 0; addSize < n_point; addSize++) {
			$2->addArrSize("0");
		}
		$$ = $2;
		n_point = 0;}
	| direct_declarator	{ $$ = $1;}
	;


direct_declarator
	: IDENTIFIER		{ $$ = $1; }
	| '(' declarator ')' { $$ = $2; }
	| direct_declarator '[' type_qualifier_list assignment_expression ']'{
		if(!$1->isArray()){
			$1->setIsArray(true);
		}
		$1->addArrSize(($4->getSymbolName()));
		$$ = $1;
	}
	| direct_declarator '[' type_qualifier_list ']'{
		$1->setIsArray(true);
		$$ = $1;
	}
	| direct_declarator '[' assignment_expression ']' {
		if(!$1->isArray()){
			$1->setIsArray(true);
		}
		$1->addArrSize(($3->getSymbolName()));
		$$ = $1;
	}
	| direct_declarator '[' STATIC type_qualifier_list assignment_expression ']'{
		if(!$1->isArray()){
			$1->setIsArray(true);
		}
		$1->addArrSize(($5->getSymbolName()));
		$$ = $1;
	}
	| direct_declarator '[' type_qualifier_list STATIC assignment_expression ']'{
		if(!$1->isArray()){
			$1->setIsArray(true);
		}
		$1->addArrSize(($5->getSymbolName()));
		$$ = $1;
	}
	| direct_declarator '[' type_qualifier_list '*' ']'{
		$1->setIsArray(true);
		$$ = $1;
	}
	| direct_declarator '[' '*' ']'{
		$1->setIsArray(true);
		$$ = $1;
	}
	| direct_declarator '[' ']' {
		$1->setIsArray(true);
		$$ = $1;
	}
	| direct_declarator '(' parameter_type_list ')' {
		$1->setParamList($3);
		$$ = $1;
	}
	| direct_declarator '(' identifier_list ')' { $$ = $1;}
	| direct_declarator '(' ')' { $$ = $1; }
	;

pointer
	: '*' {n_point++;}
	| '*' type_qualifier_list
	| '*' {n_point++;} pointer
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
	: parameter_declaration { $$ = new vector<SymbolInfo*>(); $$->push_back($1); }
	| parameter_list ',' parameter_declaration { $1->push_back($3); $$ = $1; }
	;

parameter_declaration
	: declaration_specifiers declarator {
		$2->setVariableType($1->getSymbolType());
		$$ = $2;
	}
	| declaration_specifiers abstract_declarator 
	| declaration_specifiers
	;

identifier_list
	: IDENTIFIER { $$ = new vector<SymbolInfo*>(); $$->push_back($1); }
	| identifier_list ',' IDENTIFIER { $1->push_back($3); $$ = $1; }
	;

type_name
	: specifier_qualifier_list
	| specifier_qualifier_list abstract_declarator
	;

abstract_declarator
	: pointer {n_point = 0;}
	| direct_abstract_declarator
	| pointer {n_point = 0;} direct_abstract_declarator
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
			if(ta_in_task_body_parse){
				fprintf(stderr, "Error: labeled_statement not supported yet in task body\n");
				exit(901);
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
	| task_body_statement_head { table.enterScope(); } selection_statement { table.exitScope(); }
		{
			if(ta_in_task_body_parse){
				fprintf(stderr, "Error: selection_statement not supported yet in task body\n");
				exit(901);
			}
			$$ = NULL;
			task_body_finish_if_root((task_body_stmt_t *)$$);
		}
	| task_body_statement_head { table.enterScope(); } iteration_statement { table.exitScope(); }
		{
			if(ta_in_task_body_parse){
				fprintf(stderr, "Error: iteration_statement not supported yet in task body\n");
				exit(901);
			}
			$$ = NULL;
			task_body_finish_if_root((task_body_stmt_t *)$$);
		}
	| task_body_statement_head jump_statement
		{
			if(ta_in_task_body_parse){
				fprintf(stderr, "Error: jump_statement not supported yet in task body\n");
				exit(901);
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
          	if(ta_in_task_body_parse){
              	$$ = task_body_stmt_make_compound(task_body_stmt_list_create());
          	} else {
              	$$ = NULL;
          	}
      	}
	| '{' {
			if (enMain > 0) {
				enMain++;
			}
			else {
				enFuncion++;
			}
		}
	block_item_list  
	'}' {
			if (enMain == 2 && MPIInitDone == 1 && main_end == 1) {
				flush_pending_clusters_at_eof();
				MPIFinalize();
				main_end = 0;
			}
			else if (enMain > 0) {
				enMain--;
			}

			if (!enMain){/*
				if (enFuncion == 2 && enSecuencial) {
					output << "}" << endl;
					enFuncion = 0;
				}
				else*/ if (enFuncion > 0) {
					enFuncion--;
				}
			}

            if(ta_in_task_body_parse){
                $$ = task_body_stmt_make_compound((task_body_stmt_list_t *)$3);
            } else {
                $$ = NULL;
            }
		}
	;

block_item_list
	: block_item
    	{
          	if(ta_in_task_body_parse){
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
          	if(ta_in_task_body_parse){
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
	: {if((/*enFuncion == 2 ||*/ enMain == 2) && activarDeclaracion && enSecuencial){output << "}" << endl; enSecuencial = 0;}} declaration {if ((enFuncion == 2 || enMain == 2) && activarDeclaracion) {escribirSeq = 1;}}
		{
			if(ta_in_task_body_parse){
				task_body_stmt_t *decl_stmt;
				size_t i;

				decl_stmt = task_body_stmt_make_declaration();
				if(decl_stmt == NULL){
					fprintf(stderr, "Error: task_body_stmt_make_declaration\n");
					exit(901);
				}

				for(i = 0; i < $2->size(); i++){
					SymbolInfo *sym = $2->at(i);

					if(sym == NULL){
						continue;
					}

					if(task_body_stmt_add_decl(decl_stmt, sym->getSymbolName().c_str(), sym->getVariableType().c_str(), sym->getTaskInitExpr()) != 0){
						fprintf(stderr, "Error: task_body_stmt_add_decl\n");
						exit(901);
					}
				}

				$$ = decl_stmt;
			} else {
				$$ = NULL;
			}
		}
	|
	{
		ensure_mpi_init_slot();
	}
	statement
		{
			$$ = $2;
		}
	;

expression_statement
	: ';'
      	{
          	if(ta_in_task_body_parse){
        		$$ = task_body_stmt_make_empty();
          	} else {
              	$$ = NULL;
          	}
      	}
	| expression ';'
      	{
          	if(ta_in_task_body_parse){
              	$$ = task_body_stmt_make_expr(task_expr_of($1));
          	} else {
              	$$ = NULL;
          	}
      	}
	;

selection_statement
	: IF '(' expression ')' statement
	| IF '(' expression ')' statement ELSE { table.exitScope(); table.enterScope(); } statement
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
		if(main_end == 1 && MPIInitDone == 1 && enMain == 2){
			flush_pending_clusters_at_eof();
			MPIFinalize();
			main_end = 0;
		}
	}  ';'
	|
	RETURN
	{	
		if(main_end == 1 && MPIInitDone == 1 && enMain == 2){
			flush_pending_clusters_at_eof();
			MPIFinalize();
			main_end = 0;
		}
	}
	expression ';'

translation_unit
    : {
        std::string __header_slot;

        for(int __slot_i = 0; __slot_i < 8; ++__slot_i){
            __header_slot += std::string(128, ' ');
            __header_slot += '\n';
        }

        output.write(__header_slot.c_str(), __header_slot.size());

        if(posTaskGlobalDefs == -100){
            posTaskGlobalDefs = output.tellp();
            posTaskGlobalDefsCursor = posTaskGlobalDefs;

            for(int __slot_i = 0; __slot_i < 256; ++__slot_i){
                output << "                                                                                                                                " << '\n';
            }
        }
      } external_declaration
    | translation_unit external_declaration
    ;

external_declaration
	: function_definition
	| declaration
	;

function_definition
	: declaration_specifiers declarator {
		$2->setIsFunction(true);
		$2->setVariableType($1->getSymbolType());
		ompd_emit_forward_decl($2);
		if (table.insert($2)) {
			logFile << "Inserted Function: " << $2->getSymbolName() << " in scope " << table.printScopeId() << endl;
			if($2->getSymbolName() == "main"){
				main_init = 1;
			}
		}
		else {
			logFile << "Error: " << $2->getSymbolName() << " already exists in scope " << endl;
			errFile << "Error: " << $2->getSymbolName() << " already exists in scope " << endl;
			error_count++;
		}
		table.enterScope();
	} declaration_list {
		table.getSymbolInfo($2->getSymbolName())->setParamList($4);
		if($2->getSymbolName() == "main"){
			enMain = 1;
			enFuncion = 0;
		}
		else{
			enFuncion = 1;
			enMain = 0;
		}

		activarDeclaracion = 0;
		enSecuencial = 0;
	} compound_statement {
		$2->setIsDefined(true);
		table.exitScope();
		if($2->getSymbolName() == "main"){
			enMain = 0;
		}

		enFuncion = 0;
		activarDeclaracion = 0;
	}
	| declaration_specifiers declarator {
		$2->setIsFunction(true);
		$2->setVariableType($1->getSymbolType());
		ompd_emit_forward_decl($2);
		if (table.insert($2)) {
			logFile << "Inserted Function: " << $2->getSymbolName() << " in scope " << table.printScopeId() << endl;
			if($2->getSymbolName() == "main"){
				main_init = 1;
			}
		}
		else {
			logFile << "Error: " << $2->getSymbolName() << " already exists in scope " << endl;
			errFile << "Error: " << $2->getSymbolName() << " already exists in scope " << endl;
			error_count++;
		}
		table.enterScope();
		if ($2->getParamList() != nullptr) {
			for(std::vector<SymbolInfo*>::size_type i = 0; i < $2->getParamList()->size(); i++){
				SymbolInfo* symbol = new SymbolInfo(*$2->getParamList()->at(i));
				if (table.insert(symbol)) {
					logFile << "Inserted Parameter: " << symbol->getSymbolName() << " in scope " << table.printScopeId() << endl;
				}
				else {
					logFile << "Error: " << symbol->getSymbolName() << " already exists in scope " << endl;
					errFile << "Error: " << symbol->getSymbolName() << " already exists in scope " << endl;
					error_count++;
				}
			}
		}
		if($2->getSymbolName() == "main"){
			enMain = 1;
			enFuncion = 0;
		}
		else{
			enFuncion = 1;
			enMain = 0;
		}
		activarDeclaracion = 0;
		enSecuencial = 0;
	} compound_statement {
		$2->setIsDefined(true);
		table.exitScope();
		if($2->getSymbolName() == "main"){
			enMain = 0;
		}

		enFuncion = 0;
		activarDeclaracion = 0;
	}
	;

declaration_list
    : declaration { 
        $$ = new vector<SymbolInfo*>();
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

	/* fflush(stdout);
	printf("\n%*s\n%*s\n", line_count, "^", column, s); */
}