//
// Created by tezza on 10/12/2025.
//

#include "engine.h"

#include <assert.h>
#include <malloc.h>
#include <stb_ds.h>
#include <stb_image.h>
#include <string.h>
#include <stdbool.h>

// TODO WT Sprite shader

const char * vert_shader_src = ""
        "#version 460\n\n"
        "layout (location=0) in vec3 a_pos;\n"
        "layout (location=1) in vec2 a_tex;\n"
        "out vec2 v_tex_coord;\n"
        "void main()\n"
        "{\n"
        "    gl_Position = vec4(a_pos, 1.0);\n"
        "    v_tex_coord = a_tex;\n"
        "}";


const char* frag_shader_src = ""
        "#version 460\n\n"
        "in vec2 v_tex_coord;\n"
        "out vec4 out_frag_color;\n"
        "layout(binding=0) uniform sampler2D u_tex;\n"
        "void main()\n"
        "{\n"
        "    out_frag_color = texture(u_tex, v_tex_coord.xy);\n"
        "}";

struct sprite_vertex {
    vec3 pos;
    vec2 tex;
};

bool vec2_inside_rect(const vec2 p, const vec4 rect) {
    return p[0] > rect[0] &&
           p[0] < (rect[0] + rect[2]) &&
           p[1] > rect[1] &&
           p[1] < (rect[1] + rect[3]);

}

void glfw_error_fun(int error_code, const char* description) {
    LOG(stderr, "GLFW ERROR %d: %s", error_code, description);
}

void check_shader(GLuint shader) {
    int code;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &code);
    if (GL_TRUE != code) {
        int log_length;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);
        if (log_length > 0) {
            char* log = alloca(log_length * sizeof(char));
            glGetShaderInfoLog(shader, log_length, &log_length, log);
            LOG(stderr, "Error compiling shader:\n%s\n", log);
        } else {
            LOG(stderr, "Error compiling shader (no log available)")
        }
    }
}

void engine_resize(struct engine* engine, int width, int height) {
    mat4x4_ortho(engine->view, 0, (float)width, 0, (float)height, -1.0f, 1.0f);
}

void on_scroll(GLFWwindow* window, double xoff, double yoff) {
    struct engine* engine = glfwGetWindowUserPointer(window);
    event_notify(engine->scroll_callbacks, xoff, yoff);
}
void on_char(GLFWwindow* window, unsigned int c) {
    struct engine* engine = glfwGetWindowUserPointer(window);
    event_notify(engine->char_callbacks, c);
}
void on_key(GLFWwindow* window, int key, int scancode, int action, int mods) {
    struct engine* engine = glfwGetWindowUserPointer(window);
    event_notify(engine->key_callbacks, key, scancode, action, mods);
}

void on_mouse_button(GLFWwindow* window, int button, int action, int mods) {
    struct engine* engine = glfwGetWindowUserPointer(window);
    event_notify(engine->mouse_button_callbacks, button, action, mods);

    if (action != 1) return;

    // Iterate through all sprites to see what overlaps the mouse position.
    for (size_t i = 0; i < arrlen(engine->sprites); i++) {
        struct sprite* sprite = &engine->sprites[i];
        const float* bounds = shget(engine->textures, engine->sprites[i].texture_name).rect;
        if (vec2_inside_rect(engine->cursor_pos, (vec4){
            sprite->pos[0],
            sprite->pos[1],
            sprite->pos[0] + bounds[2],
            sprite->pos[1] + bounds[3],
        })) {
            printf("You Clicked on sprite %lld, %s\n", i, sprite->texture_name);
        }
    }
}


void on_framebuffer_size(GLFWwindow* window, int width, int height) {
    struct engine* engine = glfwGetWindowUserPointer(window);
    engine_resize(engine, width, height);
}

void on_cursor_pos(GLFWwindow* window, double x_pos, double y_pos) {
    struct engine*engine = glfwGetWindowUserPointer(window);
    int ww, wh;
    glfwGetWindowSize(window, &ww, &wh);
    engine->cursor_pos[0] = (float)x_pos;
    engine->cursor_pos[1] = (float)wh - (float)y_pos;
}


struct engine *engine_init(struct engine_config engine_config) {
    stbi_set_flip_vertically_on_load(1);

