/* SPDX-License-Identifier: GPL-3.0-only */
/* FM-1 Arcade (10-in-1): Snake, Tetris, Arkanoid, OutRun, Synth Hero, Frogger, Galaga, Pac-Man, Dino Run, Flappy Bird.
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
#include "game_hero.h"
#include "game_frogger.h"
#include "game_galaga.h"
#include "game_pacman.h"
#include "game_dino.h"
#include "game_flappy.h"

static void app_idle(void)
{
    /* Keep DMA fast; polling happens in app_main */
}

#define NUM_GAMES 10u
static uint8_t current_game = GAME_MENU;
static uint8_t menu_sel = 0u;
static uint8_t menu_top = 0u;

static void draw_menu(void)
{
    static const char *const NAMES[NUM_GAMES] = {
        "1. SNAKE", "2. TETRIS", "3. ARKANOID", "4. OUTRUN",
        "5. SYNTH HERO", "6. FROGGER", "7. GALAGA", "8. PAC-MAN",
        "9. DINO RUN", "10. FLAPPY"
    };
    static const uint16_t COLS[NUM_GAMES] = {
        RGB(110, 255, 120),
        RGB(60, 230, 255),
        RGB(255, 170, 70),
        RGB(255, 70, 90),
        RGB(255, 90, 210),
        RGB(80, 255, 100),
        RGB(100, 220, 255),
        RGB(255, 230, 40),
        RGB(160, 240, 160),
        RGB(255, 220, 50)
    };
    uint32_t row;
    uint16_t his[NUM_GAMES];
    his[0] = save_data.hi_snake;
    his[1] = save_data.hi_tetris;
    his[2] = save_data.hi_arkanoid;
    his[3] = save_data.hi_outrun;
    his[4] = save_data.hi_hero;
    his[5] = save_data.hi_frogger;
    his[6] = save_data.hi_galaga;
    his[7] = save_data.hi_pacman;
    his[8] = save_data.hi_dino;
    his[9] = save_data.hi_flappy;

    if (menu_sel < menu_top) menu_top = menu_sel;
    if (menu_sel >= menu_top + 4u) menu_top = (uint8_t)(menu_sel - 3u);

    bgm_set(0u);
    fm1_led_key(BTN_PLAY, 0);
    lcd_fill(0, 0, 240, 240, C_BLACK);
    draw_text_box(12, 10, 216, &FONT_L, "ARCADE", RGB(255, 215, 70), 1);
    draw_text_box(12, 44, 216, &FONT_S, "FM-1 10-IN-1  v" APP_VER, RGB(130, 145, 160), 1);

    for (row = 0u; row < 4u; row++) {
        uint32_t idx = menu_top + row;
        int32_t y = 66 + (int32_t)row * 36;
        uint16_t bg = (idx == menu_sel) ? RGB(28, 38, 56) : RGB(12, 15, 22);
        uint16_t border = (idx == menu_sel) ? COLS[idx] : RGB(45, 55, 70);
        char hi[10] = "HI 0000";
        fmt_num(hi + 3, his[idx], 4u);
        lcd_fill(14, y, 204, 31, border);
        lcd_fill(16, y + 2, 200, 27, bg);
        draw_text_box(22, y + 7, 122, &FONT_S, NAMES[idx], COLS[idx], 0);
        draw_text_box(144, y + 7, 68, &FONT_S, hi, RGB(220, 220, 180), 2);
    }

    /* Simple scroll indicator on right edge */
    lcd_fill(224, 66, 4, 139, RGB(25, 30, 42));
    lcd_fill(224, 66 + (int32_t)menu_top * 16, 4, 42, RGB(120, 145, 190));

    draw_text_box(12, 216, 216, &FONT_S, "KNOB1/C#-D: PICK  PLAY: GO", RGB(150, 165, 185), 1);
}

