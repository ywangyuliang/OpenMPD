#include "mpi_type_transform.h"

#include "codegen_utils.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <vector>

using namespace std;

extern ofstream output;

char *mpi_type_declaration_buffer = NULL;
int mpi_type_declaration_buffer_size = 0;
int mpi_type_declaration_scope_active = 0;

string mpi_type_declarations = "void ompd_init_mpi_types() {\n";

void mpi_type_emit_cluster_declarations() {
    if ((mpi_type_declaration_buffer = (char *) realloc(mpi_type_declaration_buffer, mpi_type_declaration_buffer_size + 1)) == NULL) {
        fprintf(stderr, "Dynamic memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    mpi_type_declaration_buffer[mpi_type_declaration_buffer_size] = '\0';
    
    int positionDeclare = 0;
    char *struct_start;
    std::vector<std::vector<std::vector<std::string>>> struct_fields;
    int struct_index = 0;
    std::vector<int> field_counts;

    while ((struct_start = strstr(mpi_type_declaration_buffer + positionDeclare, "struct")) != NULL) {
        mpi_declared_types.push_back({});
        struct_fields.push_back({});
        field_counts.push_back(0);
        
        int is_typedef;
        if (strstr(mpi_type_declaration_buffer, "typedef") != NULL) {
            is_typedef = 1;
        }
        else {
            is_typedef = 0;
        }

        int parser_state = 0;
        int type_token_count = 0;
        std::string name = "";
        int field_index = 0;
        int named_type_added = 0;

        for (int i = struct_start - mpi_type_declaration_buffer + 6; i < mpi_type_declaration_buffer_size; i++) {
            if (mpi_type_declaration_buffer[i] == ' ' || mpi_type_declaration_buffer[i] == '\t' || mpi_type_declaration_buffer[i] == '\n') {
                continue;
            }

            switch(parser_state) {
                case 0: if (mpi_type_declaration_buffer[i] == '{') {
                            parser_state = 1;
                            if (name.size() > 0) {
                                for (unsigned long int j = 0; j < mpi_declared_types.size(); j++) {
                                    for (unsigned long int k = 1; k < mpi_declared_types.at(j).size(); k++) {
                                        if (name.compare(mpi_declared_types.at(j).at(k)) == 0) {
                                            fprintf(stderr, "MPI type declared multiple times\n");
                                            exit(EXIT_FAILURE);
                                        }
                                    }
                                }

                                string mpi_type_name = name + "_type_MPI";

                                mpi_declared_types.at(mpi_declared_types.size() - 1).push_back(mpi_type_name);
                                mpi_declared_types.at(mpi_declared_types.size() - 1).push_back(name);

                                named_type_added = 1;
                            }
                            else {
                                if (!is_typedef) {
                                    fprintf(stderr, "The MPI type declaration is missing a name\n");
                                    exit(EXIT_FAILURE);
                                }
                            }
                            name = "";
                            continue;
                        }

                        name += mpi_type_declaration_buffer[i];
                        break;

                case 1: if (mpi_type_declaration_buffer[i] == '}') {
                            if (is_typedef) {
                                parser_state = 3;
                            }
                            else {
                                parser_state = 4;
                            }
                            break;
                        }

                        if (strncmp(mpi_type_declaration_buffer + i, "char", 4) == 0) {
                            if (type_token_count) {
                                name += " ";
                            }
                            name += "CHAR";
                            type_token_count++;
                            i += 3;
                        }
                        else if (strncmp(mpi_type_declaration_buffer + i, "int", 3) == 0) {
                            if (type_token_count) {
                                name += " ";
                            }
                            name += "INT";
                            type_token_count++;
                            i += 2;
                        }
                        else if (strncmp(mpi_type_declaration_buffer + i, "float", 5) == 0) {
                            if (type_token_count) {
                                name += " ";
                            }
                            name += "FLOAT";
                            type_token_count++;
                            i += 4;
                        }
                        else if (strncmp(mpi_type_declaration_buffer + i, "double", 6) == 0) {
                            if (type_token_count) {
                                name += " ";
                            }
                            name += "DOUBLE";
                            type_token_count++;
                            i += 5;
                        }
                        else if (strncmp(mpi_type_declaration_buffer + i, "bool", 4) == 0) {
                            if (type_token_count) {
                                name += " ";
                            }
                            name += "BOOL";
                            type_token_count++;
                            i += 3;
                        }
                        else if (strncmp(mpi_type_declaration_buffer + i, "short", 5) == 0) {
                            if (type_token_count) {
                                name += " ";
                            }
                            name += "SHORT";
                            type_token_count++;
                            i += 4;
                        }
                        else if (strncmp(mpi_type_declaration_buffer + i, "long", 4) == 0) {
                            if (type_token_count) {
                                name += " ";
                            }
                            name += "LONG";
                            type_token_count++;
                            i += 3;
                        }
                        else if (strncmp(mpi_type_declaration_buffer + i, "byte", 4) == 0) {
                            if (type_token_count) {
                                name += " ";
                            }
                            name += "BYTE";
                            type_token_count++;
                            i += 3;
                        }
                        else if (strncmp(mpi_type_declaration_buffer + i, "complex", 7) == 0) {
                            if (type_token_count) {
                                name += " ";
                            }
                            name += "COMPLEX";
                            type_token_count++;
                            i += 6;
                        }
                        else if (strncmp(mpi_type_declaration_buffer + i, "signed", 6) == 0) {
                            if (type_token_count) {
                                name += " ";
                            }
                            name += "SIGNED";
                            type_token_count++;
                            i += 5;
                        }
                        else if (strncmp(mpi_type_declaration_buffer + i, "unsigned", 8) == 0) {
                            if (type_token_count) {
                                name += " ";
                            }
                            name += "UNSIGNED";
                            type_token_count++;
                            i += 7;
                        }
                        else {
                            parser_state = 2;

                            struct_fields.at(struct_index).push_back({});
                            struct_fields.at(struct_index).at(field_index).push_back(mpi_type_for_c_type(name));
                            struct_fields.at(struct_index).at(field_index).push_back(name);
                            name = "";
                            name += mpi_type_declaration_buffer[i];
                            continue;
                        }
                        break;

                case 2: if (mpi_type_declaration_buffer[i] == ',') {
                            struct_fields.at(struct_index).at(field_index).push_back(name);
                            field_counts.at(struct_index)++;
                            name = "";
                            continue;
                        }

                        if (mpi_type_declaration_buffer[i] == ';') {
                            struct_fields.at(struct_index).at(field_index).push_back(name);
                            field_counts.at(struct_index)++;
                            name = "";
                            parser_state = 1;
                            field_index++;
                            continue;
                        }

                        name += mpi_type_declaration_buffer[i];
                        break;

                case 3: if (mpi_type_declaration_buffer[i] == ';') {
                            for (unsigned long int j = 0; j < mpi_declared_types.size(); j++) {
                                for (unsigned long int k = 1; k < mpi_declared_types.at(j).size(); k++) {
                                    if (name.compare(mpi_declared_types.at(j).at(k)) == 0) {
                                        fprintf(stderr, "MPI type declared multiple times\n");
                                        exit(EXIT_FAILURE);
                                    }
                                }
                            }

                            if (!named_type_added) {
                                string mpi_type_name = name + "_type_MPI";
                                mpi_declared_types.at(mpi_declared_types.size() - 1).push_back(mpi_type_name);
                            }

                            mpi_declared_types.at(mpi_declared_types.size() - 1).push_back(name);
                            parser_state = 4;
                            break;
                        }

                        name += mpi_type_declaration_buffer[i];
                        break;
            }

            if (parser_state == 4) {
                positionDeclare = i;
                break;
            }
        }

        struct_index++;
    }

    for (unsigned long int i = 0; i < struct_fields.size(); i++) {
        string declaration;
        int type_index = mpi_declared_types.size() - struct_fields.size() + i;

        declaration = ("int __blocklengths_" + mpi_declared_types.at(type_index).at(1) + "[" + std::to_string(field_counts.at(i)) + "];\n" +
            "MPI_Datatype __old_types_" + mpi_declared_types.at(type_index).at(1) + "[" + std::to_string(field_counts.at(i)) + "];\n" +
            "MPI_Aint __disp_" + mpi_declared_types.at(type_index).at(1) + "[" + std::to_string(field_counts.at(i)) + "];\n" +
            "MPI_Aint __lb_" + mpi_declared_types.at(type_index).at(1) + ";\n" +
            "MPI_Aint __extent_" + mpi_declared_types.at(type_index).at(1) + ";\n");
        
        int field_flat_index = 0;
        
        for (unsigned long int j = 0; j < struct_fields.at(i).size(); j++) {
            for (unsigned long int k = 2; k < struct_fields.at(i).at(j).size(); k++) {
                string field_mpi_type = mpi_type_for_c_type(struct_fields.at(i).at(j).at(0));
                declaration += ("__blocklengths_" + mpi_declared_types.at(type_index).at(1) + "[" + std::to_string(field_flat_index) + "] = 1;\n");
                declaration += ("__old_types_" + mpi_declared_types.at(type_index).at(1) + "[" + std::to_string(field_flat_index) + "] = " + field_mpi_type + ";\n");
                
                if (field_flat_index == 0) {
                    declaration += ("MPI_Type_get_extent(" + field_mpi_type + ", &__lb_" + mpi_declared_types.at(type_index).at(1) + ", &__extent_" + mpi_declared_types.at(type_index).at(1) + ");\n");
                    declaration += ("__disp_" + mpi_declared_types.at(type_index).at(1) + "[" + std::to_string(field_flat_index) + "] = __lb_" + mpi_declared_types.at(type_index).at(1) + ";\n");
                }
                else {
                    declaration += ("__disp_" + mpi_declared_types.at(type_index).at(1) + "[" + std::to_string(field_flat_index) + "] = __disp_" + mpi_declared_types.at(type_index).at(1) + 
                    "[" + std::to_string(field_flat_index - 1) + "] + __extent_" + mpi_declared_types.at(type_index).at(1) + ";\n");
                    declaration += ("MPI_Type_get_extent(" + field_mpi_type + ", &__lb_" + mpi_declared_types.at(type_index).at(1) + ", &__extent_" + mpi_declared_types.at(type_index).at(1) + ");\n");
                }

                field_flat_index++;
            }
        }

        declaration += ("MPI_Type_create_struct(" + std::to_string(field_counts.at(i)) + ", __blocklengths_" + mpi_declared_types.at(type_index).at(1) + ", __disp_" 
        + mpi_declared_types.at(type_index).at(1) + ", __old_types_" + mpi_declared_types.at(type_index).at(1) + ", &" + mpi_declared_types.at(type_index).at(0) + ");\n");
        declaration += ("MPI_Type_commit(&" + mpi_declared_types.at(type_index).at(0) + ");\n");

        mpi_type_declarations += (declaration + "\n");

        output << "MPI_Datatype " << mpi_declared_types.at(type_index).at(0) << ";" << endl;
    }

    mpi_type_declaration_buffer_size = 0;
    free(mpi_type_declaration_buffer);
    mpi_type_declaration_buffer = NULL;
}

