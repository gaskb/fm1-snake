/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
#include "arcade_common.h"
#include "arcade_audio.h"

#define SNK_W 19u
#define SNK_H 17u
#define SNK_CELL 12u
#define SNK_BX 6
#define SNK_BY 28
#define SNK_MAX (SNK_W * SNK_H)

#define SNK_BG     RGB(10, 14, 12)
#define SNK_BORDER RGB(50, 140, 70)
#define SNK_HEAD   RGB(110, 255, 120)
#define SNK_BODY   RGB(30, 185, 65)
#define SNK_FOOD   RGB(255, 65, 65)
#define SNK_LEAF   RGB(100, 230, 90)
#define SNK_HUD    RGB(180, 255, 190)
#define SNK_GOLD   RGB(255, 215, 70)
#define SNK_RED    RGB(255, 90, 90)
#define SNK_DIM    RGB(130, 150, 135)

enum { SNK_RIGHT = 0, SNK_DOWN = 1, SNK_LEFT = 2, SNK_UP = 3 };
enum { SNK_TITLE = 0, SNK_PLAY = 1, SNK_PAUSE = 2, SNK_OVER = 3 };

static struct {
    uint8_t x[SNK_MAX], y[SNK_MAX];
    uint16_t len, score;
    uint8_t dir, q[2], qn;
    uint8_t fx, fy, st, new_hi;
    uint32_t last_ms;
} snk;

static void snk_draw_hud(void)
{
    char s1[12] = "SCORE 000", s2[10] = "HI 000";
    fmt_num(s1 + 6, snk.score, 3u);
    fmt_num(s2 + 3, save_data.hi_snake, 3u);
    lcd_fill(0, 0, 240, 24, C_BLACK);
    draw_text_box(6, 4, 120, &FONT_S, s1, SNK_HUD, 0);
    draw_text_box(126, 4, 108, &FONT_S, s2, SNK_GOLD, 2);
}

static void snk_cell(uint8_t gx, uint8_t gy, uint16_t col)
{
    lcd_fill(SNK_BX + (int32_t)gx * 12 + 1, SNK_BY + (int32_t)gy * 12 + 1, 10, 10, col);
}

static void snk_clear(uint8_t gx, uint8_t gy)
{
    lcd_fill(SNK_BX + (int32_t)gx * 12, SNK_BY + (int32_t)gy * 12, 12, 12, SNK_BG);
}

static void snk_head(uint8_t gx, uint8_t gy, uint8_t d)
{
    int32_t px = SNK_BX + (int32_t)gx * 12, py = SNK_BY + (int32_t)gy * 12;
    lcd_fill(px + 1, py + 1, 10, 10, SNK_HEAD);
    if (d == SNK_RIGHT)      { lcd_fill(px + 8, py + 3, 2, 2, C_BLACK); lcd_fill(px + 8, py + 7, 2, 2, C_BLACK); }
    else if (d == SNK_LEFT)  { lcd_fill(px + 2, py + 3, 2, 2, C_BLACK); lcd_fill(px + 2, py + 7, 2, 2, C_BLACK); }
    else if (d == SNK_UP)    { lcd_fill(px + 3, py + 2, 2, 2, C_BLACK); lcd_fill(px + 7, py + 2, 2, 2, C_BLACK); }
    else                     { lcd_fill(px + 3, py + 8, 2, 2, C_BLACK); lcd_fill(px + 7, py + 8, 2, 2, C_BLACK); }
}

static void snk_food(void)
{
    int32_t px = SNK_BX + (int32_t)snk.fx * 12, py = SNK_BY + (int32_t)snk.fy * 12;
    lcd_fill(px + 2, py + 3, 8, 7, SNK_FOOD);
    lcd_fill(px + 5, py + 1, 3, 2, SNK_LEAF);
}

static void snk_frame(void)
{
    lcd_fill(0, 24, 240, 216, C_BLACK);
    lcd_fill(SNK_BX - 2, SNK_BY - 2, (int32_t)(SNK_W * 12u) + 4, (int32_t)(SNK_H * 12u) + 4, SNK_BORDER);
    lcd_fill(SNK_BX, SNK_BY, (int32_t)(SNK_W * 12u), (int32_t)(SNK_H * 12u), SNK_BG);
}

