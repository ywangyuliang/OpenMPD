#include "hash_map.h"

#include <stdlib.h>
#include <string.h>

typedef struct node
{
    char* key;
    int* values;
    size_t len;
    size_t cap;

    struct node* next;
} node_t;

struct hash_map {
    node_t** buckets;
    size_t num_buckets;
};

static unsigned int hash(const char* key){
    
    unsigned int hash;

    hash = 0;
    while(*key != '\0'){
        hash += (unsigned char)*key; 
        key++;
    }
    return hash;
}

static node_t* find_node(node_t* head, const char* key){
    
    node_t* current;

    current = head;
    while (current != NULL){
        if(strcmp(current->key, key) == 0){
            return current;
        }
        current = current->next;
    }
    return NULL;
}

static char* local_strdup(const char* s){
    
    size_t len;
    char* ss;

    len = strlen(s);
    ss = (char*)malloc(len + 1);
    if(ss == NULL){
        return NULL;
    }
    memcpy(ss, s, len + 1);
    return ss;
}

static int node_append(node_t* node, int value){
    
    size_t new_cap;
    int* new_values;

    if(node->len == node->cap){
        if(node->cap == 0){
            new_cap = 4;
        } else {
            new_cap = node->cap * 2;
        }

        new_values = (int*)realloc(node->values, new_cap * sizeof(int));
        if(new_values == NULL){
            return -1;
        }
        node->values = new_values;
        node->cap = new_cap;
    }

    node->values[node->len++] = value;
    return 0;
}

static size_t calculate_bucket_index(const hash_map_t* map, const char* key){
    return (size_t)(hash(key) % (unsigned int)map->num_buckets);
}

hash_map_t* hash_map_create(size_t num_buckets){

    hash_map_t* map;

    if(num_buckets == 0){
        num_buckets = 64;
    }
    
    map = (hash_map_t*)malloc(sizeof(hash_map_t));
    if(map == NULL){
        return NULL;
    }

    map->buckets = (node_t**)calloc(num_buckets, sizeof(node_t*));
    if(map->buckets == NULL){
        free(map);
        return NULL;
    }

    map->num_buckets = num_buckets;
    return map;
}

void hash_map_destroy(hash_map_t* map){

    size_t i;
    node_t* current;
    node_t* next;

    if(map == NULL){
        return;
    }

    for(i = 0; i < map->num_buckets; i++){
        current = map->buckets[i];
        while (current != NULL){
            next = current->next;
            free(current->key);
            free(current->values);
            free(current);
            current = next;
        }
    }

    free(map->buckets);
    free(map);
}

int hash_map_add(hash_map_t* map, const char* key, int value){
    
    size_t index;
    node_t* node;

    if(map == NULL || key == NULL){
        return -1;
    }

    index = calculate_bucket_index(map, key);
    node = find_node(map->buckets[index], key);

    if(node == NULL){
        node = (node_t*)calloc(1, sizeof(node_t));
        if(node == NULL){
            return -1;
        }

        node->key = local_strdup(key);
        if(node->key == NULL){
            free(node);
            return -1;
        }

        node->next = map->buckets[index];
        map->buckets[index] = node;
    }

    return node_append(node, value);
}

int hash_map_get(const hash_map_t* map, const char* key, int** out_values, size_t* out_len){
    
    size_t index;
    node_t* node;

    if(map == NULL || key == NULL || out_values == NULL || out_len == NULL){
        return -1;
    }

    index = calculate_bucket_index(map, key);
    node = find_node(map->buckets[index], key);
    if(node == NULL){
        *out_values = NULL;
        *out_len = 0;
        return -1;
    }

    int* copy = (int*)malloc(node->len * sizeof(int));
    if(copy == NULL){
        *out_values = NULL;
        *out_len = 0;
        return -1;
    }

    memcpy(copy, node->values, node->len * sizeof(int));
    *out_values = copy;
    *out_len = node->len;

    return 0;
}

int hash_map_contains(const hash_map_t* map, const char* key){
    
    size_t index;

    if(map == NULL || key == NULL){
        return 0;
    }
    index = calculate_bucket_index(map, key);
    return (find_node(map->buckets[index], key) != NULL) ? 1 : 0;
}
