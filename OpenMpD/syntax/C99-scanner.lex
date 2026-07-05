D			[0-9]
L			[a-zA-Z_]
H			[a-fA-F0-9]
E			([Ee][+-]?{D}+)
P                       ([Pp][+-]?{D}+)
FS			(f|F|l|L)
IS                      ((u|U)|(u|U)?(l|L|ll|LL)|(l|L|ll|LL)(u|U))

%{
#include<bits/stdc++.h>
#include "symbol_table.h"
#include "writer.h"
#include "tasking_emit.h"
#include "translator_state.h"
#include "distribute_transform.h"
#include "pragma_args.h"
#include "reduction_transform.h"
#include "scatter_gather_transform.h"
#include "mpi_lifecycle.h"
#include "mpi_type_transform.h"
#include <stdio.h>
#include <cstring>
#include <fstream>
#include "y.tab.hh"
#include "cluster_stack.h"

extern ofstream output;
extern void parse_openmp_pragma(const char* _input, void * _exprParse(const char*));
extern void yyerror(const char *);
static int scanner_identifier_equals(const char *actual, const char *expected);

static void scanner_count_token(void);
static void scanner_skip_comment(void);
static char * scanner_read_pragma();

extern ofstream logFile;
extern ofstream errFile;
extern symbol_table table;

int column = 0;
int line_count = 1;
char * main_argc_name = NULL;
char * main_argv_name = NULL;

static int paren_depth = 0;

/* Stores the name of a main function argument for deferred MPI initialization */
static void remember_main_arg(char **target, const char *name)
{
	if(target == NULL || name == NULL){
		return;
	}

	free(*target);
	*target = strdup(name);
	if(*target == NULL){
		perror("strdup");
		exit(EXIT_FAILURE);
	}
}

extern int mpi_runtime_initialized;
extern int mpi_initialization_pending_in_main;
extern int function_scope_depth;
extern int main_scope_depth;
extern int sequential_region_active;
extern int use_ompd_runtime_main;

using namespace std;

extern YYSTYPE yylval;

extern int error_count;

%}

%option nounput

