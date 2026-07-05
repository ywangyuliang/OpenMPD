#ifndef MEMORY_TRANSFORM_H
#define MEMORY_TRANSFORM_H

/*
 * Emits memory-related code for cluster data. Allocation clauses generate
 * malloc-based storage on worker ranks, while broadcast clauses generate
 * MPI_Bcast calls for scalars, arrays and explicitly sized references.
 */

/* Emits MPI-backed allocation code for cluster data */
void memory_emit_cluster_allocations();

/* Emits broadcast code for cluster data */
void memory_emit_broadcasts();

#endif
