/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
#include "arcade_common.h"
#include "arcade_audio.h"

#define ARK_BX 6
#define ARK_BY 28
#define ARK_BW 228
#define ARK_BH 206

#define ARK_ROWS 5u
#define ARK_COLS 9u
#define ARK_BRK_W 24
#define ARK_BRK_H 11
#define ARK_BRK_X0 (ARK_BX + 6)
#define ARK_BRK_Y0 (ARK_BY + 14)

#define ARK_PAD_W 42
#define ARK_PAD_H 7
#define ARK_PAD_Y (ARK_BY + ARK_BH - 14)
#define ARK_BALL_SZ 6

#define ARK_BG     RGB(8, 12, 22)
#define ARK_BORDER RGB(90, 150, 230)
#define ARK_PAD_C  RGB(120, 220, 255)
#define ARK_BALL_C RGB(255, 255, 255)

enum { ARK_TITLE = 0, ARK_PLAY = 1, ARK_PAUSE = 2, ARK_OVER = 3 };

static const uint16_t ARK_ROW_COLS[ARK_ROWS] = {
    RGB(255, 70, 80),
    RGB(255, 160, 50),
    RGB(255, 225, 60),
    RGB(80, 225, 100),
    RGB(70, 170, 255),
};

static struct {
    uint8_t bricks[ARK_ROWS][ARK_COLS];
    int32_t pad_x, old_pad_x;
    int32_t bx, by, old_bx, old_by; /* Q8 fixed-point (px * 256) */
    int32_t vx, vy;
    uint16_t score;
    uint8_t lives, level, bricks_left;
    uint8_t st, stuck, new_hi;
    uint32_t last_ms;
} ark;

static void ark_draw_hud(void)
{
    char s1[12] = "SCR 0000", s2[16] = "LV1  L3  HI000";
    fmt_num(s1 + 4, ark.score, 4u);
    s2[2] = (char)('0' + (ark.level % 10u));
    s2[6] = (char)('0' + (ark.lives % 10u));
    fmt_num(s2 + 11, save_data.hi_arkanoid > 999u ? 999u : save_data.hi_arkanoid, 3u);
    lcd_fill(0, 0, 240, 24, C_BLACK);
    draw_text_box(6, 4, 96, &FONT_S, s1, RGB(140, 220, 255), 0);
    draw_text_box(102, 4, 132, &FONT_S, s2, RGB(255, 215, 70), 2);
}

static void ark_draw_brick(uint32_t r, uint32_t c, uint8_t alive)
{
    int32_t px = ARK_BRK_X0 + (int32_t)c * ARK_BRK_W;
    int32_t py = ARK_BRK_Y0 + (int32_t)r * ARK_BRK_H;
    if (alive) {
        lcd_fill(px + 1, py + 1, ARK_BRK_W - 2, ARK_BRK_H - 2, ARK_ROW_COLS[r]);
    } else {
        lcd_fill(px, py, ARK_BRK_W, ARK_BRK_H, ARK_BG);
    }
}

static void ark_draw_frame(void)
{
    uint32_t r, c;
    lcd_fill(0, 24, 240, 216, C_BLACK);
    lcd_fill(ARK_BX - 2, ARK_BY - 2, ARK_BW + 4, ARK_BH + 4, ARK_BORDER);
    lcd_fill(ARK_BX, ARK_BY, ARK_BW, ARK_BH, ARK_BG);
    for (r = 0u; r < ARK_ROWS; r++) {
        for (c = 0u; c < ARK_COLS; c++) {
            if (ark.bricks[r][c]) ark_draw_brick(r, c, 1u);
        }
    }
    lcd_fill(ark.pad_x, ARK_PAD_Y, ARK_PAD_W, ARK_PAD_H, ARK_PAD_C);
    lcd_fill(ark.bx >> 8, ark.by >> 8, ARK_BALL_SZ, ARK_BALL_SZ, ARK_BALL_C);
    ark.old_pad_x = ark.pad_x;
    ark.old_bx = ark.bx >> 8;
    ark.old_by = ark.by >> 8;
}

static void ark_reset_bricks(void)
{
    uint32_t r, c;
    ark.bricks_left = (uint8_t)(ARK_ROWS * ARK_COLS);
    for (r = 0u; r < ARK_ROWS; r++)
        for (c = 0u; c < ARK_COLS; c++)
            ark.bricks[r][c] = 1u;
}

static void ark_reset_ball(void)
{
    ark.stuck = 1u;
    ark.bx = (ark.pad_x + ARK_PAD_W / 2 - ARK_BALL_SZ / 2) << 8;
    ark.by = (ARK_PAD_Y - ARK_BALL_SZ - 1) << 8;
    ark.vx = 420 + (int32_t)(ark.level * 30u);
    ark.vy = -(520 + (int32_t)(ark.level * 35u));
}

