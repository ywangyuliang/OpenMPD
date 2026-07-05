#ifndef REDUCTION_TRANSFORM_H
#define REDUCTION_TRANSFORM_H

/*
 * Emits reduction and allreduction code for cluster and distribute scopes. It
 * groups pending variables by operator and can also rebuild OpenMP reduction
 * clauses for loops that remain parallel after distribution.
 */

#include <string>
#include <vector>

extern std::vector<std::string> pending_reduction_clauses;
extern std::vector<std::vector<std::string>> grouped_reduction_vars;

/* Reduction/allreduction mode flags */
extern int cluster_reduction_active;
extern int distribute_reduction_active;
extern int cluster_allreduction_active;
extern int distribute_allreduction_active;

/* Emits final reduction code for the current cluster or distribute scope */
void reduction_emit_final(bool cluster);

/* Emits final allreduction code for the current cluster or distribute scope */
void allreduction_emit_final(bool cluster);

/* Groups pending reduction variables by their reduction operator */
void reduction_group_pending_vars_by_operator();

/* Builds an OpenMP reduction clause for a distributed loop */
std::string reduction_build_distribute_clause(int reduction_index);

/* Builds an OpenMP allreduction clause for a distributed loop */
std::string allreduction_build_distribute_clause(int reduction_index);

#endif
