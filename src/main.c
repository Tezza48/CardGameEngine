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

typedef enum STATE {
    WAITING,
    DRAGGING,
} STATE;

typedef struct app {
    engine* engine;

    int life_points;


    card_data* full_deck;

    card_data* current_deck;

    card_data room_data[4];
    bool free_room_positions[4];
    sprite_handle room_parent;
    sprite_handle room_sprites[4];

    bool has_weapon;
    card_data weapon;
    sprite_handle weapon_sprite;
    int last_slain_monster_size;

    card_data* slain_monsters;
    sprite_handle* slain_monster_sprites;

    STATE state;
    int currently_dragging_index;
} app;

// Allocates returned string with calloc
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

    app->life_points = 20;
}

void on_mouse_button_for_sprite(void* app_user, int button, int action, int mods) {
    app* app = (struct app*)app_user;
    struct engine* engine = app->engine;

    engine_try_clean_sprite_hierarchy(app->engine);

    switch (app->state) {
        case WAITING:
            if (action == GLFW_PRESS) {
                for (size_t i = 0; i < 4; i++) {
                    sprite_handle card_handle = app->room_sprites[i];
                    struct sprite* spr = &app->engine->sprites[card_handle];
                    if (vec2_inside_aabb(app->engine->cursor_pos, spr->global_bounds)) {
                        app->state = DRAGGING;
                        app->currently_dragging_index = (int)i;
                        break;
                    }
                }
            }
            break;
        case DRAGGING:
            if (action == GLFW_RELEASE) {
                // Stop dragging, if it's over the weapon, try to kill with weapon.
                // if it's over Life Points area, kill using life points
                // Reset the position of the sprite to starting pos
                const sprite* weapon = &engine->sprites[app->weapon_sprite];
                if (vec2_inside_aabb(app->engine->cursor_pos, weapon->global_bounds)) {
                    // Consume the card using the weapon
                    app->state = WAITING;
                }

                // If it's over the life points sprite

                // Otherwise, reset the position
                engine_set_sprite_pos(engine, app->room_sprites[app->currently_dragging_index], (vec2){0});
            }
            break;
    }
}

void refill_room(app* app) {
    for (size_t i = 0; i < 4; i++) {
        if (app->free_room_positions[i]) {
            app->room_data[i] = arrpop(app->current_deck);

            char* sprite_name = card_data_to_texture(app->room_data[i]);
            engine_sprite_set_texture(app->engine, app->room_sprites[i], sprite_name);
            free(sprite_name);

            app->free_room_positions[i] = false;

            // we want to stop once we've dealt the last card in the deck.
            if (arrlen(app->current_deck) == 0) break;
        }
    }
}

void create_full_deck(app* app) {
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
}

void start(engine *engine, app *app) {
    memset(app->free_room_positions, true, 4);

    event_push(engine->mouse_button_callbacks, mouse_button_callback, app, on_mouse_button_for_sprite);

    create_full_deck(app);

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

    // Create the card area backgrounds
    for (size_t i = 0; i < 4; i++) {
        sprite_handle bg = engine_create_sprite(engine, "card_area", (vec2){
            (float)i * (card_size[0] + padding) + bg_diff[0],
            bg_diff[1]
        });
        engine_add_sprite_child(engine, app->room_parent, bg);
    }

    // create the actual room card sprites
    for (size_t i = 0; i < 4; i++) {
        // create container spr in correct pos, create card sprite as child so we can drag it and reset pos
        sprite_handle parent = engine_create_sprite(engine, "", (vec2){
            (float)i * (card_size[0] + padding),
            0.0f
        });
        engine_add_sprite_child(engine, app->room_parent, parent);

        app->room_sprites[i] = engine_create_sprite(engine, "", (vec2){0});
        engine_add_sprite_child(engine, parent, app->room_sprites[i]);
    }

    refill_room(app);
}

void update(engine* engine, app* app) {
    switch (app->state) {
        case WAITING:

            break;
        case DRAGGING:
            engine->sprites_dirty = true;
            engine_try_clean_sprite_hierarchy(engine);

            sprite_handle dragged_sprite = app->room_sprites[app->currently_dragging_index];
            sprite_handle parent = engine_get_parent(engine, dragged_sprite);

            // Move the card sprite to be under the cursor, in global space
            const float* cursor_pos = engine->cursor_pos;
            vec2 parent_global;
            engine_get_global_pos(engine, parent, parent_global);

            vec2 spr_pos;
            vec2_sub(spr_pos, cursor_pos, parent_global);

            aabb bounds;
            engine_get_sprite_bounds(engine, dragged_sprite, bounds);
            vec2 bound_offset = {bounds[2] / 2.0f, bounds[3] / 2.0f};
            vec2_sub(spr_pos, spr_pos, bound_offset);
            engine_set_sprite_pos(engine, dragged_sprite, spr_pos);

            break;
    }
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
    srand(time(NULL));

    struct engine *engine = engine_init((struct engine_config){});
    if (!engine) {
        return -1;
    }

    struct nk_context *nk = nk_glfw3_init(engine->window, NK_GLFW3_DEFAULT, MAX_VERTEX_BUFFER, MAX_ELEMENT_BUFFER);

    {
        struct nk_font_atlas *atlas;
        nk_glfw3_font_stash_begin(&atlas);
        struct nk_font *roboto = nk_font_atlas_add_from_file(atlas, "./assets/Roboto-Regular.ttf", 14, NULL);
        nk_glfw3_font_stash_end();
        nk_style_set_font(nk, &roboto->handle);
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

        editor_ui_render(engine, nk);

        if (nk_begin(nk, "Player Info", nk_rect(1280 - 300, 50, 280, 200), NK_WINDOW_TITLE)) {
            nk_layout_row_dynamic(nk, 20, 1);
            char life_points[128];
            snprintf(life_points, _countof(life_points), "Life Points: %d/20", app.life_points);
            nk_label(nk, life_points, NK_TEXT_ALIGN_LEFT);
        }
        nk_end(nk);

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
