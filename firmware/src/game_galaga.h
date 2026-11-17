/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
#include "arcade_common.h"
#include "arcade_audio.h"

enum { GAL_TITLE = 0, GAL_PLAY = 1, GAL_PAUSE = 2, GAL_OVER = 3 };

#define GAL_ROWS 4u
#define GAL_COLS 7u
#define GAL_MAX_SHOTS 4u
#define GAL_MAX_EBUL 5u

static const uint16_t GAL_COLS_PAL[GAL_ROWS] = {
    RGB(60, 230, 120),  /* Boss */
    RGB(255, 70, 90),   /* Red butterfly */
    RGB(70, 190, 255),  /* Blue bee */
    RGB(255, 215, 60),  /* Yellow bee */
};

static struct {
    int16_t px;
    int16_t form_x;
    int8_t form_dir;
    uint16_t score;
    uint8_t lives, wave, st, alive_cnt, star_y;
    uint32_t last_ms, last_fire_ms;

    struct { uint8_t alive; int16_t dive_x, dive_y; int8_t vx; uint8_t diving; } alien[GAL_ROWS][GAL_COLS];
    struct { int16_t x, y; uint8_t active; } shot[GAL_MAX_SHOTS];
    struct { int16_t x, y; uint8_t active; } ebul[GAL_MAX_EBUL];
} gal;

static void gal_init_wave(void)
{
    uint32_t r, c;
    gal.form_x = 24;
    gal.form_dir = 1;
    gal.alive_cnt = (uint8_t)(GAL_ROWS * GAL_COLS);
    for (r = 0u; r < GAL_ROWS; r++) {
        for (c = 0u; c < GAL_COLS; c++) {
            gal.alien[r][c].alive = 1u;
            gal.alien[r][c].diving = 0u;
        }
    }
    for (r = 0u; r < GAL_MAX_SHOTS; r++) gal.shot[r].active = 0u;
    for (r = 0u; r < GAL_MAX_EBUL; r++) gal.ebul[r].active = 0u;
}

static void gal_draw(void)
{
    uint32_t strip, r, c, i;
    char h1[12] = "SCR 0000", h2[12] = "L3   HI 000";
    fmt_num(h1 + 4, gal.score, 4u);
    h2[1] = (char)('0' + (gal.lives % 10u));
    fmt_num(h2 + 8, save_data.hi_galaga > 999u ? 999u : save_data.hi_galaga, 3u);

    for (strip = 0u; strip < 2u; strip++) {
        int32_t y_off = (int32_t)(strip * 120u);
        cv_begin(240, 120, RGB(6, 8, 18));

        /* Scrolling starfield */
        for (i = 0u; i < 14u; i++) {
            int32_t sx = (int32_t)((i * 59u + 17u) % 236u);
            int32_t sy = (int32_t)((i * 37u + gal.star_y) % 240u) - y_off;
            if (sy >= 0 && sy < 120) cv_rect(sx, sy, 2, 2, (i & 1u) ? RGB(110, 120, 160) : RGB(210, 210, 255));
        }

        if (strip == 0u) {
            cv_rect(0, 0, 240, 22, C_BLACK);
            cv_text(6, 3, &FONT_S, h1, RGB(120, 220, 255));
            cv_text(132, 3, &FONT_S, h2, RGB(255, 215, 70));
        }

        /* Aliens */
        for (r = 0u; r < GAL_ROWS; r++) {
            for (c = 0u; c < GAL_COLS; c++) {
                int32_t ax, ay;
                if (!gal.alien[r][c].alive) continue;
                if (gal.alien[r][c].diving) {
                    ax = gal.alien[r][c].dive_x;
                    ay = gal.alien[r][c].dive_y - y_off;
                } else {
                    ax = gal.form_x + (int32_t)c * 26;
                    ay = 34 + (int32_t)r * 20 - y_off;
                }
                if (ay + 12 <= 0 || ay >= 120) continue;
                cv_rect(ax, ay + 2, 14, 8, GAL_COLS_PAL[r]);
                cv_rect(ax + 3, ay, 8, 12, GAL_COLS_PAL[r]);
                cv_rect(ax + 4, ay + 3, 2, 2, C_BLACK);
                cv_rect(ax + 8, ay + 3, 2, 2, C_BLACK);
            }
        }

        /* Player shots */
        for (i = 0u; i < GAL_MAX_SHOTS; i++) {
            if (!gal.shot[i].active) continue;
            int32_t sy = gal.shot[i].y - y_off;
            if (sy + 8 > 0 && sy < 120) cv_rect(gal.shot[i].x, sy, 3, 8, RGB(255, 255, 120));
        }

        /* Enemy bullets */
        for (i = 0u; i < GAL_MAX_EBUL; i++) {
            if (!gal.ebul[i].active) continue;
            int32_t ey = gal.ebul[i].y - y_off;
            if (ey + 6 > 0 && ey < 120) cv_rect(gal.ebul[i].x, ey, 3, 6, RGB(255, 90, 90));
        }

        /* Player starfighter (y = 214) */
        if (strip == 1u) {
            int32_t py = 214 - y_off;
            cv_rect(gal.px - 9, py + 8, 18, 6, RGB(220, 230, 250));
            cv_rect(gal.px - 2, py, 4, 14, RGB(255, 70, 70));
            cv_rect(gal.px - 9, py + 4, 3, 6, RGB(70, 170, 255));
            cv_rect(gal.px + 6, py + 4, 3, 6, RGB(70, 170, 255));
        }

        cv_blit(0, y_off);
    }
}

