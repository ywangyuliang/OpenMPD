#include "tasking_emit.h"
#include "tasking_region.h"
#include "output_slots.h"

#include <fstream>
#include <string>
#include <cstdio>
#include <cstdlib>

using namespace std;

extern ofstream output;

static const char *TASK_GLOBAL_DEFINITIONS_SLOT = "task_global_definitions";

static std::string read_temp_file(FILE *file)
{
    char buffer[8192];
    size_t n;
    std::string text;

    rewind(file);
    while((n = fread(buffer, 1, sizeof(buffer), file)) > 0){
        text.append(buffer, n);
    }

    return text;
}

static void copy_temp_file_to_output(FILE *file)
{
    char buffer[8192];
    size_t n;

    rewind(file);
    while((n = fread(buffer, 1, sizeof(buffer), file)) > 0){
        output.write(buffer, n);
    }
}

void tasking_emit_finalize_region(cluster_stack *c)
{
    FILE *tmp_global_defs;
    FILE *tmp_body;

    if(c == NULL || c->tasking_region == NULL){
        return;
    }

    tmp_global_defs = tmpfile();
    tmp_body = tmpfile();

    if(tmp_global_defs == NULL || tmp_body == NULL){
        perror("tmpfile");
        exit(EXIT_FAILURE);
    }

    if(tasking_region_emit_global_defs(c->tasking_region, tmp_global_defs) != 0){
        fprintf(stderr, "Error: tasking_region_emit_global_defs\n");
        exit(EXIT_FAILURE);
    }

    if(tasking_region_emit_body(c->tasking_region, tmp_body) != 0){
        fprintf(stderr, "Error: tasking_region_emit_body\n");
        exit(EXIT_FAILURE);
    }

    output_slot_append_replacement(TASK_GLOBAL_DEFINITIONS_SLOT, read_temp_file(tmp_global_defs));
    copy_temp_file_to_output(tmp_body);

    fclose(tmp_global_defs);
    fclose(tmp_body);

    tasking_region_destroy(c->tasking_region);
    c->tasking_region = NULL;
}

void tasking_emit_region_body(cluster_stack *c)
{
    FILE *tmp_body;

    if(c == NULL || c->tasking_region == NULL){
        return;
    }

    tmp_body = tmpfile();
    if(tmp_body == NULL){
        perror("tmpfile");
        exit(EXIT_FAILURE);
    }

    if(tasking_region_emit_body(c->tasking_region, tmp_body) != 0){
        fprintf(stderr, "Error: tasking_region_emit_body\n");
        exit(EXIT_FAILURE);
    }

    copy_temp_file_to_output(tmp_body);

    fclose(tmp_body);

    tasking_region_destroy(c->tasking_region);
    c->tasking_region = NULL;
}
