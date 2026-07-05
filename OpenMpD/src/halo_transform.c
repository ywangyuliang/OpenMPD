#include "halo_transform.h"
#include "translator_state.h"
#include "symbol_table.h"
#include "pragma_args.h"
#include "scatter_gather_transform.h"

#include <stdio.h>
#include <stdlib.h>

extern symbol_table table;

int halo_update_pending = 0;

static std::string halo_var_name(const std::string &arg)
{
    size_t pos = arg.find('[');

    if(pos == std::string::npos){
        return arg;
    }

    return arg.substr(0, pos);
}

void halo_add_declared(const char *arg)
{
    std::string name;

    if(arg == 0){
        return;
    }

    name = halo_var_name(std::string(arg));
    translator_halo_state_t *state = &translator_context_get()->halo;

    for(unsigned long int i = 0; i < state->declared_vars.size(); i++){
        if(state->declared_vars.at(i) == name){
            return;
        }
    }

    state->declared_vars.push_back(name);
}

int halo_is_declared(const char *arg)
{
    std::string name;

    if(arg == 0){
        return 0;
    }

    name = halo_var_name(std::string(arg));
    translator_halo_state_t *state = &translator_context_get()->halo;

    for(unsigned long int i = 0; i < state->declared_vars.size(); i++){
        if(state->declared_vars.at(i) == name){
            return 1;
        }
    }

    return 0;
}

void halo_clear_declared(void)
{
    translator_context_get()->halo.declared_vars.clear();
}

void halo_set_last_distribute_bounds(const char *start_expr, const char *end_expr)
{
    translator_halo_state_t *state = &translator_context_get()->halo;

    state->last_distribute_start = start_expr != 0 ? start_expr : "";
    state->last_distribute_end = end_expr != 0 ? end_expr : "";
}

void halo_prepare_update(const char *var, const char *elems, const char *mpi_type)
{
    std::string halo_elems = elems != 0 ? elems : "";
    std::string halo_rows = halo_elems;
    size_t pos = halo_elems.find('*');

    if(pos != std::string::npos){
        halo_rows = halo_elems.substr(0, pos);
    }

    translator_halo_state_t *state = &translator_context_get()->halo;

    state->pending_var = var != 0 ? var : "";
    state->pending_elems = halo_elems;
    state->pending_rows = halo_rows;
    state->pending_type = mpi_type != 0 ? mpi_type : "";
    state->pending_distribute = 1;
}

int halo_has_pending_update(void)
{
    return translator_context_get()->halo.pending_distribute;
}

std::string halo_build_pending_code(void)
{
    translator_halo_state_t *state = &translator_context_get()->halo;

    if(state->last_distribute_start.empty() || state->last_distribute_end.empty()){
        fprintf(stderr, "Error: update halo requires a previous distribute\n");
        exit(EXIT_FAILURE);
    }

    if(state->pending_var.empty() || state->pending_elems.empty() || state->pending_rows.empty() || state->pending_type.empty()){
        fprintf(stderr, "Error: incomplete pending update halo\n");
        exit(EXIT_FAILURE);
    }


    return
        "\t{\n"
        "\t\tMPI_Request __halo_reqs[2];\n"
        "\t\tint __halo_nreqs = 0;\n"
        "\t\tif (__taskid > 0)\n"
        "\t\t\tMPI_Isend(" + state->pending_var + "[__start], " + state->pending_elems + ", " + state->pending_type + ", __taskid - 1, 100, MPI_COMM_WORLD, &__halo_reqs[__halo_nreqs++]);\n"
        "\t\tif (__taskid < __numprocs - 1)\n"
        "\t\t\tMPI_Isend(" + state->pending_var + "[__end-" + state->pending_rows + "], " + state->pending_elems + ", " + state->pending_type + ", __taskid + 1, 101, MPI_COMM_WORLD, &__halo_reqs[__halo_nreqs++]);\n"
        "\t\tif (__taskid > 0)\n"
        "\t\t\tMPI_Recv(" + state->pending_var + "[__start-" + state->pending_rows + "], " + state->pending_elems + ", " + state->pending_type + ", __taskid - 1, 101, MPI_COMM_WORLD, MPI_STATUS_IGNORE);\n"
        "\t\tif (__taskid < __numprocs - 1)\n"
        "\t\t\tMPI_Recv(" + state->pending_var + "[__end], " + state->pending_elems + ", " + state->pending_type + ", __taskid + 1, 100, MPI_COMM_WORLD, MPI_STATUS_IGNORE);\n"
        "\t\tif (__halo_nreqs > 0)\n"
        "\t\t\tMPI_Waitall(__halo_nreqs, __halo_reqs, MPI_STATUSES_IGNORE);\n"
        "\t}\n\n";
}

void halo_clear_pending_update(void)
{
    translator_halo_state_t *state = &translator_context_get()->halo;

    state->pending_distribute = 0;
    state->pending_var.clear();
    state->pending_elems.clear();
    state->pending_rows.clear();
    state->pending_type.clear();
}

void halo_prepare_pending_update_from_clause(){

    vector<string> halo_args = scatter_build_halo_argument_parts();

    string variable_name = halo_args.at(0);
    symbol_info *symbol = table.get_symbol_info(variable_name);
    if (symbol == NULL) {
        fprintf(stderr, "Error: halo variable not found in symbol table\n");
        exit(EXIT_FAILURE);
    }

    string halo_elems = halo_args.at(3);
    string halo_type = "MPI_" + symbol->get_variable_type();
    halo_prepare_update(variable_name.c_str(), halo_elems.c_str(), halo_type.c_str());

    current_pragma_args.clear();
}

void halo_handle_clause() {
    if (current_pragma_args.size() < 2) {
        fprintf(stderr, "Error: halo clause requires variable and halo expression\n");
        exit(EXIT_FAILURE);
    }

    const char *halo_arg = current_pragma_args.at(0);

    if (halo_update_pending) {
        if (!halo_is_declared(halo_arg)) {
            fprintf(stderr, "Error: update halo requires a previous halo(...) declaration in the cluster\n");
            exit(EXIT_FAILURE);
        }

        halo_prepare_pending_update_from_clause();
        halo_update_pending = 0;
    }
    else {
        halo_add_declared(halo_arg);
        current_pragma_args.clear();
    }
}