static void gal_die(void)
{
    sfx_crash();
    if (gal.lives > 1u) {
        uint32_t i;
        gal.lives--;
        gal.px = 120;
        for (i = 0u; i < GAL_MAX_EBUL; i++) gal.ebul[i].active = 0u;
    } else {
        gal.lives = 0u;
        gal.st = GAL_OVER;
        fm1_led_key(BTN_PLAY, 0);
        sfx_gameover();
        gal_draw();
        lcd_fill(20, 84, 200, 82, C_BLACK);
        draw_text_box(24, 90, 192, &FONT_L, "GAME OVER", RGB(255, 90, 90), 1);
        draw_text_box(24, 134, 192, &FONT_S, "PLAY: RETRY  HOME: MENU", C_WHITE, 1);
    }
}

static void gal_start(void)
{
    gal.px = 120;
    gal.score = 0u; gal.lives = 3u; gal.wave = 1u;
    gal.st = GAL_PLAY;
    gal.last_ms = fm1_ms; gal.last_fire_ms = 0u;
    gal_init_wave();
    fm1_led_key(BTN_PLAY, 1);
    sfx_start();
    gal_draw();
}

static void galaga_enter(void)
{
    bgm_set(0u);
    gal.st = GAL_TITLE;
    fm1_led_key(BTN_PLAY, 0);
    lcd_fill(0, 0, 240, 240, C_BLACK);
    draw_text_box(16, 40, 208, &FONT_L, "GALAGA", RGB(100, 220, 255), 1);
    draw_text_box(16, 92, 208, &FONT_S, "KNOB1 / C-E: STEER", C_WHITE, 1);
    draw_text_box(16, 114, 208, &FONT_S, "C#-D# / F: FIRE", C_WHITE, 1);
    draw_text_box(16, 136, 208, &FONT_S, "HOME: BACK TO MENU", RGB(130, 140, 160), 1);
    draw_text_box(16, 186, 208, &FONT_S, "PRESS PLAY TO START", RGB(255, 215, 70), 1);
}

