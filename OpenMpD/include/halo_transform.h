#ifndef HALO_TRANSFORM_H
#define HALO_TRANSFORM_H

/*
 * Handles OpenMPD halo clauses. It records variables that need halo exchange,
 * remembers the last distributed loop bounds and builds the MPI send/receive
 * code that updates border rows between neighboring processes.
 */

#include <string>

/* Set while parsing a halo update clause */
extern int halo_update_pending;

/* Records a variable declared for halo exchange handling */
void halo_add_declared(const char *arg);

/* Returns whether a variable was declared for halo handling */
int halo_is_declared(const char *arg);

/* Clears all variables declared for halo handling */
void halo_clear_declared(void);

/* Stores the last distributed loop bounds for halo code */
void halo_set_last_distribute_bounds(const char *start_expr, const char *end_expr);

/* Prepares a pending halo update from explicit values */
void halo_prepare_update(const char *var, const char *elems, const char *mpi_type);

/* Returns whether a halo update is waiting to be emitted */
int halo_has_pending_update(void);

/* Builds code for the pending halo update */
std::string halo_build_pending_code(void);

/* Clears the pending halo update state */
void halo_clear_pending_update(void);

/* Prepares a pending halo update from the current pragma clause */
void halo_prepare_pending_update_from_clause();

/* Handles the current halo clause and emits or stores its update */
void halo_handle_clause();

#endif
