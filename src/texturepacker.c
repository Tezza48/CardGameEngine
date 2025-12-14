//
// Created by tezza on 13/12/2025.
//

#include "texturepacker.h"
#include "engine.h"

#include <stdio.h>
#include <malloc.h>
#include <stb_ds.h>

texturepacker_data* load_spritesheet(char *filename) {
    char *lastSlash = strrchr(filename, '\\');
    if (NULL == lastSlash) {
        lastSlash = strrchr(filename, '/');
    }

    char *dirname;
    if (NULL != lastSlash) {
        // Increment 1 char ahead so we include the trailing slash.
        lastSlash++;

        intptr_t base_len = lastSlash - filename;
        dirname = alloca(base_len + 1);
        memset(dirname, 0, base_len + 1);
        memcpy(dirname, filename, base_len);
    } else {
        dirname = "";
    }

    FILE *f = fopen(filename, "r");
    if (!f) {
        LOG(stderr, "Failed to open spritesheet atlas");
        return NULL;
    }

    char img_name[256];
    int written = fscanf_s(f, "%s\n",
                           img_name, (unsigned) _countof(img_name)
    );

    int width, height;
    written = fscanf_s(f, "size: %d, %d\n",
                       &width,
                       &height
    );

    char color_format[256];
    written = fscanf_s(f, "format: %s\n",
                       color_format, (unsigned) _countof(color_format)
    );

    char filter_min[256], filter_max[256];
    written = fscanf_s(f, "filter: %[^,], %s\n",
                       filter_min, (unsigned) _countof(filter_min),
                       filter_max, (unsigned) _countof(filter_max)
    );

    char repeat[256], pma[256];
    written = fscanf_s(f, "repeat: %s\npma: %s\n",
                       repeat, (unsigned)_countof(repeat),
                       pma, (unsigned) _countof(pma)
    );

    texturepacker_data *data = calloc(sizeof(texturepacker_data), 1);
    if (!data) {
        LOG(stderr, "Failed to allocate texturepacker_data");
        return NULL;
    }

    sh_new_arena(data->spritesheet);

    char* img_path_format = "%s%s";
    int img_path_len = snprintf(NULL, 0, img_path_format, dirname, img_name);
    data->img_path = calloc(img_path_len + 1, sizeof(char));
    snprintf(data->img_path, img_path_len + 1, img_path_format, dirname, img_name);

    while (!feof(f)) {
        char sprite_name[256];
        int x, y, w, h;
        written = fscanf_s(f, "%s\n  bounds: %d, %d, %d, %d\n",
                           sprite_name, (unsigned)_countof(sprite_name),
                           &x, &y, &w, &h
        );
        shputs(data->spritesheet, ((spritesheet_entry){
            .key = sprite_name,
            // Rect is inverted because texture gets loaded upside down (opengl)
            .value = {(float)x, (float)height - (float)y - (float)h, (float)w, (float)h}})
        );
    }

    fclose(f);

    // TODO WT: the rest
    return data;
}

void free_spritesheet(texturepacker_data *data) {
    free(data->img_path);
    shfree(data->spritesheet);
    free(data);
}