    if (!glfwInit()) {
        LOG(stderr, "Failed to initialize GLFW3");
        return NULL;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef _DEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#endif

    glfwSetErrorCallback(glfw_error_fun);

    int window_width = 1280, window_height = 720;

    GLFWwindow *window = glfwCreateWindow(window_width, window_height, "Card Game Engine", NULL, NULL);
    if (!window) {
        LOG(stderr, "Failed to create GLFW3 window");
        return NULL;
    }

    glfwMakeContextCurrent(window);

    const GLenum err = glewInit();
    if (GLEW_OK != err) {
        LOG(stderr, "Failed to initialize GLEW: %s", (char*)glewGetErrorString(err));
        return NULL;
    }

    struct engine *engine = calloc(1, sizeof(struct engine));

    engine->window = window;

    engine->sprites = NULL;
    engine->free_sprites = NULL;

    engine->textures = NULL;
    sh_new_arena(engine->textures);
    engine->base_textures = NULL;
    sh_new_arena(engine->base_textures);

    engine_resize(engine, window_width, window_height);

    // Sprite Batch program
    engine->sprite_batch_program = glCreateProgram();

    GLuint v_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(v_shader, 1, &vert_shader_src, NULL);
    glCompileShader(v_shader);
    check_shader(v_shader);

    GLuint f_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(f_shader, 1, &frag_shader_src, NULL);
    glCompileShader(f_shader);
    check_shader(f_shader);

    glAttachShader(engine->sprite_batch_program, v_shader);
    glAttachShader(engine->sprite_batch_program, f_shader);
    glLinkProgram(engine->sprite_batch_program);
    int link_status = 0;
    glGetProgramiv(engine->sprite_batch_program, GL_LINK_STATUS, &link_status);
    if (GL_TRUE != link_status) {
        int log_length;
        glGetProgramiv(engine->sprite_batch_program, GL_INFO_LOG_LENGTH, &log_length);
        char* log = alloca(log_length * sizeof(char));
        glGetProgramInfoLog(engine->sprite_batch_program, log_length, &log_length, log);
        LOG(stderr, "Error linking program:\n%s\n", log);
    }
    glDeleteShader(v_shader);
    glDeleteShader(f_shader);

    // Dynamic VBuffer for sprite batch
    glCreateBuffers(1, &engine->sprite_batch_mesh);
    glNamedBufferStorage(engine->sprite_batch_mesh, sizeof(struct sprite_vertex) * MAX_SPRITES_IN_BATCH * 6, NULL,
                         GL_DYNAMIC_STORAGE_BIT);

    // VAO for sprite batch VBuffer
    glCreateVertexArrays(1, &engine->sprite_vao);

    glEnableVertexArrayAttrib(engine->sprite_vao, 0);
    glEnableVertexArrayAttrib(engine->sprite_vao, 1);

    glVertexArrayAttribFormat(engine->sprite_vao, 0, 3, GL_FLOAT, GL_FALSE, offsetof(struct sprite_vertex, pos));
    glVertexArrayAttribFormat(engine->sprite_vao, 1, 2, GL_FLOAT, GL_FALSE, offsetof(struct sprite_vertex, tex));

    glVertexArrayAttribBinding(engine->sprite_vao, 0, 0);
    glVertexArrayAttribBinding(engine->sprite_vao, 1, 0);

    glVertexArrayVertexBuffer(engine->sprite_vao, 0, engine->sprite_batch_mesh, 0, sizeof(struct sprite_vertex));

    glfwSetWindowUserPointer(engine->window, engine);

    glfwSetScrollCallback(engine->window, &on_scroll);
    glfwSetCharCallback(engine->window, &on_char);
    glfwSetKeyCallback(engine->window, &on_key);
    glfwSetMouseButtonCallback(engine->window, &on_mouse_button);

    glfwSetFramebufferSizeCallback(engine->window, &on_framebuffer_size);
    glfwSetCursorPosCallback(engine->window, &on_cursor_pos);

    double cursor_x, cursor_y;
    glfwGetCursorPos(engine->window, &cursor_x, &cursor_y);
    engine->cursor_pos[0] = engine->last_cursor_pos[0] = (float)cursor_x;
    engine->cursor_pos[1] = engine->last_cursor_pos[1] = (float)cursor_y;

    event_init(engine->scroll_callbacks);
    event_init(engine->char_callbacks);
    event_init(engine->key_callbacks);
    event_init(engine->mouse_button_callbacks);

    return engine;
}

void engine_free(struct engine *engine) {
    arrfree(engine->sprites);
    arrfree(engine->free_sprites);

    for (int i = 0; i < shlen(engine->base_textures); i++) {
        assert(engine->base_textures);
        glDeleteTextures(1, &engine->base_textures[i].value);
    }
    shfree(engine->base_textures);

    shfree(engine->textures);

    arrfree(engine->scroll_callbacks);
    arrfree(engine->char_callbacks);
    arrfree(engine->key_callbacks);
    arrfree(engine->mouse_button_callbacks);

    glDeleteBuffers(1, &engine->sprite_batch_mesh);

    glfwDestroyWindow(engine->window);
    glfwTerminate();

    free(engine);
}

// TODO WT: Support Texture Atlases
void load_texture(struct engine *engine, const char *texture_path, const spritesheet_entry* sprites) {
    size_t base_texture_index = 0;
    int x, y, bpp;
    if (-1 == shgeti(engine->base_textures, texture_path)) {
        GLuint tex;
        glCreateTextures(GL_TEXTURE_2D, 1, &tex);

        unsigned char *data = stbi_load(texture_path, &x, &y, &bpp, STBI_rgb_alpha);
        if (NULL == data) {
            LOG(stderr, "Failed to load image %s: %s", texture_path, stbi_failure_reason());
        }

        // Set up the texture params
        glTextureParameteri(tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(tex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(tex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(tex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glTextureStorage2D(tex, 1, GL_RGBA8, x, y);
        glTextureSubImage2D(tex, 0, 0, 0, x, y, GL_RGBA, GL_UNSIGNED_BYTE, data);

        stbi_image_free(data);

        base_texture_index = shlen(engine->base_textures);
        shput(engine->base_textures, texture_path, ((struct base_texture){
            .texture=tex,
            .size = {(float)x, (float)y}
        }));
    } else {
        // Base texture already loaded, add additional sprites to it.
        base_texture_index = shgeti(engine->base_textures, texture_path);
        struct base_texture base_texture = engine->base_textures[base_texture_index].value;
        x = (int)base_texture.size[0];
        y = (int)base_texture.size[1];
    }

    // If there's no sprites requested and we've not already loaded a sprite with that name, add a texture covering the whole base texture.
    if (NULL == sprites && -1 == shgeti(engine->textures, texture_path)) {
        shput(engine->textures, texture_path, ((struct texture){
            .texture_index = base_texture_index,
            .rect = {0, 0, (float)x, (float)y}
        }));
    } else {
        // Add the sprites for the whole texture
        for (size_t i = 0; i < shlen(sprites); i++) {
            // If we have a texture name clash, log it and we can overwrite.
            if (-1 != shgeti(engine->textures, sprites[i].key)) {
                LOG(stderr, "Already loaded sprite texture with the name %s", sprites[i].key);
            }
            shput(engine->textures, sprites[i].key, ((struct texture){
                .texture_index = base_texture_index,
                .rect = {
                    sprites[i].value[0],
                    sprites[i].value[1],
                    sprites[i].value[2],
                    sprites[i].value[3]
                }
            }));
        }
    }
}

sprite_handle create_sprite(struct engine *engine, char *texture_name, vec2 pos) {
    sprite_handle handle;
    if (arrlen(engine->free_sprites) > 0) {
        handle = arrpop(engine->free_sprites);
    } else {
        handle = arrlen(engine->sprites);
        arrput(engine->sprites, ((struct sprite){0}));
    }

    struct sprite* sprite = &engine->sprites[handle];
    sprite->texture_name = _strdup(texture_name);
    memcpy(sprite->pos, pos, sizeof(vec2));
    return handle;
}

void delete_sprite(struct engine *engine, sprite_handle handle) {
    struct sprite* sprite = &engine->sprites[handle];
    free(sprite->texture_name);
    sprite->texture_name = NULL;
    memset(sprite->pos, 0, sizeof(vec2));
    arrpush(engine->free_sprites, handle);
}

void engine_update(struct engine* engine) {
    vec2_sub(engine->cursor_delta, engine->last_cursor_pos, engine->last_cursor_pos);
    memcpy(engine->last_cursor_pos, engine->cursor_pos, sizeof(vec2));
}

void _flush(struct engine* engine, struct sprite_vertex* vertices, GLuint texture) {
    glNamedBufferSubData(
        engine->sprite_batch_mesh,
        0,
        arrlen(vertices) * sizeof(struct sprite_vertex),
        (void*)vertices);

    glUseProgram(engine->sprite_batch_program);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glActiveTexture(GL_TEXTURE0 + 0);
    glBindTexture(GL_TEXTURE_2D, texture);

    glBindVertexArray(engine->sprite_vao);

    glDrawArrays(GL_TRIANGLES, 0, arrlen(vertices));

    glUseProgram(0);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    engine->engine_stats.draw_calls++;
}

void engine_render(struct engine* engine) {
    engine->engine_stats.draw_calls = 0;

    if (NULL == engine->sprites) {return;}

    size_t numSprites = arrlen(engine->sprites);
    if (0 == numSprites) return;

    struct sprite_vertex* vertices = NULL;

    GLuint current_texture = 0;

#define FLUSH_AND_CLEAR() {_flush(engine, vertices, current_texture); batch_size = 0;arrfree(vertices);}

    size_t batch_size = 0;
    for (size_t i = 0; i < numSprites; i++) {
        const struct sprite spr = engine->sprites[i];
        struct texture_storage* tex_storage = shgetp(engine->textures, spr.texture_name);
        struct texture tex = shget(engine->textures, spr.texture_name);
        char* base_texture_name = engine->base_textures[tex.texture_index].key;
        struct base_texture base_tex = engine->base_textures[tex.texture_index].value;

        if ((base_tex.texture != current_texture) && batch_size > 0) {FLUSH_AND_CLEAR()}
        // If the texture has changed we need to flush the batch. Encourage the use of Atlases, the renderer currently only support one texture at a time.

        current_texture = base_tex.texture;

        float tex_x = tex.rect[0], tex_y = tex.rect[1], tex_w = tex.rect[2], tex_h = tex.rect[3], base_width = base_tex.size[0], base_height = base_tex.size[1];

        struct sprite_vertex corners[4] = {
            (struct sprite_vertex){
                .pos = {spr.pos[0], spr.pos[1], 0.0f},
                .tex = {tex_x / base_width, tex_y / base_height}
            },
            (struct sprite_vertex){
                .pos = {spr.pos[0], spr.pos[1] + tex_h, 0.0f},
                .tex = {tex_x / base_width, (tex_y + tex_h) / base_height}
            },
            (struct sprite_vertex){
                .pos = {spr.pos[0] + tex_w, spr.pos[1] + tex_h, 0.0f},
                .tex = {(tex_x + tex_w) / base_width, (tex_y + tex_h) / base_height}
            },
            (struct sprite_vertex){
                .pos = {spr.pos[0] + tex_w, spr.pos[1], 0.0f},
                .tex = {(tex_x + tex_w) / base_width, tex_y / base_height}
            },
        };

        // Move all corners into view space
        for (size_t j = 0; j < 4; j++) {
            const vec4 p = {corners[j].pos[0], corners[j].pos[1], corners[j].pos[2], 1.0f};
            vec4 r = {0};
            mat4x4_mul_vec4(r, engine->view, p);
            corners[j].pos[0] = r[0];
            corners[j].pos[1] = r[1];
            corners[j].pos[2] = r[2];
        }

        arrput(vertices, corners[0]);
        arrput(vertices, corners[1]);
        arrput(vertices, corners[2]);

        arrput(vertices, corners[0]);
        arrput(vertices, corners[2]);
        arrput(vertices, corners[3]);

        batch_size++;

        // TODO WT: Also needs to detect how many textures have been used as to not exceed the max of the shader.
        if (MAX_SPRITES_IN_BATCH == batch_size) {
            FLUSH_AND_CLEAR()
        }

#undef FLUSH_AND_CLEAR
    }

    if (batch_size > 0) {
        _flush(engine, vertices, current_texture);
    }
}
