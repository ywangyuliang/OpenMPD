#include "scatter_gather_transform.h"

#include "symbol_table.h"
#include "codegen_utils.h"
#include "pragma_args.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>

using namespace std;

extern ofstream output;
extern symbol_table table;

int scatter_clause_active = 0;
int gather_clause_active = 0;
int allgather_clause_active = 0;
int gather_clause_parsing = 0;
int allgather_clause_parsing = 0;
int chunk_pos = 0;

vector<string> scatter_build_argument_parts(){
	vector<string> result = split_matrix_reference(current_pragma_args.at(0));
	result.push_back(current_pragma_args.at(1));
	return result;
}

vector<string> scatter_build_halo_argument_parts(){
	vector<string> result;
	append_matrix_reference_parts(result, current_pragma_args.at(0));
	result.push_back(current_pragma_args.at(1));
	return result;
}

void scatter_emit_chunked(std::vector<const char *> scatter_args) {
    std::vector<std::string> parts = parse_array_reference_parts(std::string(scatter_args.at(0)));
    std::string chunk = scatter_args.at(1);

    symbol_info *symbol = table.get_symbol_info(parts.at(0));

    string element_multiplier = "";

    for (long unsigned int i = 2; i < parts.size(); i++) {
        element_multiplier += "*";
        element_multiplier += parts.at(i);
    }

    string scatter =
        "{\n\tint __offset = 0;\n\tint *__displs = (int *) malloc(sizeof(int) * __numprocs);\n\tint *__counts = (int *) malloc(sizeof(int) * __numprocs);\n\t" +
        to_lowercase(symbol->get_variable_type()) + " *__" + parts.at(0) + "Aux;\n" +
        "\tif (__taskid == 0) {\n" +
        "\t\t__" + parts.at(0) + "Aux = (" + to_lowercase(symbol->get_variable_type()) + " *) malloc(sizeof(" +
        to_lowercase(symbol->get_variable_type()) + ")*" + parts.at(1) + element_multiplier + ");\n" +
        "\t\tmemcpy(__" + parts.at(0) + "Aux, " + parts.at(0) + ", sizeof(" + to_lowercase(symbol->get_variable_type()) + ")*" + parts.at(1) + element_multiplier + ");\n" +
        "\t}\n\n" +

        "\twhile (__offset < " + parts.at(1) + element_multiplier + ") {\n" +
        "\t\tif (__taskid == 0) {\n" +
        "\t\t\tfor (int __gather = 0; __gather < __numprocs; __gather++) {\n" +
        "\t\t\t\tif (__offset < " + parts.at(1) + element_multiplier + ") {\n" +
        "\t\t\t\t\t__counts[__gather] = " + chunk + ";\n" +
        "\t\t\t\t\t__displs[__gather] = __offset;\n" +
        "\t\t\t\t\t__offset += " + chunk + ";\n" +
        "\t\t\t\t}\n" +
        "\t\t\t\telse {\n" +
        "\t\t\t\t\t__counts[__gather] = 0;\n" +
        "\t\t\t\t\t__displs[__gather] = " + parts.at(1) + element_multiplier + ";\n" +
        "\t\t\t\t}\n" +
        "\t\t\t}\n" +
        "\t\t}\n" +
        "\t\telse {\n" +
        "\t\t\tif (__offset + __taskid*" + chunk + " < " + parts.at(1) + element_multiplier + ") {\n" +
        "\t\t\t\t__counts[__taskid] = " + chunk + ";\n" +
        "\t\t\t\t__displs[__taskid] = __offset + __taskid*" + chunk + ";\n" +
        "\t\t\t\t__offset += __numprocs*" + chunk + ";\n" +
        "\t\t\t}\n" +
        "\t\t\telse {\n" +
        "\t\t\t\t__counts[__taskid] = 0;\n" +
        "\t\t\t\t__displs[__taskid] = " + parts.at(1) + element_multiplier + ";\n" +
        "\t\t\t\t__offset += __numprocs*" + chunk + ";\n" +
        "\t\t\t}\n" +
        "\t\t}\n\n" +

        "\t\tMPI_Scatterv(__" + parts.at(0) + "Aux, __counts, __displs, " + mpi_type_for_c_type(symbol->get_variable_type()) + ", &" + parts.at(0);

        for (size_t j = 0; j < symbol->get_array_dimension_count(); j++) {
            scatter += "[0]";
        }

        scatter += ("+__displs[__taskid], __counts[__taskid], " + mpi_type_for_c_type(symbol->get_variable_type()) + ", 0, MPI_COMM_WORLD);\n" +

        "\t}\n" +
        "}\n");

    output << scatter << endl;
}

