#ifndef PRAGMA_ARGS_H
#define PRAGMA_ARGS_H

/*
 * Shared storage for arguments parsed from the current OpenMPD pragma clause.
 * It groups generic clause arguments plus scatter/gather/allgather and
 * reduction/allreduction data until transform modules consume them.
 */

#include <vector>

extern std::vector<const char *> current_pragma_args;
extern std::vector<std::vector<const char *>> scatter_clause_arg_groups;
extern std::vector<std::vector<const char *>> gather_clause_arg_groups;
extern std::vector<std::vector<const char *>> allgather_clause_arg_groups;
extern std::vector<const char *> cluster_reduction_operators;
extern std::vector<std::vector<const char *>> cluster_reduction_variables;
extern std::vector<const char *> cluster_allreduction_operators;
extern std::vector<std::vector<const char *>> cluster_allreduction_variables;
extern std::vector<const char *> distribute_reduction_operators;
extern std::vector<std::vector<const char *>> distribute_reduction_variables;
extern std::vector<const char *> distribute_allreduction_operators;
extern std::vector<std::vector<const char *>> distribute_allreduction_variables;

/* Clears the generic argument list for the current clause */
void pragma_args_clear_current(void);

/* Adds one value to the generic argument list */
void pragma_args_add_current(const char *arg);

/* Starts a new scatter argument group */
void pragma_args_begin_scatter(void);

/* Starts a new gather argument group */
void pragma_args_begin_gather(void);

/* Starts a new allgather argument group */
void pragma_args_begin_allgather(void);

/* Adds one value to a scatter argument group */
void pragma_args_add_scatter(int chunk_pos, const char *arg);

/* Adds one value to a gather argument group */
void pragma_args_add_gather(int chunk_pos, const char *arg);

/* Adds one value to an allgather argument group */
void pragma_args_add_allgather(int chunk_pos, const char *arg);

/* Starts a reduction clause in cluster or distribute scope */
void pragma_args_begin_reduce(int distribute_scope);

/* Starts an allreduction clause in cluster or distribute scope */
void pragma_args_begin_allreduce(int distribute_scope);

/* Adds an operator or variable to the active reduction clause */
void pragma_args_add_reduce(int distribute_scope, int is_variable, const char *arg);

/* Adds an operator or variable to the active allreduction clause */
void pragma_args_add_allreduce(int distribute_scope, int is_variable, const char *arg);

#endif
