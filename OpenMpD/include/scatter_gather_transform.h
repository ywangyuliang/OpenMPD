#ifndef SCATTER_GATHER_TRANSFORM_H
#define SCATTER_GATHER_TRANSFORM_H

/*
 * Emits MPI data-movement code for scatter, gather and allgather clauses. It
 * supports chunked and partitioned distributions, tracks clause parsing state
 * and builds argument lists shared with halo-aware transformations.
 */

#include <string>
#include <vector>

using namespace std;

/* Scatter/gather/allgather clause flags */
extern int scatter_clause_active;
extern int gather_clause_active;
extern int allgather_clause_active;
extern int gather_clause_parsing;
extern int allgather_clause_parsing;

/* Index of the current scatter/gather argument group being parsed */
extern int chunk_pos;

/* Builds the standard argument list for scatter-like clauses */
vector<string> scatter_build_argument_parts();

/* Builds the argument list used by halo-aware scatter code */
vector<string> scatter_build_halo_argument_parts();

/* Emits scatter code for chunked distributions */
void scatter_emit_chunked(std::vector<const char *> scatter_args);

/* Emits scatter code for partitioned distributions */
void scatter_emit_partitioned(std::vector<const char *> scatter_args);

/* Emits all pending scatter operations */
void scatter_emit_all();

/* Emits gather code for chunked distributions */
void gather_emit_chunked(std::vector<const char *> gather_args);

/* Emits gather code for partitioned distributions */
void gather_emit_partitioned(std::vector<const char *> gather_args);

/* Emits all pending gather operations */
void gather_emit_all();

/* Emits allgather code for chunked distributions */
void allgather_emit_chunked(std::vector<const char *> gather_args);

/* Emits allgather code for partitioned distributions */
void allgather_emit_partitioned(std::vector<const char *> gather_args);

/* Emits all pending allgather operations */
void allgather_emit_all();

#endif
