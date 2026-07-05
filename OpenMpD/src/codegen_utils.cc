#include "codegen_utils.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

std::vector<std::vector<std::string>> mpi_declared_types;

int mpi_type_order_matrix[11][11] = {{-1, -1, -1, -1, -1, -1, -1, -1, -1, 1, 1},
                                    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 2},
                                    {-1, -1, -1, -1, -1, -1, -1, -1, 2, -1, -1},
                                    {-1, -1, -1, -1, -1, -1, 1, -1, 0, -1, -1},
                                    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -2},
                                    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 1},
                                    {-1, 0, -1, 0, -1, -1, 1, -1, -1, -1, 1},
                                    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                                    {-1, -1, -2, 1, -1, -1, -1, -1, -1, -1, -1},
                                    {0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                                    {0, -2, -1, -1, -1, 0, 0, -1, -1, -1, -1}};

string to_uppercase(string text) {
  for (string::size_type i = 0; i < text.length(); i++) text[i] = toupper(text[i]);
  return text;
}

string to_lowercase(string text) {
  for (string::size_type i = 0; i < text.length(); i++) text[i] = tolower(text[i]);
  return text;
}

/* Row/column index of a basic C type keyword in mpi_type_order_matrix, or -1 */
static int mpi_type_keyword_index(const string &keyword) {
    if (keyword == "CHAR")     return 0;
    if (keyword == "INT")      return 1;
    if (keyword == "FLOAT")    return 2;
    if (keyword == "DOUBLE")   return 3;
    if (keyword == "BOOL")     return 4;
    if (keyword == "SHORT")    return 5;
    if (keyword == "LONG")     return 6;
    if (keyword == "BYTE")     return 7;
    if (keyword == "COMPLEX")  return 8;
    if (keyword == "SIGNED")   return 9;
    if (keyword == "UNSIGNED") return 10;
    return -1;
}

string mpi_type_for_c_type(string type) {
    string mpi_type_name = "MPI";
    vector<string> parts;

    const char *user_type_name = strstr(type.data(), "STRUCT");
    if (user_type_name) {
        user_type_name += 7;
    }
    else {
        user_type_name = strstr(type.data(), "USER_DEFINED");

        if (user_type_name) {
            user_type_name += 7;
        }
    }

    for (unsigned long int i = 0; i < mpi_declared_types.size(); i++) {
        for (unsigned long int j = 1; j < mpi_declared_types.at(i).size(); j++) {
            if (user_type_name) {
                if (strcmp(user_type_name, mpi_declared_types.at(i).at(j).data()) == 0) {
                    mpi_type_name = mpi_declared_types.at(i).at(0);
                    return mpi_type_name;
                }
            }
            else {
                if (strcmp(type.data(), mpi_declared_types.at(i).at(j).data()) == 0) {
                    mpi_type_name = mpi_declared_types.at(i).at(0);
                    return mpi_type_name;
                }
            }
        }
    }

    if (user_type_name) {
        fprintf(stderr, "MPI type not declared: %s\n", type.data());
        exit(EXIT_FAILURE);
    }

    for (long unsigned int i = 0; i < type.size(); i++) {
        string x = "";

        for (; i < type.size(); i++) {
            if (type.at(i) == '_' || type.at(i) == ' ') {
                break;
            }
            else {
                x += type.at(i);
            }
        }

        if (x != "") {
            string y = to_uppercase(x);
            int type_row = mpi_type_keyword_index(y);
            if (type_row < 0) {
                continue;
            }

            if (parts.size() == 0 && type_row >= 0) {
                parts.insert(parts.begin(), x);
                continue;
            }

            for (long unsigned int j = 0; j < parts.size(); j++) {
                int type_column = mpi_type_keyword_index(parts.at(j));

                int order_action = mpi_type_order_matrix[type_row][type_column];

                switch (order_action) {
                    case -2: parts.insert(parts.begin() + j, y);
                            parts.erase(parts.begin() + j + 1);
                            break;
                    case 0: parts.insert(parts.begin() + j, y);
                            break;
                    case 1: parts.insert(parts.begin() + j + 1, y);
                            break;
                }

                if (order_action != -1) {
                    break;
                }
            }
        }
    }

    for (long unsigned int i = 0; i < parts.size(); i++) {
        mpi_type_name += ("_" + parts.at(i));
    }

    if (mpi_type_name == "MPI") {
        fprintf(stderr, "Invalid MPI type: %s\n", type.data());
        exit(EXIT_FAILURE);
    }

    return mpi_type_name;
}

vector<string> parse_array_reference_parts(string array_reference) {
    vector<string> values;
    bool variable = true;
    bool inside_brackets = false;

    for (long unsigned int i = 0; i < array_reference.size(); i++) {
        string part = "";

        for (; i < array_reference.size(); i++) {
            if(array_reference.at(i) == ']') {
                if (!inside_brackets) {
                    fprintf(stderr, "'[' expected before ']' in cluster alloc\n");
                    exit(EXIT_FAILURE);
                }
                inside_brackets = false;
                break;
            }
            else if(array_reference.at(i) == '[') {
                inside_brackets = true;
                if (variable) {
                    break;
                }
            }
            else {
                part += array_reference.at(i);
            }
        }

        if (inside_brackets && variable) {
            variable = false;
        }
        else if (inside_brackets) {
            fprintf(stderr, "']' expected in cluster alloc\n");
        }

        values.push_back(part);
    }

    return values;
}

int count_open_brackets(string text) {
    int counter = 0;
    for (char c : text) {
        if (c == '[') {
            counter++;
        }
    }
    return counter;
}

vector<string> split_matrix_reference(const string& input) {
    size_t start_bracket1 = input.find('[');
    size_t end_bracket1 = input.find(']');
    size_t start_bracket2 = input.find('[', end_bracket1);
    size_t end_bracket2 = input.find(']', start_bracket2);

    string matrix_name = input.substr(0, start_bracket1);
    string row_expr = input.substr(start_bracket1 + 1, end_bracket1 - start_bracket1 - 1);
    string column_expr = input.substr(start_bracket2 + 1, end_bracket2 - start_bracket2 - 1);

    return {matrix_name, row_expr, column_expr};
}

void append_matrix_reference_parts(vector<string>& output_parts, const string& input) {
    vector<string> parts = split_matrix_reference(input);

    output_parts.insert(output_parts.end(), parts.begin(), parts.end());
}

vector<string> parse_range_argument(string arg) {
    vector<string> result;

    size_t startBracketPos = arg.find('[');
    size_t colonPos = arg.find(':');
    size_t endBracketPos = arg.find(']');

    if (startBracketPos != string::npos && colonPos != string::npos && endBracketPos != string::npos) {
        string part1 = arg.substr(0, startBracketPos);
        result.push_back(part1);

        string part2 = arg.substr(startBracketPos + 1, colonPos - startBracketPos - 1);
        result.push_back(part2);

        string part3 = arg.substr(colonPos + 1, endBracketPos - colonPos - 1);
        result.push_back(part3);
    } else {
        cerr << "Invalid format: " << arg << endl;
    }

    return result;
}
