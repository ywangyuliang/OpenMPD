#include "reduction_transform.h"
#include "codegen_utils.h"
#include "pragma_args.h"
#include "symbol_table.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <fstream>

using namespace std;

extern symbol_table table;
extern ofstream output;

std::vector<std::string> pending_reduction_clauses;
std::vector<std::vector<std::string>> grouped_reduction_vars;

int cluster_reduction_active = 0;
int distribute_reduction_active = 0;
int cluster_allreduction_active = 0;
int distribute_allreduction_active = 0;

/* Maps an OpenMPD reduction operator to its MPI_Op name */
static string reduction_mpi_operator(const char *reduction_operator) {
    if (strcmp(reduction_operator, "+") == 0) {
        return "MPI_SUM";
    }
    if (strcmp(reduction_operator, "*") == 0) {
        return "MPI_PROD";
    }
    if (strcmp(reduction_operator, "MAX") == 0) {
        return "MPI_MAX";
    }
    if (strcmp(reduction_operator, "MIN") == 0) {
        return "MPI_MIN";
    }
    if (strcmp(reduction_operator, "&") == 0) {
        return "MPI_LAND";
    }
    if (strcmp(reduction_operator, "|") == 0) {
        return "MPI_LOR";
    }
    if (strcmp(reduction_operator, "^") == 0) {
        return "MPI_LXOR";
    }

    fprintf(stderr, "Invalid reduction operator\n");
    exit(EXIT_FAILURE);
}

void reduction_emit_final(bool cluster) {
    std::vector<std::vector<const char *>> *selected_reduction_variables;
    std::vector<const char *> *selected_reduction_operators;

    if (cluster) {
        selected_reduction_variables = &cluster_reduction_variables;
        selected_reduction_operators = &cluster_reduction_operators;
    }
    else {
        selected_reduction_variables = &distribute_reduction_variables;
        selected_reduction_operators = &distribute_reduction_operators;
    }

    std::string emitted_reduction_code = "\n";

    if ((*selected_reduction_operators).size() != (*selected_reduction_variables).size()) {
        fprintf(stderr, "Error: reduction operator/variable count mismatch\n");
        exit(EXIT_FAILURE);
    }

    for (long unsigned int i = 0; i < (*selected_reduction_operators).size(); i++) {
        string mpi_operator = reduction_mpi_operator((*selected_reduction_operators).at(i));

        for (long unsigned int variable_index = 0; variable_index < (*selected_reduction_variables).at(i).size(); variable_index++) {
            symbol_info *variable_symbol = table.get_symbol_info((*selected_reduction_variables).at(i).at(variable_index));
            std::string uppercase_type;
            std::string lowercase_type;
            uppercase_type += variable_symbol->get_variable_type();
            lowercase_type += variable_symbol->get_variable_type();

            for (long unsigned int j = 0; j < variable_symbol->get_variable_type().size(); j++) {
                uppercase_type.at(j) = toupper(variable_symbol->get_variable_type().at(j));
                lowercase_type.at(j) = tolower(variable_symbol->get_variable_type().at(j));
            }

            emitted_reduction_code += (lowercase_type + " __" + (*selected_reduction_variables).at(i).at(variable_index) + ";\n");

            emitted_reduction_code = emitted_reduction_code + "MPI_Reduce(&" + (*selected_reduction_variables).at(i).at(variable_index) + ", &__" + (*selected_reduction_variables).at(i).at(variable_index) + ", 1, " + mpi_type_for_c_type(uppercase_type) +
             ", " + mpi_operator + ", 0, MPI_COMM_WORLD);\n";

            emitted_reduction_code = emitted_reduction_code + "if (__taskid == 0) { " + (*selected_reduction_variables).at(i).at(variable_index) + " = __" + (*selected_reduction_variables).at(i).at(variable_index) + "; }\n";
        }
    }

    output << emitted_reduction_code << endl;

    (*selected_reduction_operators).clear();
    (*selected_reduction_variables).clear();
}

