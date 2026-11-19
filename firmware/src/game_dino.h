/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
#include "arcade_common.h"
#include "arcade_audio.h"

enum { DINO_TITLE = 0, DINO_PLAY = 1, DINO_PAUSE = 2, DINO_OVER = 3 };

#define DINO_GROUND_Y 182
#define DINO_X 28
#define DINO_MAX_OBS 3u

static struct {
    int32_t y;      /* Q8 vertical offset from ground (0 = on ground, negative = up) */
    int32_t vy;     /* Q8 velocity */
    uint8_t duck;
    uint8_t st, anim, night;
    uint16_t score, speed;
    uint32_t dist_acc;
    uint32_t last_ms;
    struct {
        int16_t x;
        uint8_t type; /* 0=small cactus, 1=tall cactus, 2=double cactus, 3=ptero */
        uint8_t active;
        int16_t y;
    } obs[DINO_MAX_OBS];
    uint16_t spawn_gap;
} dino;

static void dino_draw_trex(int32_t x, int32_t y_base, uint8_t duck, uint8_t anim, uint16_t fg, uint16_t bg)
{
    if (duck) {
        /* Low ducking T-Rex (24x12) */
        cv_rect(x, y_base - 12, 22, 9, fg);
        cv_rect(x + 16, y_base - 13, 8, 7, fg);
        cv_rect(x + 19, y_base - 11, 2, 2, bg);
        if (anim & 1u) { cv_rect(x + 4, y_base - 3, 3, 3, fg); cv_rect(x + 11, y_base - 2, 3, 2, fg); }
        else           { cv_rect(x + 4, y_base - 2, 3, 2, fg); cv_rect(x + 11, y_base - 3, 3, 3, fg); }
    } else {
        /* Standing/running T-Rex (18x22) */
        cv_rect(x + 6, y_base - 22, 12, 8, fg);  /* head */
        cv_rect(x + 11, y_base - 20, 2, 2, bg);  /* eye */
        cv_rect(x + 4, y_base - 14, 10, 9, fg);  /* body */
        cv_rect(x, y_base - 12, 4, 5, fg);       /* tail */
        cv_rect(x + 14, y_base - 12, 3, 2, fg);  /* arm */
        if (anim & 1u) { cv_rect(x + 5, y_base - 5, 3, 5, fg); cv_rect(x + 10, y_base - 5, 3, 3, fg); }
        else           { cv_rect(x + 5, y_base - 5, 3, 3, fg); cv_rect(x + 10, y_base - 5, 3, 5, fg); }
    }
}