void scatter_emit_partitioned(std::vector<const char *> scatter_args) {
    std::vector<std::string> parts = parse_array_reference_parts(std::string(scatter_args.at(0)));

    symbol_info *symbol = table.get_symbol_info(parts.at(0));

    string element_multiplier = "";

    for (long unsigned int i = 2; i < parts.size(); i++) {
        element_multiplier += "*";
        element_multiplier += parts.at(i);
    }

    string scatter =
        "{\n\tint __chunk;\n\tint *__displs = (int *) malloc(sizeof(int) * __numprocs);\n\tint *__counts = (int *) malloc(sizeof(int) * __numprocs);\n\t" +
        to_lowercase(symbol->get_variable_type()) + " *__" + parts.at(0) + "Aux;\n" +
        "\t__chunk = (" + parts.at(1) + " / __numprocs);\n" +
        "\t__displs[__taskid] = __chunk*__taskid" + element_multiplier + ";\n" +
        "\tif (__taskid == 0) {\n" +
        "\t\t" + "__" + parts.at(0) + "Aux = (" + to_lowercase(symbol->get_variable_type()) + " *) malloc(sizeof(" +
        to_lowercase(symbol->get_variable_type()) + ")*" + parts.at(1) + element_multiplier + ");\n" +
        "\t\tmemcpy(__" + parts.at(0) + "Aux, " + parts.at(0) + ", sizeof(" + to_lowercase(symbol->get_variable_type()) + ")*" + parts.at(1) + element_multiplier + ");\n" +
        "\t}\n\n" +

        "\tif (__taskid < (" + parts.at(1) + " % __numprocs)) {\n" +
        "\t\t__counts[__taskid] = (__chunk + 1)" + element_multiplier + ";\n" +
        "\t\t__displs[__taskid] += __taskid" + element_multiplier + ";\n" +
        "\t}\n" +
        "\telse {\n" +
        "\t\t__counts[__taskid] = __chunk" + element_multiplier + ";\n" +
        "\t\t__displs[__taskid] += (" + parts.at(1) + " % __numprocs)" + element_multiplier + ";\n" +
        "\t}\n\n" +

        "\tif (__taskid == 0) {\n" +
        "\t\t__displs[0] = 0;\n\n" +
        "\t\tfor (int __gather = 1; __gather < __numprocs; __gather++) {\n" +
        "\t\t\tif (__gather < (" + parts.at(1) + " % __numprocs)) {\n" +
        "\t\t\t\t__counts[__gather] = (__chunk + 1)" + element_multiplier + ";\n" +
        "\t\t\t\t__displs[__gather] = __displs[__gather - 1] + (__chunk + 1)" + element_multiplier + ";\n" +
        "\t\t\t}\n" +
        "\t\t\telse if (__gather == (" + parts.at(1) + " % __numprocs)) {\n" +
        "\t\t\t\t__counts[__gather] = __chunk" + element_multiplier + ";\n" +
        "\t\t\t\t__displs[__gather] = __displs[__gather - 1] + (__chunk + 1)" + element_multiplier + ";\n" +
        "\t\t\t}\n" +
        "\t\t\telse {\n" +
        "\t\t\t\t__counts[__gather] = __chunk" + element_multiplier + ";\n" +
        "\t\t\t\t__displs[__gather] = __displs[__gather - 1] + __chunk" + element_multiplier + ";\n" +
        "\t\t\t}\n" +
        "\t\t}\n\n" +
        "\t\tassert((__displs[__numprocs - 1] + __counts[__numprocs - 1]) == " + parts.at(1) + element_multiplier + ");\n" +
        "\t}\n\n" +

        "\tMPI_Scatterv(__" + parts.at(0) + "Aux, __counts, __displs, " + mpi_type_for_c_type(symbol->get_variable_type()) + ", &" + parts.at(0);

        for (size_t j = 0; j < symbol->get_array_dimension_count(); j++) {
            scatter += "[0]";
        }

        scatter += ("+__displs[__taskid], __counts[__taskid], " + mpi_type_for_c_type(symbol->get_variable_type()) + ", 0, MPI_COMM_WORLD);\n" +

        "}\n");

    output << scatter << endl;
}

