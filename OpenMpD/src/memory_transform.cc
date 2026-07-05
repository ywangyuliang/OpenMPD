#include "memory_transform.h"

#include "symbol_table.h"
#include "codegen_utils.h"
#include "pragma_args.h"

#include <cstdlib>
#include <fstream>

using namespace std;

extern ofstream output, errFile;
extern symbol_table table;

    void memory_emit_cluster_allocations() {
        if(current_pragma_args.size() < 1){
            errFile << "Error: Alloc pragma must have 1 argument and it has " << current_pragma_args.size() << endl;
            exit(EXIT_FAILURE);
        }

        string allocation_code = "";

        for (long unsigned int i = 0; i < current_pragma_args.size(); i++) {
            const char *arg = current_pragma_args.at(i);
            vector<string> values = parse_array_reference_parts(std::string(arg));
            symbol_info *symbol = table.get_symbol_info(values.at(0));

            allocation_code += "if (__taskid != 0) {\n";
            allocation_code += ("\t" + values.at(0) + " = (" + to_lowercase(symbol->get_variable_type()) + " ");
            for (long unsigned int j = 1; j < values.size(); j++) {
                allocation_code += "*";
            }
            allocation_code += (") malloc(" + values.at(1) + " * sizeof(" + to_lowercase(symbol->get_variable_type()) + " ");
            for (long unsigned int j = 2; j < values.size(); j++) {
                allocation_code += "*";
            }
            allocation_code += "));\n";

            for (long unsigned int j = 2; j < values.size(); j++) {
                string indentation = "";
                for (long unsigned int k = 1; k < j; k++) {
                    indentation += "\t";
                }

                allocation_code += (indentation + "for (int __alloc" + std::to_string(j-2) + " = 0; __alloc" + std::to_string(j-2)
                + " < " + values.at(j) + "; __alloc" + std::to_string(j-2) + "++) {\n");

                allocation_code += "\t" + indentation + values.at(0);

                for (long unsigned int k = 0; k < j - 1; k++) {
                    allocation_code += "[__alloc" + std::to_string(k) + "]";
                }

                allocation_code += (" = (" + to_lowercase(symbol->get_variable_type()) + " ");
                for (long unsigned int k = j; k < values.size(); k++) {
                    allocation_code += "*";
                }
                allocation_code += (") malloc(" + values.at(j) + " * sizeof(" + to_lowercase(symbol->get_variable_type()) + " ");
                for (long unsigned int k = j + 1; k < values.size(); k++) {
                    allocation_code += "*";
                }
                allocation_code += ("));\n");
            }

            for (long unsigned int j = 2; j < values.size(); j++) {
                allocation_code += "\t}\n";
            }

            allocation_code += "}\n";

            output  << allocation_code << endl;

            allocation_code = "";
        }

        current_pragma_args.clear();
    }

    void memory_emit_broadcasts(){
	string broadcast_code = "";
        for(const auto& arg : current_pragma_args){
            std::vector<std::string> values = parse_array_reference_parts(arg);
            if (values.size() == 0) {
                fprintf(stderr, "Missing variable in broadcast\n");
                exit(EXIT_FAILURE);
            }
            else if (values.size() == 1) {
                symbol_info *symbol = table.get_symbol_info(values.at(0));
                if(symbol->is_array()){
                    broadcast_code += "MPI_Bcast(" + string(values.at(0)) + ", " + symbol->get_size_list() + ", " + mpi_type_for_c_type(symbol->get_variable_type())  + ", 0, MPI_COMM_WORLD);\n";
                }
                else{
                    broadcast_code += "MPI_Bcast(&" + string(values.at(0)) + ", 1, " + mpi_type_for_c_type(symbol->get_variable_type()) + ", 0, MPI_COMM_WORLD);\n";
                }
            }
            else {
                symbol_info *symbol = table.get_symbol_info(values.at(0));
                string mult = "";
                for (unsigned long int i = 2; i < values.size(); i++) {
                    mult += "*";
                    mult += values.at(i);
                }

		    broadcast_code += "MPI_Bcast(" + string(values.at(0)) + ", " + values.at(1) + mult + ", " + mpi_type_for_c_type(symbol->get_variable_type())  + ", 0, MPI_COMM_WORLD);\n";
            }
	}
	output << broadcast_code << endl;
        current_pragma_args.clear();
    }
