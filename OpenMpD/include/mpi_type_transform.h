#ifndef MPI_TYPE_TRANSFORM_H
#define MPI_TYPE_TRANSFORM_H

/*
 * Captures C struct/type declarations from declare-cluster scopes and emits
 * the matching MPI_Datatype declarations, creation code and commits needed by
 * generated communication code.
 */

#include <string>

using namespace std;

extern char *mpi_type_declaration_buffer;
extern int mpi_type_declaration_buffer_size;
extern string mpi_type_declarations;

/*
 * Set while parsing a declare cluster block, so source lines are captured
 * as MPI datatype declarations instead of emitted normally
 */
extern int mpi_type_declaration_scope_active;

/* Emits MPI datatype declarations collected inside declare cluster blocks */
void mpi_type_emit_cluster_declarations();

#endif