void scatter_emit_all() {
    for (unsigned long int i = 0; i < scatter_clause_arg_groups.size(); i++) {
        if (scatter_clause_arg_groups.at(i).size() == 1) {
            scatter_emit_partitioned(scatter_clause_arg_groups.at(i));
        }
        else if (scatter_clause_arg_groups.at(i).size() == 2) {
            scatter_emit_chunked(scatter_clause_arg_groups.at(i));
        }
        else {
            fprintf(stderr, "Invalid scatter argument count: %ld\n", scatter_clause_arg_groups.at(i).size());
            exit(EXIT_FAILURE);
        }
    }

    scatter_clause_arg_groups.clear();
}

void gather_emit_chunked(std::vector<const char *> gather_args) {
    std::vector<std::string> parts = parse_array_reference_parts(std::string(gather_args.at(0)));
    std::string chunk = gather_args.at(1);

    symbol_info *symbol = table.get_symbol_info(parts.at(0));

    string element_multiplier = "";

    for (long unsigned int i = 2; i < parts.size(); i++) {
        element_multiplier += "*";
        element_multiplier += parts.at(i);
    }

    string gather =
        "{\n\tint __offset = 0;\n\tint *__displs = (int *) malloc(sizeof(int) * __numprocs);\n\tint *__counts = (int *) malloc(sizeof(int) * __numprocs);\n\t" +
        to_lowercase(symbol->get_variable_type()) + " *__" + parts.at(0) + "Aux;\n" +
        "\tif (__taskid == 0) {\n" +
        "\t\t__" + parts.at(0) + "Aux = (" + to_lowercase(symbol->get_variable_type()) + " *) malloc(sizeof(" +
        to_lowercase(symbol->get_variable_type()) + ")*" + parts.at(1) + element_multiplier + ");\n" +
        "\t}\n\n" +

        "\twhile (__offset < " + parts.at(1) + element_multiplier + ") {\n" +
        "\t\tif (__taskid == 0) {\n" +
        "\t\t\tfor (int __gather = 0; __gather < __numprocs; __gather++) {\n" +
        "\t\t\t\tif (__offset < " + parts.at(1) + element_multiplier + ") {\n" +
        "\t\t\t\t\t__counts[__gather] = " + chunk + ";\n" +
        "\t\t\t\t\t__displs[__gather] = __offset;\n" +
        "\t\t\t\t\t__offset += " + chunk + ";\n" +
        "\t\t\t\t}\n" +
        "\t\t\t\telse {\n" +
        "\t\t\t\t\t__counts[__gather] = 0;\n" +
        "\t\t\t\t\t__displs[__gather] = " + parts.at(1) + element_multiplier + ";\n" +
        "\t\t\t\t}\n" +
        "\t\t\t}\n" +
        "\t\t}\n" +
        "\t\telse {\n" +
        "\t\t\tif (__offset + __taskid*" + chunk + " < " + parts.at(1) + element_multiplier + ") {\n" +
        "\t\t\t\t__counts[__taskid] = " + chunk + ";\n" +
        "\t\t\t\t__displs[__taskid] = __offset + __taskid*" + chunk + ";\n" +
        "\t\t\t\t__offset += __numprocs*" + chunk + ";\n" +
        "\t\t\t}\n" +
        "\t\t\telse {\n" +
        "\t\t\t\t__counts[__taskid] = 0;\n" +
        "\t\t\t\t__displs[__taskid] = " + parts.at(1) + element_multiplier + ";\n" +
        "\t\t\t\t__offset += __numprocs*" + chunk + ";\n" +
        "\t\t\t}\n" +
        "\t\t}\n\n" +

        "\t\tMPI_Gatherv(&" + parts.at(0);

        for (size_t j = 0; j < symbol->get_array_dimension_count(); j++) {
            gather += "[0]";
        }

        gather += ("+__displs[__taskid], __counts[__taskid], " + mpi_type_for_c_type(symbol->get_variable_type()) +
        ", __" + parts.at(0) + "Aux, __counts, __displs, " + mpi_type_for_c_type(symbol->get_variable_type()) + ", 0, MPI_COMM_WORLD);\n" +

        "\t}\n" +
        "\tif (__taskid == 0) {\n" +
        "\t\tmemcpy(" + parts.at(0) + ", __" + parts.at(0) + "Aux, sizeof(" + to_lowercase(symbol->get_variable_type()) + ")*" + parts.at(1) + element_multiplier + ");\n" +
        "\t}\n" +
        "}\n");

    output << gather << endl;
}

