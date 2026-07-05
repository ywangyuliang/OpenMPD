#include "mpi_lifecycle.h"

#include "cluster_stack.h"
#include "output_slots.h"
#include "translator_state.h"
#include "writer.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>

using namespace std;

extern ofstream output;

int string_header_include_requested = 0;

extern char *main_argc_name;
extern char *main_argv_name;
extern int sequential_region_active;
extern int use_ompd_runtime_main;
extern int main_scope_depth;
extern int mpi_initialization_pending_in_main;
extern int mpi_runtime_initialized;
extern int declaration_handling_enabled;

static const char *STRING_HEADER_SLOT = "string_header";
static const char *MPI_HEADERS_SLOT = "mpi_headers";
static const char *MPI_GLOBAL_DECLARATIONS_SLOT = "mpi_global_declarations";
static const char *MPI_INITIALIZATION_SLOT = "mpi_initialization";
static int output_prelude_slots_reserved = 0;
static int mpi_global_declarations_slot_reserved = 0;
static int mpi_initialization_slot_reserved = 0;

static std::string mpi_initialization_code()
{
    std::string text = "\tMPI_Init(";

    if (main_argc_name != NULL && main_argv_name != NULL) {
        text += "&";
        text += main_argc_name;
        text += ",&";
        text += main_argv_name;
    }
    else if (main_argc_name != NULL) {
        text += "&";
        text += main_argc_name;
        text += ", NULL";
    }
    else if (main_argv_name != NULL) {
        text += "NULL, &";
        text += main_argv_name;
    }
    else {
        text += "NULL , NULL";
    }

    text += ");\n\tMPI_Comm_size(MPI_COMM_WORLD,&__numprocs);\n\tMPI_Comm_rank(MPI_COMM_WORLD,&__taskid);\n\tompd_init_mpi_types();\n";

    /* The rank-0 sequential guard is opened by the first executable statement, */
    /* not here: opening it next to the deferred init would let a following */
    /* declaration inside a #ifdef/#else/#endif close it in just one branch */
    return text;
}

static void mpi_initialization_reserve_slot()
{
    if (use_ompd_runtime_main || !main_scope_depth || mpi_initialization_slot_reserved) {
        return;
    }

    output << output_slot_marker(MPI_INITIALIZATION_SLOT) << std::endl;
    output_slot_set_replacement(MPI_INITIALIZATION_SLOT, "");
    mpi_initialization_slot_reserved = 1;
}

void sequential_region_close(){
    writer_mark_sequential_region_closed();
    output << "\t}" << endl;
}

void master_guard_emit_open() {
    if (use_ompd_runtime_main) {
        output << "if (ompd_current_rank_is_master()) {" << endl;
    }
    else {
        output << "if (__taskid == 0) {" << endl;
    }
}

void mpi_runtime_reserve_output_prelude_slots()
{
    if(output_prelude_slots_reserved){
        return;
    }

    output << output_slot_marker(STRING_HEADER_SLOT)
           << output_slot_marker(MPI_HEADERS_SLOT)
           << output_slot_marker(MPI_GLOBAL_DECLARATIONS_SLOT);

    output_slot_set_replacement(STRING_HEADER_SLOT, "");
    output_slot_set_replacement(MPI_HEADERS_SLOT, "");
    output_slot_set_replacement(MPI_GLOBAL_DECLARATIONS_SLOT, "");

    output_prelude_slots_reserved = 1;
    mpi_global_declarations_slot_reserved = 1;
}

void ompd_runtime_emit_initialization() {
    output << "\tompd_initialize_runtime(";

    if (main_argc_name != NULL) {
        output << "&" << main_argc_name;
    }
    else {
        output << "NULL";
    }

    output << ", ";

    if (main_argv_name != NULL) {
        output << "(char ***)&" << main_argv_name;
    }
    else {
        output << "NULL";
    }

    output << ");" << endl;
}

void mpi_runtime_emit_headers_and_globals(){
    std::string mpi_headers;

    if (use_ompd_runtime_main) {
        mpi_headers =
            "#include <assert.h>\n"
            "#include <mpi.h>\n"
            "#include <stdio.h>\n"
            "#include \"ompd_runtime.h\"\n";
    }
    else {
        mpi_headers = "#include <assert.h>\n#include <mpi.h>\n";
    }

    output_slot_set_replacement(MPI_HEADERS_SLOT, mpi_headers);
    output_slot_set_replacement(
        MPI_GLOBAL_DECLARATIONS_SLOT,
        "void ompd_init_mpi_types();\n\nint __taskid = -1, __numprocs = -1;\nMPI_Status __status;\n\n");

    writer_mark_statement_output_started();
}

void mpi_initialization_emit_deferred_code(){
    mpi_initialization_reserve_slot();
    output_slot_set_replacement(MPI_INITIALIZATION_SLOT, mpi_initialization_code());

    writer_mark_statement_output_started();
}

void string_header_request_include() {
    output_slot_set_replacement(STRING_HEADER_SLOT, "#include <string.h>\n");
}

void sequential_region_emit_open_if_needed() {
    if (!sequential_region_active) {
        master_guard_emit_open();
        sequential_region_active = 1;
    }
}

void mpi_global_declarations_reserve_slot()
{
    if(mpi_global_declarations_slot_reserved || mpi_runtime_initialized){
        return;
    }

    output << output_slot_marker(MPI_GLOBAL_DECLARATIONS_SLOT) << std::endl;
    output_slot_set_replacement(MPI_GLOBAL_DECLARATIONS_SLOT, "");
    mpi_global_declarations_slot_reserved = 1;
}

void mpi_runtime_prepare_statement_emission(int open_sequential_guard){
    if(!use_ompd_runtime_main && main_scope_depth && !mpi_initialization_slot_reserved){
        mpi_initialization_reserve_slot();
        if(mpi_initialization_pending_in_main != 0){
            mpi_initialization_emit_deferred_code();
        }
    }

    if(open_sequential_guard && cluster_stack_is_empty() && (main_scope_depth == 2) && !sequential_region_active){
        if(!use_ompd_runtime_main || (mpi_runtime_initialized == 1 && mpi_initialization_pending_in_main == 0)){
            sequential_region_emit_open_if_needed();
        }
    }

    declaration_handling_enabled = 1;
}

void num_teams_emit_runtime_check(){
    if(num_teams_is_active()){
        const std::string &expr = num_teams_expr();

        if (use_ompd_runtime_main) {
            output << "ompd_check_region_num_teams(" << expr << ");\n";
        }
        else {
            output << "if (__numprocs != " << expr << ") {\n";
            output << "\tprintf(\"Error, num_teams must be " << expr << "\\n\");\n";
            output << "\tMPI_Finalize();\n";
            output << "\treturn(-1);\n";
            output << "}\n";
        }

        num_teams_clear();
    }
}

void sequential_region_emit_close_if_open() {
    if (sequential_region_active) {
        output << "}" << endl;
        sequential_region_active = 0;
    }
}

void mpi_runtime_emit_finalize(){
    if(cluster_stack_is_empty()){
        sequential_region_emit_close_if_open();
    }
    writer_mark_translation_finalized();

    if (use_ompd_runtime_main) {
        output << "\tompd_finalize_runtime();" << endl;
    }
    else {
        output << "\tMPI_Finalize();" << endl;
    }
}