static void ark_start(void)
{
    ark.score = 0u;
    ark.lives = 3u;
    ark.level = 1u;
    ark.new_hi = 0u;
    ark.st = ARK_PLAY;
    ark.pad_x = ARK_BX + (ARK_BW - ARK_PAD_W) / 2;
    ark_reset_bricks();
    ark_reset_ball();
    ark.last_ms = fm1_ms;
    ark_draw_hud();
    ark_draw_frame();
    fm1_led_key(BTN_PLAY, 1);
    sfx_start();
}

static void ark_gameover(void)
{
    ark.st = ARK_OVER;
    fm1_led_key(BTN_PLAY, 0);
    sfx_gameover();
    lcd_fill(20, 84, 200, 92, C_BLACK);
    draw_text_box(24, 90, 192, &FONT_L, "GAME OVER", RGB(255, 90, 90), 1);
    draw_text_box(24, 134, 192, &FONT_S, "PLAY: RETRY  HOME: MENU", C_WHITE, 1);
}

static void ark_step(uint32_t keys_held, uint32_t btns_held, int32_t enc_turn)
{
    int32_t nx, ny, b_px, b_py;
    uint32_t r, c;

    /* Move paddle: KNOB1 / SELECT (14 px per detent) + held C/E or OCT-/+ */
    if (enc_turn != 0) ark.pad_x += enc_turn * 14;
    if ((keys_held & MASK_KEY_C) || (btns_held & (1u << BTN_OCT_DOWN))) ark.pad_x -= 5;
    if ((keys_held & MASK_KEY_E) || (btns_held & (1u << BTN_OCT_UP)))   ark.pad_x += 5;

    if (ark.pad_x < ARK_BX + 1) ark.pad_x = ARK_BX + 1;
    if (ark.pad_x > ARK_BX + ARK_BW - ARK_PAD_W - 1) ark.pad_x = ARK_BX + ARK_BW - ARK_PAD_W - 1;

    if (ark.pad_x != ark.old_pad_x) {
        lcd_fill(ark.old_pad_x, ARK_PAD_Y, ARK_PAD_W, ARK_PAD_H, ARK_BG);
        lcd_fill(ark.pad_x, ARK_PAD_Y, ARK_PAD_W, ARK_PAD_H, ARK_PAD_C);
        ark.old_pad_x = ark.pad_x;
    }

    if (ark.stuck) {
        ark.bx = (ark.pad_x + ARK_PAD_W / 2 - ARK_BALL_SZ / 2) << 8;
        ark.by = (ARK_PAD_Y - ARK_BALL_SZ - 1) << 8;
    } else {
        nx = ark.bx + ark.vx;
        ny = ark.by + ark.vy;
        b_px = nx >> 8;
        b_py = ny >> 8;

        /* Wall collisions */
        if (b_px <= ARK_BX) {
            b_px = ARK_BX; nx = b_px << 8;
            if (ark.vx < 0) { ark.vx = -ark.vx; sfx_wall(); }
        } else if (b_px + ARK_BALL_SZ >= ARK_BX + ARK_BW) {
            b_px = ARK_BX + ARK_BW - ARK_BALL_SZ; nx = b_px << 8;
            if (ark.vx > 0) { ark.vx = -ark.vx; sfx_wall(); }
        }
        if (b_py <= ARK_BY) {
            b_py = ARK_BY; ny = b_py << 8;
            if (ark.vy < 0) { ark.vy = -ark.vy; sfx_wall(); }
        }

        /* Paddle collision */
        if (ark.vy > 0 &&
            b_py + ARK_BALL_SZ >= ARK_PAD_Y &&
            b_py <= ARK_PAD_Y + ARK_PAD_H &&
            b_px + ARK_BALL_SZ >= ark.pad_x &&
            b_px <= ark.pad_x + ARK_PAD_W) {
            int32_t hit_center = (b_px + ARK_BALL_SZ / 2) - (ark.pad_x + ARK_PAD_W / 2);
            ark.vx = hit_center * 32;
            if (ark.vx > -120 && ark.vx < 120) ark.vx = (hit_center >= 0) ? 140 : -140;
            ark.vy = -ark.vy;
            b_py = ARK_PAD_Y - ARK_BALL_SZ;
            ny = b_py << 8;
            sfx_paddle();
        }

        /* Brick collisions */
        for (r = 0u; r < ARK_ROWS; r++) {
            int32_t py = ARK_BRK_Y0 + (int32_t)r * ARK_BRK_H;
            if (b_py + ARK_BALL_SZ < py || b_py >= py + ARK_BRK_H) continue;
            for (c = 0u; c < ARK_COLS; c++) {
                int32_t px;
                if (!ark.bricks[r][c]) continue;
                px = ARK_BRK_X0 + (int32_t)c * ARK_BRK_W;
                if (b_px + ARK_BALL_SZ >= px && b_px < px + ARK_BRK_W) {
                    ark.bricks[r][c] = 0u;
                    ark.bricks_left--;
                    ark_draw_brick(r, c, 0u);
                    ark.vy = -ark.vy;
                    ark.score = (uint16_t)(ark.score + (ARK_ROWS - r) * 5u);
                    if (ark.score > save_data.hi_arkanoid) {
                        save_data.hi_arkanoid = ark.score;
                        ark.new_hi = 1u;
                        save_scores();
                    }
                    ark_draw_hud();
                    sfx_brick();
                    if (ark.bricks_left == 0u) {
                        ark.level++;
                        ark_reset_bricks();
                        ark_reset_ball();
                        ark_draw_hud();
                        ark_draw_frame();
                        sfx_line();
                        return;
                    }
                    goto done_bricks;
                }
            }
        }
done_bricks:

        /* Bottom miss */
        if (b_py + ARK_BALL_SZ >= ARK_BY + ARK_BH) {
            lcd_fill(ark.old_bx, ark.old_by, ARK_BALL_SZ, ARK_BALL_SZ, ARK_BG);
            if (ark.lives > 1u) {
                ark.lives--;
                ark_reset_ball();
                ark_draw_hud();
                sfx_gameover();
            } else {
                ark.lives = 0u;
                ark_draw_hud();
                ark_gameover();
                return;
            }
        } else {
            ark.bx = nx;
            ark.by = ny;
        }
    }

    b_px = ark.bx >> 8;
    b_py = ark.by >> 8;
    if (b_px != ark.old_bx || b_py != ark.old_by) {
        lcd_fill(ark.old_bx, ark.old_by, ARK_BALL_SZ, ARK_BALL_SZ, ARK_BG);
        lcd_fill(b_px, b_py, ARK_BALL_SZ, ARK_BALL_SZ, ARK_BALL_C);
        ark.old_bx = b_px;
        ark.old_by = b_py;
    }
}