void gather_emit_partitioned(std::vector<const char *> gather_args) {
    std::vector<std::string> parts = parse_array_reference_parts(std::string(gather_args.at(0)));

    symbol_info *symbol = table.get_symbol_info(parts.at(0));

    string element_multiplier = "";

    for (long unsigned int i = 2; i < parts.size(); i++) {
        element_multiplier += "*";
        element_multiplier += parts.at(i);
    }

    string gather =
        "{\n\tint __chunk;\n\tint *__displs = (int *) malloc(sizeof(int) * __numprocs);\n\tint *__counts = (int *) malloc(sizeof(int) * __numprocs);\n\t" +
        to_lowercase(symbol->get_variable_type()) + " *__" + parts.at(0) + "Aux;\n" +
        "\t__chunk = (" + parts.at(1) + " / __numprocs);\n" +
        "\t__displs[__taskid] = __chunk*__taskid" + element_multiplier + ";\n" +
        "\tif (__taskid == 0) {\n" +
        "\t\t" + "__" + parts.at(0) + "Aux = (" + to_lowercase(symbol->get_variable_type()) + " *) malloc(sizeof(" +
        to_lowercase(symbol->get_variable_type()) + ")*" + parts.at(1) + element_multiplier + ");\n" +
        "\t}\n\n" +

        "\tif (__taskid < (" + parts.at(1) + " % __numprocs)) {\n" +
        "\t\t__counts[__taskid] = (__chunk + 1)" + element_multiplier + ";\n" +
        "\t\t__displs[__taskid] += __taskid" + element_multiplier + ";\n" +
        "\t}\n" +
        "\telse {\n" +
        "\t\t__counts[__taskid] = __chunk" + element_multiplier + ";\n" +
        "\t\t__displs[__taskid] += (" + parts.at(1) + " % __numprocs)" + element_multiplier + ";\n" +
        "\t}\n\n" +

        "\tif (__taskid == 0) {\n" +
        "\t\t__displs[0] = 0;\n\n" +
        "\t\tfor (int __gather = 1; __gather < __numprocs; __gather++) {\n" +
        "\t\t\tif (__gather < (" + parts.at(1) + " % __numprocs)) {\n" +
        "\t\t\t\t__counts[__gather] = (__chunk + 1)" + element_multiplier + ";\n" +
        "\t\t\t\t__displs[__gather] = __displs[__gather - 1] + (__chunk + 1)" + element_multiplier + ";\n" +
        "\t\t\t}\n" +
        "\t\t\telse if (__gather == (" + parts.at(1) + " % __numprocs)) {\n" +
        "\t\t\t\t__counts[__gather] = __chunk" + element_multiplier + ";\n" +
        "\t\t\t\t__displs[__gather] = __displs[__gather - 1] + (__chunk + 1)" + element_multiplier + ";\n" +
        "\t\t\t}\n" +
        "\t\t\telse {\n" +
        "\t\t\t\t__counts[__gather] = __chunk" + element_multiplier + ";\n" +
        "\t\t\t\t__displs[__gather] = __displs[__gather - 1] + __chunk" + element_multiplier + ";\n" +
        "\t\t\t}\n" +
        "\t\t}\n\n" +
        "\t\tassert((__displs[__numprocs - 1] + __counts[__numprocs - 1]) == " + parts.at(1) + element_multiplier + ");\n" +
        "\t}\n\n" +

        "\tMPI_Gatherv(&" + parts.at(0);

        for (size_t j = 0; j < symbol->get_array_dimension_count(); j++) {
            gather += "[0]";
        }

        gather += ("+__displs[__taskid], __counts[__taskid], " + mpi_type_for_c_type(symbol->get_variable_type()) +
        ", __" + parts.at(0) + "Aux, __counts, __displs, " + mpi_type_for_c_type(symbol->get_variable_type()) + ", 0, MPI_COMM_WORLD);\n" +

        "\tif (__taskid == 0) {\n" +
        "\t\tmemcpy(" + parts.at(0) + ", __" + parts.at(0) + "Aux, sizeof(" + to_lowercase(symbol->get_variable_type()) + ")*" + parts.at(1) + element_multiplier + ");\n" +
        "\t}\n"
        "}\n");

    output << gather << endl;
}

