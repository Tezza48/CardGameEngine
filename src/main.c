#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "engine.h"
#include "nuklear_include.h"

#include <stb_ds.h>
#include "editor_ui.h"
#include "texturepacker.h"
#include <time.h>

#define MAX_VERTEX_BUFFER 512 * 1024
#define MAX_ELEMENT_BUFFER 128 * 1024

typedef enum SUIT {
    CLUBS,
    HEARTS,
    SPADES,
    DIAMONDS,
} SUIT;

typedef struct card_data {
    SUIT suit;
    int rank;
} card_data;

typedef struct app {
    engine* engine;

    card_data* full_deck;

    card_data* current_deck;

    card_data room_data[4];
    sprite_handle room_parent;
    sprite_handle room_cards[4];
} app;

// Allocates returned value.
char *card_data_to_texture(struct card_data data) {
    char *suit = NULL;
    switch (data.suit) {
        case CLUBS: suit = "Clubs"; break;
        case HEARTS: suit = "Hearts"; break;
        case SPADES: suit = "Spades"; break;
        case DIAMONDS: suit = "Diamonds"; break;
        default: LOG(stderr, "Unknown card suit '%d'", data.rank); break;
    }

    char *rank = NULL;
    switch (data.rank) {
        case 1: rank = "A"; break;
        case 2: rank = "2"; break;
        case 3: rank = "3"; break;
        case 4: rank = "4"; break;
        case 5: rank = "5"; break;
        case 6: rank = "6"; break;
        case 7: rank = "7"; break;
        case 8: rank = "8"; break;
        case 9: rank = "9"; break;
        case 10: rank = "10"; break;
        case 11: rank = "J"; break;
        case 12: rank = "Q"; break;
        case 13: rank = "K"; break;
        default: LOG(stderr, "Unknown card rank '%d'", data.rank); break;
    }

    const char* format = "Cards/card%s%s";
    const int count = snprintf(NULL, 0, format, suit, rank);
    char* result = calloc(count + 1, sizeof(char));
    snprintf(result, count + 1, format, suit, rank);

    return result;
}

void start_new_deck(engine* engine, app* app) {
    assert(engine && app);

    if (arrlen(app->current_deck) > 0) {
        arrfree(app->current_deck);
    }

    for (size_t i = 0; i < arrlen(app->full_deck); i++) {
        arrput(app->current_deck, app->full_deck[i]);
    }

    assert(app->full_deck && app->current_deck);
    for (size_t i = 0; i < arrlen(app->current_deck); i++) {
        int swap_idx = rand() % (int)arrlen(app->current_deck);
        card_data temp = app->full_deck[swap_idx];
        app->full_deck[swap_idx] = app->current_deck[i];
        app->current_deck[i] = temp;
    }
}

void on_mouse_button_for_sprite(void* app_user, int button, int action, int mods) {
    app* app = (struct app*)app_user;

    engine_clean_sprite_hierarchy(app->engine);


}

