/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
#include "arcade_common.h"
#include "arcade_audio.h"

#define TET_W 10u
#define TET_H 20u
#define TET_CELL 11u
#define TET_BX 14
#define TET_BY 10

#define TET_BG     RGB(10, 12, 20)
#define TET_BORDER RGB(90, 120, 200)
#define TET_GRID   RGB(22, 26, 40)

enum { TET_TITLE = 0, TET_PLAY = 1, TET_PAUSE = 2, TET_OVER = 3 };

/* 7 classic tetrominoes, 4 rotations, 4x4 bitmask (MSB = top-left) */
static const uint16_t TET_SHAPES[7][4] = {
    {0x0F00u, 0x2222u, 0x00F0u, 0x4444u}, /* I */
    {0x6600u, 0x6600u, 0x6600u, 0x6600u}, /* O */
    {0x4E00u, 0x4640u, 0x0E40u, 0x4C40u}, /* T */
    {0x6C00u, 0x4620u, 0x06C0u, 0x8C40u}, /* S */
    {0xC600u, 0x2640u, 0x0C60u, 0x4C80u}, /* Z */
    {0x8E00u, 0x6440u, 0x0E20u, 0x44C0u}, /* J */
    {0x2E00u, 0x4460u, 0x0E80u, 0xC440u}, /* L */
};

static const uint16_t TET_COLS[8] = {
    TET_BG,
    RGB(60, 230, 255),  /* I cyan */
    RGB(255, 220, 50),  /* O yellow */
    RGB(190, 70, 255),  /* T purple */
    RGB(70, 230, 80),   /* S green */
    RGB(255, 65, 65),   /* Z red */
    RGB(60, 110, 255),  /* J blue */
    RGB(255, 150, 40),  /* L orange */
};

static struct {
    uint8_t board[TET_H][TET_W];
    int8_t px, py;
    uint8_t piece, rot, next_piece;
    uint16_t score, lines;
    uint8_t st, music_on, new_hi;
    uint32_t last_drop_ms;
} tet;

static int tet_cell_on(uint8_t p, uint8_t r, int32_t cx, int32_t cy)
{
    uint16_t mask = TET_SHAPES[p][r & 3u];
    uint32_t bit = 15u - (uint32_t)(cy * 4 + cx);
    return (mask >> bit) & 1u;
}

static int tet_fits(uint8_t p, uint8_t r, int8_t bx, int8_t by)
{
    int32_t cx, cy;
    for (cy = 0; cy < 4; cy++) {
        for (cx = 0; cx < 4; cx++) {
            if (!tet_cell_on(p, r, cx, cy)) continue;
            int32_t gx = (int32_t)bx + cx;
            int32_t gy = (int32_t)by + cy;
            if (gx < 0 || gx >= (int32_t)TET_W || gy >= (int32_t)TET_H) return 0;
            if (gy >= 0 && tet.board[gy][gx] != 0u) return 0;
        }
    }
    return 1;
}

static void tet_draw_block(int32_t x, int32_t y, uint8_t col_idx)
{
    if (col_idx == 0u) {
        lcd_fill(x, y, 11, 11, TET_BG);
        lcd_fill(x + 5, y + 5, 1, 1, TET_GRID);
    } else {
        lcd_fill(x, y, 11, 11, TET_BG);
        lcd_fill(x + 1, y + 1, 9, 9, TET_COLS[col_idx]);
    }
}

static void tet_draw_board(void)
{
    uint32_t r, c;
    for (r = 0u; r < TET_H; r++) {
        for (c = 0u; c < TET_W; c++) {
            tet_draw_block(TET_BX + (int32_t)c * 11, TET_BY + (int32_t)r * 11, tet.board[r][c]);
        }
    }
    if (tet.st == TET_PLAY || tet.st == TET_PAUSE) {
        int32_t cx, cy;
        for (cy = 0; cy < 4; cy++) {
            for (cx = 0; cx < 4; cx++) {
                if (!tet_cell_on(tet.piece, tet.rot, cx, cy)) continue;
                int32_t gx = (int32_t)tet.px + cx, gy = (int32_t)tet.py + cy;
                if (gy >= 0 && gy < (int32_t)TET_H && gx >= 0 && gx < (int32_t)TET_W)
                    tet_draw_block(TET_BX + gx * 11, TET_BY + gy * 11, (uint8_t)(tet.piece + 1u));
            }
        }
    }
}

