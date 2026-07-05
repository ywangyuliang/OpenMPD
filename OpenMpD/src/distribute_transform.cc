#include "distribute_transform.h"
#include "translator_state.h"
#include "halo_transform.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>

using namespace std;

extern ofstream output;

int clause_forwarding_active = 0;

static translator_distribute_state_t *distribute_state(void)
{
    return &translator_context_get()->distribute;
}

static bool line_has_content(const char *line)
{
    if(line == NULL){
        return false;
    }

    for(unsigned long int i = 0; i < strlen(line); i++){
        if(line[i] != ' ' && line[i] != '\t' && line[i] != '\n'){
            return true;
        }
    }

    return false;
}

void distribute_begin(void)
{
    translator_distribute_state_t *state = distribute_state();

    state->active = 1;
}

void distribute_begin_extended(int extended_mode, const char *pending_pragma)
{
    translator_distribute_state_t *state = distribute_state();

    distribute_begin();
    state->extended_mode = extended_mode;

    if(pending_pragma != NULL){
        state->pending_lines += pending_pragma;
    }
}

void distribute_end_extended(void)
{
    distribute_state()->extended_mode = 0;
}

int distribute_is_active(void)
{
    return distribute_state()->active;
}

int distribute_extended_mode(void)
{
    return distribute_state()->extended_mode;
}

/*
 * Extended-distribute kinds recorded by distribute_begin_extended():
 *     1  distribute simd, and every "teams distribute ..." form
 *     2  distribute parallel for
 *     3  distribute parallel do
 *     4  distribute parallel for simd
 *     5  distribute parallel do simd
 *   The predicates below decide which OpenMP clauses the parser forwards into
 *   the generated worksharing pragma for each kind
 */

int distribute_extended_is_active(void)
{
    return distribute_extended_mode() != 0;
}

int distribute_extended_accepts_simd_clauses(void)
{
    int mode = distribute_extended_mode();

    return mode == 1 || mode == 4 || mode == 5;
}

int distribute_extended_accepts_parallel_clauses(void)
{
    return distribute_extended_mode() > 1;
}

int distribute_extended_is_parallel_without_simd(void)
{
    int mode = distribute_extended_mode();

    return mode == 2 || mode == 3;
}

int distribute_extended_is_parallel_simd(void)
{
    int mode = distribute_extended_mode();

    return mode == 4 || mode == 5;
}

int distribute_extended_is_parallel_for(void)
{
    int mode = distribute_extended_mode();

    return mode == 2 || mode == 4;
}

int distribute_should_buffer_line(void)
{
    translator_distribute_state_t *state = distribute_state();

    return distribute_is_active() && state->for_state == 0;
}

void distribute_start_body_tracking(void)
{
    distribute_state()->brace_count = 0;
}

void distribute_note_open_brace(void)
{
    translator_distribute_state_t *state = distribute_state();

    if(distribute_is_active()){
        state->brace_count++;
    }
}

void distribute_note_close_brace(void)
{
    translator_distribute_state_t *state = distribute_state();

    if(!distribute_is_active()){
        return;
    }

    state->brace_count--;
    if(state->brace_count < 0){
        fprintf(stderr, "Error: unbalanced closing brace inside distribute region\n");
        exit(EXIT_FAILURE);
    }
    if(state->brace_count == 0){
        state->brace_count = DISTRIBUTE_BRACE_CLOSED;
    }
}

void distribute_note_token(const char *token)
{
    translator_distribute_state_t *state = distribute_state();

    if(distribute_is_active() && token != NULL && strcmp(token, "for") == 0 && state->for_state == 0){
        state->for_state = 1;
    }
}

void distribute_rewrite_loop_variable_token(char **token)
{
    translator_distribute_state_t *state = distribute_state();
    char *new_token;

    if(!distribute_is_active() || state->for_state != -1 || token == NULL || *token == NULL){
        return;
    }

    if(state->loop_variable.empty() || strcmp(*token, state->loop_variable.c_str()) != 0){
        return;
    }

    new_token = (char *)realloc(*token, 10);
    if(new_token == NULL){
        perror("realloc");
        exit(EXIT_FAILURE);
    }

    strcpy(new_token, "__distrib");
    *token = new_token;
}

void distribute_finish_single_line_if_needed(const char *line)
{
    translator_distribute_state_t *state = distribute_state();

    if(state->for_state == -1 && state->brace_count == 0 && line != NULL){
        if(line_has_content(line) &&
           strstr(line, "for") == NULL &&
           strstr(line, "while") == NULL &&
           strstr(line, "if") == NULL &&
           strstr(line, "switch") == NULL){
            state->brace_count = DISTRIBUTE_BRACE_CLOSED;
        }
    }
}

int distribute_should_rewrite_loop(void)
{
    return distribute_state()->for_state == 1;
}