void gather_emit_all() {
    for (unsigned long int i = 0; i < gather_clause_arg_groups.size(); i++) {
        if (gather_clause_arg_groups.at(i).size() == 1) {
            gather_emit_partitioned(gather_clause_arg_groups.at(i));
        }
        else if (gather_clause_arg_groups.at(i).size() == 2) {
            gather_emit_chunked(gather_clause_arg_groups.at(i));
        }
        else {
            fprintf(stderr, "Invalid gather argument count: %ld\n", gather_clause_arg_groups.at(i).size());
            exit(EXIT_FAILURE);
        }
    }

    gather_clause_arg_groups.clear();
}

void allgather_emit_chunked(std::vector<const char *> gather_args) {
    std::vector<std::string> parts = parse_array_reference_parts(gather_args.at(0));
    std::string chunk = gather_args.at(1);

    symbol_info *symbol = table.get_symbol_info(parts.at(0));

    string element_multiplier = "";

    for (long unsigned int i = 2; i < parts.size(); i++) {
        element_multiplier += "*";
        element_multiplier += parts.at(i);
    }

    string gather =
        "{\n\tint __offset = 0;\n\tint *__displs = (int *) malloc(sizeof(int) * __numprocs);\n\tint *__counts = (int *) malloc(sizeof(int) * __numprocs);\n\t" +
        to_lowercase(symbol->get_variable_type()) + " *__" + parts.at(0) + "Aux = (" + to_lowercase(symbol->get_variable_type()) + " *) malloc(sizeof(" +
        to_lowercase(symbol->get_variable_type()) + ")*" + parts.at(1) + element_multiplier + ");\n\n" +

        "\twhile (__offset < " + parts.at(1) + element_multiplier + ") {\n" +
        "\t\tfor (int __gather = 0; __gather < __numprocs; __gather++) {\n" +
        "\t\t\tif (__offset < " + parts.at(1) + element_multiplier + ") {\n" +
        "\t\t\t\t__counts[__gather] = " + chunk + ";\n" +
        "\t\t\t\t__displs[__gather] = __offset;\n" +
        "\t\t\t\t__offset += " + chunk + ";\n" +
        "\t\t\t}\n" +
        "\t\t\telse {\n" +
        "\t\t\t\t__counts[__gather] = 0;\n" +
        "\t\t\t\t__displs[__gather] = " + parts.at(1) + element_multiplier + ";\n" +
        "\t\t\t}\n" +
        "\t\t}\n" +

        "\t\tMPI_Allgatherv(" + parts.at(0) + "+__displs[__taskid], __counts[__taskid], " + mpi_type_for_c_type(symbol->get_variable_type()) +
        ", __" + parts.at(0) + "Aux, __counts, __displs, " + mpi_type_for_c_type(symbol->get_variable_type()) + ", MPI_COMM_WORLD);\n" +

        "\t}\n" +
        "\tmemcpy(" + parts.at(0) + ", __" + parts.at(0) + "Aux, sizeof(" + to_lowercase(symbol->get_variable_type()) + ")*" + parts.at(1) + element_multiplier + ");\n" +
        "}\n";

    output << gather << endl;
}

