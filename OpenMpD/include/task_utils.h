#ifndef TASK_UTILS_H
#define TASK_UTILS_H

/*
 * Small utility helpers shared by tasking code. They duplicate strings,
 * normalize C type names and map normalized types to runtime suffixes used by
 * generated send/receive helper names.
 */

/* Duplicates a string with heap storage owned by the caller */
char *ompd_strdup(const char *s);

/* Normalizes a C type name for runtime code generation */
const char *ompd_normalize_c_type_name(const char *value_type);

/* Returns the OMPD runtime suffix for a normalized value type */
const char *ompd_runtime_suffix_for_value_type(const char *value_type);

#endif
