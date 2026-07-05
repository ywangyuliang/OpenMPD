#ifndef MPI_LIFECYCLE_H
#define MPI_LIFECYCLE_H

/*
 * Coordinates MPI-related code emitted around the translated program. It
 * reserves deferred output slots, writes runtime headers and globals, opens
 * and closes master-rank guards, validates num_teams and emits final cleanup.
 */

/* Set once the string header has been requested for the generated output */
extern int string_header_include_requested;

/* Emits the closing brace for a generated sequential master region */
void sequential_region_close();

/* Emits the guard that keeps generated code on the master rank */
void master_guard_emit_open();

/* Reserves named prelude slots before normal output starts */
void mpi_runtime_reserve_output_prelude_slots();

/* Emits initialization for the OMPD runtime wrapper */
void ompd_runtime_emit_initialization();

/* Writes MPI headers and global runtime declarations into the prelude */
void mpi_runtime_emit_headers_and_globals();

/* Writes deferred MPI initialization code into its reserved slot */
void mpi_initialization_emit_deferred_code();

/* Requests the string header in the generated output */
void string_header_request_include();

/* Opens the sequential master region when it is not already open */
void sequential_region_emit_open_if_needed();

/* Prepares runtime state before a normal C statement is emitted */
void mpi_runtime_prepare_statement_emission(int open_sequential_guard);

/* Reserves the slot used for MPI global declarations */
void mpi_global_declarations_reserve_slot();

/* Emits the runtime validation for a num_teams clause */
void num_teams_emit_runtime_check();

/* Closes the sequential master region when it is open */
void sequential_region_emit_close_if_open();

/* Emits final runtime cleanup at the end of the translated program */
void mpi_runtime_emit_finalize();

#endif