static void snk_spawn_food(void)
{
    uint32_t tries = 0u;
    for (;;) {
        uint8_t fx = (uint8_t)(arcade_rand() % SNK_W);
        uint8_t fy = (uint8_t)(arcade_rand() % SNK_H);
        uint16_t i;
        uint8_t hit = 0u;
        for (i = 0u; i < snk.len; i++) {
            if (snk.x[i] == fx && snk.y[i] == fy) { hit = 1u; break; }
        }
        if (!hit || ++tries > 500u) { snk.fx = fx; snk.fy = fy; break; }
    }
}

static void snk_draw_all(void)
{
    uint16_t i;
    for (i = 1u; i < snk.len; i++) snk_cell(snk.x[i], snk.y[i], SNK_BODY);
    snk_head(snk.x[0], snk.y[0], snk.dir);
    snk_food();
}

static void snk_queue(uint8_t nd)
{
    uint8_t ref = snk.qn > 0u ? snk.q[snk.qn - 1u] : snk.dir;
    if (nd == ref || (((nd ^ ref) & 1u) == 0u)) return;
    if (snk.qn < 2u) snk.q[snk.qn++] = nd;
}

static void snk_start(void)
{
    snk.len = 3u;
    snk.x[0] = 9u; snk.y[0] = 8u;
    snk.x[1] = 8u; snk.y[1] = 8u;
    snk.x[2] = 7u; snk.y[2] = 8u;
    snk.dir = SNK_RIGHT; snk.qn = 0u;
    snk.score = 0u; snk.new_hi = 0u;
    snk.st = SNK_PLAY;
    snk.last_ms = fm1_ms;
    snk_spawn_food();
    snk_draw_hud();
    snk_frame();
    snk_draw_all();
    fm1_led_key(BTN_PLAY, 1);
    sfx_start();
}

static void snk_gameover(void)
{
    snk.st = SNK_OVER;
    fm1_led_key(BTN_PLAY, 0);
    sfx_gameover();
    lcd_fill(20, 74, 200, 108, C_BLACK);
    draw_text_box(24, 82, 192, &FONT_L, "GAME OVER", SNK_RED, 1);
    if (snk.new_hi) {
        draw_text_box(24, 124, 192, &FONT_S, "NEW HIGH SCORE!", SNK_GOLD, 1);
    } else {
        char buf[16] = "SCORE: 000";
        fmt_num(buf + 7, snk.score, 3u);
        draw_text_box(24, 124, 192, &FONT_S, buf, SNK_HUD, 1);
    }
    draw_text_box(24, 154, 192, &FONT_S, "PLAY: RETRY  HOME: MENU", C_WHITE, 1);
}

static void snk_step(void)
{
    int32_t nx, ny;
    uint16_t i, chk;
    uint8_t ate, ox = snk.x[snk.len - 1u], oy = snk.y[snk.len - 1u];

    if (snk.qn > 0u) {
        snk.dir = snk.q[0];
        if (snk.qn == 2u) snk.q[0] = snk.q[1];
        snk.qn--;
    }
    nx = (int32_t)snk.x[0]; ny = (int32_t)snk.y[0];
    if (snk.dir == SNK_RIGHT) nx++;
    else if (snk.dir == SNK_LEFT) nx--;
    else if (snk.dir == SNK_DOWN) ny++;
    else ny--;

    if (nx < 0 || nx >= (int32_t)SNK_W || ny < 0 || ny >= (int32_t)SNK_H) { snk_gameover(); return; }
    ate = ((uint8_t)nx == snk.fx && (uint8_t)ny == snk.fy) ? 1u : 0u;
    chk = ate ? snk.len : (snk.len - 1u);
    for (i = 0u; i < chk; i++) {
        if (snk.x[i] == (uint8_t)nx && snk.y[i] == (uint8_t)ny) { snk_gameover(); return; }
    }
    if (ate && snk.len < SNK_MAX) snk.len++;
    for (i = snk.len - 1u; i > 0u; i--) { snk.x[i] = snk.x[i - 1u]; snk.y[i] = snk.y[i - 1u]; }
    snk.x[0] = (uint8_t)nx; snk.y[0] = (uint8_t)ny;

    if (!ate) snk_clear(ox, oy);
    snk_cell(snk.x[1], snk.y[1], SNK_BODY);
    snk_head(snk.x[0], snk.y[0], snk.dir);

    if (ate) {
        snk.score++;
        if (snk.score > save_data.hi_snake) {
            save_data.hi_snake = snk.score;
            snk.new_hi = 1u;
            save_scores();
        }
        snk_draw_hud();
        snk_spawn_food();
        snk_food();
        sfx_eat();
    }
}