static void tet_draw_side(void)
{
    char s[12];
    int32_t cx, cy;
    lcd_fill(134, 0, 106, 240, C_BLACK);
    draw_text_box(136, 12, 98, &FONT_S, "NEXT", RGB(160, 180, 255), 0);
    lcd_fill(138, 32, 48, 48, TET_BG);
    for (cy = 0; cy < 4; cy++) {
        for (cx = 0; cx < 4; cx++) {
            if (tet_cell_on(tet.next_piece, 0u, cx, cy)) {
                lcd_fill(140 + cx * 11, 36 + cy * 11, 9, 9, TET_COLS[tet.next_piece + 1u]);
            }
        }
    }
    draw_text_box(136, 92, 98, &FONT_S, "SCORE", RGB(160, 180, 255), 0);
    fmt_num(s, tet.score, 4u);
    draw_text_box(136, 110, 98, &FONT_S, s, C_WHITE, 0);

    draw_text_box(136, 138, 98, &FONT_S, "LINES", RGB(160, 180, 255), 0);
    fmt_num(s, tet.lines, 3u);
    draw_text_box(136, 156, 98, &FONT_S, s, C_WHITE, 0);

    draw_text_box(136, 184, 98, &FONT_S, "HI", RGB(255, 215, 70), 0);
    fmt_num(s, save_data.hi_tetris, 4u);
    draw_text_box(136, 202, 98, &FONT_S, s, RGB(255, 215, 70), 0);

    draw_text_box(136, 222, 98, &FONT_S, tet.music_on ? "FX: BGM ON" : "FX: BGM OFF", RGB(120, 130, 160), 0);
}

static void tet_draw_frame(void)
{
    lcd_fill(0, 0, 240, 240, C_BLACK);
    lcd_fill(TET_BX - 2, TET_BY - 2, (int32_t)(TET_W * 11u) + 4, (int32_t)(TET_H * 11u) + 4, TET_BORDER);
    tet_draw_board();
    tet_draw_side();
}

static void tet_spawn(void)
{
    tet.piece = tet.next_piece;
    tet.next_piece = (uint8_t)(arcade_rand() % 7u);
    tet.rot = 0u;
    tet.px = 3;
    tet.py = -1;
    if (!tet_fits(tet.piece, tet.rot, tet.px, tet.py)) {
        tet.st = TET_OVER;
        bgm_set(0u);
        fm1_led_key(BTN_PLAY, 0);
        sfx_gameover();
        lcd_fill(16, 84, 208, 78, C_BLACK);
        draw_text_box(20, 90, 200, &FONT_L, "GAME OVER", RGB(255, 90, 90), 1);
        draw_text_box(20, 134, 200, &FONT_S, "PLAY: RETRY  HOME: MENU", C_WHITE, 1);
        return;
    }
    tet_draw_side();
}

static void tet_lock(void)
{
    int32_t cx, cy;
    uint32_t cleared = 0u;
    int32_t r;
    for (cy = 0; cy < 4; cy++) {
        for (cx = 0; cx < 4; cx++) {
            if (!tet_cell_on(tet.piece, tet.rot, cx, cy)) continue;
            int32_t gx = (int32_t)tet.px + cx, gy = (int32_t)tet.py + cy;
            if (gy >= 0 && gy < (int32_t)TET_H && gx >= 0 && gx < (int32_t)TET_W)
                tet.board[gy][gx] = (uint8_t)(tet.piece + 1u);
        }
    }
    for (r = (int32_t)TET_H - 1; r >= 0; r--) {
        uint32_t c, full = 1u;
        for (c = 0u; c < TET_W; c++) if (tet.board[r][c] == 0u) { full = 0u; break; }
        if (full) {
            int32_t rr;
            cleared++;
            for (rr = r; rr > 0; rr--) {
                for (c = 0u; c < TET_W; c++) tet.board[rr][c] = tet.board[rr - 1][c];
            }
            for (c = 0u; c < TET_W; c++) tet.board[0][c] = 0u;
            r++;
        }
    }
    if (cleared) {
        static const uint16_t PTS[5] = {0u, 10u, 30u, 50u, 80u};
        tet.lines = (uint16_t)(tet.lines + cleared);
        tet.score = (uint16_t)(tet.score + PTS[cleared > 4u ? 4u : cleared]);
        if (tet.score > save_data.hi_tetris) {
            save_data.hi_tetris = tet.score;
            tet.new_hi = 1u;
            save_scores();
        }
        sfx_line();
    } else {
        sfx_blip(48u, 18u);
    }
    tet_spawn();
    if (tet.st == TET_PLAY) tet_draw_board();
}

static void tet_start(void)
{
    uint32_t r, c;
    for (r = 0u; r < TET_H; r++) for (c = 0u; c < TET_W; c++) tet.board[r][c] = 0u;
    tet.score = 0u; tet.lines = 0u; tet.new_hi = 0u;
    tet.st = TET_PLAY;
    tet.next_piece = (uint8_t)(arcade_rand() % 7u);
    tet.last_drop_ms = fm1_ms;
    fm1_led_key(BTN_PLAY, 1);
    bgm_set(tet.music_on);
    tet_spawn();
    tet_draw_frame();
}

