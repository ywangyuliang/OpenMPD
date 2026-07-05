%{
#include<bits/stdc++.h>
#include "symbol_table.h"

void yyerror(char const *s);
extern int yylex (void);
extern void start_lexer_prepro(FILE *input_file);

extern ofstream logFile;
extern ofstream errFile;

extern symbol_table table;
extern int n_lineas_prepro;
extern int num_errores_prepro;

extern int prepro_lineno;
%}

%define api.prefix {prepro_}

%union{
	symbol_info * symbol;
	vector <symbol_info*> *symbol_list;
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

%start translation_unit
%%

primary_expression
    : IDENTIFIER
    | CONSTANT {$$ = $1;}
    | STRING_LITERAL
    | '(' expression ')' {$$ = $2;}
    ;

postfix_expression
	: primary_expression {$$ = $1;}
	| postfix_expression '[' expression ']'
	| postfix_expression '(' ')'
	| postfix_expression '(' argument_expression_list ')'
	| postfix_expression '.' IDENTIFIER
	| postfix_expression PTR_OP IDENTIFIER
	| postfix_expression INC_OP
	| postfix_expression DEC_OP
	| '(' type_name ')' '{' initializer_list '}' {$$ = $5;}
	| '(' type_name ')' '{' initializer_list ',' '}'{$$ = $5;}
	;

argument_expression_list
	: assignment_expression
	| argument_expression_list ',' assignment_expression
	;

unary_expression
	: postfix_expression {$$ = $1;}
	| INC_OP unary_expression {$$ = $2;}
	| DEC_OP unary_expression {$$ = $2;}
	| unary_operator cast_expression {$$ = $2;}
	| SIZEOF unary_expression {$$ = $2;}
	| SIZEOF '(' type_name ')' {$$ = $3;}
	;

unary_operator
	: '&'
	| '*'
	| '+'
	| '-'
	| '~'
	| '!'
	;

cast_expression
	: unary_expression {$$ = $1;}
	| '(' type_name ')' cast_expression {$$ = $4;}
	;

multiplicative_expression
	: cast_expression {$$ = $1;}
	| multiplicative_expression '*' cast_expression
	| multiplicative_expression '/' cast_expression
	| multiplicative_expression '%' cast_expression
	;

additive_expression
	: multiplicative_expression {$$ = $1;}
	| additive_expression '+' multiplicative_expression
	| additive_expression '-' multiplicative_expression
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
	: conditional_expression {$$ = $1;}
	| unary_expression assignment_operator assignment_expression
	;

assignment_operator
	: '='
	| MUL_ASSIGN
	| DIV_ASSIGN
	| MOD_ASSIGN
	| ADD_ASSIGN
	| SUB_ASSIGN
	| LEFT_ASSIGN
	| RIGHT_ASSIGN
	| AND_ASSIGN
	| XOR_ASSIGN
	| OR_ASSIGN
	;

expression
	: assignment_expression
	| expression ',' assignment_expression
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
		logFile << "Started declaration section" << endl;
		$$ = new vector<symbol_info*>();
		logFile << "HASTYPEDEF: " << $1->is_struct() << endl;
		bool hasTypedef = (strstr($1->get_symbol_type().c_str(), "TYPEDEF") != NULL);
		if($1->is_struct()){
			if($1->get_parameter_list() != nullptr){
				for(std::vector<symbol_info*>::size_type i = 0; i < $2->size(); i++){
					logFile << "TYPE A: " << $2->at(i)->get_symbol_name() << endl;
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
					logFile << "PARAM[" << i << "] = " << $2->at(i)->get_symbol_name() << endl;
					if(hasTypedef) $2->at(i)->set_is_type_symbol(true);
					$2->at(i)->set_variable_type($1->get_symbol_name());
					table.insert($2->at(i));
				}
			}
		}
		else{
			for(std::vector<symbol_info*>::size_type i = 0; i < $2->size(); i++){
				$2->at(i)->set_variable_type($1->get_symbol_type());
				logFile << "TYPE A: " << $2->at(i)->get_symbol_name() << endl;
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
	: declarator	{ $$ = $1; }
	| declarator '=' initializer	{ $1->set_is_defined(true); $$ = $1;}
	;

storage_class_specifier
	: TYPEDEF { $$ = new symbol_info("typedef", "TYPEDEF"); }
	| EXTERN { $$ = new symbol_info("extern", "EXTERN"); }
	| STATIC		{ $$ = new symbol_info("static", "STATIC"); }
	| AUTO { $$ = new symbol_info("auto", "AUTO"); }
	| REGISTER { $$ = new symbol_info("register", "REGISTER"); }
	;

type_specifier
    : VOID          { $$ = new symbol_info("void", "VOID"); }
    | CHAR          { $$ = new symbol_info("char", "CHAR"); }
    | SHORT         { $$ = new symbol_info("short", "SHORT"); }
    | INT           { $$ = new symbol_info("int", "INT"); }
    | LONG          { $$ = new symbol_info("long", "LONG"); }
    | FLOAT         { $$ = new symbol_info("float", "FLOAT"); }
    | DOUBLE        { $$ = new symbol_info("double", "DOUBLE"); }
    | SIGNED        { $$ = new symbol_info("signed", "SIGNED"); }
    | UNSIGNED      { $$ = new symbol_info("unsigned", "UNSIGNED"); }
    | BOOL          { $$ = new symbol_info("bool", "BOOL"); }
    | COMPLEX       { $$ = new symbol_info("complex", "COMPLEX"); }
    | IMAGINARY     { $$ = new symbol_info("imaginary", "IMAGINARY"); }
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
			table.insert($2);
		}
		$2->set_parameter_list($4);
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
			table.insert($2);
		}
		$$ = $2;
	}
	;

