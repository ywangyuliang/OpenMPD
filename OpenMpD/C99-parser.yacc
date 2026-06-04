%{
#include<bits/stdc++.h>
#include "SymbolTable.h"
#include "templates.h"
void yyerror(char const *s);
extern int yylex (void);

int error_count = 0;

int main_init = 0;
int main_end = 0;
int MPIInitDone = 0;
int MPIInitMainDone = 0;
long posInit = -100;
long posVarsInit = -100;
int enMain = 0;
int enFuncion = 0;
int llamadaFuncion = 0;
int activarDeclaracion = 0;
int escribirSeq = 0;
int enSecuencial = 0;
int n_point = 0;

extern ofstream logFile;
extern ofstream errFile;

extern SymbolTable table;
%}

%union{
	SymbolInfo * sym;
	vector <SymbolInfo*> *symList;
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
	| postfix_expression '(' {/*if($1->isFunction() && cluster_stack_isEmpty()){output << "}" << endl; llamadaFuncion = 1;}*/} argument_expression_list ')'
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
	: declarator	{ $$ = $1; }
	| declarator '=' initializer	{ $1->setIsDefined(true); $$ = $1;}
	;

storage_class_specifier
	: { if (posVarsInit == -100 && !MPIInitDone) {
			posVarsInit = output.tellp();
			output.write("                                                            \n", 61);
		} } TYPEDEF { $$ = new SymbolInfo("typedef", "TYPEDEF"); }
	| { if (posVarsInit == -100 && !MPIInitDone) {
			posVarsInit = output.tellp();
			output.write("                                                            \n", 61);
		} } EXTERN { $$ = new SymbolInfo("extern", "EXTERN"); }
	| { if (posVarsInit == -100 && !MPIInitDone) {
			posVarsInit = output.tellp();
			output.write("                                                            \n", 61);
		} } STATIC		{ $$ = new SymbolInfo("static", "STATIC"); }
	| { if (posVarsInit == -100 && !MPIInitDone) {
			posVarsInit = output.tellp();
			output.write("                                                            \n", 61);
		} } AUTO { $$ = new SymbolInfo("auto", "AUTO"); }
	| { if (posVarsInit == -100 && !MPIInitDone) {
			posVarsInit = output.tellp();
			output.write("                                                            \n", 61);
		} } REGISTER { $$ = new SymbolInfo("register", "REGISTER"); }
	;

type_specifier
    : { if (posVarsInit == -100 && !MPIInitDone) {
			posVarsInit = output.tellp();
			output.write("                                                            \n", 61);
		}} VOID          { $$ = new SymbolInfo("void", "VOID"); }
    | { if (posVarsInit == -100 && !MPIInitDone) {
			posVarsInit = output.tellp();
			output.write("                                                            \n", 61);
		}} CHAR          { $$ = new SymbolInfo("char", "CHAR"); }
    | { if (posVarsInit == -100 && !MPIInitDone) {
			posVarsInit = output.tellp();
			output.write("                                                            \n", 61);
		}} SHORT         { $$ = new SymbolInfo("short", "SHORT"); }
    | { if (posVarsInit == -100 && !MPIInitDone) {
			posVarsInit = output.tellp();
			output.write("                                                            \n", 61);
		}} INT           { $$ = new SymbolInfo("int", "INT"); }
    | { if (posVarsInit == -100 && !MPIInitDone) {
			posVarsInit = output.tellp();
			output.write("                                                            \n", 61);
		}} LONG          { $$ = new SymbolInfo("long", "LONG"); }
    | { if (posVarsInit == -100 && !MPIInitDone) {
			posVarsInit = output.tellp();
			output.write("                                                            \n", 61);
		}} FLOAT         { $$ = new SymbolInfo("float", "FLOAT"); }
    | { if (posVarsInit == -100 && !MPIInitDone) {
			posVarsInit = output.tellp();
			output.write("                                                            \n", 61);
		}} DOUBLE        { $$ = new SymbolInfo("double", "DOUBLE"); }
    | { if (posVarsInit == -100 && !MPIInitDone) {
			posVarsInit = output.tellp();
			output.write("                                                            \n", 61);
		}} SIGNED        { $$ = new SymbolInfo("signed", "SIGNED"); }
    | { if (posVarsInit == -100 && !MPIInitDone) {
			posVarsInit = output.tellp();
			output.write("                                                            \n", 61);
		}} UNSIGNED      { $$ = new SymbolInfo("unsigned", "UNSIGNED"); }
    | { if (posVarsInit == -100 && !MPIInitDone) {
			posVarsInit = output.tellp();
			output.write("                                                            \n", 61);
		}} BOOL          { $$ = new SymbolInfo("bool", "BOOL"); }
    | { if (posVarsInit == -100 && !MPIInitDone) {
			posVarsInit = output.tellp();
			output.write("                                                            \n", 61);
		}} COMPLEX       { $$ = new SymbolInfo("complex", "COMPLEX"); }
    | { if (posVarsInit == -100 && !MPIInitDone) {
			posVarsInit = output.tellp();
			output.write("                                                            \n", 61);
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

statement
	: labeled_statement
	| compound_statement 
	| expression_statement
	| { table.enterScope(); } selection_statement { table.exitScope(); }
	| { table.enterScope(); } iteration_statement { table.exitScope(); }
	| jump_statement
	;

labeled_statement
	: IDENTIFIER ':' statement
	| CASE constant_expression ':' statement
	| DEFAULT ':' statement
	;

compound_statement
	: '{' '}'
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
		}
	;

block_item_list
	: block_item
	| block_item_list block_item
	;

block_item
	: {if((/*enFuncion == 2 ||*/ enMain == 2) && activarDeclaracion && enSecuencial){output << "}" << endl; enSecuencial = 0;}} declaration {if ((enFuncion == 2 || enMain == 2) && activarDeclaracion) {escribirSeq = 1;}}
	| 
	{
		if(enMain && MPIInitMainDone == 0 && posInit == -100){
			posInit = output.tellp();
			output.write("                                                                                                                                                      \n", 151);
		}
		else if (enMain && MPIInitMainDone == 1 && posInit == -100) {
			long posActual = output.tellp();
			posInit = output.tellp();
			MPIInitParte2();
			output.seekp(posActual + 151);
		}

		if (cluster_stack_isEmpty() && (enMain == 2 /*|| enFuncion == 2*/) && !enSecuencial) {
			output << "if (__taskid == 0) {" << endl;
			enSecuencial = 1;
		}

		activarDeclaracion = 1;
	}
	statement
	;

expression_statement
	: ';'
	| expression ';'
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
		if(main_end == 1 && MPIInitDone == 1 && enMain){
			MPIFinalize();
			main_end = 0;
		}/*
		if (enFuncion == 2 && enSecuencial) {
			output << "}" << endl;
			enFuncion = 0;
		}*/
	}  ';'
	|
	RETURN
	{	
		if(main_end == 1 && MPIInitDone == 1 && enMain){
			MPIFinalize();
			main_end = 0;
		}/*
		if (enFuncion == 2 && enSecuencial) {
			output << "}" << endl;
			enFuncion = 0;
		}*/
	}
	expression ';'
	;

translation_unit
	: {output.write("                   \n                                    \n", 57);} external_declaration
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
	// 	for (auto symbol : *$4) {
    //     logFile << "Debug Simbol: " << symbol->getSymbolName() << "\n";
    // }
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