void distribute_prepare_loop_rewrite(void)
{
    translator_distribute_state_t *state = distribute_state();

    if(state->brace_count == 0){
        state->insert_for_braces = 1;
    }
}

void distribute_mark_loop_rewritten(void)
{
    distribute_state()->for_state = -1;
}

std::string &distribute_pending_lines(void)
{
    return distribute_state()->pending_lines;
}

std::string distribute_take_pending_lines(void)
{
    translator_distribute_state_t *state = distribute_state();
    std::string pending = state->pending_lines;

    state->pending_lines.clear();
    return pending;
}

int distribute_has_schedule(void)
{
    return !distribute_state()->schedule_expr.empty();
}

const std::string &distribute_schedule_expr(void)
{
    return distribute_state()->schedule_expr;
}

void distribute_set_schedule_expr(const char *expr)
{
    translator_distribute_state_t *state = distribute_state();

    state->schedule_expr = expr ? expr : "";
}

void distribute_clear_schedule(void)
{
    distribute_state()->schedule_expr.clear();
}

int distribute_should_close(void)
{
    return distribute_is_active() && distribute_state()->brace_count == DISTRIBUTE_BRACE_CLOSED;
}

int distribute_inserted_loop_brace(void)
{
    return distribute_state()->insert_for_braces;
}

void distribute_reset_after_close(void)
{
    translator_distribute_state_t *state = distribute_state();

    state->active = 0;
    state->for_state = 0;
    state->insert_for_braces = 0;
    state->loop_variable.clear();
}

/* Returns how many extra chars to skip if text starts with a type keyword
   followed by whitespace (keyword length minus the current char), or -1 */
static int distribute_type_keyword_skip(const char *text)
{
    static const char *keywords[] = {
        "int", "long", "short", "char", "float", "double",
        "register", "signed", "unsigned", "static", "const"
    };

    for(unsigned long int i = 0; i < sizeof(keywords) / sizeof(keywords[0]); i++){
        size_t length = strlen(keywords[i]);
        if(strncmp(text, keywords[i], length) == 0 && (text[length] == ' ' || text[length] == '\t')){
            return (int)length - 1;
        }
    }

    return -1;
}

/* Replaces the first occurrence of the loop variable in the increment
   expression with the generated __distrib index */
static string distribute_increment_with_distrib(const string &increment_expr, const string &loop_variable)
{
    size_t pos = increment_expr.find(loop_variable);

    if(loop_variable.empty() || pos == string::npos){
        return increment_expr;
    }

    return increment_expr.substr(0, pos) + "__distrib" + increment_expr.substr(pos + loop_variable.size());
}

/* Guard that skips iterations landing outside the local block, per operator */
static string distribute_wraparound_guard(const string &comparison_operator)
{
    if(comparison_operator == "<"){
        return "if(__distrib >= __end) {continue;}";
    }
    if(comparison_operator == ">"){
        return "if(__distrib <= __end) {continue;}";
    }
    if(comparison_operator == "<="){
        return "if(__distrib > __end) {continue;}";
    }
    return "if(__distrib < __end) {continue;}";
}

