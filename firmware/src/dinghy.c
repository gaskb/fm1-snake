/* SPDX-License-Identifier: GPL-3.0-only */
/* FM-1 Arcade (3-in-1): Snake, Tetris (with Korobeiniki BGM), Arkanoid.
 * Built on Dinghy (Felucca / SLOOP platform).
 *
 * Rules of the road (no FPU, no libgcc): integers only, no float, no 64-bit division.
 */
#define APP_NAME "ARCADE"
#define CV_MAX (240u * 120u)
static void app_idle(void);
#define LCD_IDLE app_idle
#include "platform.c"

#include "arcade_common.h"
#include "arcade_audio.h"
#include "game_snake.h"
#include "game_tetris.h"
#include "game_arkanoid.h"
#include "game_outrun.h"

static void app_idle(void)
{
    /* Keep DMA fast; polling happens in app_main */
}

static uint8_t current_game = GAME_MENU;
static uint8_t menu_sel = 0u; /* 0 = Snake, 1 = Tetris, 2 = Arkanoid, 3 = OutRun */

static void draw_menu(void)
{
    static const char *const NAMES[4] = {"1. SNAKE", "2. TETRIS", "3. ARKANOID", "4. OUTRUN"};
    static const uint16_t COLS[4] = {
        RGB(110, 255, 120),
        RGB(60, 230, 255),
        RGB(255, 170, 70),
        RGB(255, 70, 90)
    };
    uint32_t i;
    uint16_t his[4];
    his[0] = save_data.hi_snake;
    his[1] = save_data.hi_tetris;
    his[2] = save_data.hi_arkanoid;
    his[3] = save_data.hi_outrun;

    bgm_set(0u);
    fm1_led_key(BTN_PLAY, 0);
    lcd_fill(0, 0, 240, 240, C_BLACK);
    draw_text_box(12, 10, 216, &FONT_L, "ARCADE", RGB(255, 215, 70), 1);
    draw_text_box(12, 44, 216, &FONT_S, "FM-1 4-IN-1  v" APP_VER, RGB(130, 145, 160), 1);

    for (i = 0u; i < 4u; i++) {
        int32_t y = 66 + (int32_t)i * 36;
        uint16_t bg = (i == menu_sel) ? RGB(28, 38, 56) : RGB(12, 15, 22);
        uint16_t border = (i == menu_sel) ? COLS[i] : RGB(45, 55, 70);
        char hi[10] = "HI 0000";
        fmt_num(hi + 3, his[i], 4u);
        lcd_fill(16, y, 208, 31, border);
        lcd_fill(18, y + 2, 204, 27, bg);
        draw_text_box(26, y + 7, 120, &FONT_S, NAMES[i], COLS[i], 0);
        draw_text_box(148, y + 7, 68, &FONT_S, hi, RGB(220, 220, 180), 2);
    }

    draw_text_box(12, 216, 216, &FONT_S, "KNOB1/C#-D: PICK  PLAY: GO", RGB(150, 165, 185), 1);
}

static void launch_game(uint8_t sel)
{
    if (sel == 0u)      { current_game = GAME_SNAKE;    snake_enter(); }
    else if (sel == 1u) { current_game = GAME_TETRIS;   tetris_enter(); }
    else if (sel == 2u) { current_game = GAME_ARKANOID; arkanoid_enter(); }
    else                { current_game = GAME_OUTRUN;   outrun_enter(); }
}

static void app_main(void)
{
    uint32_t prev_keys = 0u;

    if (settings_load(&save_data, sizeof save_data) ||
        save_data.hi_snake > 999u || save_data.hi_tetris > 9999u ||
        save_data.hi_arkanoid > 9999u || save_data.hi_outrun > 9999u) {
        save_data.hi_snake = 0u;
        save_data.hi_tetris = 0u;
        save_data.hi_arkanoid = 0u;
        save_data.hi_outrun = 0u;
    }

    current_game = GAME_MENU;
    draw_menu();

    for (;;) {
        uint32_t keys, down_keys, released_btns = 0u, down_btns;
        int32_t turn;

        platform_poll();

        down_btns = fm1_input_edges(&released_btns);
        keys = fm1_in.notes;
        down_keys = keys & ~prev_keys;
        prev_keys = keys;

        turn = fm1_enc_take(ENC_KNOB1) + fm1_enc_take(ENC_SELECT);

        if (current_game == GAME_MENU) {
            if (turn > 0 || (down_keys & MASK_KEY_D) || (down_btns & (1u << BTN_SEL))) {
                menu_sel = (uint8_t)((menu_sel + 1u) % 4u);
                sfx_turn();
                draw_menu();
            } else if (turn < 0 || (down_keys & MASK_KEY_UP) || (down_btns & (1u << BTN_ENV))) {
                menu_sel = (uint8_t)((menu_sel + 3u) % 4u);
                sfx_turn();
                draw_menu();
            } else if ((down_btns & (1u << BTN_PLAY)) || (down_keys & (MASK_KEY_E | MASK_KEY_F))) {
                sfx_start();
                launch_game(menu_sel);
            }
            continue;
        }

        if (current_game == GAME_SNAKE) {
            if (snake_tick(down_btns, down_keys, turn)) {
                current_game = GAME_MENU;
                draw_menu();
            }
        } else if (current_game == GAME_TETRIS) {
            if (tetris_tick(down_btns, down_keys, turn)) {
                current_game = GAME_MENU;
                draw_menu();
            }
        } else if (current_game == GAME_ARKANOID) {
            if (arkanoid_tick(down_btns, down_keys, keys, fm1_in.buttons, turn)) {
                current_game = GAME_MENU;
                draw_menu();
            }
        } else if (current_game == GAME_OUTRUN) {
            if (outrun_tick(down_btns, down_keys, keys, fm1_in.buttons, turn)) {
                current_game = GAME_MENU;
                draw_menu();
            }
        }
    }
}
