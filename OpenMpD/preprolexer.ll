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
#include "SymbolTable.h"
#include <stdio.h>
#include <cstring>
#include <fstream>
#include "preproparser.hh"

extern void yyerror(const char *);

static void count(void);
static void comment(void);
void get_pragma(void);
// static int check_type(void);

extern ofstream logFile;
extern ofstream errFile;
extern SymbolTable table;

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
"#"           	{ get_pragma(); }
"/*"			{ comment(); }
"//"[^\n]*    	{ /* consume //-comment */ }

"auto"			{ count(); return(AUTO); }
"_Bool"			{ count(); return(BOOL); }
"break"			{ count(); return(BREAK); }
"case"			{ count(); return(CASE); }
"char"			{ count(); return(CHAR); }
"_Complex"		{ count(); return(COMPLEX); }
"const"			{ count(); return(CONST); }
"continue"		{ count(); return(CONTINUE); }
"default"		{ count(); return(DEFAULT); }
"do"			{ count(); return(DO); }
"double"		{ count(); return(DOUBLE); }
"else"			{ count(); return(ELSE); }
"enum"			{ count(); return(ENUM); }
"extern"		{ count(); return(EXTERN); }
"float"			{ count(); return(FLOAT); }
"for"			{ count(); return(FOR); }
"goto"			{ count(); return(GOTO); }
"if"			{ count(); return(IF); }
"_Imaginary"	{ count(); return(IMAGINARY); }
"inline"		{ count(); return(INLINE); }
"int"			{ count(); return(INT); }
"long"			{ count(); return(LONG); }
"register"		{ count(); return(REGISTER); }
"restrict"		{ count(); return(RESTRICT); }
"return"		{ count(); return(RETURN); }
"short"			{ count(); return(SHORT); }
"signed"		{ count(); return(SIGNED); }
"sizeof"		{ count(); return(SIZEOF); }
"static"		{ count(); return(STATIC); }
"struct"		{ count(); return(STRUCT); }
"switch"		{ count(); return(SWITCH); }
"typedef"		{ count(); logFile << "LEE TYPEDEF" << endl; return(TYPEDEF); }
"union"			{ count(); return(UNION); }
"unsigned"		{ count(); return(UNSIGNED); }
"void"			{ count(); return(VOID); }
"volatile"		{ count(); return(VOLATILE); }
"while"			{ count(); return(WHILE); }
"__extension__" { /* consume token */ }
"__restrict" 	{ /* consume token */ }
"__inline" 		{ /* consume token */ }
[\n" "\t]*"__attribute__"[\n" "\t]*"(("({L}({L}|{D})*|","|[" "\t]+|{D}+|"("|")"|"*")+"))" 	{ logFile << "En att" << endl; /* consume token */ }
[\n" "\t]*"__asm__"[\n" "\t]*"("({L}({L}|{D})*|","|[" "\t]+|{D}+|"("|")"|"\""|"*")+")" 		{ logFile << "En asm" << endl; /* consume token */ }

"("({WS}|{CMTB}|{CMTA})*"{" {
	stmexp_paren_n = 1;
	stmexp_brace_n = 1;
	BEGIN(STMEXP);

	count();
	SymbolInfo *s = new SymbolInfo("0", (char *)"CONSTANT");
	prepro_lval.sym = s;
	return(CONSTANT); 
}

<STMEXP>"/*"			{BEGIN(STMEXP_CMTA);}
<STMEXP_CMTA>[^*]+ 		{ /* consume */}
<STMEXP_CMTA>"*"+[^*/]	{ /* consume */}
<STMEXP_CMTA>"*/"		{BEGIN(STMEXP);}

<STMEXP>"//"			{BEGIN(STMEXP_CMTB);}
<STMEXP_CMTB>[^\n]* 	{ /* consume */}
<STMEXP_CMTB>\n			{n_lineas_prepro++; BEGIN(STMEXP);}

