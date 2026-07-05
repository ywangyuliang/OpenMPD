D			[0-9]
L			[a-zA-Z_]
H			[a-fA-F0-9]
E			([Ee][+-]?{D}+)
P           ([Pp][+-]?{D}+)
FS			(f|F|l|L)
IS          ((u|U)|(u|U)?(l|L|ll|LL)|(l|L|ll|LL)(u|U))

WS			[ \t\r]+
CMTA		"//"[^\n]*
CMTB		"/\*"([^*]|\*+[^*/])*\*+"/"

%{
#include<bits/stdc++.h>
#include "symbol_table.h"
#include <stdio.h>
#include <cstring>
#include <fstream>
#include "preproparser.hh"

extern void yyerror(const char *);

static void preprocessor_count_token(void);
static void preprocessor_skip_block_comment(void);
static void consume_attribute(void);
void preprocessor_skip_directive(void);

extern ofstream logFile;
extern ofstream errFile;
extern symbol_table table;

using namespace std;
int n_lineas_prepro = 1;
extern int num_errores_prepro;

extern PREPRO_STYPE prepro_lval;

static int stmexp_paren_n = 0;
static int stmexp_brace_n = 0;

%}

%option nounput
%option prefix="prepro_"
%option outfile="preprolexer.cc"

%option yylineno

%x STMEXP
%x STMEXP_MAYBE_END
%x STMEXP_STR
%x STMEXP_CHR
%x STMEXP_CMTA
%x STMEXP_CMTB

%%
"#"           	{ preprocessor_skip_directive(); }
"/*"			{ preprocessor_skip_block_comment(); }
"//"[^\n]*    	{ }

"auto"			{ preprocessor_count_token(); return(AUTO); }
"_Bool"			{ preprocessor_count_token(); return(BOOL); }
"break"			{ preprocessor_count_token(); return(BREAK); }
"case"			{ preprocessor_count_token(); return(CASE); }
"char"			{ preprocessor_count_token(); return(CHAR); }
"_Complex"		{ preprocessor_count_token(); return(COMPLEX); }
"const"			{ preprocessor_count_token(); return(CONST); }
"continue"		{ preprocessor_count_token(); return(CONTINUE); }
"default"		{ preprocessor_count_token(); return(DEFAULT); }
"do"			{ preprocessor_count_token(); return(DO); }
"double"		{ preprocessor_count_token(); return(DOUBLE); }
"else"			{ preprocessor_count_token(); return(ELSE); }
"enum"			{ preprocessor_count_token(); return(ENUM); }
"extern"		{ preprocessor_count_token(); return(EXTERN); }
"float"			{ preprocessor_count_token(); return(FLOAT); }
"for"			{ preprocessor_count_token(); return(FOR); }
"goto"			{ preprocessor_count_token(); return(GOTO); }
"if"			{ preprocessor_count_token(); return(IF); }
"_Imaginary"	{ preprocessor_count_token(); return(IMAGINARY); }
"inline"		{ preprocessor_count_token(); return(INLINE); }
"int"			{ preprocessor_count_token(); return(INT); }
"long"			{ preprocessor_count_token(); return(LONG); }
"register"		{ preprocessor_count_token(); return(REGISTER); }
"restrict"		{ preprocessor_count_token(); return(RESTRICT); }
"return"		{ preprocessor_count_token(); return(RETURN); }
"short"			{ preprocessor_count_token(); return(SHORT); }
"signed"		{ preprocessor_count_token(); return(SIGNED); }
"sizeof"		{ preprocessor_count_token(); return(SIZEOF); }
"static"		{ preprocessor_count_token(); return(STATIC); }
"struct"		{ preprocessor_count_token(); return(STRUCT); }
"switch"		{ preprocessor_count_token(); return(SWITCH); }
"typedef"		{ preprocessor_count_token(); logFile << "Reading TYPEDEF token" << endl; return(TYPEDEF); }
"union"			{ preprocessor_count_token(); return(UNION); }
"unsigned"		{ preprocessor_count_token(); return(UNSIGNED); }
"void"			{ preprocessor_count_token(); return(VOID); }
"volatile"		{ preprocessor_count_token(); return(VOLATILE); }
"while"			{ preprocessor_count_token(); return(WHILE); }
"__extension__" { }
"__restrict" 	{ }
"__inline" 		{ }
"__attribute__"	{ logFile << "Reading compiler attribute" << endl; consume_attribute(); }
[\n" "\t]*"__asm__"[\n" "\t]*"("({L}({L}|{D})*|","|[" "\t]+|{D}+|"("|")"|"\""|"*")+")" 		{ logFile << "Reading asm block" << endl; }