static void dino_draw(void)
{
    uint32_t strip, i;
    uint16_t bg = dino.night ? RGB(16, 18, 28) : RGB(238, 240, 232);
    uint16_t fg = dino.night ? RGB(220, 228, 240) : RGB(45, 50, 48);
    uint16_t cac = dino.night ? RGB(90, 220, 120) : RGB(35, 130, 55);
    char h1[12] = "SCR 0000", h2[10] = "HI 0000";
    fmt_num(h1 + 4, dino.score, 4u);
    fmt_num(h2 + 3, save_data.hi_dino, 4u);

    for (strip = 0u; strip < 2u; strip++) {
        int32_t y_off = (int32_t)(strip * 120u);
        cv_begin(240, 120, bg);

        if (strip == 0u) {
            cv_text(8, 6, &FONT_S, h1, fg);
            cv_text(160, 6, &FONT_S, h2, fg);
            /* Clouds */
            int32_t cx1 = 220 - (int32_t)((dino.dist_acc >> 3) % 260u);
            int32_t cx2 = 110 - (int32_t)(((dino.dist_acc >> 3) + 120u) % 260u);
            cv_rect(cx1, 48, 26, 7, dino.night ? RGB(40, 46, 70) : RGB(205, 210, 202));
            cv_rect(cx2, 72, 20, 6, dino.night ? RGB(40, 46, 70) : RGB(205, 210, 202));
        }

        if (strip == 1u) {
            int32_t gy = DINO_GROUND_Y - y_off;
            cv_rect(0, gy, 240, 2, fg);
            for (i = 0u; i < 8u; i++) {
                int32_t gx = 240 - (int32_t)(((dino.dist_acc >> 1) + i * 33u) % 248u);
                cv_rect(gx, gy + 5 + (int32_t)(i & 1u) * 3, 5, 2, fg);
            }

            /* Obstacles */
            for (i = 0u; i < DINO_MAX_OBS; i++) {
                int32_t ox, oy;
                if (!dino.obs[i].active) continue;
                ox = dino.obs[i].x;
                oy = dino.obs[i].y - y_off;
                if (dino.obs[i].type == 0u) {
                    cv_rect(ox + 3, oy - 16, 6, 16, cac);
                    cv_rect(ox, oy - 12, 3, 6, cac);
                    cv_rect(ox + 9, oy - 10, 3, 5, cac);
                } else if (dino.obs[i].type == 1u) {
                    cv_rect(ox + 4, oy - 24, 6, 24, cac);
                    cv_rect(ox, oy - 18, 4, 8, cac);
                    cv_rect(ox + 10, oy - 15, 4, 7, cac);
                } else if (dino.obs[i].type == 2u) {
                    cv_rect(ox + 2, oy - 18, 5, 18, cac);
                    cv_rect(ox + 11, oy - 14, 5, 14, cac);
                } else {
                    /* Pterodactyl */
                    int32_t wing_y = (dino.anim & 1u) ? (oy - 14) : (oy - 6);
                    cv_rect(ox, oy - 10, 18, 5, RGB(210, 75, 75));
                    cv_rect(ox + 5, wing_y, 8, 6, RGB(230, 95, 95));
                }
            }

            /* T-Rex */
            dino_draw_trex(DINO_X, DINO_GROUND_Y + (dino.y >> 8) - y_off, dino.duck, dino.anim, fg, bg);
        }

        cv_blit(0, y_off);
    }
}

static void dino_start(void)
{
    uint32_t i;
    dino.y = 0; dino.vy = 0; dino.duck = 0u;
    dino.score = 0u; dino.speed = 4u; dino.night = 0u;
    dino.dist_acc = 0u; dino.spawn_gap = 60u;
    dino.st = DINO_PLAY;
    dino.last_ms = fm1_ms;
    for (i = 0u; i < DINO_MAX_OBS; i++) dino.obs[i].active = 0u;
    fm1_led_key(BTN_PLAY, 1);
    sfx_start();
    dino_draw();
}

static void dino_enter(void)
{
    bgm_set(0u);
    dino.st = DINO_TITLE;
    fm1_led_key(BTN_PLAY, 0);
    lcd_fill(0, 0, 240, 240, C_BLACK);
    draw_text_box(16, 40, 208, &FONT_L, "DINO RUN", RGB(120, 255, 140), 1);
    draw_text_box(16, 92, 208, &FONT_S, "C#-D# / F / PLAY: JUMP", C_WHITE, 1);
    draw_text_box(16, 114, 208, &FONT_S, "D / SEL: DUCK", C_WHITE, 1);
    draw_text_box(16, 136, 208, &FONT_S, "HOME: BACK TO MENU", RGB(130, 140, 160), 1);
    draw_text_box(16, 186, 208, &FONT_S, "PRESS PLAY TO START", RGB(255, 215, 70), 1);
}

