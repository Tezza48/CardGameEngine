//
// Created by tezza on 13/12/2025.
//

#include "editor_ui.h"

#include <assert.h>
#include <stb_ds.h>
#include <stdio.h>
#include <stdbool.h>
#include <float.h>

void _sprite_list_op_recursive(engine* engine, sprite_handle handle, struct nk_context* nk) {
    sprite* sprite = &engine->sprites[handle];

    const texture* texture = &shget(engine->textures, sprite->texture_name);

    char title_format[256];
    snprintf(title_format, 256, "[ %d ]{ texture: %s }", (int)handle, sprite->texture_name, sprite->pos[0], sprite->pos[1], texture->rect[2], texture->rect[3]);

    if (nk_tree_push_id(nk, NK_TREE_NODE, title_format, NK_MINIMIZED, handle)) {
        char tex_label_format[256];
        snprintf(tex_label_format, 256, "texture: %s", sprite->texture_name);
        nk_label(nk, tex_label_format, NK_TEXT_ALIGN_LEFT);

        sprite->visible = nk_check_label(nk, "visible", sprite->visible);

        nk_layout_row_begin(nk, NK_DYNAMIC, 0, 2);
        nk_layout_row_push(nk, 0.5f);
        nk_layout_row_push(nk, 0.5f);
        nk_property_float(nk, "#pos x", -FLT_MAX, &sprite->pos[0], FLT_MAX, 1, 0.5);
        nk_property_float(nk, "#pos y", -FLT_MAX, &sprite->pos[1], FLT_MAX, 1, 0.5);
        nk_layout_row_end(nk);

        nk_label(nk, "Global Bounds", NK_TEXT_ALIGN_LEFT);

        nk_layout_row_begin(nk, NK_DYNAMIC, 0, 2);
        nk_layout_row_push(nk, 0.5f);
        nk_layout_row_push(nk, 0.5f);
        char buff[128];
        snprintf(buff, 128, "X1: %.1f", sprite->global_bounds[0]);
        nk_label(nk, buff, NK_TEXT_ALIGN_LEFT);

        snprintf(buff, 128, "Y1: %.1f", sprite->global_bounds[1]);
        nk_label(nk, buff, NK_TEXT_ALIGN_LEFT);
        nk_layout_row_end(nk);
        nk_layout_row_begin(nk, NK_DYNAMIC, 0, 2);
        nk_layout_row_push(nk, 0.5f);
        nk_layout_row_push(nk, 0.5f);

        snprintf(buff, 128, "X2: %.1f", sprite->global_bounds[2]);
        nk_label(nk, buff, NK_TEXT_ALIGN_LEFT);

        snprintf(buff, 128, "Y2: %.1f", sprite->global_bounds[3]);
        nk_label(nk, buff, NK_TEXT_ALIGN_LEFT);
        nk_layout_row_end(nk);


        for (size_t i = 0; i < arrlen(sprite->children); i++) {
            _sprite_list_op_recursive(engine, sprite->children[i], nk);
        }
        nk_tree_pop(nk);
    }
}

void _sprite_list(struct engine* engine, struct nk_context* nk) {
    if (nk_begin(nk, "Sprites", nk_rect(50, 50, 300, 500), NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_TITLE)) {
        _sprite_list_op_recursive(engine, engine->root, nk);
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
        nk_layout_row_static(nk, 20, 180, 1);

        nk_label(nk, "FPS: __COMING SOON__", NK_TEXT_ALIGN_LEFT);

        char draw_calls[128];
        snprintf(draw_calls, _countof(draw_calls), "Draw Calls: %d", engine->engine_stats.draw_calls);
        nk_label(nk, draw_calls, NK_TEXT_ALIGN_LEFT);

        char cursor_pos[128];
        snprintf(cursor_pos, _countof(cursor_pos), "Cursor Pos: %.0f, %.0f", engine->cursor_pos[0], engine->cursor_pos[1]);
        nk_label(nk, cursor_pos, NK_TEXT_ALIGN_LEFT);
    }
    nk_end(nk);
}

void editor_ui_render(struct engine *engine, struct nk_context *nk) {
    _sprite_list(engine, nk);
    //
    // _textures(engine, nk);
    //
    // _stats(engine, nk);
}