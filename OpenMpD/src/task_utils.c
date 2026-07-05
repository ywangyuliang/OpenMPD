#include "task_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *ompd_strdup(const char *s)
{
    size_t len;
    char *copy;

    if(s == NULL){
        return NULL;
    }

    len = strlen(s);
    copy = (char *)malloc(len + 1);
    if(copy == NULL){
        return NULL;
    }

    memcpy(copy, s, len + 1);
    return copy;
}

const char *ompd_normalize_c_type_name(const char *value_type)
{
    if(value_type == NULL || value_type[0] == '\0'){
        fprintf(stderr, "Error: missing value_type in normalize_c_type_name\n");
        exit(EXIT_FAILURE);
    }

    if(strcmp(value_type, "INT") == 0) return "int";
    if(strcmp(value_type, "LONG") == 0) return "long";
    if(strcmp(value_type, "DOUBLE") == 0) return "double";
    if(strcmp(value_type, "FLOAT") == 0) return "float";
    if(strcmp(value_type, "CHAR") == 0) return "char";
    if(strcmp(value_type, "SHORT") == 0) return "short";
    if(strcmp(value_type, "VOID") == 0) return "void";
    if(strcmp(value_type, "SIGNED") == 0) return "signed";
    if(strcmp(value_type, "UNSIGNED") == 0) return "unsigned";

    return value_type;
}

const char *ompd_runtime_suffix_for_value_type(const char *value_type)
{
    const char *t = ompd_normalize_c_type_name(value_type);

    if(strcmp(t, "int") == 0) return "int";
    if(strcmp(t, "long") == 0) return "long";
    if(strcmp(t, "double") == 0) return "double";

    return "";
}