void start(engine *engine, app *app) {
    srand(time(NULL));

    event_push(engine->mouse_button_callbacks, mouse_button_callback, app, on_mouse_button_for_sprite);

    // No Red Face Cards or Aces
    arrput(app->full_deck, ((struct card_data){CLUBS, 1}));
    arrput(app->full_deck, ((struct card_data){CLUBS, 2}));
    arrput(app->full_deck, ((struct card_data){CLUBS, 3}));
    arrput(app->full_deck, ((struct card_data){CLUBS, 4}));
    arrput(app->full_deck, ((struct card_data){CLUBS, 5}));
    arrput(app->full_deck, ((struct card_data){CLUBS, 6}));
    arrput(app->full_deck, ((struct card_data){CLUBS, 7}));
    arrput(app->full_deck, ((struct card_data){CLUBS, 8}));
    arrput(app->full_deck, ((struct card_data){CLUBS, 9}));
    arrput(app->full_deck, ((struct card_data){CLUBS, 10}));
    arrput(app->full_deck, ((struct card_data){CLUBS, 11}));
    arrput(app->full_deck, ((struct card_data){CLUBS, 12}));
    arrput(app->full_deck, ((struct card_data){CLUBS, 13}));

    // arrput(app->full_deck, ((struct card_data){HEARTS, 1}));
    arrput(app->full_deck, ((struct card_data){HEARTS, 2}));
    arrput(app->full_deck, ((struct card_data){HEARTS, 3}));
    arrput(app->full_deck, ((struct card_data){HEARTS, 4}));
    arrput(app->full_deck, ((struct card_data){HEARTS, 5}));
    arrput(app->full_deck, ((struct card_data){HEARTS, 6}));
    arrput(app->full_deck, ((struct card_data){HEARTS, 7}));
    arrput(app->full_deck, ((struct card_data){HEARTS, 8}));
    arrput(app->full_deck, ((struct card_data){HEARTS, 9}));
    arrput(app->full_deck, ((struct card_data){HEARTS, 10}));
    // arrput(app->full_deck, ((struct card_data){HEARTS, 11}));
    // arrput(app->full_deck, ((struct card_data){HEARTS, 12}));
    // arrput(app->full_deck, ((struct card_data){HEARTS, 13}));

    arrput(app->full_deck, ((struct card_data){SPADES, 1}));
    arrput(app->full_deck, ((struct card_data){SPADES, 2}));
    arrput(app->full_deck, ((struct card_data){SPADES, 3}));
    arrput(app->full_deck, ((struct card_data){SPADES, 4}));
    arrput(app->full_deck, ((struct card_data){SPADES, 5}));
    arrput(app->full_deck, ((struct card_data){SPADES, 6}));
    arrput(app->full_deck, ((struct card_data){SPADES, 7}));
    arrput(app->full_deck, ((struct card_data){SPADES, 8}));
    arrput(app->full_deck, ((struct card_data){SPADES, 9}));
    arrput(app->full_deck, ((struct card_data){SPADES, 10}));
    arrput(app->full_deck, ((struct card_data){SPADES, 11}));
    arrput(app->full_deck, ((struct card_data){SPADES, 12}));
    arrput(app->full_deck, ((struct card_data){SPADES, 13}));

    // arrput(app->full_deck, ((struct card_data){DIAMONDS, 1}));
    arrput(app->full_deck, ((struct card_data){DIAMONDS, 2}));
    arrput(app->full_deck, ((struct card_data){DIAMONDS, 3}));
    arrput(app->full_deck, ((struct card_data){DIAMONDS, 4}));
    arrput(app->full_deck, ((struct card_data){DIAMONDS, 5}));
    arrput(app->full_deck, ((struct card_data){DIAMONDS, 6}));
    arrput(app->full_deck, ((struct card_data){DIAMONDS, 7}));
    arrput(app->full_deck, ((struct card_data){DIAMONDS, 8}));
    arrput(app->full_deck, ((struct card_data){DIAMONDS, 9}));
    arrput(app->full_deck, ((struct card_data){DIAMONDS, 10}));
    // arrput(app->full_deck, ((struct card_data){DIAMONDS, 11}));
    // arrput(app->full_deck, ((struct card_data){DIAMONDS, 12}));
    // arrput(app->full_deck, ((struct card_data){DIAMONDS, 13}));


    texturepacker_data *tps = load_spritesheet("assets/spritesheet.atlas");
    engine_load_texture(engine, tps->img_path, tps->spritesheet);
    free_spritesheet(tps);

    tps = load_spritesheet("assets/background.atlas");
    engine_load_texture(engine, tps->img_path, tps->spritesheet);
    free_spritesheet(tps);

    start_new_deck(engine, app);

    sprite_handle felt = engine_create_sprite(engine, "felt", (vec2){0});
    engine_add_sprite_child(engine, engine->root, felt);

    vec2 card_size = {140, 190};
    vec2 card_background_size = {160, 210};

    app->room_parent = engine_create_sprite(engine, "", (vec2){1280 / 2.0f - (float)card_size[0] * 2.0f, 720 - (float)card_size[1] - 50});
    engine_add_sprite_child(engine, engine->root, app->room_parent);


    float padding = 20.0f;

    vec2 bg_diff;
    vec2_sub(bg_diff, card_size, card_background_size);
    vec2_scale(bg_diff, bg_diff, 0.5f);

    for (size_t i = 0; i < 4; i++) {
        sprite_handle bg = engine_create_sprite(engine, "card_area", (vec2){
            (float)i * (card_size[0] + padding) + bg_diff[0],
            bg_diff[1]
        });
        engine_add_sprite_child(engine, app->room_parent, bg);
    }

    for (size_t i = 0; i < 4; i++) {
        app->room_data[i] = (card_data){SPADES, 1};
        char* tex_name = card_data_to_texture(app->room_data[i]);
        app->room_cards[i] = engine_create_sprite(engine, tex_name, (vec2){
            (float)i * (card_size[0] + padding),
            0.0f
        });
        free(tex_name);
        engine_add_sprite_child(engine, app->room_parent, app->room_cards[i]);
    }
}