static void arkanoid_enter(void)
{
    bgm_set(0u);
    ark.st = ARK_TITLE;
    ark.score = 0u; ark.level = 1u; ark.lives = 3u;
    fm1_led_key(BTN_PLAY, 0);
    lcd_fill(0, 0, 240, 240, C_BLACK);
    draw_text_box(16, 40, 208, &FONT_L, "ARKANOID", RGB(120, 220, 255), 1);
    draw_text_box(16, 92, 208, &FONT_S, "KNOB1 / SELECT: PADDLE", C_WHITE, 1);
    draw_text_box(16, 114, 208, &FONT_S, "C / E: KEYS  PLAY: LAUNCH", C_WHITE, 1);
    draw_text_box(16, 136, 208, &FONT_S, "HOME: BACK TO MENU", RGB(130, 140, 170), 1);
    draw_text_box(16, 186, 208, &FONT_S, "PRESS PLAY TO START", RGB(255, 215, 70), 1);
}

static int arkanoid_tick(uint32_t down_btns, uint32_t down_keys, uint32_t keys_held, uint32_t btns_held, int32_t enc_turn)
{
    if (down_btns & (1u << BTN_HOME)) { fm1_led_key(BTN_PLAY, 0); return 1; }

    if (ark.st == ARK_TITLE || ark.st == ARK_OVER) {
        if ((down_btns & (1u << BTN_PLAY)) || down_keys) ark_start();
        return 0;
    }

    if (down_btns & (1u << BTN_PLAY)) {
        if (ark.stuck) {
            ark.stuck = 0u;
            sfx_paddle();
        } else if (ark.st == ARK_PLAY) {
            ark.st = ARK_PAUSE;
            fm1_led_key(BTN_PLAY, 0);
            draw_text_box(50, 114, 140, &FONT_L, "PAUSE", RGB(255, 215, 70), 1);
        } else {
            ark.st = ARK_PLAY;
            fm1_led_key(BTN_PLAY, 1);
            ark_draw_frame();
            ark.last_ms = fm1_ms;
        }
    }
    if (ark.st != ARK_PLAY) return 0;

    if (ark.stuck && (down_keys & (MASK_KEY_UP | MASK_KEY_F))) {
        ark.stuck = 0u;
        sfx_paddle();
    }

    if ((uint32_t)(fm1_ms - ark.last_ms) >= 16u) {
        ark.last_ms = fm1_ms;
        ark_step(keys_held, btns_held, enc_turn);
    } else if (enc_turn != 0) {
        ark_step(keys_held, btns_held, enc_turn);
    }
    return 0;
}
