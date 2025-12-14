#include <stdio.h>
#include <stdlib.h>

#include "engine.h"
#include "nuklear_include.h"

#include <stb_ds.h>
#include "editor_ui.h"
#include "texturepacker.h"

#define MAX_VERTEX_BUFFER 512 * 1024
#define MAX_ELEMENT_BUFFER 128 * 1024

void start(struct engine * engine) {

    texturepacker_data *tps = load_spritesheet("assets/spritesheet.atlas");
    load_texture(engine, tps->img_path, tps->spritesheet);
    load_texture(engine, tps->img_path, NULL);
    load_texture(engine, "assets/kenney_boardgame/Cards/cardBack_blue1.png", NULL);
    create_sprite(engine, tps->img_path, (vec2){0});
    free_spritesheet(tps);


    create_sprite(engine, "Cards/cardClubs2", (vec2){30.0f, 30.0f});

    create_sprite(engine, "assets/kenney_boardgame/Cards/cardBack_blue1.png", (vec2){50.0f, 0});
}

void nk_scroll_callback_fwd(void* ctx, double xoff, double yoff) {
    nk_gflw3_scroll_callback((GLFWwindow*)ctx, xoff, yoff);
}
void nk_char_callback_fwd(void* ctx, unsigned int c) {
    nk_glfw3_char_callback((GLFWwindow*)ctx, c);
}
void nk_key_callback_fwd(void* ctx, int key, int scancode, int action, int mods) {
    nk_glfw3_key_callback((GLFWwindow*)ctx, key, scancode, action, mods);
}
void nk_mouse_button_callback_fwd(void* ctx, int button, int action, int mods) {
    nk_glfw3_mouse_button_callback((GLFWwindow*)ctx, button, action, mods);
}

int main(int argc, char** argv) {
    struct engine* engine = engine_init((struct engine_config){});
    if (!engine) {
        return -1;
    }

    struct nk_context* ctx = nk_glfw3_init(engine->window, NK_GLFW3_DEFAULT, MAX_VERTEX_BUFFER, MAX_ELEMENT_BUFFER);

    {
        struct nk_font_atlas * atlas;
        nk_glfw3_font_stash_begin(&atlas);
        struct nk_font* roboto = nk_font_atlas_add_from_file(atlas, "./assets/Roboto-Regular.ttf", 14, NULL);
        nk_glfw3_font_stash_end();
        nk_style_set_font(ctx, &roboto->handle);
    }

    event_push(engine->scroll_callbacks, scroll_callback, engine->window, nk_scroll_callback_fwd);
    event_push(engine->char_callbacks, char_callback, engine->window, nk_char_callback_fwd);
    event_push(engine->key_callbacks, key_callback, engine->window, nk_key_callback_fwd);
    event_push(engine->mouse_button_callbacks, mouse_button_callback, engine->window, nk_mouse_button_callback_fwd);

    start(engine);

    while (!glfwWindowShouldClose(engine->window)) {
        glfwPollEvents();
        nk_glfw3_new_frame();

        int width;
        int height;
        glfwGetWindowSize(engine->window, &width, &height);
        glViewport(0, 0, width, height);

        editor_ui_render(engine, ctx);

        glClear(GL_COLOR_BUFFER_BIT);
        glClearColor(0.5f, 0.5f, 0.5f, 1.0f);

        engine_render(engine);

        nk_glfw3_render(NK_ANTI_ALIASING_ON);

        glfwSwapBuffers(engine->window);
    }

    nk_glfw3_shutdown();

    engine_free(engine);

    return 0;
}