//
// Created by tezza on 13/12/2025.
//

#include "editor_ui.h"

#include <assert.h>
#include <stb_ds.h>
#include <stdio.h>


void _sprite_list(struct engine* engine, struct nk_context* nk) {

    if (nk_begin(nk, "Sprites", nk_rect(50, 50, 300, 500), NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_TITLE)) {
        size_t num_sprites = arrlen(engine->sprites);
        nk_layout_row_static(nk, 30, 300, 1);
        for (size_t spr_index = 0; spr_index < num_sprites; spr_index++) {
            const struct sprite* sprite = &engine->sprites[spr_index];

            char format[256];
            snprintf(format, 256, "[ %d ]{texture: %s, pos: [%2f,%2f]", (int)spr_index, sprite->texture_name, sprite->pos[0], sprite->pos[1]);

            nk_label(nk, format, NK_TEXT_ALIGN_LEFT);
        }
    }
    nk_end(nk);
}

void _textures(struct engine* engine, struct nk_context* nk) {
    if (nk_begin(nk, "Textures", nk_rect(50, 50, 200, 500), NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_DYNAMIC | NK_WINDOW_TITLE)) {
        nk_layout_row_dynamic(nk, 20, 1);
        for (size_t i = 0; i < shlen(engine->textures); i++) {
            assert(engine->textures);
            nk_label(nk, engine->textures[i].key, NK_TEXT_ALIGN_LEFT);
        }
    }
    nk_end(nk);
}

void _stats(struct engine* engine, struct nk_context* nk) {

    if (nk_begin(nk, "Stats", nk_rect(500, 50, 200, 200),  NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_DYNAMIC | NK_WINDOW_TITLE)) {
        nk_layout_row_static(nk, 30, 180, 1);

        nk_label(nk, "FPS: __COMING SOON__", NK_TEXT_ALIGN_LEFT);

        char draw_calls[128];
        snprintf(draw_calls, _countof(draw_calls), "Draw Calls: %d", engine->engine_stats.draw_calls);
        nk_label(nk, draw_calls, NK_TEXT_ALIGN_LEFT);
    }
    nk_end(nk);
}

void editor_ui_render(struct engine *engine, struct nk_context *nk) {
    _sprite_list(engine, nk);

    _textures(engine, nk);

    _stats(engine, nk);

}