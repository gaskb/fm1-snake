/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
#include "arcade_common.h"
#include "arcade_audio.h"

enum { FLP_TITLE = 0, FLP_PLAY = 1, FLP_OVER = 2 };

#define FLP_NPIPES 3u
#define FLP_PIPE_W 28
#define FLP_GAP_H  62
#define FLP_BIRD_X 52

static const uint8_t FLP_PENTA[8] = {60u, 62u, 64u, 67u, 69u, 72u, 74u, 76u};

static struct {
    int32_t by, vy; /* Q8 */
    uint16_t score;
    uint8_t st, note_idx, anim;
    uint32_t last_ms;
    struct { int16_t x, gap_y; uint8_t passed; } pipe[FLP_NPIPES];
} flp;

static void flp_draw(void)
{
    uint32_t strip, i;
    char h1[12] = "SCR 0000", h2[10] = "HI 0000";
    fmt_num(h1 + 4, flp.score, 4u);
    fmt_num(h2 + 3, save_data.hi_flappy, 4u);

    for (strip = 0u; strip < 2u; strip++) {
        int32_t y_off = (int32_t)(strip * 120u);
        cv_begin(240, 120, RGB(70, 185, 235));

        /* Pipes */
        for (i = 0u; i < FLP_NPIPES; i++) {
            int32_t px = flp.pipe[i].x;
            int32_t top_h = flp.pipe[i].gap_y;
            int32_t bot_y = flp.pipe[i].gap_y + FLP_GAP_H;
            int32_t ty0 = 24 - y_off, ty1 = top_h - y_off;
            int32_t by0 = bot_y - y_off, by1 = 216 - y_off;
            if (ty1 > 0 && ty0 < 120) {
                int32_t y0 = ty0 < 0 ? 0 : ty0;
                int32_t h = (ty1 > 120 ? 120 : ty1) - y0;
                if (h > 0) cv_rect(px, y0, FLP_PIPE_W, h, RGB(50, 190, 65));
                if (ty1 >= 6 && ty1 - 6 < 120) cv_rect(px - 2, ty1 - 6, FLP_PIPE_W + 4, 6, RGB(70, 225, 85));
            }
            if (by1 > 0 && by0 < 120) {
                int32_t y0 = by0 < 0 ? 0 : by0;
                int32_t h = (by1 > 120 ? 120 : by1) - y0;
                if (h > 0) cv_rect(px, y0, FLP_PIPE_W, h, RGB(50, 190, 65));
                if (by0 >= 0 && by0 < 114) cv_rect(px - 2, by0, FLP_PIPE_W + 4, 6, RGB(70, 225, 85));
            }
        }

        /* Ground */
        if (strip == 1u) {
            cv_rect(0, 216 - y_off, 240, 4, RGB(80, 210, 75));
            cv_rect(0, 220 - y_off, 240, 20, RGB(195, 165, 95));
        }

        /* Bird */
        {
            int32_t by = (flp.by >> 8) - y_off;
            if (by + 12 > 0 && by < 120) {
                cv_rect(FLP_BIRD_X, by, 14, 11, RGB(255, 220, 45));
                cv_rect(FLP_BIRD_X + 9, by + 2, 3, 3, C_WHITE);
                cv_rect(FLP_BIRD_X + 10, by + 3, 2, 2, C_BLACK);
                cv_rect(FLP_BIRD_X + 13, by + 5, 4, 3, RGB(255, 110, 30));
                cv_rect(FLP_BIRD_X + 2, (flp.vy < 0) ? (by + 6) : (by + 3), 6, 4, RGB(255, 245, 140));
            }
        }

        if (strip == 0u) {
            cv_rect(0, 0, 240, 24, C_BLACK);
            cv_text(8, 4, &FONT_S, h1, C_WHITE);
            cv_text(160, 4, &FONT_S, h2, RGB(255, 215, 70));
        }
        cv_blit(0, y_off);
    }
}

