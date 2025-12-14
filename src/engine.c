//
// Created by tezza on 10/12/2025.
//

#include "engine.h"

#include <assert.h>
#include <malloc.h>
#include <stb_ds.h>
#include <stb_image.h>
#include <string.h>


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

    glfwSetErrorCallback(_glfw_error_fun);

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

    engine_resize(engine, window_width, window_height);

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
        assert(engine->base_textures);
        glDeleteTextures(1, &engine->base_textures[i].value);
    }
    shfree(engine->base_textures);

    shfree(engine->textures);

    glDeleteBuffers(1, &engine->sprite_batch_mesh);

    glfwDestroyWindow(engine->window);
    glfwTerminate();

    free(engine);
}

void engine_resize(struct engine *engine, int width, int height) {
    mat4x4_ortho(engine->view, 0, (float)width, 0, (float)height, -1.0f, 1.0f);
}

// TODO WT: Support Texture Atlases
void load_texture(struct engine *engine, const char *texture_path, const spritesheet_entry* sprites) {
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

    size_t texture_index = arrlen(engine->base_textures);
    shputs(engine->base_textures, ((struct base_texture){
        .key = (char*)texture_path,
        .value=tex,
        .size = {(float)x, (float)y}
    }));

    if (NULL == sprites) {
        shput(engine->textures, texture_path, ((struct texture){
            .texture_index = texture_index,
            .rect = {0, 0, (float)x, (float)y}
        }));
    } else {
        for (size_t i = 0; i < shlen(sprites); i++) {
            shput(engine->textures, sprites[i].key, ((struct texture){
                .texture_index = texture_index,
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

void render(struct engine* engine) {
    engine->engine_stats.draw_calls = 0;

    if (NULL == engine->sprites) {return;}

    size_t numSprites = arrlen(engine->sprites);
    if (0 == numSprites) return;

    struct sprite_vertex* vertices = NULL;

    struct base_texture* current_texture = NULL;

#define FLUSH_AND_CLEAR() {_flush(engine, vertices, current_texture->value); batch_size = 0;arrfree(vertices);}

    size_t batch_size = 0;
    for (size_t i = 0; i < numSprites; i++) {
        const struct sprite spr = engine->sprites[i];
        struct texture* tex = &shget(engine->textures, spr.texture_name);
        struct base_texture* base_texture = &engine->base_textures[tex->texture_index];

        if ((base_texture != current_texture) && batch_size > 0) {FLUSH_AND_CLEAR()}
        // If the texture has changed we need to flush the batch. Encourage the use of Atlases, the renderer currently only support one texture at a time.

        current_texture = base_texture;

        float px = tex->rect[0], py = tex->rect[1], pw = tex->rect[2], ph = tex->rect[3], w = current_texture->size[0], h = current_texture->size[1];

        struct sprite_vertex corners[4] = {
            (struct sprite_vertex){.pos = {spr.pos[0]     , spr.pos[1]     , 0.0f}, .tex = {px/w,      py/h}},
            (struct sprite_vertex){.pos = {spr.pos[0]     , spr.pos[1] + ph, 0.0f}, .tex = {px/w,      (py+ph)/h}},
            (struct sprite_vertex){.pos = {spr.pos[0] + pw, spr.pos[1] + ph, 0.0f}, .tex = {(px+pw)/w, (py+ph)/h}},
            (struct sprite_vertex){.pos = {spr.pos[0] + pw, spr.pos[1]     , 0.0f}, .tex = {(px+pw)/w, py/h}},
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
        _flush(engine, vertices, current_texture->value);
    }
}
