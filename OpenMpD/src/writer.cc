#include "writer.h"
#include "tasking_region.h"
#include "tasking_emit.h"
#include "halo_transform.h"
#include "distribute_transform.h"
#include "reduction_transform.h"
#include "scatter_gather_transform.h"
#include "mpi_lifecycle.h"
#include "mpi_type_transform.h"
#include "translator_state.h"

#include <cstdlib>

static int writer_statement_output_started = 0;
static int writer_sequential_region_closed = 0;
static int writer_translation_finalized = 0;

static char *writer_current_token = NULL;
static char *writer_current_line = NULL;

extern int tasking_parse_is_capture_active(void);

extern string mpi_type_declarations;
extern int function_call_seen;
extern int sequential_declaration_pending;
extern int main_scope_depth;
extern int function_scope_depth;
extern int sequential_region_active;
extern std::string halo_build_pending_code();
extern void halo_clear_declared();
extern void distribute_emit_mpi_worksharing(string start_expr, string end_expr);
extern int use_ompd_runtime_main;

static char *duplicate_token_text(const char *text);
static void append_token_to_line(const char *text);
static void close_pending_cluster_if_needed(cluster_stack *c);
static int writer_is_task_body_capture_active(void);
static void writer_clear_current_line(void);
static void writer_handle_token_text(void);
static void writer_finish_single_line_distribute_if_needed(void);
static void writer_rewrite_distribute_loop_if_needed(void);
static void writer_open_sequential_guard_if_needed(void);
static int writer_capture_tasking_normal_block_if_needed(void);
static void writer_emit_or_buffer_current_line(void);
static void writer_update_master_state(void);
static void writer_capture_declaration_line(void);
static void writer_emit_num_teams_if_needed(void);
static void writer_close_distribute_if_needed(void);
static void writer_handle_completed_source_line(void);

void writer_mark_statement_output_started(void)
{
    writer_statement_output_started = 1;
}

void writer_mark_sequential_region_closed(void)
{
    writer_sequential_region_closed = 1;
}

int writer_is_sequential_region_closed(void)
{
    return writer_sequential_region_closed;
}

void writer_mark_translation_finalized(void)
{
    writer_translation_finalized = 1;
}

static char *duplicate_token_text(const char *text)
{
    char *copy;

    if(text == NULL){
        return NULL;
    }

    copy = strdup(text);
    if(copy == NULL){
        perror("strdup");
        exit(EXIT_FAILURE);
    }

    return copy;
}

static void append_token_to_line(const char *text)
{
    size_t current_len;
    size_t text_len;
    char *new_line;

    if(text == NULL){
        return;
    }

    current_len = writer_current_line ? strlen(writer_current_line) : 0;
    text_len = strlen(text);

    new_line = (char *)realloc(writer_current_line, current_len + text_len + 1);
    if(new_line == NULL){
        perror("realloc");
        exit(EXIT_FAILURE);
    }

    memcpy(new_line + current_len, text, text_len + 1);
    writer_current_line = new_line;
}

static void close_pending_cluster_if_needed(cluster_stack *c)
{
    int emit_captured_block_close;

    if(c == NULL || c->close_state != CL_CLOSE_PENDING){
        return;
    }

    emit_captured_block_close = (c->tasking_region != NULL && c->kind == CL_BLOCK);

    if(c->has_task_pragma){
        tasking_emit_finalize_region(c);

        output << "\t " << endl;
        output << "\tompd_wait_for_child_tasks();" << endl;
        if(emit_captured_block_close){
            output << "}" << endl;
        }
    } else {
        if(c->allgather_clause_active){
            allgather_emit_all();
        }
        if(c->gather_clause_active){
            gather_emit_all();
        }
        if(c->allreduction_clause_active){
            allreduction_emit_final(true);
        }
        if(c->reduction_clause_active){
            reduction_emit_final(true);
        }
        if(c->tasking_region != NULL){
            tasking_emit_region_body(c);
            if(emit_captured_block_close){
                output << "}" << endl;
            }
        }
    }

    if(main_scope_depth > 0){
        sequential_region_emit_open_if_needed();
    }

    cluster_stack_pop();
}

void writer_set_token_text(char *yytext)
{
    if(writer_current_token != NULL){
        free(writer_current_token);
    }

    writer_current_token = duplicate_token_text(yytext);
}

static int writer_is_task_body_capture_active(void)
{
    return tasking_parse_is_capture_active();
}

static void writer_clear_current_line(void)
{
    if(writer_current_line != NULL){
        free(writer_current_line);
        writer_current_line = NULL;
    }
}

static void writer_handle_token_text(void)
{
    distribute_note_token(writer_current_token);
    distribute_rewrite_loop_variable_token(&writer_current_token);

    append_token_to_line(writer_current_token);
}

static void writer_finish_single_line_distribute_if_needed(void)
{
    distribute_finish_single_line_if_needed(writer_current_line);
}

