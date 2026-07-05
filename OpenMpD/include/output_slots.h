#ifndef OUTPUT_SLOTS_H
#define OUTPUT_SLOTS_H

/*
 * Deferred output slots for generated code that must be discovered late but
 * inserted earlier in the output file, such as headers, global declarations
 * or generated task definitions.
 */

#include <string>

/* Builds the marker text used as a deferred output slot */
std::string output_slot_marker(const char *slot_name);

/* Replaces the full content of a deferred output slot */
void output_slot_set_replacement(const char *slot_name, const std::string &replacement);

/* Appends content to a deferred output slot */
void output_slot_append_replacement(const char *slot_name, const std::string &replacement);

/* Applies all deferred slot replacements to an output file */
int output_slots_apply(const char *path);

/* Clears all registered deferred output slots */
void output_slots_reset(void);

#endif