void allreduction_emit_final(bool cluster) {
    std::vector<std::vector<const char *>> *selected_allreduction_variables;
    std::vector<const char *> *selected_allreduction_operators;

    if (cluster) {
        selected_allreduction_variables = &cluster_allreduction_variables;
        selected_allreduction_operators = &cluster_allreduction_operators;
    }
    else {
        selected_allreduction_variables = &distribute_allreduction_variables;
        selected_allreduction_operators = &distribute_allreduction_operators;
    }

    std::string emitted_reduction_code = "\n";

    if ((*selected_allreduction_operators).size() != (*selected_allreduction_variables).size()) {
        fprintf(stderr, "Error: allreduction operator/variable count mismatch\n");
        exit(EXIT_FAILURE);
    }

    for (long unsigned int i = 0; i < (*selected_allreduction_operators).size(); i++) {
        string mpi_operator = reduction_mpi_operator((*selected_allreduction_operators).at(i));

        for (long unsigned int variable_index = 0; variable_index < (*selected_allreduction_variables).at(i).size(); variable_index++) {
            symbol_info *variable_symbol = table.get_symbol_info((*selected_allreduction_variables).at(i).at(variable_index));
            std::string uppercase_type;
            std::string lowercase_type;
            uppercase_type += variable_symbol->get_variable_type();
            lowercase_type += variable_symbol->get_variable_type();

            for (long unsigned int j = 0; j < variable_symbol->get_variable_type().size(); j++) {
                uppercase_type.at(j) = toupper(variable_symbol->get_variable_type().at(j));
                lowercase_type.at(j) = tolower(variable_symbol->get_variable_type().at(j));
            }

            emitted_reduction_code += (lowercase_type + " __" + (*selected_allreduction_variables).at(i).at(variable_index) + ";\n");

            emitted_reduction_code = emitted_reduction_code + "MPI_Allreduce(&" + (*selected_allreduction_variables).at(i).at(variable_index) + ", &__" + (*selected_allreduction_variables).at(i).at(variable_index) + ", 1, " + mpi_type_for_c_type(uppercase_type) +
             ", " + mpi_operator + ", MPI_COMM_WORLD);\n";

            emitted_reduction_code = emitted_reduction_code + (*selected_allreduction_variables).at(i).at(variable_index) + " = __" + (*selected_allreduction_variables).at(i).at(variable_index) + ";\n";
        }
    }

    output << emitted_reduction_code << endl;

    (*selected_allreduction_operators).clear();
    (*selected_allreduction_variables).clear();
}

void reduction_group_pending_vars_by_operator() {
    for (int i = 0; i < 7; i++) {
        grouped_reduction_vars.push_back({});
    }

    for (unsigned long int i = 0; i < pending_reduction_clauses.size(); i++) {
        string variable_name;
        string reduction_operator;
        int parse_state = 0;
        grouped_reduction_vars.push_back({});

        for (unsigned long int j = 0; j < pending_reduction_clauses.at(i).size(); j++) {
            switch(parse_state) {
                case 0: if (pending_reduction_clauses.at(i).at(j) == ':') {parse_state = 1; continue;}
                        reduction_operator += pending_reduction_clauses.at(i).at(j);
                        break;
                case 1: if (pending_reduction_clauses.at(i).at(j) == ',') {
                            if (reduction_operator == "+") {
                                grouped_reduction_vars.at(0).push_back(variable_name);
                            }
                            else if (reduction_operator == "*") {
                                grouped_reduction_vars.at(1).push_back(variable_name);
                            }
                            else if (reduction_operator == "max") {
                                grouped_reduction_vars.at(2).push_back(variable_name);
                            }
                            else if (reduction_operator == "min") {
                                grouped_reduction_vars.at(3).push_back(variable_name);
                            }
                            else if (reduction_operator == "&") {
                                grouped_reduction_vars.at(4).push_back(variable_name);
                            }
                            else if (reduction_operator == "|") {
                                grouped_reduction_vars.at(5).push_back(variable_name);
                            }
                            else if (reduction_operator == "^") {
                                grouped_reduction_vars.at(6).push_back(variable_name);
                            }
                            else {
                                fprintf(stderr, "Invalid reduction operator\n");
                                exit(EXIT_FAILURE);
                            }

                            variable_name = "";
                            continue;
                        }

                        variable_name += pending_reduction_clauses.at(i).at(j);
                        
                        if (j == pending_reduction_clauses.at(i).size() - 1) {
                            if (reduction_operator == "+") {
                                grouped_reduction_vars.at(0).push_back(variable_name);
                            }
                            else if (reduction_operator == "*") {
                                grouped_reduction_vars.at(1).push_back(variable_name);
                            }
                            else if (reduction_operator == "max") {
                                grouped_reduction_vars.at(2).push_back(variable_name);
                            }
                            else if (reduction_operator == "min") {
                                grouped_reduction_vars.at(3).push_back(variable_name);
                            }
                            else if (reduction_operator == "&") {
                                grouped_reduction_vars.at(4).push_back(variable_name);
                            }
                            else if (reduction_operator == "|") {
                                grouped_reduction_vars.at(5).push_back(variable_name);
                            }
                            else if (reduction_operator == "^") {
                                grouped_reduction_vars.at(6).push_back(variable_name);
                            }
                            else {
                                fprintf(stderr, "Invalid reduction operator\n");
                                exit(EXIT_FAILURE);
                            }
                        }
            }
        }
    }

    pending_reduction_clauses.clear();
}

