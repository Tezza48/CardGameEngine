#include <stdio.h>
#include <stdlib.h>

#define ENGINE_IMPLEMENTATION
#include "engine.h"
#undef ENGINE_IMPLEMENTATION

#define NK_IMPLEMENTATION
#define NK_GLFW_GL4_IMPLEMENTATION
#include "nuklear_include.h"
#undef NK_IMPLEMENTATION
#undef NK_GLFW_GL4_IMPLEMENTATION

#define STB_DS_IMPLEMENTATION
#include <stb_ds.h>
#undef STB_DS_IMPLEMENTATION

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#undef STB_IMAGE_IMPLEMENTATION

#define MAX_VERTEX_BUFFER (512 * 1024)
#define MAX_ELEMENT_BUFFER (128 * 1024)

int main(int argc, char** argv) {
    const int initial_width = 1280;
    const int initial_height = 720;

    struct engine* engine = engine_init("Card Game Engine", initial_width, initial_height);
    if (!engine) {
        return -1;
    }

    struct nk_context* ctx = nk_glfw3_init(engine->window, NK_GLFW3_INSTALL_CALLBACKS, MAX_VERTEX_BUFFER, MAX_ELEMENT_BUFFER);

    {
        struct nk_font_atlas * atlas;
        nk_glfw3_font_stash_begin(&atlas);
        struct nk_font* roboto = nk_font_atlas_add_from_file(atlas, "./assets/Roboto-Regular.ttf", 14, NULL);
        nk_glfw3_font_stash_end();
        nk_style_set_font(ctx, &roboto->handle);
    }

    enum {EASY, HARD};
    static int op = EASY;
    static float value = 0.6f;
    static int i = 20;

    load_texture(engine, "assets/cardClubs2.png");

    arrput(engine->sprites, ((struct sprite){.texture_name = "assets/cardClubs2.png", .pos = {0, 0}}));

    while (!glfwWindowShouldClose(engine->window)) {
        glfwPollEvents();
        nk_glfw3_new_frame();

        int width;
        int height;
        glfwGetWindowSize(engine->window, &width, &height);
        glViewport(0, 0, width, height);

        if (nk_begin(ctx, "Sprites", nk_rect(50, 50, 300, 500), NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_TITLE)) {
            size_t num_sprites = arrlen(engine->sprites);
            nk_layout_row_static(ctx, 30, 300, 1);
            for (size_t spr_index = 0; spr_index < num_sprites; spr_index++) {
                const struct sprite* sprite = &engine->sprites[spr_index];

                char format[256];
                snprintf(format, 256, "[ %d ]{texture: %s, pos: [%2f,%2f]", (int)spr_index, sprite->texture_name, sprite->pos[0], sprite->pos[1]);

                nk_button_label(ctx, format);
            }
        }
        nk_end(ctx);

        if (nk_begin(ctx, "Show", nk_rect(50, 50, 220, 220), NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_CLOSABLE)) {
            nk_layout_row_static(ctx, 30, 80, 1);

            if (nk_button_label(ctx, "button")) {

            }

            nk_layout_row_dynamic(ctx, 30, 2);
            if (nk_option_label(ctx, "easy", op == EASY)) op = EASY;
            if (nk_option_label(ctx, "hard", op == HARD)) op = HARD;

            nk_layout_row_begin(ctx, NK_STATIC, 30, 2);
            {
                nk_layout_row_push(ctx, 50);
                nk_label(ctx, "Volume:", NK_TEXT_LEFT);
                nk_layout_row_push(ctx, 110);
                nk_slider_float(ctx, 0, &value, 1.0f, 0.1f);
            }
            nk_layout_row_end(ctx);
        }
        nk_end(ctx);

        glClear(GL_COLOR_BUFFER_BIT);
        glClearColor(0.5f, 0.5f, 0.5f, 1.0f);

        render(engine);

        nk_glfw3_render(NK_ANTI_ALIASING_ON);

        glfwSwapBuffers(engine->window);
    }

    nk_glfw3_shutdown();

    engine_free(engine);

    return 0;
}