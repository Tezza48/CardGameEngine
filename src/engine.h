//
// Created by tezza on 08/12/2025.
//


#pragma once
#include <stdlib.h>

#include <gl/glew.h>
#include <GLFW/glfw3.h>
#include <linmath.h>
#include <stdbool.h>

#define LOG(stream, message, ...) fprintf(stream, "LOG (%s:%d)" message "\n", __FILE__, __LINE__, ##__VA_ARGS__);

typedef float aabb[4];
typedef float rect[4];
inline bool vec2_inside_aabb(const vec2 p, const aabb a) {
    return p[0] > a[0] &&
           p[0] < a[2] &&
           p[1] > a[1] &&
           p[1] < a[3];
}
inline bool vec2_inside_rect(const vec2 p, const rect a) {
    return p[0] > a[0] &&
           p[0] < (a[0] + a[2]) &&
           p[1] > a[1] &&
           p[1] < (a[1] + a[3]);
}

inline void aabb_union(aabb r, aabb a, aabb b) {
    r[0] = fminf(a[0], b[0]);
    r[1] = fminf(a[1], b[1]);
    r[2] = fmaxf(a[2], b[2]);
    r[3] = fmaxf(a[3], b[3]);
}
inline void aabb_offset(aabb r, const aabb a, const vec2 o) {
    r[0] = a[0] + o[0];
    r[1] = a[1] + o[1];
    r[2] = a[2] + o[0];
    r[3] = a[3] + o[1];
}

inline void aabb_to_rect(rect r, aabb a) {
    r[0] = a[0];
    r[1] = a[1];
    r[2] = a[2] - a[0];
    r[3] = a[3] - a[1];
}

inline void aabb_bottom_left(vec2 r, aabb b) {
    r[0] = b[0];
    r[1] = b[1];
}
inline void aabb_top_left(vec2 r, aabb b) {
    r[0] = b[0];
    r[1] = b[3];
}
inline void aabb_top_right(vec2 r, aabb b) {
    r[0] = b[2];
    r[1] = b[3];
}
inline void aabb_bottom_right(vec2 r, aabb b) {
    r[0] = b[2];
    r[1] = b[1];
}
// Calculated clockwise corners of an aabb
inline void aabb_to_corners(vec2 r[4], aabb bounds) {
    r[0][0] = bounds[0];
    r[0][1] = bounds[1];

    r[1][0] = bounds[0];
    r[1][2] = bounds[3];

    r[2][0] = bounds[2];
    r[2][1] = bounds[3];

    r[3][0] = bounds[2];
    r[3][1] = bounds[1];
}

typedef int sprite_handle;
#define invalid_sprite_handle (-1)
typedef struct sprite {
    /// Owned malloced
    char *texture_name;
    vec2 pos;

    bool visible;

    sprite_handle parent;
    /// stb array
    sprite_handle* children;

    // cached data dependent on parent/children
    vec2 global_pos;

    // X, Y, W, H
    aabb local_bounds;
    aabb global_bounds;
} sprite;

inline void sprite_set_pos(sprite* sprite, vec2 pos) {
    memcpy(sprite->pos, pos, sizeof(sprite->pos));
    // sprite->dirty = true;
}
inline void sprite_set_x(sprite* sprite, float x) {
    sprite->pos[0] = x;
    // sprite->dirty = true;
}
inline void sprite_set_y(sprite* sprite, float y) {
    sprite->pos[1] = y;
    // sprite->dirty = true;
}


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
void engine_load_texture(struct engine *engine, const char *texture_path, const spritesheet_entry* sprites);

/// SPRITES
sprite_handle engine_create_sprite(struct engine *engine, char* texture_name, vec2 pos);
void engine_remove_sprite_child(struct engine* engine, sprite_handle sprite, sprite_handle child);
void engine_add_sprite_child(struct engine* engine, sprite_handle parent, sprite_handle child);
void engine_delete_sprite(struct engine *engine, sprite_handle sprite);

void engine_clean_sprite_hierarchy(engine* engine);

// Returns whether propogation should continue down the tree
typedef bool (* sprite_fn_down)(engine* engine, sprite_handle handle, void* user);
typedef void (* sprite_fn_up)(engine* engine, sprite_handle handle, void* user);
void engine_sprite_traverse(engine* engine, sprite_handle handle, sprite_fn_down op_before, sprite_fn_up op_after, void* user);


void engine_update(struct engine* engine);
void engine_render(struct engine *engine);
