/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
#include "arcade_common.h"
#include "arcade_audio.h"

enum { FR_TITLE = 0, FR_PLAY = 1, FR_PAUSE = 2, FR_OVER = 3 };

/* 13 rows of 16 px height (y = 24..232):
 * Row 0: 5 goal slots
 * Rows 1..5: River (logs & turtles)
 * Row 6: Safe median
 * Rows 7..11: Road (cars & trucks)
 * Row 12: Start bank */
#define FR_ROWS 13u
#define FR_COLS 15u
#define FR_CELL 16

static struct {
    int16_t fx;     /* pixel x (0..224) */
    int8_t fy;      /* row 0..12 */
    uint8_t goals[5];
    uint16_t score;
    uint8_t lives, level, st, new_hi;
    uint32_t last_ms;
    struct {
        int16_t x;
        int8_t dir; /* -1 or +1 */
        uint8_t spd;
        uint8_t len;
        uint16_t col;
    } lane[12];
} frg;

static void frg_init_lanes(void)
{
    static const int8_t DIRS[12]   = {0,  1, -1,  1, -1,  1,  0, -1,  1, -1,  1, -1};
    static const uint8_t LENS[12]  = {0, 48, 36, 56, 36, 44,  0, 28, 22, 36, 22, 26};
    static const uint16_t COLS[12] = {
        0, RGB(140,85,40), RGB(40,180,90), RGB(150,95,45), RGB(40,180,90), RGB(140,85,40),
        0, RGB(230,60,60), RGB(255,200,50), RGB(210,210,220), RGB(70,170,255), RGB(240,110,40)
    };
    uint32_t r;
    for (r = 0u; r < 12u; r++) {
        frg.lane[r].x = (int16_t)((r * 53u) % 240u);
        frg.lane[r].dir = DIRS[r];
        frg.lane[r].spd = (uint8_t)(1u + (r % 2u) + (frg.level > 2u ? 1u : 0u));
        frg.lane[r].len = LENS[r];
        frg.lane[r].col = COLS[r];
    }
}

static void frg_reset_frog(void)
{
    frg.fx = 112;
    frg.fy = 12;
}

static void frg_draw(void)
{
    uint32_t r, strip;
    char h1[12] = "SCR 0000", h2[12] = "L3   HI 000";
    fmt_num(h1 + 4, frg.score, 4u);
    h2[1] = (char)('0' + (frg.lives % 10u));
    fmt_num(h2 + 8, save_data.hi_frogger > 999u ? 999u : save_data.hi_frogger, 3u);

    for (strip = 0u; strip < 2u; strip++) {
        int32_t y_off = (int32_t)(strip * 120u);
        cv_begin(240, 120, C_BLACK);

        if (strip == 0u) {
            cv_text(6, 4, &FONT_S, h1, RGB(120, 255, 140));
            cv_text(132, 4, &FONT_S, h2, RGB(255, 215, 70));
        }

        for (r = 0u; r < FR_ROWS; r++) {
            int32_t ry = 24 + (int32_t)r * FR_CELL - y_off;
            if (ry + FR_CELL <= 0 || ry >= 120) continue;

            if (r == 0u) {
                uint32_t g;
                cv_rect(0, ry, 240, FR_CELL, RGB(20, 110, 40));
                for (g = 0u; g < 5u; g++) {
                    int32_t gx = 16 + (int32_t)g * 46;
                    cv_rect(gx, ry + 2, 24, 12, frg.goals[g] ? RGB(100, 255, 100) : RGB(15, 50, 90));
                }
            } else if (r >= 1u && r <= 5u) {
                int32_t lx = frg.lane[r].x, len = frg.lane[r].len;
                cv_rect(0, ry, 240, FR_CELL, RGB(20, 55, 140));
                cv_rect(lx, ry + 2, len, 12, frg.lane[r].col);
                cv_rect((lx + 120) % 240, ry + 2, len, 12, frg.lane[r].col);
            } else if (r == 6u || r == 12u) {
                cv_rect(0, ry, 240, FR_CELL, RGB(75, 45, 95));
            } else {
                int32_t cx = frg.lane[r].x, len = frg.lane[r].len;
                cv_rect(0, ry, 240, FR_CELL, RGB(35, 38, 45));
                cv_rect(cx, ry + 3, len, 10, frg.lane[r].col);
                cv_rect((cx + 120) % 240, ry + 3, len, 10, frg.lane[r].col);
            }
        }

        /* Draw frog */
        {
            int32_t py = 24 + (int32_t)frg.fy * FR_CELL - y_off;
            if (py + FR_CELL > 0 && py < 120) {
                cv_rect(frg.fx + 2, py + 2, 12, 12, RGB(80, 255, 80));
                cv_rect(frg.fx + 4, py + 3, 2, 2, C_BLACK);
                cv_rect(frg.fx + 10, py + 3, 2, 2, C_BLACK);
            }
        }
        cv_blit(0, y_off);
    }
}

static void frg_die(void)
{
    sfx_crash();
    if (frg.lives > 1u) {
        frg.lives--;
        frg_reset_frog();
    } else {
        frg.lives = 0u;
        frg.st = FR_OVER;
        fm1_led_key(BTN_PLAY, 0);
        sfx_gameover();
        frg_draw();
        lcd_fill(20, 84, 200, 82, C_BLACK);
        draw_text_box(24, 90, 192, &FONT_L, "GAME OVER", RGB(255, 90, 90), 1);
        draw_text_box(24, 134, 192, &FONT_S, "PLAY: RETRY  HOME: MENU", C_WHITE, 1);
    }
}