struct_or_union
	: STRUCT		{ $$ = new symbol_info("struct", "STRUCT"); }
	| UNION			{ $$ = new symbol_info("union", "UNION"); }
	;

struct_declaration_list
	: struct_declaration { $$ = $1; }
	| struct_declaration_list struct_declaration
	{
		for(std::vector<symbol_info*>::size_type i = 0; i < $2->size(); i++){
			$1->push_back($2->at(i));
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
	: pointer direct_declarator { $2->set_is_pointer(true); $$ = $2; }
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
	: '*'
	| '*' type_qualifier_list
	| '*' pointer
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
	: pointer
	| direct_abstract_declarator
	| pointer direct_abstract_declarator
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

statement
	: labeled_statement
	| compound_statement
	| expression_statement
	| { table.enter_scope(); } selection_statement { table.exit_scope(); }
	| { table.enter_scope(); } iteration_statement { table.exit_scope(); }
	| jump_statement
	;

labeled_statement
	: IDENTIFIER ':' statement
	| CASE constant_expression ':' statement
	| DEFAULT ':' statement
	;

compound_statement
	: '{' '}'
	| '{'
	block_item_list
	'}'
	;

block_item_list
	: block_item
	| block_item_list block_item
	;

block_item
	: declaration
	|
	statement
	;

expression_statement
	: ';'
	| expression ';'
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
	';'
	|
	RETURN
	expression ';'
	;

translation_unit
	: external_declaration
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
		table.insert($2);
		table.enter_scope();
	} declaration_list {
		table.get_symbol_info($2->get_symbol_name())->set_parameter_list($4);
	} compound_statement {
		$2->set_is_defined(true);
		table.exit_scope();
	}
	| declaration_specifiers declarator {
		$2->set_is_function(true);
		$2->set_variable_type($1->get_symbol_type());
		table.insert($2);
		table.enter_scope();
	} compound_statement {
		$2->set_is_defined(true);
		table.exit_scope();
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

/* Reports a preprocessor parser error */
void yyerror(char const *s)
{
	num_errores_prepro++;
	logFile << "Error in preprocessor at line " << prepro_lineno << " column: " << column << ": syntax error" << endl << endl;
	errFile << "Error in preprocessor at line " << prepro_lineno << " column: " << column << ": syntax error" << endl << endl;
}