static void flp_start(void)
{
    uint32_t i;
    flp.by = 110 << 8;
    flp.vy = -850;
    flp.score = 0u;
    flp.note_idx = 0u;
    flp.st = FLP_PLAY;
    flp.last_ms = fm1_ms;
    for (i = 0u; i < FLP_NPIPES; i++) {
        flp.pipe[i].x = (int16_t)(240 + (int32_t)i * 96);
        flp.pipe[i].gap_y = (int16_t)(48 + (arcade_rand() % 92u));
        flp.pipe[i].passed = 0u;
    }
    fm1_led_key(BTN_PLAY, 1);
    sfx_start();
    flp_draw();
}

static void flp_gameover(void)
{
    flp.st = FLP_OVER;
    fm1_led_key(BTN_PLAY, 0);
    sfx_crash();
    flp_draw();
    lcd_fill(20, 84, 200, 82, C_BLACK);
    draw_text_box(24, 90, 192, &FONT_L, "GAME OVER", RGB(255, 90, 90), 1);
    draw_text_box(24, 134, 192, &FONT_S, "PLAY: RETRY  HOME: MENU", C_WHITE, 1);
}

static void flappy_enter(void)
{
    bgm_set(0u);
    flp.st = FLP_TITLE;
    fm1_led_key(BTN_PLAY, 0);
    lcd_fill(0, 0, 240, 240, C_BLACK);
    draw_text_box(16, 40, 208, &FONT_L, "FLAPPY BIRD", RGB(255, 220, 45), 1);
    draw_text_box(16, 94, 208, &FONT_S, "ANY KEY / PLAY: FLAP!", C_WHITE, 1);
    draw_text_box(16, 118, 208, &FONT_S, "HOME: BACK TO MENU", RGB(130, 140, 160), 1);
    draw_text_box(16, 184, 208, &FONT_S, "PRESS PLAY TO FLY", RGB(120, 255, 140), 1);
}

static int flappy_tick(uint32_t down_btns, uint32_t down_keys, int32_t enc_turn)
{
    (void)enc_turn;
    if (down_btns & (1u << BTN_HOME)) { fm1_led_key(BTN_PLAY, 0); return 1; }
    if (flp.st == FLP_TITLE || flp.st == FLP_OVER) {
        if ((down_btns & (1u << BTN_PLAY)) || down_keys) flp_start();
        return 0;
    }

    if ((down_btns & ((1u << BTN_PLAY) | (1u << BTN_ENV))) || down_keys) {
        flp.vy = -1050;
        sfx_blip(FLP_PENTA[flp.note_idx & 7u], 35u);
        flp.note_idx++;
    }

    if ((uint32_t)(fm1_ms - flp.last_ms) >= 24u) {
        uint32_t i;
        int32_t by_px;
        flp.last_ms = fm1_ms;

        flp.vy += 88;
        if (flp.vy > 1600) flp.vy = 1600;
        flp.by += flp.vy;
        by_px = flp.by >> 8;

        if (by_px < 24) { by_px = 24; flp.by = 24 << 8; flp.vy = 0; }
        if (by_px + 11 >= 216) { flp_gameover(); return 0; }

        for (i = 0u; i < FLP_NPIPES; i++) {
            flp.pipe[i].x -= 3;
            if (!flp.pipe[i].passed && flp.pipe[i].x + FLP_PIPE_W < FLP_BIRD_X) {
                flp.pipe[i].passed = 1u;
                flp.score++;
                if (flp.score > save_data.hi_flappy) { save_data.hi_flappy = flp.score; save_scores(); }
                sfx_eat();
            }
            if (flp.pipe[i].x < -FLP_PIPE_W - 4) {
                int16_t max_x = 0;
                uint32_t j;
                for (j = 0u; j < FLP_NPIPES; j++) if (flp.pipe[j].x > max_x) max_x = flp.pipe[j].x;
                flp.pipe[i].x = (int16_t)(max_x + 96);
                flp.pipe[i].gap_y = (int16_t)(48 + (arcade_rand() % 92u));
                flp.pipe[i].passed = 0u;
            }
            /* Pipe collision */
            if (FLP_BIRD_X + 13 >= flp.pipe[i].x && FLP_BIRD_X + 1 <= flp.pipe[i].x + FLP_PIPE_W) {
                if (by_px + 1 < flp.pipe[i].gap_y || by_px + 10 > flp.pipe[i].gap_y + FLP_GAP_H) {
                    flp_gameover();
                    return 0;
                }
            }
        }
        flp_draw();
    }
    return 0;
}