static void tetris_enter(void)
{
    uint32_t r, c;
    for (r = 0u; r < TET_H; r++) for (c = 0u; c < TET_W; c++) tet.board[r][c] = 0u;
    tet.st = TET_TITLE;
    tet.music_on = 1u;
    bgm_set(0u);
    fm1_led_key(BTN_PLAY, 0);
    lcd_fill(0, 0, 240, 240, C_BLACK);
    draw_text_box(16, 36, 208, &FONT_L, "TETRIS", RGB(60, 230, 255), 1);
    draw_text_box(16, 86, 208, &FONT_S, "C / E: MOVE   D: DROP", C_WHITE, 1);
    draw_text_box(16, 108, 208, &FONT_S, "C#-D# / KNOB1: ROTATE", C_WHITE, 1);
    draw_text_box(16, 130, 208, &FONT_S, "F: HARD DROP  FX: MUSIC", RGB(160, 180, 255), 1);
    draw_text_box(16, 152, 208, &FONT_S, "HOME: BACK TO MENU", RGB(130, 140, 170), 1);
    draw_text_box(16, 192, 208, &FONT_S, "PRESS PLAY TO START", RGB(255, 215, 70), 1);
}

static int tetris_tick(uint32_t down_btns, uint32_t down_keys, int32_t enc_turn)
{
    uint32_t drop_ms, dec;
    if (down_btns & (1u << BTN_HOME)) { bgm_set(0u); fm1_led_key(BTN_PLAY, 0); return 1; }

    if (down_btns & (1u << BTN_FX)) {
        tet.music_on ^= 1u;
        if (tet.st == TET_PLAY) { bgm_set(tet.music_on); tet_draw_side(); }
    }

    if (tet.st == TET_TITLE || tet.st == TET_OVER) {
        if ((down_btns & (1u << BTN_PLAY)) || down_keys) tet_start();
        return 0;
    }

    if (down_btns & (1u << BTN_PLAY)) {
        if (tet.st == TET_PLAY) {
            tet.st = TET_PAUSE;
            bgm_set(0u);
            fm1_led_key(BTN_PLAY, 0);
            draw_text_box(20, 106, 100, &FONT_L, "PAUSE", RGB(255, 215, 70), 1);
        } else {
            tet.st = TET_PLAY;
            bgm_set(tet.music_on);
            fm1_led_key(BTN_PLAY, 1);
            tet_draw_board();
            tet.last_drop_ms = fm1_ms;
        }
    }
    if (tet.st != TET_PLAY) return 0;

    int8_t moved = 0;
    if ((down_keys & MASK_KEY_C) || (down_btns & (1u << BTN_OCT_DOWN))) {
        if (tet_fits(tet.piece, tet.rot, (int8_t)(tet.px - 1), tet.py)) { tet.px--; moved = 1; }
    }
    if ((down_keys & MASK_KEY_E) || (down_btns & (1u << BTN_OCT_UP))) {
        if (tet_fits(tet.piece, tet.rot, (int8_t)(tet.px + 1), tet.py)) { tet.px++; moved = 1; }
    }
    if ((down_keys & MASK_KEY_UP) || (down_btns & (1u << BTN_ENV)) || enc_turn != 0) {
        uint8_t nr = (uint8_t)((tet.rot + (enc_turn < 0 ? 3u : 1u)) & 3u);
        if (tet_fits(tet.piece, nr, tet.px, tet.py)) { tet.rot = nr; moved = 1; sfx_turn(); }
        else if (tet_fits(tet.piece, nr, (int8_t)(tet.px - 1), tet.py)) { tet.px--; tet.rot = nr; moved = 1; sfx_turn(); }
        else if (tet_fits(tet.piece, nr, (int8_t)(tet.px + 1), tet.py)) { tet.px++; tet.rot = nr; moved = 1; sfx_turn(); }
    }
    if ((down_keys & MASK_KEY_D) || (down_btns & (1u << BTN_SEL))) {
        if (tet_fits(tet.piece, tet.rot, tet.px, (int8_t)(tet.py + 1))) { tet.py++; moved = 1; tet.last_drop_ms = fm1_ms; }
        else { tet_lock(); return 0; }
    }
    if ((down_keys & MASK_KEY_F) || (down_btns & (1u << BTN_REC))) {
        while (tet_fits(tet.piece, tet.rot, tet.px, (int8_t)(tet.py + 1))) tet.py++;
        tet_lock();
        return 0;
    }

    if (moved) tet_draw_board();

    dec = (uint32_t)tet.lines * 14u;
    if (dec > 360u) dec = 360u;
    drop_ms = 500u - dec;
    if ((uint32_t)(fm1_ms - tet.last_drop_ms) >= drop_ms) {
        tet.last_drop_ms = fm1_ms;
        if (tet_fits(tet.piece, tet.rot, tet.px, (int8_t)(tet.py + 1))) {
            tet.py++;
            tet_draw_board();
        } else {
            tet_lock();
        }
    }
    return 0;
}