%%
"#"           {
				mpi_runtime_prepare_statement_emission(0);
				if (use_ompd_runtime_main && main_scope_depth == 2 && mpi_runtime_initialized == 1 && mpi_initialization_pending_in_main == 1) {
					ompd_runtime_emit_initialization();
					mpi_initialization_pending_in_main = 0;

					if (cluster_stack_is_empty() && !sequential_region_active) {
						sequential_region_emit_open_if_needed();
					}
				}
                char * line = scanner_read_pragma();
                char * pragma = strstr(line, "pragma");
                if (pragma != NULL) {
                    parse_openmp_pragma(pragma+7, NULL);
					char *reduction_match;
					char *search_cursor = line;
					string reduction_clause_text;

					if (strstr(line, "cluster") == NULL && (cluster_stack_is_empty() || strstr(line, "distribute") == NULL)) {
						if (distribute_should_buffer_line()) {
							distribute_pending_lines() += '#';
							distribute_pending_lines() += line;

							if ((strstr(line, "for") != NULL || strstr(line, "simd") != NULL)) {
								if (distribute_reduction_active) {
									while ((reduction_match = strstr(search_cursor, "reduction")) != NULL) {
										reduction_clause_text = "";
										search_cursor = reduction_match + 10;
										while (*search_cursor != ')') {
											reduction_clause_text += *search_cursor;
											search_cursor++;
										}
										pending_reduction_clauses.push_back(reduction_clause_text);
									}
									reduction_group_pending_vars_by_operator();
									pending_reduction_clauses.clear();
									for (long unsigned int i = 0; i < distribute_reduction_operators.size(); i++) {
										distribute_pending_lines() += reduction_build_distribute_clause(i);
									}
								}
								if (distribute_allreduction_active) {
									while ((reduction_match = strstr(search_cursor, "reduction")) != NULL) {
										reduction_clause_text = "";
										search_cursor = reduction_match + 10;
										while (*search_cursor != ')') {
											reduction_clause_text += *search_cursor;
											search_cursor++;
										}
										pending_reduction_clauses.push_back(reduction_clause_text);
									}
									reduction_group_pending_vars_by_operator();
									pending_reduction_clauses.clear();
									for (long unsigned int i = 0; i < distribute_allreduction_operators.size(); i++) {
										distribute_pending_lines() += allreduction_build_distribute_clause(i);
									}
								}

								grouped_reduction_vars.clear();
							}

							distribute_pending_lines() += '\n';
						}
						else {

							if(master_region_is_active()){
								break;
							}

							if(!cluster_stack_is_empty() && (strstr(line, "task_async") != NULL || strstr(line, "taskwait") != NULL)){
								break;
							}

							output << "	#" << line;

							if ((strstr(line, "for") != NULL || strstr(line, "simd") != NULL)) {
								if (distribute_reduction_active) {
									while ((reduction_match = strstr(search_cursor, "reduction")) != NULL) {
										reduction_clause_text = "";
										search_cursor = reduction_match + 10;
										while (*search_cursor != ')') {
											reduction_clause_text += *search_cursor;
											search_cursor++;
										}
										pending_reduction_clauses.push_back(reduction_clause_text);
									}
									reduction_group_pending_vars_by_operator();
									pending_reduction_clauses.clear();
									for (long unsigned int i = 0; i < distribute_reduction_operators.size(); i++) {
										output << reduction_build_distribute_clause(i);
									}
								}
								if (distribute_allreduction_active) {
									while ((reduction_match = strstr(search_cursor, "reduction")) != NULL) {
										reduction_clause_text = "";
										search_cursor = reduction_match + 10;
										while (*search_cursor != ')') {
											reduction_clause_text += *search_cursor;
											search_cursor++;
										}
										pending_reduction_clauses.push_back(reduction_clause_text);
									}
									reduction_group_pending_vars_by_operator();
									pending_reduction_clauses.clear();
									for (long unsigned int i = 0; i < distribute_allreduction_operators.size(); i++) {
										output << allreduction_build_distribute_clause(i);
									}
								}

								grouped_reduction_vars.clear();
							}

							output << endl;
						}
					}
				}
				else{
					output << "#" << line << endl;
				}
              }
"/*"			{ scanner_skip_comment(); }
"//"[^\n]*              { }


"auto"			{ scanner_count_token(); return(AUTO); }
"_Bool"			{ scanner_count_token(); return(BOOL); }
"break"			{ scanner_count_token(); return(BREAK); }
"case"			{ scanner_count_token(); return(CASE); }
"char"			{ scanner_count_token(); return(CHAR); }
"_Complex"		{ scanner_count_token(); return(COMPLEX); }
"const"			{ scanner_count_token(); return(CONST); }
"continue"		{ scanner_count_token(); return(CONTINUE); }
"default"		{ scanner_count_token(); return(DEFAULT); }
"do"			{ scanner_count_token(); return(DO); }
"double"		{ scanner_count_token(); return(DOUBLE); }
"else"			{ scanner_count_token(); return(ELSE); }
"enum"			{ scanner_count_token(); return(ENUM); }
"extern"		{ scanner_count_token(); return(EXTERN); }
"float"			{ scanner_count_token(); return(FLOAT); }
"for"			{ scanner_count_token(); return(FOR); }
"goto"			{ scanner_count_token(); return(GOTO); }
"if"			{ scanner_count_token(); return(IF); }
"_Imaginary"	{ scanner_count_token(); return(IMAGINARY); }
"inline"		{ scanner_count_token(); return(INLINE); }
"int"			{ scanner_count_token(); return(INT); }
"long"			{ scanner_count_token(); return(LONG); }
"register"		{ scanner_count_token(); return(REGISTER); }
"restrict"		{ scanner_count_token(); return(RESTRICT); }
"return"		{ scanner_count_token(); return(RETURN); }
"short"			{ scanner_count_token(); return(SHORT); }
"signed"		{ scanner_count_token(); return(SIGNED); }
"sizeof"		{ scanner_count_token(); return(SIZEOF); }
"static"		{ scanner_count_token(); return(STATIC); }
"struct"		{ scanner_count_token(); return(STRUCT); }
"switch"		{ scanner_count_token(); return(SWITCH); }
"typedef"		{ scanner_count_token(); return(TYPEDEF); }
"union"			{ scanner_count_token(); return(UNION); }
"unsigned"		{ scanner_count_token(); return(UNSIGNED); }
"void"			{ scanner_count_token(); return(VOID); }
"volatile"		{ scanner_count_token(); return(VOLATILE); }
"while"			{ scanner_count_token(); return(WHILE); }