static int galaga_tick(uint32_t down_btns, uint32_t down_keys, uint32_t keys_held, uint32_t btns_held, int32_t enc_turn)
{
    if (down_btns & (1u << BTN_HOME)) { fm1_led_key(BTN_PLAY, 0); return 1; }
    if (gal.st == GAL_TITLE || gal.st == GAL_OVER) {
        if ((down_btns & (1u << BTN_PLAY)) || down_keys) gal_start();
        return 0;
    }
    if (down_btns & (1u << BTN_PLAY)) {
        if (gal.st == GAL_PLAY) {
            gal.st = GAL_PAUSE;
            fm1_led_key(BTN_PLAY, 0);
            draw_text_box(50, 104, 140, &FONT_L, "PAUSE", RGB(255, 215, 70), 1);
        } else {
            gal.st = GAL_PLAY;
            fm1_led_key(BTN_PLAY, 1);
            gal.last_ms = fm1_ms;
        }
    }
    if (gal.st != GAL_PLAY) return 0;

    /* Fire */
    if (((down_keys & (MASK_KEY_UP | MASK_KEY_F)) || (down_btns & (1u << BTN_ENV))) &&
        (uint32_t)(fm1_ms - gal.last_fire_ms) >= 160u) {
        uint32_t i;
        for (i = 0u; i < GAL_MAX_SHOTS; i++) {
            if (!gal.shot[i].active) {
                gal.shot[i].active = 1u;
                gal.shot[i].x = gal.px - 1;
                gal.shot[i].y = 208;
                gal.last_fire_ms = fm1_ms;
                sfx_blip(88u, 22u);
                break;
            }
        }
    }

    if ((uint32_t)(fm1_ms - gal.last_ms) >= 30u) {
        uint32_t r, c, i;
        gal.last_ms = fm1_ms;
        gal.star_y = (uint8_t)(gal.star_y + 2u);

        if (enc_turn != 0) gal.px += (int16_t)(enc_turn * 10);
        if ((keys_held & MASK_KEY_C) || (btns_held & (1u << BTN_OCT_DOWN))) gal.px -= 6;
        if ((keys_held & MASK_KEY_E) || (btns_held & (1u << BTN_OCT_UP)))   gal.px += 6;
        if (gal.px < 14) gal.px = 14;
        if (gal.px > 226) gal.px = 226;

        /* Sway formation */
        gal.form_x += gal.form_dir * 2;
        if (gal.form_x <= 6)   { gal.form_x = 6; gal.form_dir = 1; }
        if (gal.form_x >= 46)  { gal.form_x = 46; gal.form_dir = -1; }

        /* Shots */
        for (i = 0u; i < GAL_MAX_SHOTS; i++) {
            if (!gal.shot[i].active) continue;
            gal.shot[i].y -= 9;
            if (gal.shot[i].y < 22) { gal.shot[i].active = 0u; continue; }

            for (r = 0u; r < GAL_ROWS; r++) {
                for (c = 0u; c < GAL_COLS; c++) {
                    int32_t ax, ay;
                    if (!gal.alien[r][c].alive) continue;
                    ax = gal.alien[r][c].diving ? gal.alien[r][c].dive_x : (gal.form_x + (int32_t)c * 26);
                    ay = gal.alien[r][c].diving ? gal.alien[r][c].dive_y : (34 + (int32_t)r * 20);
                    if (gal.shot[i].x + 3 >= ax && gal.shot[i].x <= ax + 14 &&
                        gal.shot[i].y <= ay + 12 && gal.shot[i].y + 8 >= ay) {
                        gal.alien[r][c].alive = 0u;
                        gal.shot[i].active = 0u;
                        gal.alive_cnt--;
                        gal.score = (uint16_t)(gal.score + (4u - r) * 10u);
                        if (gal.score > save_data.hi_galaga) { save_data.hi_galaga = gal.score; save_scores(); }
                        sfx_brick();
                        if (gal.alive_cnt == 0u) {
                            gal.wave++;
                            gal_init_wave();
                            sfx_line();
                        }
                        goto next_shot;
                    }
                }
            }
next_shot:;
        }

        /* Random alien dive & enemy fire */
        if ((arcade_rand() % 45u) == 0u) {
            uint32_t rr = arcade_rand() % GAL_ROWS, cc = arcade_rand() % GAL_COLS;
            if (gal.alien[rr][cc].alive && !gal.alien[rr][cc].diving) {
                gal.alien[rr][cc].diving = 1u;
                gal.alien[rr][cc].dive_x = gal.form_x + (int16_t)cc * 26;
                gal.alien[rr][cc].dive_y = 34 + (int16_t)rr * 20;
                gal.alien[rr][cc].vx = (gal.alien[rr][cc].dive_x < gal.px) ? 2 : -2;
            }
        }
        if ((arcade_rand() % 28u) == 0u) {
            uint32_t rr = arcade_rand() % GAL_ROWS, cc = arcade_rand() % GAL_COLS;
            if (gal.alien[rr][cc].alive) {
                for (i = 0u; i < GAL_MAX_EBUL; i++) {
                    if (!gal.ebul[i].active) {
                        gal.ebul[i].active = 1u;
                        gal.ebul[i].x = (gal.alien[rr][cc].diving ? gal.alien[rr][cc].dive_x : (gal.form_x + (int16_t)cc * 26)) + 6;
                        gal.ebul[i].y = (gal.alien[rr][cc].diving ? gal.alien[rr][cc].dive_y : (34 + (int16_t)rr * 20)) + 12;
                        break;
                    }
                }
            }
        }

        /* Update divers */
        for (r = 0u; r < GAL_ROWS; r++) {
            for (c = 0u; c < GAL_COLS; c++) {
                if (!gal.alien[r][c].alive || !gal.alien[r][c].diving) continue;
                gal.alien[r][c].dive_y += 4;
                gal.alien[r][c].dive_x += gal.alien[r][c].vx;
                if (gal.alien[r][c].dive_y >= 206 && gal.alien[r][c].dive_y <= 228) {
                    int32_t d = gal.alien[r][c].dive_x + 7 - gal.px;
                    if (d < 0) d = -d;
                    if (d < 15) {
                        gal.alien[r][c].diving = 0u;
                        gal_die();
                        if (gal.st != GAL_PLAY) return 0;
                    }
                }
                if (gal.alien[r][c].dive_y > 240) gal.alien[r][c].diving = 0u;
            }
        }

        /* Enemy bullets */
        for (i = 0u; i < GAL_MAX_EBUL; i++) {
            if (!gal.ebul[i].active) continue;
            gal.ebul[i].y += 5;
            if (gal.ebul[i].y > 240) { gal.ebul[i].active = 0u; continue; }
            if (gal.ebul[i].y >= 210 && gal.ebul[i].y <= 228) {
                int32_t d = gal.ebul[i].x - gal.px;
                if (d < 0) d = -d;
                if (d < 10) {
                    gal.ebul[i].active = 0u;
                    gal_die();
                    if (gal.st != GAL_PLAY) return 0;
                }
            }
        }

        gal_draw();
    }
    return 0;
}