static void launch_game(uint8_t sel)
{
    if (sel == 0u)      { current_game = GAME_SNAKE;    snake_enter(); }
    else if (sel == 1u) { current_game = GAME_TETRIS;   tetris_enter(); }
    else if (sel == 2u) { current_game = GAME_ARKANOID; arkanoid_enter(); }
    else if (sel == 3u) { current_game = GAME_OUTRUN;   outrun_enter(); }
    else if (sel == 4u) { current_game = GAME_HERO;     hero_enter(); }
    else if (sel == 5u) { current_game = GAME_FROGGER;  frogger_enter(); }
    else if (sel == 6u) { current_game = GAME_GALAGA;   galaga_enter(); }
    else if (sel == 7u) { current_game = GAME_PACMAN;   pacman_enter(); }
    else if (sel == 8u) { current_game = GAME_DINO;     dino_enter(); }
    else                { current_game = GAME_FLAPPY;   flappy_enter(); }
}

static void app_main(void)
{
    uint32_t prev_keys = 0u;

    if (settings_load(&save_data, sizeof save_data) ||
        save_data.hi_snake > 999u || save_data.hi_tetris > 9999u ||
        save_data.hi_arkanoid > 9999u || save_data.hi_outrun > 9999u ||
        save_data.hi_hero > 9999u || save_data.hi_frogger > 9999u ||
        save_data.hi_galaga > 9999u || save_data.hi_pacman > 9999u ||
        save_data.hi_dino > 9999u || save_data.hi_flappy > 9999u) {
        save_data.hi_snake = 0u; save_data.hi_tetris = 0u;
        save_data.hi_arkanoid = 0u; save_data.hi_outrun = 0u;
        save_data.hi_hero = 0u; save_data.hi_frogger = 0u;
        save_data.hi_galaga = 0u; save_data.hi_pacman = 0u;
        save_data.hi_dino = 0u; save_data.hi_flappy = 0u;
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
                menu_sel = (uint8_t)((menu_sel + 1u) % NUM_GAMES);
                sfx_turn();
                draw_menu();
            } else if (turn < 0 || (down_keys & MASK_KEY_UP) || (down_btns & (1u << BTN_ENV))) {
                menu_sel = (uint8_t)((menu_sel + NUM_GAMES - 1u) % NUM_GAMES);
                sfx_turn();
                draw_menu();
            } else if ((down_btns & (1u << BTN_PLAY)) || (down_keys & (MASK_KEY_E | MASK_KEY_F))) {
                sfx_start();
                launch_game(menu_sel);
            }
            continue;
        }

        if (current_game == GAME_SNAKE) {
            if (snake_tick(down_btns, down_keys, turn)) { current_game = GAME_MENU; draw_menu(); }
        } else if (current_game == GAME_TETRIS) {
            if (tetris_tick(down_btns, down_keys, turn)) { current_game = GAME_MENU; draw_menu(); }
        } else if (current_game == GAME_ARKANOID) {
            if (arkanoid_tick(down_btns, down_keys, keys, fm1_in.buttons, turn)) { current_game = GAME_MENU; draw_menu(); }
        } else if (current_game == GAME_OUTRUN) {
            if (outrun_tick(down_btns, down_keys, keys, fm1_in.buttons, turn)) { current_game = GAME_MENU; draw_menu(); }
        } else if (current_game == GAME_HERO) {
            if (hero_tick(down_btns, down_keys, turn)) { current_game = GAME_MENU; draw_menu(); }
        } else if (current_game == GAME_FROGGER) {
            if (frogger_tick(down_btns, down_keys, turn)) { current_game = GAME_MENU; draw_menu(); }
        } else if (current_game == GAME_GALAGA) {
            if (galaga_tick(down_btns, down_keys, keys, fm1_in.buttons, turn)) { current_game = GAME_MENU; draw_menu(); }
        } else if (current_game == GAME_PACMAN) {
            if (pacman_tick(down_btns, down_keys, turn)) { current_game = GAME_MENU; draw_menu(); }
        } else if (current_game == GAME_DINO) {
            if (dino_tick(down_btns, down_keys, keys, fm1_in.buttons, turn)) { current_game = GAME_MENU; draw_menu(); }
        } else if (current_game == GAME_FLAPPY) {
            if (flappy_tick(down_btns, down_keys, turn)) { current_game = GAME_MENU; draw_menu(); }
        }
    }
}