{L}({L}|{D})* {
	logFile << "Reading token: " << yytext << endl;
	scanner_count_token();
	symbol_info *symbol = table.get_symbol_info(yytext);
	if(symbol != NULL && symbol->is_type_symbol()){
		yylval.symbol = new symbol_info(yytext, (char *) yytext);
		return USER_DEFINED;
	}
	else if (symbol != NULL) {
		yylval.symbol = symbol;

		if (scanner_identifier_equals(yytext, "argc") == 1) {
			remember_main_arg(&main_argc_name, yytext);
		}
		if (scanner_identifier_equals(yytext, "argv") == 1) {
			remember_main_arg(&main_argv_name, yytext);
		}

		return IDENTIFIER;
	}
	else {
		symbol = new symbol_info(yytext, (char *)"IDENTIFIER");
		yylval.symbol = symbol;

		if (scanner_identifier_equals(yytext, "argc") == 1) {
			remember_main_arg(&main_argc_name, yytext);
		}
		if (scanner_identifier_equals(yytext, "argv") == 1) {
			remember_main_arg(&main_argv_name, yytext);
		}

		return IDENTIFIER;
	}
}

0[xX]{H}+{IS}? {
	scanner_count_token();
	yylval.symbol = new symbol_info(yytext, (char *)"CONSTANT");
	return(CONSTANT);
}

0[0-7]*{IS}? {
	scanner_count_token();
	yylval.symbol = new symbol_info(yytext, (char *)"CONSTANT");
	return(CONSTANT);
}

[1-9]{D}*{IS}? {
	scanner_count_token();
	yylval.symbol = new symbol_info(yytext, (char *)"CONSTANT");
	return(CONSTANT);
}

L?'(\\.|[^\\'\n])+' {
	scanner_count_token();
	yylval.symbol = new symbol_info(yytext, (char *)"CONSTANT");
	return(CONSTANT);
}

{D}+{E}{FS}? {
	scanner_count_token();
	yylval.symbol = new symbol_info(yytext, (char *)"CONSTANT");
	return(CONSTANT);
}

{D}*"."{D}+{E}?{FS}? {
	scanner_count_token();
	yylval.symbol = new symbol_info(yytext, (char *)"CONSTANT");
	return(CONSTANT);
}

{D}+"."{D}*{E}?{FS}? {
	scanner_count_token();
	yylval.symbol = new symbol_info(yytext, (char *)"CONSTANT");
	return(CONSTANT);
}

0[xX]{H}+{P}{FS}? {
	scanner_count_token();
	yylval.symbol = new symbol_info(yytext, (char *)"CONSTANT");
	return(CONSTANT);
}

0[xX]{H}*"."{H}+{P}{FS}? {
	scanner_count_token();
	yylval.symbol = new symbol_info(yytext, (char *)"CONSTANT");
	return(CONSTANT);
}

0[xX]{H}+"."{H}*{P}{FS}? {
	scanner_count_token();
	yylval.symbol = new symbol_info(yytext, (char *)"CONSTANT");
	return(CONSTANT);
}


