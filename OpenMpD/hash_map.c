#include "hash_map.h"

#include <stdlib.h>
#include <string.h>

typedef struct hash_map_node {
    char *key;
    int *values;
    size_t count;
    size_t capacity;
    struct hash_map_node *next;
} hash_map_node_t;

struct hash_map {
    hash_map_node_t **buckets;
    size_t bucket_count;
};

static unsigned int hash_map_hash(const char *key) {
    unsigned int hash;
    int c;

    hash = 5381;

    while((c = (unsigned char) *key) != '\0') {
        hash = ((hash << 5) + hash) + (unsigned int) c;
        key++;
    }

    return hash;
}

static char *sstrdup(const char *text) {
    size_t len;
    char *copy;

    if(text == NULL) {
        return NULL;
    }

    len = strlen(text);
    copy = (char *) malloc(len + 1);

    if(copy == NULL) {
        return NULL;
    }

    memcpy(copy, text, len + 1);
    return copy;
}

static size_t hash_map_bucket_index(const hash_map_t *map, const char *key) {
    return (size_t) (hash_map_hash(key) % (unsigned int) map->bucket_count);
}

static hash_map_node_t *hash_map_find_node(const hash_map_t *map, const char *key) {
    size_t index;
    hash_map_node_t *node;

    if(map == NULL || key == NULL || map->bucket_count == 0) {
        return NULL;
    }

    index = hash_map_bucket_index(map, key);
    node = map->buckets[index];

    while(node != NULL) {
        if(strcmp(node->key, key) == 0) {
            return node;
        }

        node = node->next;
    }

    return NULL;
}

static hash_map_node_t *hash_map_create_node(const char *key) {
    hash_map_node_t *node;

    node = (hash_map_node_t *) calloc(1, sizeof(*node));

    if(node == NULL) {
        return NULL;
    }

    node->key = sstrdup(key);

    if(node->key == NULL) {
        free(node);
        return NULL;
    }

    return node;
}

static hash_map_node_t *hash_map_get_or_create_node(hash_map_t *map, const char *key) {
    size_t index;
    hash_map_node_t *node;

    if(map == NULL || key == NULL || map->bucket_count == 0) {
        return NULL;
    }

    node = hash_map_find_node(map, key);

    if(node != NULL) {
        return node;
    }

    node = hash_map_create_node(key);

    if(node == NULL) {
        return NULL;
    }

    index = hash_map_bucket_index(map, key);
    node->next = map->buckets[index];
    map->buckets[index] = node;

    return node;
}

static int hash_map_node_reserve(hash_map_node_t *node, size_t needed) {
    size_t new_capacity;
    int *new_values;

    if(node == NULL) {
        return -1;
    }

    if(node->capacity >= needed) {
        return 0;
    }

    if(node->capacity == 0) {
        new_capacity = 4;
    } else {
        new_capacity = node->capacity;
    }

    while(new_capacity < needed) {
        new_capacity *= 2;
    }

    new_values = (int *) realloc(node->values, new_capacity * sizeof(*new_values));

    if(new_values == NULL) {
        return -1;
    }

    node->values = new_values;
    node->capacity = new_capacity;

    return 0;
}

static int hash_map_node_add(hash_map_node_t *node, int value) {
    if(hash_map_node_reserve(node, node->count + 1) != 0) {
        return -1;
    }

    node->values[node->count] = value;
    node->count++;

    return 0;
}

hash_map_t *hash_map_create(size_t num_buckets) {
    hash_map_t *map;

    if(num_buckets == 0) {
        num_buckets = 64;
    }

    map = (hash_map_t *) calloc(1, sizeof(*map));

    if(map == NULL) {
        return NULL;
    }

    map->buckets = (hash_map_node_t **) calloc(num_buckets, sizeof(*map->buckets));

    if(map->buckets == NULL) {
        free(map);
        return NULL;
    }

    map->bucket_count = num_buckets;

    return map;
}

void hash_map_destroy(hash_map_t *map) {
    size_t i;
    hash_map_node_t *node;
    hash_map_node_t *next;

    if(map == NULL) {
        return;
    }

    for(i = 0; i < map->bucket_count; i++) {
        node = map->buckets[i];

        while(node != NULL) {
            next = node->next;

            free(node->key);
            free(node->values);
            free(node);

            node = next;
        }
    }

    free(map->buckets);
    free(map);
}

int hash_map_add(hash_map_t *map, const char *key, int value) {
    hash_map_node_t *node;

    node = hash_map_get_or_create_node(map, key);

    if(node == NULL) {
        return -1;
    }

    return hash_map_node_add(node, value);
}

int hash_map_contains_value(const hash_map_t *map, const char *key, int value) {
    size_t i;
    hash_map_node_t *node;

    node = hash_map_find_node(map, key);

    if(node == NULL) {
        return 0;
    }

    for(i = 0; i < node->count; i++) {
        if(node->values[i] == value) {
            return 1;
        }
    }

    return 0;
}

int hash_map_add_unique(hash_map_t *map, const char *key, int value) {
    if(map == NULL || key == NULL) {
        return -1;
    }

    if(hash_map_contains_value(map, key, value)) {
        return 0;
    }

    return hash_map_add(map, key, value);
}

int hash_map_set_single(hash_map_t *map, const char *key, int value) {
    hash_map_node_t *node;

    node = hash_map_get_or_create_node(map, key);

    if(node == NULL) {
        return -1;
    }

    if(hash_map_node_reserve(node, 1) != 0) {
        return -1;
    }

    node->values[0] = value;
    node->count = 1;

    return 0;
}

int hash_map_get(const hash_map_t *map, const char *key, int **out_values, size_t *out_len) {
    hash_map_node_t *node;
    int *copy;

    if(map == NULL || key == NULL || out_values == NULL || out_len == NULL) {
        return -1;
    }

    node = hash_map_find_node(map, key);

    if(node == NULL) {
        *out_values = NULL;
        *out_len = 0;
        return -1;
    }

    if(node->count == 0) {
        *out_values = NULL;
        *out_len = 0;
        return 0;
    }

    copy = (int *) malloc(node->count * sizeof(*copy));

    if(copy == NULL) {
        *out_values = NULL;
        *out_len = 0;
        return -1;
    }

    memcpy(copy, node->values, node->count * sizeof(*copy));

    *out_values = copy;
    *out_len = node->count;

    return 0;
}

int hash_map_get_single(const hash_map_t *map, const char *key, int *out_value) {
    hash_map_node_t *node;

    if(map == NULL || key == NULL || out_value == NULL) {
        return -1;
    }

    node = hash_map_find_node(map, key);

    if(node == NULL || node->count == 0) {
        return -1;
    }

    *out_value = node->values[0];

    return 0;
}

int hash_map_contains(const hash_map_t *map, const char *key) {
    if(hash_map_find_node(map, key) == NULL) {
        return 0;
    }

    return 1;
}