string reduction_build_distribute_clause(int reduction_index) {
    string reduction = " reduction(";

    reduction += to_lowercase(distribute_reduction_operators.at(reduction_index));

    reduction += ":";

    int op;
    int written_variable_count = 0;

    for (unsigned long int i = 0; i < distribute_reduction_variables.at(reduction_index).size(); i++) {
        if (strcmp(distribute_reduction_operators.at(reduction_index), "+") == 0) {
            op = 0;
        }
        else if (strcmp(distribute_reduction_operators.at(reduction_index), "*") == 0) {
            op = 1;
        }
        else if (strcmp(distribute_reduction_operators.at(reduction_index), "MAX") == 0) {
            op = 2;
        }
        else if (strcmp(distribute_reduction_operators.at(reduction_index), "MIN") == 0) {
            op = 3;
        }
        else if (strcmp(distribute_reduction_operators.at(reduction_index), "&") == 0) {
            op = 4;
        }
        else if (strcmp(distribute_reduction_operators.at(reduction_index), "|") == 0) {
            op = 5;
        }
        else if (strcmp(distribute_reduction_operators.at(reduction_index), "^") == 0) {
            op = 6;
        }
        else {
            fprintf(stderr, "Invalid reduction operator\n");
            exit(EXIT_FAILURE);
        }

        int already_grouped = 0;
        for (unsigned long int j = 0; j < grouped_reduction_vars.at(op).size(); j++) {
            if (strcmp(distribute_reduction_variables.at(reduction_index).at(i), grouped_reduction_vars.at(op).at(j).data()) == 0) {
                already_grouped = 1;
                break;
            }
        }

        if (!already_grouped) {
            if (written_variable_count > 0) {
                reduction += ", ";
            }
            reduction += distribute_reduction_variables.at(reduction_index).at(i);
            written_variable_count++;
        }
    }

    reduction += ")";

    if (written_variable_count == 0) {
        reduction = "";
    }

    return reduction;
}

string allreduction_build_distribute_clause(int reduction_index) {
    string allreduction_clause = " reduction(";

    allreduction_clause += to_lowercase(distribute_allreduction_operators.at(reduction_index));

    allreduction_clause += ":";

    for (long unsigned int i = 0; i < distribute_allreduction_variables.at(reduction_index).size(); i++) {
        if (i != 0) {
            allreduction_clause += ", ";
        }
        allreduction_clause += distribute_allreduction_variables.at(reduction_index).at(i);
    }

    allreduction_clause += ")";

    return allreduction_clause;
}
