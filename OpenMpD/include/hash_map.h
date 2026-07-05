#ifndef HASH_MAP_H
#define HASH_MAP_H

/*
 * Small string-to-integer-list hash map used by the tasking runtime and
 * analysis code. Each key can store one or more integer values, with helpers
 * for unique insertion, single-value replacement and lookup.
 */

#include <stddef.h>

typedef struct hash_map hash_map_t;

/* Creates a hash map with the requested bucket count */
hash_map_t* hash_map_create(size_t num_buckets);

/* Releases all memory owned by a hash map */
void hash_map_destroy(hash_map_t* map);

/* Adds one value for a key */
int hash_map_add(hash_map_t* map, const char* key, int value);

/* Adds one value only when it is not already present for the key */
int hash_map_add_unique(hash_map_t* map, const char* key, int value);

/* Replaces all values for a key with one value */
int hash_map_set_single(hash_map_t* map, const char* key, int value);

/* Returns all values stored for a key */
int hash_map_get(const hash_map_t* map, const char* key, int** out_values, size_t* out_len);

/* Returns the single value stored for a key */
int hash_map_get_single(const hash_map_t* map, const char* key, int* out_value);

/* Returns whether a key exists */
int hash_map_contains(const hash_map_t* map, const char* key);

/* Returns whether a key contains a specific value */
int hash_map_contains_value(const hash_map_t* map, const char* key, int value);

#endif