void distribute_rewrite_loop_header(string *start_expr, string *end_expr, char **line)
{
    translator_distribute_state_t *state = distribute_state();
    int parse_state = 0;
    int skip_chars = 0;
    int capture_after_operator = 0;
    int capture_start_from_condition = 0;
    string initializer_lhs = "";
    string condition_lhs = "";
    string increment_expr = "";
    string comparison_operator;

    for(int i = 0; (*line)[i]; i++){
        switch(parse_state) {
            case 0: if((*line)[i] == '('){parse_state = 1;}
                    break;

            case 1: if(skip_chars > 0){skip_chars--; continue;}
                    if((*line)[i] == ' ' || (*line)[i] == '\t'){continue;}
                    if((*line)[i] == ';'){
                        if(!capture_after_operator){
                            *start_expr += initializer_lhs;
                        }
                        if(*start_expr == ""){
                            capture_start_from_condition = 1;
                        }
                        capture_after_operator = 0;
                        parse_state = 2;
                        continue;
                    }

                    if((skip_chars = distribute_type_keyword_skip((*line) + i)) >= 0){continue;}
                    skip_chars = 0;

                    if((*line)[i] == '='){capture_after_operator = 1; continue;}

                    if(capture_after_operator){*start_expr += (*line)[i];}
                    else{initializer_lhs += (*line)[i];}
                    break;

            case 2: if(skip_chars > 0){cout << "Loop declarations in the condition are not supported in distribute" << endl; exit(EXIT_FAILURE);}
                    if((*line)[i] == ' ' || (*line)[i] == '\t'){continue;}
                    if((*line)[i] == ';'){
                        if(!capture_after_operator){
                            cout << "This for-loop form is not supported in distribute" << endl; exit(EXIT_FAILURE);
                        }
                        if(*end_expr == ""){
                            cout << "Missing distribute loop stop condition" << endl;
                            exit(EXIT_FAILURE);
                        }
                        if(*start_expr == ""){
                            cout << "Could not determine the distribute loop start" << endl;
                            exit(EXIT_FAILURE);
                        }
                        parse_state = 3;
                        continue;
                    }

                    if((skip_chars = distribute_type_keyword_skip((*line) + i)) >= 0){continue;}
                    skip_chars = 0;

                    if((*line)[i] == '<' && (*line)[i + 1] == '='){capture_after_operator = 1; i++; capture_start_from_condition = 0; comparison_operator = "<="; continue;}
                    else if((*line)[i] == '<'){capture_after_operator = 1; capture_start_from_condition = 0; comparison_operator = "<"; continue;}
                    if((*line)[i] == '>' && (*line)[i + 1] == '='){capture_after_operator = 1; i++; capture_start_from_condition = 0; comparison_operator = ">="; continue;}
                    else if((*line)[i] == '>'){capture_after_operator = 1; capture_start_from_condition = 0; comparison_operator = ">"; continue;}

                    if((*line)[i] == '=' && (*line)[i + 1] == '='){cout << "The == operator is not supported in distribute" << endl; exit(EXIT_FAILURE);}
                    else if((*line)[i] == '='){cout << "Loop declarations in the condition are not supported in distribute" << endl; exit(EXIT_FAILURE);}
                    if((*line)[i] == '!' && (*line)[i + 1] == '='){cout << "The != operator is not supported in distribute" << endl; exit(EXIT_FAILURE);}

                    if(capture_after_operator){*end_expr += (*line)[i];}
                    else{condition_lhs += (*line)[i];}

                    if(capture_start_from_condition){*start_expr += (*line)[i];}
                    break;

            case 3: if((*line)[i] == ' ' || (*line)[i] == '\t'){continue;}
                    increment_expr += (*line)[i];
                    break;
        }
    }

    state->loop_variable = condition_lhs;

    string increment_rewritten = distribute_increment_with_distrib(increment_expr, state->loop_variable);
    string rewritten;

    if(!state->schedule_expr.empty()){
        string step_sign = (comparison_operator.find('<') != string::npos) ? "+ " : "- ";

        rewritten = "for (int __distrib = __distribSched; __distrib " + comparison_operator +
                    " __distribSched " + step_sign + state->schedule_expr + "; " + increment_rewritten;
        if(state->insert_for_braces){
            rewritten += "{";
        }
        rewritten += distribute_wraparound_guard(comparison_operator);

        state->pending_lines += "\tfor (int __distribSched = __start; __distribSched " + comparison_operator +
                                " __end; __distribSched ";
        state->pending_lines += (comparison_operator.find('<') != string::npos) ? "+= __iter" : "-= __iter";
        state->pending_lines += ") {\n";
    }
    else{
        rewritten = "for (int __distrib = __start; __distrib " + comparison_operator + " __end; " + increment_rewritten;
    }

    char *new_line = (char *)realloc(*line, rewritten.size() + 1);
    if(new_line == NULL){
        perror("realloc");
        exit(EXIT_FAILURE);
    }
    memcpy(new_line, rewritten.c_str(), rewritten.size() + 1);
    *line = new_line;
}

void distribute_emit_mpi_worksharing(string start_expr, string end_expr) {
    string distribute;

    halo_set_last_distribute_bounds(start_expr.c_str(), end_expr.c_str());

    if (distribute_has_schedule()) {
        const std::string &schedule_expr = distribute_schedule_expr();
        distribute =
            "{\nint __iter;\nint __start;\nint __end;\n__iter = __numprocs * " + schedule_expr + ";\n" +
            "__start = " + start_expr + " + __taskid * " + schedule_expr + ";\n" +
            "__end = " + end_expr + ";\n";
    }
    else {
        distribute =
            "{\nint __iter;\nint __start;\nint __end;\n__iter = (((" + end_expr + ") - (" + start_expr + ")) / __numprocs);\n" +
            "if (__taskid < (((" + end_expr + ") - (" + start_expr + ")) % __numprocs))\n" +
            "\t__iter++;\n" +
            "__start = ((" + start_expr + ") + __iter * __taskid);\n" +
            "if (__taskid >= (((" + end_expr + ") - (" + start_expr + ")) % __numprocs))\n" +
            "\t__start += (((" + end_expr + ") - (" + start_expr + ")) % __numprocs);\n" +
            "__end = __start + __iter;\nif (__taskid == (__numprocs - 1)) assert(__end == (" + end_expr + "));\n";
    }
    
    output << distribute << endl;
}
