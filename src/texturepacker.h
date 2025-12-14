//
// Created by tezza on 13/12/2025.
//


#pragma once


#include "engine.h"

typedef struct {
    char* img_path;
    /// STB DS string hash map of spritesheet entries
    spritesheet_entry* spritesheet;
} texturepacker_data;

texturepacker_data* load_spritesheet(char* filename);
void free_spritesheet(texturepacker_data* data);