L?\"(\\.|[^\\"\n])*\"	{
	scanner_count_token();

	symbol_info *literal_symbol = new symbol_info(yytext, (char *)"STRING_LITERAL");
	yylval.symbol = literal_symbol;

	return(STRING_LITERAL);
}

"..."			{ scanner_count_token(); return(ELLIPSIS); }
">>="			{ scanner_count_token(); return(RIGHT_ASSIGN); }
"<<="			{ scanner_count_token(); return(LEFT_ASSIGN); }
"+="			{ scanner_count_token(); return(ADD_ASSIGN); }
"-="			{ scanner_count_token(); return(SUB_ASSIGN); }
"*="			{ scanner_count_token(); return(MUL_ASSIGN); }
"/="			{ scanner_count_token(); return(DIV_ASSIGN); }
"%="			{ scanner_count_token(); return(MOD_ASSIGN); }
"&="			{ scanner_count_token(); return(AND_ASSIGN); }
"^="			{ scanner_count_token(); return(XOR_ASSIGN); }
"|="			{ scanner_count_token(); return(OR_ASSIGN); }
">>"			{ scanner_count_token(); return(RIGHT_OP); }
"<<"			{ scanner_count_token(); return(LEFT_OP); }
"++"			{ scanner_count_token(); return(INC_OP); }
"--"			{ scanner_count_token(); return(DEC_OP); }
"->"			{ scanner_count_token(); return(PTR_OP); }
"&&"			{ scanner_count_token(); return(AND_OP); }
"||"			{ scanner_count_token(); return(OR_OP); }
"<="			{ scanner_count_token(); return(LE_OP); }
">="			{ scanner_count_token(); return(GE_OP); }
"=="			{ scanner_count_token(); return(EQ_OP); }
"!="			{ scanner_count_token(); return(NE_OP); }

";"			{
				scanner_count_token();
				if(paren_depth == 0 && !cluster_stack_is_empty()){
					cluster_stack *c = cluster_stack_peek();
					if(c->kind == CL_ONE_STMT && c->close_state == CL_OPEN){
						c->close_state = CL_CLOSE_PENDING;
					}
				}
				return(';');
			}

("{"|"<%")	{
				scanner_count_token();
				if (!cluster_stack_is_empty()) {
					cluster_stack *c = cluster_stack_peek();

					if(c->kind == CL_ONE_STMT){
						c->kind = CL_BLOCK;
						c->brace_depth = 1;
						c->close_state = CL_OPEN;
					} else if(c->kind == CL_BLOCK){
						c->brace_depth++;
					}
				}
				distribute_note_open_brace();
				return('{');
			}

("}"|"%>")	{
				scanner_count_token();
				if (!cluster_stack_is_empty()) {
					cluster_stack *c = cluster_stack_peek();

					if(c->kind == CL_BLOCK && c->close_state == CL_OPEN){
						c->brace_depth--;
						if(c->brace_depth < 0){
							exit(EXIT_FAILURE);
						}
						if(c->brace_depth == 0){
							c->close_state = CL_CLOSE_PENDING;
						}
					}
				}
				distribute_note_close_brace();
				return('}');
			}
","			{ scanner_count_token(); return(','); }
":"			{ scanner_count_token(); return(':'); }
"="			{ scanner_count_token(); return('='); }
"("			{ scanner_count_token(); paren_depth++; return('('); }
")"			{ scanner_count_token(); if(paren_depth > 0) paren_depth--; return(')'); }
("["|"<:")	{ scanner_count_token(); return('['); }
("]"|":>")	{ scanner_count_token(); return(']'); }
"."			{ scanner_count_token(); return('.'); }
"&"			{ scanner_count_token(); return('&'); }
"!"			{ scanner_count_token(); return('!'); }
"~"			{ scanner_count_token(); return('~'); }
"-"			{ scanner_count_token(); return('-'); }
"+"			{ scanner_count_token(); return('+'); }
"*"			{ scanner_count_token(); return('*'); }
"/"			{ scanner_count_token(); return('/'); }
"%"			{ scanner_count_token(); return('%'); }
"<"			{ scanner_count_token(); return('<'); }
">"			{ scanner_count_token(); return('>'); }
"^"			{ scanner_count_token(); return('^'); }
"|"			{ scanner_count_token(); return('|'); }
"?"			{ scanner_count_token(); return('?'); }