<STMEXP>\"				{BEGIN(STMEXP_STR);}
<STMEXP_STR>\\(.|\n) 	{ /* consume */}
<STMEXP_STR>[^\\\"\n]+	{ /* consume */}
<STMEXP_STR>\"			{BEGIN(STMEXP);}
<STMEXP_STR>\n			{n_lineas_prepro++;}

<STMEXP>\'				{BEGIN(STMEXP_CHR);}
<STMEXP_CHR>\\(.|\n) 	{ /* consume */}
<STMEXP_CHR>[^\\\'\n]+	{ /* consume */}
<STMEXP_CHR>\'			{BEGIN(STMEXP);}
<STMEXP_CHR>\n			{n_lineas_prepro++;}

<STMEXP>"("				{stmexp_paren_n++;}
<STMEXP>")"				{stmexp_paren_n--;}
<STMEXP>"{"				{stmexp_brace_n++;}
<STMEXP>"}"				{
							stmexp_brace_n--;
							BEGIN(STMEXP_MAYBE_END);
						}

<STMEXP_MAYBE_END>({WS}|{CMTB}|{CMTA})* 	{/* consume */}
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
<STMEXP>({WS}|{CMTB}|{CMTA})* 	{/* consume */}
<STMEXP>.						{/* consume */}

{L}({L}|{D})* {
	count();
	SymbolInfo *s = table.getSymbolInfo(yytext);
	if(s != NULL && s->getSymIsType()){
		prepro_lval.sym = new SymbolInfo(yytext, (char *) yytext);
		logFile << "LEYENDO COMO USER_DEFINED: " << yytext << " con tipo: " << s->getSymbolType() << endl;
		return USER_DEFINED;
	}
	else if (s != NULL) {
		prepro_lval.sym = s;
		logFile << "LEYENDO COMO IDENTIFIER: " << yytext << " con tipo: " << s->getSymbolType() << endl;
		return IDENTIFIER;
	}
	else {
		prepro_lval.sym = new SymbolInfo(yytext, (char *)"IDENTIFIER");
		logFile << "LEYENDO COMO IDENTIFIER: " << yytext << " con tipo: " << prepro_lval.sym->getSymbolType() << endl;
		return IDENTIFIER;
	}
}

0[xX]{H}+{IS}?		{ count(); return(CONSTANT); }
0[0-7]*{IS}?		{ count(); return(CONSTANT); }
[1-9]{D}*{IS}?		{ 
	count();
	SymbolInfo *s = new SymbolInfo(yytext, (char *)"CONSTANT");
	prepro_lval.sym = s;
	return(CONSTANT); 
}
L?'(\\.|[^\\'\n])+'	{ count(); return(CONSTANT); }

{D}+{E}"i"?{FS}?				{ count(); return(CONSTANT); }
{D}*"."{D}+{E}?"i"?{FS}?		{ count(); return(CONSTANT); }
{D}+"."{D}*{E}?"i"?{FS}?		{ count(); return(CONSTANT); }
0[xX]{H}+{P}"i"?{FS}?			{ count(); return(CONSTANT); }
0[xX]{H}*"."{H}+{P}"i"?{FS}?    { count(); return(CONSTANT); }
0[xX]{H}+"."{H}*{P}"i"?{FS}?    { count(); return(CONSTANT); }

L?\"(\\.|[^\\"\n])*\"	{ count(); return(STRING_LITERAL); }

"..."			{ count(); return(ELLIPSIS); }
">>="			{ count(); return(RIGHT_ASSIGN); }
"<<="			{ count(); return(LEFT_ASSIGN); }
"+="			{ count(); return(ADD_ASSIGN); }
"-="			{ count(); return(SUB_ASSIGN); }
"*="			{ count(); return(MUL_ASSIGN); }
"/="			{ count(); return(DIV_ASSIGN); }
"%="			{ count(); return(MOD_ASSIGN); }
"&="			{ count(); return(AND_ASSIGN); }
"^="			{ count(); return(XOR_ASSIGN); }
"|="			{ count(); return(OR_ASSIGN); }
">>"			{ count(); return(RIGHT_OP); }
"<<"			{ count(); return(LEFT_OP); }
"++"			{ count(); return(INC_OP); }
"--"			{ count(); return(DEC_OP); }
"->"			{ count(); return(PTR_OP); }
"&&"			{ count(); return(AND_OP); }
"||"			{ count(); return(OR_OP); }
"<="			{ count(); return(LE_OP); }
">="			{ count(); return(GE_OP); }
"=="			{ count(); return(EQ_OP); }
"!="			{ count(); return(NE_OP); }
";"				{ count(); return(';'); }
("{"|"<%")		{ count(); return('{'); }
("}"|"%>")		{ count(); return('}'); }
","				{ count(); return(','); }
":"				{ count(); return(':'); }
"="				{ count(); return('='); }
"("				{ count(); return('('); }
")"				{ count(); return(')'); }
("["|"<:")		{ count(); return('['); }
("]"|":>")		{ count(); return(']'); }
"."				{ count(); return('.'); }
"&"				{ count(); return('&'); }
"!"				{ count(); return('!'); }
"~"				{ count(); return('~'); }
"-"				{ count(); return('-'); }
"+"				{ count(); return('+'); }
"*"				{ count(); return('*'); }
"/"				{ count(); return('/'); }
"%"				{ count(); return('%'); }
"<"				{ count(); return('<'); }
">"				{ count(); return('>'); }
"^"				{ count(); return('^'); }
"|"				{ count(); return('|'); }
"?"				{ count(); return('?'); }

[ \t\v\n\f]		{ count(); if (strstr(yytext, "\n") != NULL) {n_lineas_prepro++;} }
.				{ /* Add code to complain about unmatched characters */ }

%%

int yywrap() {
	logFile << "Finalizando lectura de preprocesador" << endl;
	return 1;
}


void get_pragma(void)
{
	char c;

    while ((c = yyinput()) != 0 && (c != '\n' && c != '\r')) {}
}

void comment(void)
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

void count(void)
{
}

/* int check_type(void)
{ */
/*
* pseudo code --- this is what it should check
*
*	if ((yyget_text(yyscanner)) == type_name)
*		return TYPE_NAME;
*
*	return IDENTIFIER;
*/

/*
*	it actually will only return IDENTIFIER
*/

	/* return IDENTIFIER;
} */