"("({WS}|{CMTB}|{CMTA})*"{" {
	stmexp_paren_n = 1;
	stmexp_brace_n = 1;
	BEGIN(STMEXP);

	preprocessor_count_token();
	symbol_info *constant_symbol = new symbol_info("0", (char *)"CONSTANT");
	prepro_lval.symbol = constant_symbol;
	return(CONSTANT);
}

<STMEXP>"/*"			{BEGIN(STMEXP_CMTA);}
<STMEXP_CMTA>[^*]+ 		{ }
<STMEXP_CMTA>"*"+[^*/]	{ }
<STMEXP_CMTA>"*/"		{BEGIN(STMEXP);}

<STMEXP>"//"			{BEGIN(STMEXP_CMTB);}
<STMEXP_CMTB>[^\n]* 	{ }
<STMEXP_CMTB>\n			{n_lineas_prepro++; BEGIN(STMEXP);}

<STMEXP>\"				{BEGIN(STMEXP_STR);}
<STMEXP_STR>\\(.|\n) 	{ }
<STMEXP_STR>[^\\\"\n]+	{ }
<STMEXP_STR>\"			{BEGIN(STMEXP);}
<STMEXP_STR>\n			{n_lineas_prepro++;}

<STMEXP>\'				{BEGIN(STMEXP_CHR);}
<STMEXP_CHR>\\(.|\n) 	{ }
<STMEXP_CHR>[^\\\'\n]+	{ }
<STMEXP_CHR>\'			{BEGIN(STMEXP);}
<STMEXP_CHR>\n			{n_lineas_prepro++;}

<STMEXP>"("				{stmexp_paren_n++;}
<STMEXP>")"				{stmexp_paren_n--;}
<STMEXP>"{"				{stmexp_brace_n++;}
<STMEXP>"}"				{
							stmexp_brace_n--;
							BEGIN(STMEXP_MAYBE_END);
						}

<STMEXP_MAYBE_END>({WS}|{CMTB}|{CMTA})* 	{ }
<STMEXP_MAYBE_END>\n						{n_lineas_prepro++;}
<STMEXP_MAYBE_END>")"						{
												stmexp_paren_n--;
												if(stmexp_paren_n == 0 && stmexp_brace_n == 0){
													BEGIN(INITIAL);
												} else {
													BEGIN(STMEXP);
												}
											}
<STMEXP_MAYBE_END>.							{BEGIN(STMEXP);}

<STMEXP>\n						{n_lineas_prepro++;}
<STMEXP>({WS}|{CMTB}|{CMTA})* 	{ }
<STMEXP>.						{ }

{L}({L}|{D})* {
	preprocessor_count_token();
	symbol_info *symbol = table.get_symbol_info(yytext);
	if(symbol != NULL && symbol->is_type_symbol()){
		prepro_lval.symbol = new symbol_info(yytext, (char *) yytext);
		logFile << "Reading USER_DEFINED token: " << yytext << " with type: " << symbol->get_symbol_type() << endl;
		return USER_DEFINED;
	}
	else if (symbol != NULL) {
		prepro_lval.symbol = symbol;
		logFile << "Reading IDENTIFIER token: " << yytext << " with type: " << symbol->get_symbol_type() << endl;
		return IDENTIFIER;
	}
	else {
		prepro_lval.symbol = new symbol_info(yytext, (char *)"IDENTIFIER");
		logFile << "Reading IDENTIFIER token: " << yytext << " with type: " << prepro_lval.symbol->get_symbol_type() << endl;
		return IDENTIFIER;
	}
}

0[xX]{H}+{IS}?		{ preprocessor_count_token(); return(CONSTANT); }
0[0-7]*{IS}?		{ preprocessor_count_token(); return(CONSTANT); }
[1-9]{D}*{IS}?		{
	preprocessor_count_token();
	symbol_info *symbol = new symbol_info(yytext, (char *)"CONSTANT");
	prepro_lval.symbol = symbol;
	return(CONSTANT);
}
L?'(\\.|[^\\'\n])+'	{ preprocessor_count_token(); return(CONSTANT); }

{D}+{E}"i"?{FS}?				{ preprocessor_count_token(); return(CONSTANT); }
{D}*"."{D}+{E}?"i"?{FS}?		{ preprocessor_count_token(); return(CONSTANT); }
{D}+"."{D}*{E}?"i"?{FS}?		{ preprocessor_count_token(); return(CONSTANT); }
0[xX]{H}+{P}"i"?{FS}?			{ preprocessor_count_token(); return(CONSTANT); }
0[xX]{H}*"."{H}+{P}"i"?{FS}?    { preprocessor_count_token(); return(CONSTANT); }
0[xX]{H}+"."{H}*{P}"i"?{FS}?    { preprocessor_count_token(); return(CONSTANT); }

