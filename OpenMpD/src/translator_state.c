#include "translator_state.h"

#include <string.h>

static translator_state_t translator_state;

translator_state_t *translator_state_get(void)
{
    return &translator_state;
}

translator_context_t *translator_context_get(void)
{
    return &translator_state;
}

void translator_tasking_state_reset(translator_tasking_state_t *state)
{
    if(state == 0){
        return;
    }

    memset(state, 0, sizeof(*state));
}

void translator_distribute_state_reset(translator_distribute_state_t *state)
{
    if(state == 0){
        return;
    }

    state->active = 0;
    state->extended_mode = 0;
    state->for_state = 0;
    state->brace_count = DISTRIBUTE_BRACE_CLOSED;
    state->insert_for_braces = 0;
    state->pending_lines.clear();
    state->loop_variable.clear();
    state->schedule_expr.clear();
}

void translator_master_state_reset(translator_master_state_t *state)
{
    if(state == 0){
        return;
    }

    memset(state, 0, sizeof(*state));
}

void translator_halo_state_reset(translator_halo_state_t *state)
{
    if(state == 0){
        return;
    }

    state->pending_distribute = 0;
    state->last_distribute_start.clear();
    state->last_distribute_end.clear();
    state->pending_var.clear();
    state->pending_elems.clear();
    state->pending_rows.clear();
    state->pending_type.clear();
    state->declared_vars.clear();
}

void translator_num_teams_state_reset(translator_num_teams_state_t *state)
{
    if(state == 0){
        return;
    }

    state->active = 0;
    state->expr.clear();
}

void translator_state_reset(translator_state_t *state)
{
    if(state == 0){
        return;
    }

    translator_tasking_state_reset(&state->tasking);
    translator_distribute_state_reset(&state->distribute);
    translator_master_state_reset(&state->master);
    translator_halo_state_reset(&state->halo);
    translator_num_teams_state_reset(&state->num_teams);
}

static translator_tasking_state_t *tasking_state(void)
{
    return &translator_state_get()->tasking;
}

int tasking_parse_is_capture_active(void)
{
    translator_tasking_state_t *state = tasking_state();

    return state->expect_task_body || state->in_task_body_parse;
}

int tasking_parse_should_begin_body(void)
{
    return tasking_state()->expect_task_body;
}

int tasking_parse_is_in_body(void)
{
    return tasking_state()->in_task_body_parse;
}

void tasking_parse_expect_body(void)
{
    tasking_state()->expect_task_body = 1;
}

void tasking_parse_begin_body(void)
{
    translator_tasking_state_t *state = tasking_state();

    state->expect_task_body = 0;
    state->in_task_body_parse = 1;
}

void tasking_parse_finish_body(void)
{
    tasking_state()->in_task_body_parse = 0;
}

static translator_num_teams_state_t *num_teams_state(void)
{
    return &translator_state_get()->num_teams;
}

int num_teams_is_active(void)
{
    return num_teams_state()->active;
}

const std::string &num_teams_expr(void)
{
    return num_teams_state()->expr;
}

void num_teams_set_expr(const char *expr)
{
    translator_num_teams_state_t *state = num_teams_state();

    state->active = 1;
    state->expr = expr != 0 ? expr : "";
}

void num_teams_clear(void)
{
    translator_num_teams_state_reset(num_teams_state());
}

static translator_master_state_t *master_state(void)
{
    return &translator_state_get()->master;
}

void master_region_begin(void)
{
    translator_master_state_t *state = master_state();

    state->active = 1;
    state->brace_count = 0;
    state->closes_by_braces = 0;
}

int master_region_is_active(void)
{
    return master_state()->active;
}

int master_region_consume_line(const char *line)
{
    translator_master_state_t *state = master_state();

    if(!state->active || line == 0){
        return 0;
    }

    for(unsigned long int i = 0; i < strlen(line); i++){
        if(line[i] == '{'){
            state->brace_count++;
            state->closes_by_braces = 1;
        }
        else if(line[i] == '}'){
            state->brace_count--;
        }
    }

    if(state->closes_by_braces && state->brace_count == 0){
        state->active = 0;
        return 1;
    }

    if(!state->closes_by_braces && strchr(line, ';') != 0){
        state->active = 0;
        return 1;
    }

    return 0;
}
