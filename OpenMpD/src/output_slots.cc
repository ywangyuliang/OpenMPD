#include "output_slots.h"

#include <fstream>
#include <map>
#include <sstream>

static std::map<std::string, std::string> output_slot_replacements;

std::string output_slot_marker(const char *slot_name)
{
    std::string name = slot_name != 0 ? slot_name : "";

    return "/*__OMPD_OUTPUT_SLOT_" + name + "__*/";
}

void output_slot_set_replacement(const char *slot_name, const std::string &replacement)
{
    std::string name = slot_name != 0 ? slot_name : "";

    output_slot_replacements[name] = replacement;
}

void output_slot_append_replacement(const char *slot_name, const std::string &replacement)
{
    std::string name = slot_name != 0 ? slot_name : "";

    output_slot_replacements[name] += replacement;
}

int output_slots_apply(const char *path)
{
    if(path == 0){
        return -1;
    }

    std::ifstream in(path);
    if(!in){
        return -1;
    }

    std::ostringstream buffer;
    buffer << in.rdbuf();
    std::string content = buffer.str();
    in.close();

    for(std::map<std::string, std::string>::const_iterator it = output_slot_replacements.begin();
        it != output_slot_replacements.end();
        ++it){
        std::string marker = output_slot_marker(it->first.c_str());
        std::string::size_type pos = 0;

        while((pos = content.find(marker, pos)) != std::string::npos){
            content.replace(pos, marker.size(), it->second);
            pos += it->second.size();
        }
    }

    std::ofstream out(path, std::ios::trunc);
    if(!out){
        return -1;
    }

    out << content;
    return out ? 0 : -1;
}

void output_slots_reset(void)
{
    output_slot_replacements.clear();
}