L?\"(\\.|[^\\"\n])*\"	{ preprocessor_count_token(); return(STRING_LITERAL); }

"..."			{ preprocessor_count_token(); return(ELLIPSIS); }
">>="			{ preprocessor_count_token(); return(RIGHT_ASSIGN); }
"<<="			{ preprocessor_count_token(); return(LEFT_ASSIGN); }
"+="			{ preprocessor_count_token(); return(ADD_ASSIGN); }
"-="			{ preprocessor_count_token(); return(SUB_ASSIGN); }
"*="			{ preprocessor_count_token(); return(MUL_ASSIGN); }
"/="			{ preprocessor_count_token(); return(DIV_ASSIGN); }
"%="			{ preprocessor_count_token(); return(MOD_ASSIGN); }
"&="			{ preprocessor_count_token(); return(AND_ASSIGN); }
"^="			{ preprocessor_count_token(); return(XOR_ASSIGN); }
"|="			{ preprocessor_count_token(); return(OR_ASSIGN); }
">>"			{ preprocessor_count_token(); return(RIGHT_OP); }
"<<"			{ preprocessor_count_token(); return(LEFT_OP); }
"++"			{ preprocessor_count_token(); return(INC_OP); }
"--"			{ preprocessor_count_token(); return(DEC_OP); }
"->"			{ preprocessor_count_token(); return(PTR_OP); }
"&&"			{ preprocessor_count_token(); return(AND_OP); }
"||"			{ preprocessor_count_token(); return(OR_OP); }
"<="			{ preprocessor_count_token(); return(LE_OP); }
">="			{ preprocessor_count_token(); return(GE_OP); }
"=="			{ preprocessor_count_token(); return(EQ_OP); }
"!="			{ preprocessor_count_token(); return(NE_OP); }
";"				{ preprocessor_count_token(); return(';'); }
("{"|"<%")		{ preprocessor_count_token(); return('{'); }
("}"|"%>")		{ preprocessor_count_token(); return('}'); }
","				{ preprocessor_count_token(); return(','); }
":"				{ preprocessor_count_token(); return(':'); }
"="				{ preprocessor_count_token(); return('='); }
"("				{ preprocessor_count_token(); return('('); }
")"				{ preprocessor_count_token(); return(')'); }
("["|"<:")		{ preprocessor_count_token(); return('['); }
("]"|":>")		{ preprocessor_count_token(); return(']'); }
"."				{ preprocessor_count_token(); return('.'); }
"&"				{ preprocessor_count_token(); return('&'); }
"!"				{ preprocessor_count_token(); return('!'); }
"~"				{ preprocessor_count_token(); return('~'); }
"-"				{ preprocessor_count_token(); return('-'); }
"+"				{ preprocessor_count_token(); return('+'); }
"*"				{ preprocessor_count_token(); return('*'); }
"/"				{ preprocessor_count_token(); return('/'); }
"%"				{ preprocessor_count_token(); return('%'); }
"<"				{ preprocessor_count_token(); return('<'); }
">"				{ preprocessor_count_token(); return('>'); }
"^"				{ preprocessor_count_token(); return('^'); }
"|"				{ preprocessor_count_token(); return('|'); }
"?"				{ preprocessor_count_token(); return('?'); }

[ \t\v\n\f]		{ preprocessor_count_token(); if (strstr(yytext, "\n") != NULL) {n_lineas_prepro++;} }
.				{ }

%%

/* Ends the preprocessor lexer scan */
int yywrap() {
	logFile << "Finished reading preprocessor" << endl;
	return 1;
}


/* Consumes the rest of one preprocessor directive */
void preprocessor_skip_directive(void)
{
	char c;

    while ((c = yyinput()) != 0 && (c != '\n' && c != '\r')) {}
}

/* Consumes a preprocessor block comment */
void preprocessor_skip_block_comment(void)
{
	char c, prev = 0;

	while ((c = yyinput()) != 0)      /* (EOF maps to 0) */
	{
		if (c == '/' && prev == '*')
			return;
		prev = c;
	}
	yyerror("unterminated comment");
}

/* Consumes a compiler attribute argument list */
void consume_attribute(void)
{
	char c;
	int depth = 0;
	int seen_paren = 0;

	while ((c = yyinput()) != 0) {
		if (c == '\n') {
			n_lineas_prepro++;
		}

		if (!seen_paren) {
			if (c == '(') {
				seen_paren = 1;
				depth = 1;
			}
			else if (c != ' ' && c != '\t' && c != '\r' && c != '\n') {
				return;
			}
		}
		else {
			if (c == '(') {
				depth++;
			}
			else if (c == ')') {
				depth--;
				if (depth == 0) {
					return;
				}
			}
		}
	}
}

/* Counts one preprocessor token */
void preprocessor_count_token(void)
{
}