void update(engine* engine, app* app) {

}

void cleanup(engine* engine, app* app) {
    arrfree(app->full_deck);
    arrfree(app->current_deck);
    // Freeing room_parent will also free the children
    engine_delete_sprite(engine, app->room_parent);
}

void nk_scroll_callback_fwd(void *ctx, double xoff, double yoff) {
    nk_gflw3_scroll_callback((GLFWwindow *) ctx, xoff, yoff);
}

void nk_char_callback_fwd(void *ctx, unsigned int c) {
    nk_glfw3_char_callback((GLFWwindow *) ctx, c);
}

void nk_key_callback_fwd(void *ctx, int key, int scancode, int action, int mods) {
    nk_glfw3_key_callback((GLFWwindow *) ctx, key, scancode, action, mods);
}

void nk_mouse_button_callback_fwd(void *ctx, int button, int action, int mods) {
    nk_glfw3_mouse_button_callback((GLFWwindow *) ctx, button, action, mods);
}

int main(int argc, char **argv) {
    struct engine *engine = engine_init((struct engine_config){});
    if (!engine) {
        return -1;
    }

    struct nk_context *ctx = nk_glfw3_init(engine->window, NK_GLFW3_DEFAULT, MAX_VERTEX_BUFFER, MAX_ELEMENT_BUFFER);

    {
        struct nk_font_atlas *atlas;
        nk_glfw3_font_stash_begin(&atlas);
        struct nk_font *roboto = nk_font_atlas_add_from_file(atlas, "./assets/Roboto-Regular.ttf", 14, NULL);
        nk_glfw3_font_stash_end();
        nk_style_set_font(ctx, &roboto->handle);
    }

    event_push(engine->scroll_callbacks, scroll_callback, engine->window, nk_scroll_callback_fwd);
    event_push(engine->char_callbacks, char_callback, engine->window, nk_char_callback_fwd);
    event_push(engine->key_callbacks, key_callback, engine->window, nk_key_callback_fwd);
    event_push(engine->mouse_button_callbacks, mouse_button_callback, engine->window, nk_mouse_button_callback_fwd);

    app app = {0};
    app.engine = engine;
    start(engine, &app);

    while (!glfwWindowShouldClose(engine->window)) {
        glfwPollEvents();
        nk_glfw3_new_frame();

        int width;
        int height;
        glfwGetWindowSize(engine->window, &width, &height);
        glViewport(0, 0, width, height);

        editor_ui_render(engine, ctx);

        glClear(GL_COLOR_BUFFER_BIT);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

        update(engine, &app);
        engine_render(engine);

        nk_glfw3_render(NK_ANTI_ALIASING_ON);

        glfwSwapBuffers(engine->window);
    }

    cleanup(engine, &app);

    nk_glfw3_shutdown();

    engine_free(engine);

    return 0;
}