[ \t\v\n\f]		{ scanner_count_token(); }
.			{ }

%%

/* Closes any pending cluster scopes when the scanner reaches EOF */
void flush_pending_clusters_at_eof(){
	while(!cluster_stack_is_empty()){
		cluster_stack *c = cluster_stack_peek();

		if(c != NULL && c->close_state == CL_CLOSE_PENDING){

            if(c->has_task_pragma){
                tasking_emit_finalize_region(c);
            } else {
                if(c->allgather_clause_active){
                    allgather_emit_all();
                }
                if (c->gather_clause_active){
                    gather_emit_all();
                }
                if (c->allreduction_clause_active){
                    allreduction_emit_final(true);
                }
                if(c->reduction_clause_active){
                    reduction_emit_final(true);
                }
            }

            if(main_scope_depth > 0){
                sequential_region_emit_open_if_needed();
            }

            cluster_stack_pop();
        }
	}
}

/* Finalizes scanner output and writes deferred MPI type declarations */
int yywrap(void)
{
	flush_pending_clusters_at_eof();
	writer_flush_current_line();

	if (mpi_runtime_initialized) {
		output << endl << mpi_type_declarations << "}";
	}

	return 1;
}


/* Reads one preprocessor pragma line from the input stream */
char * scanner_read_pragma(void)
{
        char c;
        char * line = (char *) malloc(512);
        int i=0;

        ECHO;

        while ((c = yyinput()) != 0 && (c != '\n' && c != '\r'))
        {
                if (yyout != NULL)
                        fputc(c, yyout);
                line[i++] = c;
        }
        if (yyout != NULL)
                fputc(c, yyout);
        line[i] = 0;
        return(line);
}

/* Consumes a C block comment while preserving scanner output */
void scanner_skip_comment(void)
{
	char c, prev = 0;

        ECHO;

	while ((c = yyinput()) != 0)
	{
		if (yyout != NULL)
			fputc(c, yyout);
		if (c == '/' && prev == '*')
			return;
		prev = c;
	}
	yyerror("unterminated comment");
}

/* Tracks one token, updates positions, and forwards it to the writer */
void scanner_count_token(void)
{
	int i;
	writer_set_token_text(yytext);
	writer_process_current_token();

	for (i = 0; yytext[i] != '\0'; i++) {
		if (yytext[i] == '\n'){
			line_count++;
			column = 0;
		}
		else if (yytext[i] == '\t')
			column += 8 - (column % 8);
		else
			column++;
	}

}

/* Compares identifiers using the scanner's case-insensitive rules */
static int scanner_identifier_equals(const char *actual, const char *expected) {
    string actual_text = actual != NULL ? actual : "";
    string expected_text = expected != NULL ? expected : "";
	int matches = 1;

	unsigned long int offset;

	for (offset = 0; offset < actual_text.size() && offset < expected_text.size(); offset++) {
		char actual_char = actual_text.at(offset);

		if (actual_char < 123 && actual_char > 64) {
			if (expected_text.at(offset) - actual_char != 0 && expected_text.at(offset) - actual_char != 32) {
				matches = 0;
				break;
			}
		}
		else if (actual_char != expected_text.at(offset)) {
			matches = 0;
			break;
		}
	}

	if (actual_text.size() - offset > 0 || expected_text.size() - offset > 0) {
		matches = 0;
	}

	return matches;
}