void allgather_emit_partitioned(std::vector<const char *> gather_args) {
    std::vector<std::string> parts = parse_array_reference_parts(std::string(gather_args.at(0)));

    symbol_info *symbol = table.get_symbol_info(parts.at(0));

    string element_multiplier = "";

    for (long unsigned int i = 2; i < parts.size(); i++) {
        element_multiplier += "*";
        element_multiplier += parts.at(i);
    }

    string gather =
        "{\n\tint __chunk;\n\tint *__displs = (int *) malloc(sizeof(int) * __numprocs);\n\tint *__counts = (int *) malloc(sizeof(int) * __numprocs);\n\t" +
        to_lowercase(symbol->get_variable_type()) + " *__" + parts.at(0) + "Aux = (" + to_lowercase(symbol->get_variable_type()) + " *) malloc(sizeof(" +
        to_lowercase(symbol->get_variable_type()) + ")*" + parts.at(1) + element_multiplier + ");\n" +
        "\t__chunk = (" + parts.at(1) + " / __numprocs);\n\n" +

        "\t__displs[0] = 0;\n" +
        "\tif (0 < (" + parts.at(1) + " % __numprocs)) {\n" +
        "\t\t__counts[0] = (__chunk + 1)" + element_multiplier + ";\n" +
        "\t}\n" +
        "\telse {\n" +
        "\t\t__counts[0] = __chunk" + element_multiplier + ";\n" +
        "\t}\n\n" +

        "\tfor (int __gather = 1; __gather < __numprocs; __gather++) {\n" +
        "\t\tif (__gather < (" + parts.at(1) + " % __numprocs)) {\n" +
        "\t\t\t__counts[__gather] = (__chunk + 1)" + element_multiplier + ";\n" +
        "\t\t\t__displs[__gather] = __displs[__gather - 1] + (__chunk + 1)" + element_multiplier + ";\n" +
        "\t\t}\n" +
        "\t\telse if (__gather == (" + parts.at(1) + " % __numprocs)) {\n" +
        "\t\t\t__counts[__gather] = __chunk" + element_multiplier + ";\n" +
        "\t\t\t__displs[__gather] = __displs[__gather - 1] + (__chunk + 1)" + element_multiplier + ";\n" +
        "\t\t}\n" +
        "\t\telse {\n" +
        "\t\t\t__counts[__gather] = __chunk" + element_multiplier + ";\n" +
        "\t\t\t__displs[__gather] = __displs[__gather - 1] + __chunk" + element_multiplier + ";\n" +
        "\t\t}\n" +
        "\t}\n\n" +
        "\tassert((__displs[__numprocs - 1] + __counts[__numprocs - 1]) == " + parts.at(1) + element_multiplier + ");\n\n" +

        "\tMPI_Allgatherv(" + parts.at(0) + "+__displs[__taskid], __counts[__taskid], " + mpi_type_for_c_type(symbol->get_variable_type()) +
        ", __" + parts.at(0) + "Aux, __counts, __displs, " + mpi_type_for_c_type(symbol->get_variable_type()) + ", MPI_COMM_WORLD);\n" +

        "\tmemcpy(" + parts.at(0) + ", __" + parts.at(0) + "Aux, sizeof(" + to_lowercase(symbol->get_variable_type()) + ")*" + parts.at(1) + element_multiplier + ");\n" +
        "}\n";

    output << gather << endl;
}

void allgather_emit_all() {
    for (unsigned long int i = 0; i < allgather_clause_arg_groups.size(); i++) {
        if (allgather_clause_arg_groups.at(i).size() == 1) {
            allgather_emit_partitioned(allgather_clause_arg_groups.at(i));
        }
        else if (allgather_clause_arg_groups.at(i).size() == 2) {
            allgather_emit_chunked(allgather_clause_arg_groups.at(i));
        }
        else {
            fprintf(stderr, "Invalid allgather argument count: %ld\n", allgather_clause_arg_groups.at(i).size());
            exit(EXIT_FAILURE);
        }
    }

    allgather_clause_arg_groups.clear();
}

