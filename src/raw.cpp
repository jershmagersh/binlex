#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "raw.h"

using namespace binlex;

Raw::Raw(){
    for (int i = 0; i < RAW_MAX_SECTIONS; i++){
        sections[i].data = NULL;
        sections[i].size = 0;
    }
}

int Raw::GetFileSize(FILE *fd){
    int start = ftell(fd);
    fseek(fd, 0, SEEK_END);
    int size = ftell(fd);
    fseek(fd, start, SEEK_SET);
    return size;
}

bool Raw::ReadFile(char *file_path, int section_index){
    FILE *fd = fopen(file_path, "rb");
    sections[section_index].offset = ftell(fd);
    sections[section_index].size = GetFileSize(fd);
    sections[section_index].data = malloc(sections[section_index].size);
    memset(sections[section_index].data, 0, sections[section_index].size);
    fread(sections[section_index].data, sections[section_index].size, 1, fd);
    fclose(fd);
    return true;
}

Raw::~Raw(){
    for (int i = 0; i < RAW_MAX_SECTIONS; i++){
        if (sections[i].data != NULL){
            free(sections[i].data);
            sections[i].size = 0;
        }
    }
}
