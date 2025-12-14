//
// Created by tezza on 08/12/2025.
//


#pragma once
#include <stdlib.h>

#include <gl/glew.h>
#include <GLFW/glfw3.h>
#include <linmath.h>

#define LOG(stream, message, ...) fprintf(stream, "LOG (%s:%d)" message "\n", __FILE__, __LINE__, ##__VA_ARGS__);

struct sprite {
    /// Owned malloced
    char *texture_name;
    vec2 pos;
    // TODO WT Children/Parent Hiararchy
};
typedef int sprite_handle;

struct texture {
    size_t texture_index;

    /// pixel space rect where the sprite resides on the atlas/base texture.
    vec4 rect;
};

#define MAX_SPRITES_IN_BATCH 10000

struct engine_config {
    size_t _;
};

struct engine_stats {
    int draw_calls;
};

struct engine {
    GLFWwindow *window;

    struct sprite *sprites;
    size_t *free_sprites;

    struct {
        char *key;
        struct texture value;
    } *textures;

    struct base_texture {
        char* key;
        GLuint value;
        vec2 size;
    }* base_textures;

    mat4x4 view;

    GLuint sprite_batch_program;

    GLuint sprite_vao;
    GLuint sprite_batch_mesh;

    struct engine_stats engine_stats;
};

struct engine *engine_init(struct engine_config engine_config);

void engine_free(struct engine *engine);

void engine_resize(struct engine* engine, int width, int height);

typedef struct {
    char* key;
    vec4 value;
} spritesheet_entry;
void load_texture(struct engine *engine, const char *texture_path, const spritesheet_entry* sprites);

sprite_handle create_sprite(struct engine *engine, char* texture_name, vec2 pos);
void delete_sprite(struct engine *engine, sprite_handle sprite);

void render(struct engine *engine);
