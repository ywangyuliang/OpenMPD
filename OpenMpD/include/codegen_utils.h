#ifndef CODEGEN_UTILS_H
#define CODEGEN_UTILS_H

/*
 * Shared helpers for code generation. These utilities normalize text, map C
 * types to MPI datatypes and split array, matrix and range expressions into
 * reusable pieces for the transform modules.
 */

#include <string>
#include <vector>

using namespace std;

extern std::vector<std::vector<std::string>> mpi_declared_types;
extern int mpi_type_order_matrix[11][11];

/* Returns a copy of the text converted to uppercase */
string to_uppercase(string text);

/* Returns a copy of the text converted to lowercase */
string to_lowercase(string text);

/* Maps a C type name to the matching MPI datatype name */
string mpi_type_for_c_type(string type);

/* Parses an array reference into its base name and index expressions */
vector<string> parse_array_reference_parts(string array_reference);

/* Counts opening brackets in a text fragment */
int count_open_brackets(string text);

/* Splits a matrix reference into reusable code generation parts */
vector<string> split_matrix_reference(const string& input);

/* Appends matrix reference parts to an existing vector */
void append_matrix_reference_parts(vector<string>& output_parts, const string& input);

/* Parses a range argument used by cluster clauses */
vector<string> parse_range_argument(string arg);

#endif