static void writer_rewrite_distribute_loop_if_needed(void)
{
    string start_expr = "";
    string end_expr = "";

    if(!distribute_should_rewrite_loop()){
        return;
    }

    distribute_prepare_loop_rewrite();

    distribute_rewrite_loop_header(&start_expr, &end_expr, &writer_current_line);
    distribute_emit_mpi_worksharing(start_expr, end_expr);
    output << distribute_take_pending_lines();
    distribute_mark_loop_rewritten();
}

static void writer_open_sequential_guard_if_needed(void)
{
    if(main_scope_depth > 0 && writer_sequential_region_closed == 1 && writer_translation_finalized == 0 &&
       !cluster_stack_is_empty() && !sequential_region_active){
        output << "\tif (__taskid == 0) {\n" << endl;
        sequential_region_active = 1;
        writer_sequential_region_closed = 0;
    }
}

static int writer_capture_tasking_normal_block_if_needed(void)
{
    cluster_stack *c;

    if(writer_current_line == NULL || sequential_region_active || cluster_stack_is_empty()){
        return 0;
    }

    c = cluster_stack_peek();
    if(c != NULL && c->tasking_region != NULL && c->close_state == CL_CLOSE_PENDING){
        writer_clear_current_line();
        return 0;
    }

    if(c != NULL && c->close_state != CL_CLOSE_PENDING && !c->has_non_task_pragma &&
       (c->tasking_region != NULL || use_ompd_runtime_main)){
        if(num_teams_is_active()){
            num_teams_emit_runtime_check();
        }

        if(c->tasking_region == NULL){
            c->tasking_region = tasking_region_create(tasking_region_id_counter++);
            if(c->tasking_region == NULL){
                fprintf(stderr, "Error: tasking_region_create\n");
                exit(EXIT_FAILURE);
            }
        }

        if(tasking_region_begin_normal_block(c->tasking_region, writer_current_line) != 0){
            fprintf(stderr, "Error: tasking_region_begin_normal_block\n");
            exit(EXIT_FAILURE);
        }

        writer_clear_current_line();
        return 1;
    }

    return 0;
}

static void writer_emit_or_buffer_current_line(void)
{
    if(distribute_should_buffer_line()){
        distribute_pending_lines() += (writer_current_line ? writer_current_line : "");
        return;
    }

    if(writer_statement_output_started == 1 && writer_sequential_region_closed == 0 && writer_translation_finalized == 0){
        output << (writer_current_line ? "\t" + string(writer_current_line) : "") << endl;
    }
    else{
        output << (writer_current_line ? writer_current_line : "") << endl;
    }
}

static void writer_update_master_state(void)
{
    if(!master_region_is_active() || writer_current_line == NULL){
        return;
    }

    if(master_region_consume_line(writer_current_line)){
        sequential_region_emit_close_if_open();
    }
}

static void writer_capture_declaration_line(void)
{
    int line_length;

    if(!mpi_type_declaration_scope_active || writer_current_line == NULL){
        return;
    }

    line_length = strlen(writer_current_line);

    if((mpi_type_declaration_buffer = (char *)realloc(mpi_type_declaration_buffer, line_length + mpi_type_declaration_buffer_size)) == NULL){
        fprintf(stderr, "Dynamic memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    strncpy(mpi_type_declaration_buffer + mpi_type_declaration_buffer_size, writer_current_line, line_length);
    mpi_type_declaration_buffer_size += line_length;
}

static void writer_emit_num_teams_if_needed(void)
{
    if(num_teams_is_active() && writer_current_line){
        num_teams_emit_runtime_check();
    }
}

static void writer_close_distribute_if_needed(void)
{
    if(!distribute_should_close()){
        return;
    }

    if(distribute_inserted_loop_brace() && distribute_has_schedule()){
        output << "}" << endl;
    }

    if(distribute_has_schedule()){
        output << "\t}" << endl;
        distribute_clear_schedule();
    }

    if(halo_has_pending_update()){
        output << halo_build_pending_code() << endl;
        halo_clear_pending_update();
    }

    output << "}" << endl;

    if(distribute_reduction_active){
        reduction_emit_final(false);
        distribute_reduction_active = 0;
    }
    if(distribute_allreduction_active){
        allreduction_emit_final(false);
        distribute_allreduction_active = 0;
    }

    distribute_reset_after_close();
}

static void writer_handle_completed_source_line(void)
{
    cluster_stack *c;

    writer_finish_single_line_distribute_if_needed();
    writer_rewrite_distribute_loop_if_needed();
    writer_open_sequential_guard_if_needed();

    if(writer_capture_tasking_normal_block_if_needed()){
        return;
    }

    writer_emit_or_buffer_current_line();
    writer_update_master_state();
    writer_capture_declaration_line();
    writer_emit_num_teams_if_needed();
    writer_clear_current_line();
    writer_close_distribute_if_needed();

    c = cluster_stack_peek();
    close_pending_cluster_if_needed(c);
}

void writer_process_current_token()
{
    if(writer_is_task_body_capture_active()){
        writer_clear_current_line();
        return;
    }

    if(strcmp(writer_current_token, "\n") != 0){
        writer_handle_token_text();
        return;
    }

    writer_handle_completed_source_line();
}

void writer_flush_current_line(){
    output << (writer_current_line ? writer_current_line : "") << endl;
    writer_current_line = NULL;
}