static void frg_start(void)
{
    uint32_t g;
    frg.score = 0u; frg.lives = 3u; frg.level = 1u; frg.new_hi = 0u;
    frg.st = FR_PLAY;
    frg.last_ms = fm1_ms;
    for (g = 0u; g < 5u; g++) frg.goals[g] = 0u;
    frg_init_lanes();
    frg_reset_frog();
    fm1_led_key(BTN_PLAY, 1);
    sfx_start();
    frg_draw();
}

static void frogger_enter(void)
{
    bgm_set(0u);
    frg.st = FR_TITLE;
    fm1_led_key(BTN_PLAY, 0);
    lcd_fill(0, 0, 240, 240, C_BLACK);
    draw_text_box(16, 42, 208, &FONT_L, "FROGGER", RGB(80, 255, 100), 1);
    draw_text_box(16, 94, 208, &FONT_S, "C / C#-D# / D / E: HOP", C_WHITE, 1);
    draw_text_box(16, 118, 208, &FONT_S, "HOME: BACK TO MENU", RGB(130, 140, 160), 1);
    draw_text_box(16, 182, 208, &FONT_S, "PRESS PLAY TO START", RGB(255, 215, 70), 1);
}

static int frogger_tick(uint32_t down_btns, uint32_t down_keys, int32_t enc_turn)
{
    (void)enc_turn;
    if (down_btns & (1u << BTN_HOME)) { fm1_led_key(BTN_PLAY, 0); return 1; }
    if (frg.st == FR_TITLE || frg.st == FR_OVER) {
        if ((down_btns & (1u << BTN_PLAY)) || down_keys) frg_start();
        return 0;
    }
    if (down_btns & (1u << BTN_PLAY)) {
        if (frg.st == FR_PLAY) {
            frg.st = FR_PAUSE;
            fm1_led_key(BTN_PLAY, 0);
            draw_text_box(50, 104, 140, &FONT_L, "PAUSE", RGB(255, 215, 70), 1);
        } else {
            frg.st = FR_PLAY;
            fm1_led_key(BTN_PLAY, 1);
            frg.last_ms = fm1_ms;
        }
    }
    if (frg.st != FR_PLAY) return 0;

    if ((down_keys & MASK_KEY_C) || (down_btns & (1u << BTN_OCT_DOWN))) { frg.fx -= 16; sfx_turn(); }
    if ((down_keys & MASK_KEY_E) || (down_btns & (1u << BTN_OCT_UP)))   { frg.fx += 16; sfx_turn(); }
    if ((down_keys & MASK_KEY_UP) || (down_btns & (1u << BTN_ENV)))     {
        if (frg.fy > 0) { frg.fy--; frg.score += 1u; sfx_turn(); }
    }
    if ((down_keys & MASK_KEY_D) || (down_btns & (1u << BTN_SEL))) {
        if (frg.fy < 12) { frg.fy++; sfx_turn(); }
    }
    if (frg.fx < 0) frg.fx = 0;
    if (frg.fx > 224) frg.fx = 224;

    if ((uint32_t)(fm1_ms - frg.last_ms) >= 35u) {
        uint32_t r;
        frg.last_ms = fm1_ms;
        for (r = 1u; r < 12u; r++) {
            if (!frg.lane[r].dir) continue;
            frg.lane[r].x += frg.lane[r].dir * (int16_t)frg.lane[r].spd;
            if (frg.lane[r].x < 0) frg.lane[r].x += 240;
            if (frg.lane[r].x >= 240) frg.lane[r].x -= 240;
        }

        /* Check collisions */
        if (frg.fy == 0) {
            uint32_t g = (uint32_t)(frg.fx / 46);
            if (g < 5u && !frg.goals[g]) {
                uint32_t k, done = 1u;
                frg.goals[g] = 1u;
                frg.score += 50u;
                sfx_check();
                for (k = 0u; k < 5u; k++) if (!frg.goals[k]) done = 0u;
                if (done) {
                    frg.level++;
                    frg.score += 100u;
                    for (k = 0u; k < 5u; k++) frg.goals[k] = 0u;
                    frg_init_lanes();
                    sfx_line();
                }
                frg_reset_frog();
            } else {
                frg_die();
            }
        } else if (frg.fy >= 1 && frg.fy <= 5) {
            int32_t lx1 = frg.lane[frg.fy].x, lx2 = (lx1 + 120) % 240, len = frg.lane[frg.fy].len;
            int32_t on_log = (frg.fx + 10 >= lx1 && frg.fx + 4 <= lx1 + len) ||
                             (frg.fx + 10 >= lx2 && frg.fx + 4 <= lx2 + len);
            if (!on_log) { frg_die(); }
            else {
                frg.fx += frg.lane[frg.fy].dir * (int16_t)frg.lane[frg.fy].spd;
                if (frg.fx < 0 || frg.fx > 224) frg_die();
            }
        } else if (frg.fy >= 7 && frg.fy <= 11) {
            int32_t cx1 = frg.lane[frg.fy].x, cx2 = (cx1 + 120) % 240, len = frg.lane[frg.fy].len;
            int32_t hit = (frg.fx + 12 >= cx1 && frg.fx + 2 <= cx1 + len) ||
                          (frg.fx + 12 >= cx2 && frg.fx + 2 <= cx2 + len);
            if (hit) frg_die();
        }

        if (frg.score > save_data.hi_frogger) {
            save_data.hi_frogger = frg.score;
            frg.new_hi = 1u;
            save_scores();
        }
        if (frg.st == FR_PLAY) frg_draw();
    }
    return 0;
}
