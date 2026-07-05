#include "pragma_args.h"

std::vector<const char *> current_pragma_args;
std::vector<std::vector<const char *>> scatter_clause_arg_groups;
std::vector<std::vector<const char *>> gather_clause_arg_groups;
std::vector<std::vector<const char *>> allgather_clause_arg_groups;
std::vector<const char *> cluster_reduction_operators;
std::vector<std::vector<const char *>> cluster_reduction_variables;
std::vector<const char *> cluster_allreduction_operators;
std::vector<std::vector<const char *>> cluster_allreduction_variables;
std::vector<const char *> distribute_reduction_operators;
std::vector<std::vector<const char *>> distribute_reduction_variables;
std::vector<const char *> distribute_allreduction_operators;
std::vector<std::vector<const char *>> distribute_allreduction_variables;

void pragma_args_clear_current(void)
{
    current_pragma_args.clear();
}

void pragma_args_add_current(const char *arg)
{
    current_pragma_args.push_back(arg);
}

void pragma_args_begin_scatter(void)
{
    scatter_clause_arg_groups.push_back({});
}

void pragma_args_begin_gather(void)
{
    gather_clause_arg_groups.push_back({});
}

void pragma_args_begin_allgather(void)
{
    allgather_clause_arg_groups.push_back({});
}

void pragma_args_add_scatter(int chunk_pos, const char *arg)
{
    scatter_clause_arg_groups.at(chunk_pos).push_back(arg);
}

void pragma_args_add_gather(int chunk_pos, const char *arg)
{
    gather_clause_arg_groups.at(chunk_pos).push_back(arg);
}

void pragma_args_add_allgather(int chunk_pos, const char *arg)
{
    allgather_clause_arg_groups.at(chunk_pos).push_back(arg);
}

void pragma_args_begin_reduce(int distribute_scope)
{
    if(distribute_scope){
        distribute_reduction_variables.push_back({});
    }
    else {
        cluster_reduction_variables.push_back({});
    }
}

void pragma_args_begin_allreduce(int distribute_scope)
{
    if(distribute_scope){
        distribute_allreduction_variables.push_back({});
    }
    else {
        cluster_allreduction_variables.push_back({});
    }
}

void pragma_args_add_reduce(int distribute_scope, int is_variable, const char *arg)
{
    if(distribute_scope){
        if(is_variable){
            distribute_reduction_variables.at(distribute_reduction_variables.size() - 1).push_back(arg);
        }
        else {
            distribute_reduction_operators.push_back(arg);
        }
    }
    else {
        if(is_variable){
            cluster_reduction_variables.at(cluster_reduction_variables.size() - 1).push_back(arg);
        }
        else {
            cluster_reduction_operators.push_back(arg);
        }
    }
}

void pragma_args_add_allreduce(int distribute_scope, int is_variable, const char *arg)
{
    if(distribute_scope){
        if(is_variable){
            distribute_allreduction_variables.at(distribute_allreduction_variables.size() - 1).push_back(arg);
        }
        else {
            distribute_allreduction_operators.push_back(arg);
        }
    }
    else {
        if(is_variable){
            cluster_allreduction_variables.at(cluster_allreduction_variables.size() - 1).push_back(arg);
        }
        else {
            cluster_allreduction_operators.push_back(arg);
        }
    }
}