static int dino_tick(uint32_t down_btns, uint32_t down_keys, uint32_t keys_held, uint32_t btns_held, int32_t enc_turn)
{
    (void)enc_turn;
    if (down_btns & (1u << BTN_HOME)) { fm1_led_key(BTN_PLAY, 0); return 1; }
    if (dino.st == DINO_TITLE || dino.st == DINO_OVER) {
        if ((down_btns & (1u << BTN_PLAY)) || down_keys) dino_start();
        return 0;
    }
    if (down_btns & (1u << BTN_PLAY)) {
        if (dino.y == 0) {
            dino.vy = -1750;
            sfx_blip(76u, 30u);
        }
    }
    if (dino.st != DINO_PLAY) return 0;

    if ((down_keys & (MASK_KEY_UP | MASK_KEY_F | MASK_KEY_C | MASK_KEY_E)) || (down_btns & (1u << BTN_ENV))) {
        if (dino.y == 0) {
            dino.vy = -1750;
            sfx_blip(76u, 30u);
        }
    }
    dino.duck = ((keys_held & MASK_KEY_D) || (btns_held & (1u << BTN_SEL))) ? 1u : 0u;

    if ((uint32_t)(fm1_ms - dino.last_ms) >= 24u) {
        uint32_t i;
        int32_t dx_top, dx_bot, dx_r;
        dino.last_ms = fm1_ms;
        dino.anim++;

        /* Physics */
        if (dino.y < 0 || dino.vy < 0) {
            dino.y += dino.vy;
            dino.vy += dino.duck ? 220 : 125;
            if (dino.y >= 0) { dino.y = 0; dino.vy = 0; }
        }

        dino.dist_acc += dino.speed;
        {
            uint16_t ns = (uint16_t)(dino.dist_acc >> 4);
            if (ns > dino.score) {
                dino.score = ns;
                if ((dino.score % 100u) == 0u && dino.score > 0u) sfx_eat();
                if ((dino.score % 250u) == 0u && dino.score > 0u) dino.night ^= 1u;
                dino.speed = (uint16_t)(4u + (dino.score / 80u));
                if (dino.speed > 9u) dino.speed = 9u;
                if (dino.score > save_data.hi_dino) { save_data.hi_dino = dino.score; save_scores(); }
            }
        }

        /* Spawn obstacles */
        if (dino.spawn_gap > 0u) {
            dino.spawn_gap--;
        } else {
            for (i = 0u; i < DINO_MAX_OBS; i++) {
                if (!dino.obs[i].active) {
                    uint8_t tp = (dino.score >= 120u && (arcade_rand() % 4u) == 0u) ? 3u : (uint8_t)(arcade_rand() % 3u);
                    dino.obs[i].active = 1u;
                    dino.obs[i].x = 244;
                    dino.obs[i].type = tp;
                    dino.obs[i].y = (tp == 3u) ? (int16_t)(DINO_GROUND_Y - ((arcade_rand() & 1u) ? 14 : 26)) : (int16_t)DINO_GROUND_Y;
                    dino.spawn_gap = (uint16_t)(38u + (arcade_rand() % 35u));
                    break;
                }
            }
        }

        /* Move & collide */
        dx_bot = DINO_GROUND_Y + (dino.y >> 8);
        dx_top = dx_bot - (dino.duck ? 11 : 20);
        dx_r = DINO_X + (dino.duck ? 20 : 14);
        for (i = 0u; i < DINO_MAX_OBS; i++) {
            int32_t ox, oy, ow, oh;
            if (!dino.obs[i].active) continue;
            dino.obs[i].x -= (int16_t)dino.speed;
            if (dino.obs[i].x < -24) { dino.obs[i].active = 0u; continue; }

            ox = dino.obs[i].x + 2;
            oy = dino.obs[i].y;
            ow = (dino.obs[i].type == 2u) ? 14 : (dino.obs[i].type == 3u) ? 14 : 10;
            oh = (dino.obs[i].type == 1u) ? 22 : (dino.obs[i].type == 3u) ? 8 : 14;
            if (dx_r >= ox && DINO_X + 3 <= ox + ow && dx_bot >= oy - oh && dx_top <= oy) {
                dino.st = DINO_OVER;
                fm1_led_key(BTN_PLAY, 0);
                sfx_gameover();
                dino_draw();
                lcd_fill(20, 84, 200, 82, C_BLACK);
                draw_text_box(24, 90, 192, &FONT_L, "GAME OVER", RGB(255, 90, 90), 1);
                draw_text_box(24, 134, 192, &FONT_S, "PLAY: RETRY  HOME: MENU", C_WHITE, 1);
                return 0;
            }
        }

        dino_draw();
    }
    return 0;
}
