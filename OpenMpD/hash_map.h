#ifndef HASH_MAP_H
#define HASH_MAP_H

#include <stddef.h>

typedef struct hash_map hash_map_t;

hash_map_t* hash_map_create(size_t num_buckets);
void hash_map_destroy(hash_map_t* map);
int hash_map_add(hash_map_t* map, const char* key, int value);
int hash_map_get(const hash_map_t* map, const char* key, int** out_values, size_t* out_len);
int hash_map_contains(const hash_map_t* map, const char* key);

#endif
