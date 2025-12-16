//
// Created by tezza on 08/12/2025.
//


#pragma once
#include <stdlib.h>

#include <gl/glew.h>
#include <GLFW/glfw3.h>
#include <linmath.h>

#define LOG(stream, message, ...) fprintf(stream, "LOG (%s:%d)" message "\n", __FILE__, __LINE__, ##__VA_ARGS__);

typedef int sprite_handle;
#define invalid_sprite_handle -1
typedef struct sprite {
    /// Owned malloced
    char *texture_name;
    vec2 pos;
    // TODO WT Children/Parent Hiararchy

    sprite_handle parent;
    /// stb array
    sprite_handle* children;
} sprite;

typedef struct texture {
    size_t texture_index;

    /// pixel space rect where the sprite resides on the atlas/base texture.
    vec4 rect;
} texture;

typedef struct base_texture {
    GLuint texture;
    vec2 size;
} base_texture;

#define MAX_SPRITES_IN_BATCH 10000

struct engine_config {
    size_t _;
};

struct engine_stats {
    int draw_calls;
};

#define event_decl(name, ...) typedef struct name { void* ctx; void (*callback)(void* ctx, __VA_ARGS__); } name
#define event_push(event, type, ctx, cb) arrput(event, ((type){ctx, cb}))
#define event_init(event) do {(event) = NULL;} while (0)
#define event_free(event) arrfree(event);
#define event_notify(event, ...)  \
do {\
    for (size_t i = 0; i < arrlen((event)); i++) {\
        (event)[i].callback((event)[i].ctx, ##__VA_ARGS__);\
    }\
} while (0)

event_decl(scroll_callback, double xoff, double yoff);
event_decl(char_callback, unsigned int c);
event_decl(key_callback, int key, int scancode, int action, int mods);
event_decl(mouse_button_callback, int button, int action, int mods);
event_decl(framebuffer_size_callback, int width, int height);

typedef struct engine {
    GLFWwindow *window;



    struct sprite *sprites;
    size_t *free_sprites;

    struct texture_storage{
        char *key;
        struct texture value;
    } *textures;

    struct base_texture_storage {
        char* key;
        struct base_texture value;
    }* base_textures;

    sprite_handle root;

    mat4x4 view;

    GLuint sprite_batch_program;

    GLuint sprite_vao;
    GLuint sprite_batch_mesh;

    vec2 cursor_pos;
    vec2 last_cursor_pos;
    vec2 cursor_delta;

    scroll_callback *scroll_callbacks;
    char_callback *char_callbacks;
    key_callback *key_callbacks;
    mouse_button_callback *mouse_button_callbacks;
    framebuffer_size_callback *framebuffer_size_callbacks;

    struct engine_stats engine_stats;
} engine;

struct engine *engine_init(struct engine_config engine_config);

void engine_free(struct engine *engine);

typedef struct {
    char* key;
    vec4 value;
} spritesheet_entry;
void load_texture(struct engine *engine, const char *texture_path, const spritesheet_entry* sprites);

sprite_handle create_sprite(struct engine *engine, char* texture_name, vec2 pos);
void sprite_remove_child(struct engine* engine, sprite_handle sprite, sprite_handle child);
void sprite_add_child(struct engine* engine, sprite_handle parent, sprite_handle child);
void delete_sprite(struct engine *engine, sprite_handle sprite);

void engine_update(struct engine* engine);
void engine_render(struct engine *engine);
