//
// Created by tezza on 08/12/2025.
//


#pragma once
#include <stdlib.h>

#include <gl/glew.h>
#include <GLFW/glfw3.h>

#include <stb_ds.h>
#include <stb_image.h>
#include <linmath.h>
#include <malloc.h>

#define LOG(stream, message, ...) fprintf(stream, "LOG (%s:%d)" message "\n", __FILE__, __LINE__, ##__VA_ARGS__);

struct sprite {
    /// Owned malloced
    char *texture_name;
    vec2 pos;
    // TODO WT Children/Parent Hiararchy
};

struct texture {
    GLuint tex;
    int width;
    int height;
};

#define MAX_SPRITES_IN_BATCH 10000

struct engine {
    GLFWwindow *window;

    struct sprite *sprites;
    size_t *free_sprites;

    struct {
        char *key;
        struct texture value;
    } *textures;

    mat4x4 view;

    GLuint sprite_batch_program;

    GLuint sprite_vao;
    GLuint sprite_batch_mesh;
};

struct engine *engine_init(const char *title, int width, int height);

void engine_free(struct engine *engine);

void load_texture(struct engine *engine, const char *texture_path);

void render(struct engine *engine);

#ifdef ENGINE_IMPLEMENTATION

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
"    out_frag_color = texture(u_tex, v_tex_coord.xy);//vec4(v_tex_coord, 0.0, 1.0);\n"
"}";


struct sprite_vertex {
    vec3 pos;
    vec2 tex;
};

void _glfw_error_fun(int error_code, const char* description) {
    LOG(stderr, "GLFW ERROR %d: %s", error_code, description);
}

void _check_shader(GLuint shader) {
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

struct engine *engine_init(const char *title, const int width, const int height) {
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

    glfwSetErrorCallback(_glfw_error_fun);

    GLFWwindow *window = glfwCreateWindow(width, height, title, NULL, NULL);
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

    mat4x4_ortho(engine->view, 0, (float) width, 0, (float) height, -1.0f, 1.0f);

    // Sprite Batch program
    engine->sprite_batch_program = glCreateProgram();
    GLuint v_shader, f_shader;


    v_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(v_shader, 1, &vert_shader_src, NULL);
    glCompileShader(v_shader);
    _check_shader(v_shader);

    f_shader = glCreateShader(GL_FRAGMENT_SHADER);
    // Replace the "// INJECT_TEXTURES" string with all the samplers we need

    glShaderSource(f_shader, 1, &frag_shader_src, NULL);
    glCompileShader(f_shader);
    _check_shader(f_shader);

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
    return engine;
}

void engine_free(struct engine *engine) {
    arrfree(engine->sprites);
    arrfree(engine->free_sprites);

    for (int i = 0; i < shlen(engine->textures); i++) {
        glDeleteTextures(1, &engine->textures[i].value.tex);
    }
    shfree(engine->textures);

    glDeleteBuffers(1, &engine->sprite_batch_mesh);

    glfwDestroyWindow(engine->window);
    glfwTerminate();

    free(engine);
}

void load_texture(struct engine *engine, const char *texture_path) {
    GLuint tex;
    glCreateTextures(GL_TEXTURE_2D, 1, &tex);

    int x, y, bpp;
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

    shput(engine->textures, texture_path, ((struct texture){.tex = tex, .width = x, .height = y}));
}

void _flush(struct engine* engine, struct sprite_vertex* vertices, struct texture *texture) {
    glNamedBufferSubData(
        engine->sprite_batch_mesh,
        0,
        arrlen(vertices) * sizeof(struct sprite_vertex),
        (void*)vertices);

    glUseProgram(engine->sprite_batch_program);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glActiveTexture(GL_TEXTURE0 + 0);
    glBindTexture(GL_TEXTURE_2D, texture->tex);

    glBindVertexArray(engine->sprite_vao);

    glDrawArrays(GL_TRIANGLES, 0, arrlen(vertices));

    glUseProgram(0);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void render(struct engine* engine) {
    if (NULL == engine->sprites) {return;}

    size_t numSprites = arrlen(engine->sprites);
    if (0 == numSprites) return;

    struct sprite_vertex* vertices = NULL;

    struct texture* current_texture = NULL;

#define FLUSH_AND_CLEAR() {_flush(engine, vertices, current_texture); batch_size = 0;arrfree(vertices);}

    size_t batch_size = 0;
    for (size_t i = 0; i < numSprites; i++) {
        const struct sprite spr = engine->sprites[i];
        struct texture* tex = &shget(engine->textures, spr.texture_name);

        if (tex != current_texture && batch_size > 0) {FLUSH_AND_CLEAR()}
        // If the texture has changed we need to flush the batch. Encourage the use of Atlases, the renderer currently only support one texture at a time.

        current_texture = tex;

        struct sprite_vertex corners[4] = {
            (struct sprite_vertex){.pos = {spr.pos[0]                   , spr.pos[1]                    , 0.0f}, .tex = {0.0f, 0.0f}},
            (struct sprite_vertex){.pos = {spr.pos[0]                   , spr.pos[1] + (float)tex->height, 0.0f}, .tex = {0.0f, 1.0f}},
            (struct sprite_vertex){.pos = {spr.pos[0] + (float)tex->width, spr.pos[1] + (float)tex->height, 0.0f}, .tex = {1.0f, 1.0f}},
            (struct sprite_vertex){.pos = {spr.pos[0] + (float)tex->width, spr.pos[1]                    , 0.0f}, .tex = {1.0f, 0.0f}},
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

#endif