static void snake_enter(void)
{
    bgm_set(0u);
    snk.st = SNK_TITLE;
    snk.score = 0u;
    snk_draw_hud();
    snk_frame();
    draw_text_box(16, 54, 208, &FONT_L, "SNAKE", SNK_HEAD, 1);
    draw_text_box(16, 102, 208, &FONT_S, "C/C#-D#/D/E: DPAD", C_WHITE, 1);
    draw_text_box(16, 124, 208, &FONT_S, "KNOB1: TURN   HOME: MENU", SNK_DIM, 1);
    draw_text_box(16, 174, 208, &FONT_S, "PRESS PLAY TO START", SNK_GOLD, 1);
    fm1_led_key(BTN_PLAY, 0);
}

static int snake_tick(uint32_t down_btns, uint32_t down_keys, int32_t enc_turn)
{
    uint32_t dec, iv;
    if (down_btns & (1u << BTN_HOME)) { fm1_led_key(BTN_PLAY, 0); return 1; }

    if (snk.st == SNK_TITLE || snk.st == SNK_OVER) {
        if ((down_btns & (1u << BTN_PLAY)) || down_keys) snk_start();
        return 0;
    }
    if (down_btns & (1u << BTN_PLAY)) {
        if (snk.st == SNK_PLAY) {
            snk.st = SNK_PAUSE;
            fm1_led_key(BTN_PLAY, 0);
            draw_text_box(50, 114, 140, &FONT_L, "PAUSE", SNK_GOLD, 1);
        } else {
            snk.st = SNK_PLAY;
            fm1_led_key(BTN_PLAY, 1);
            snk_frame();
            snk_draw_all();
            snk.last_ms = fm1_ms;
        }
    }
    if (snk.st != SNK_PLAY) return 0;

    if (enc_turn > 0)      { snk_queue((uint8_t)(((snk.qn ? snk.q[snk.qn - 1u] : snk.dir) + 1u) & 3u)); sfx_turn(); }
    else if (enc_turn < 0) { snk_queue((uint8_t)(((snk.qn ? snk.q[snk.qn - 1u] : snk.dir) + 3u) & 3u)); sfx_turn(); }

    if (down_btns & (1u << BTN_OCT_DOWN)) snk_queue(SNK_LEFT);
    if (down_btns & (1u << BTN_OCT_UP))   snk_queue(SNK_RIGHT);
    if (down_btns & ((1u << BTN_ENV) | (1u << BTN_FX)))  snk_queue(SNK_UP);
    if (down_btns & ((1u << BTN_SEL) | (1u << BTN_LFO))) snk_queue(SNK_DOWN);

    if (down_keys & MASK_KEY_C)  snk_queue(SNK_LEFT);
    if (down_keys & MASK_KEY_D)  snk_queue(SNK_DOWN);
    if (down_keys & MASK_KEY_E)  snk_queue(SNK_RIGHT);
    if (down_keys & MASK_KEY_UP) snk_queue(SNK_UP);

    dec = (uint32_t)snk.score * 3u;
    if (dec > 95u) dec = 95u;
    iv = 165u - dec;
    if ((uint32_t)(fm1_ms - snk.last_ms) >= iv) {
        snk.last_ms = fm1_ms;
        snk_step();
    }
    return 0;
}
