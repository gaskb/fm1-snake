/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
#include "arcade_common.h"
#include "arcade_audio.h"

enum { PAC_TITLE = 0, PAC_PLAY = 1, PAC_PAUSE = 2, PAC_OVER = 3 };

#define PAC_W 19u
#define PAC_H 19u
#define PAC_CELL 11
#define PAC_BX 15
#define PAC_BY 26

/* '#' wall, '.' dot, 'o' power pellet, ' ' empty */
static const char *const PAC_MAP[PAC_H] = {
    "###################",
    "#o.......#.......o#",
    "#.##.###.#.###.##.#",
    "#.................#",
    "#.##.#.#####.#.##.#",
    "#....#...#...#....#",
    "####.###.#.###.####",
    "   #.#.......#.#   ",
    "####.#.## ##.#.####",
    ".......#   #.......",
    "####.#.#####.#.####",
    "   #.#.......#.#   ",
    "####.#.#####.#.####",
    "#........#........#",
    "#.##.###.#.###.##.#",
    "#o.#...........#.o#",
    "##.#.#.#####.#.#.##",
    "#....#...#...#....#",
    "###################",
};

static const uint16_t PAC_GHOST_COLS[4] = {
    RGB(255, 50, 50),   /* Blinky */
    RGB(255, 150, 200), /* Pinky */
    RGB(60, 230, 255),  /* Inky */
    RGB(255, 175, 50),  /* Clyde */
};

static struct {
    uint8_t maze[PAC_H][PAC_W]; /* 0=empty, 1=wall, 2=dot, 3=power */
    int8_t px, py;
    uint8_t dir, next_dir, lives, st, anim;
    uint16_t score, dots_left, fright_t;
    uint32_t last_ms;
    struct { int8_t x, y; uint8_t dir; } g[4];
} pac;

static void pac_reset_positions(void)
{
    pac.px = 9; pac.py = 15;
    pac.dir = 0u; pac.next_dir = 0u;
    pac.g[0].x = 9;  pac.g[0].y = 7;  pac.g[0].dir = 2u;
    pac.g[1].x = 8;  pac.g[1].y = 9;  pac.g[1].dir = 3u;
    pac.g[2].x = 9;  pac.g[2].y = 9;  pac.g[2].dir = 0u;
    pac.g[3].x = 10; pac.g[3].y = 9;  pac.g[3].dir = 2u;
}

static void pac_init_maze(void)
{
    uint32_t r, c;
    pac.dots_left = 0u;
    for (r = 0u; r < PAC_H; r++) {
        for (c = 0u; c < PAC_W; c++) {
            char ch = PAC_MAP[r][c];
            if (ch == '#') pac.maze[r][c] = 1u;
            else if (ch == '.') { pac.maze[r][c] = 2u; pac.dots_left++; }
            else if (ch == 'o') { pac.maze[r][c] = 3u; pac.dots_left++; }
            else pac.maze[r][c] = 0u;
        }
    }
}

static void pac_draw(void)
{
    uint32_t strip, r, c, i;
    char h1[12] = "SCR 0000", h2[12] = "L3   HI 000";
    fmt_num(h1 + 4, pac.score, 4u);
    h2[1] = (char)('0' + (pac.lives % 10u));
    fmt_num(h2 + 8, save_data.hi_pacman > 999u ? 999u : save_data.hi_pacman, 3u);

    for (strip = 0u; strip < 2u; strip++) {
        int32_t y_off = (int32_t)(strip * 120u);
        cv_begin(240, 120, C_BLACK);

        if (strip == 0u) {
            cv_text(6, 4, &FONT_S, h1, RGB(255, 230, 50));
            cv_text(132, 4, &FONT_S, h2, RGB(255, 215, 70));
        }

        for (r = 0u; r < PAC_H; r++) {
            int32_t py = PAC_BY + (int32_t)r * PAC_CELL - y_off;
            if (py + PAC_CELL <= 0 || py >= 120) continue;
            for (c = 0u; c < PAC_W; c++) {
                int32_t px = PAC_BX + (int32_t)c * PAC_CELL;
                uint8_t cell = pac.maze[r][c];
                if (cell == 1u) {
                    cv_rect(px + 1, py + 1, PAC_CELL - 2, PAC_CELL - 2, RGB(35, 65, 210));
                } else if (cell == 2u) {
                    cv_rect(px + 4, py + 4, 3, 3, RGB(255, 220, 180));
                } else if (cell == 3u && (pac.anim & 1u)) {
                    cv_rect(px + 3, py + 3, 5, 5, RGB(255, 240, 120));
                }
            }
        }

        /* Pac-Man */
        {
            int32_t mx = PAC_BX + (int32_t)pac.px * PAC_CELL;
            int32_t my = PAC_BY + (int32_t)pac.py * PAC_CELL - y_off;
            if (my + PAC_CELL > 0 && my < 120) {
                cv_rect(mx + 1, my + 1, 9, 9, RGB(255, 230, 30));
                if (pac.anim & 1u) {
                    if (pac.dir == 0u)      cv_rect(mx + 6, my + 4, 4, 3, C_BLACK);
                    else if (pac.dir == 2u) cv_rect(mx + 1, my + 4, 4, 3, C_BLACK);
                    else if (pac.dir == 3u) cv_rect(mx + 4, my + 1, 3, 4, C_BLACK);
                    else                    cv_rect(mx + 4, my + 6, 3, 4, C_BLACK);
                }
            }
        }

        /* Ghosts */
        for (i = 0u; i < 4u; i++) {
            int32_t gx = PAC_BX + (int32_t)pac.g[i].x * PAC_CELL;
            int32_t gy = PAC_BY + (int32_t)pac.g[i].y * PAC_CELL - y_off;
            if (gy + PAC_CELL > 0 && gy < 120) {
                uint16_t gc = pac.fright_t ? ((pac.fright_t < 12u && (pac.anim & 1u)) ? C_WHITE : RGB(40, 80, 255))
                                           : PAC_GHOST_COLS[i];
                cv_rect(gx + 1, gy + 1, 9, 9, gc);
                cv_rect(gx + 2, gy + 3, 2, 2, C_WHITE);
                cv_rect(gx + 7, gy + 3, 2, 2, C_WHITE);
            }
        }
        cv_blit(0, y_off);
    }
}

static int pac_can_go(int8_t x, int8_t y)
{
    if (y == 9 && (x < 0 || x >= (int8_t)PAC_W)) return 1; /* tunnel */
    if (x < 0 || x >= (int8_t)PAC_W || y < 0 || y >= (int8_t)PAC_H) return 0;
    return pac.maze[y][x] != 1u;
}

static void pac_move_ent(int8_t *x, int8_t *y, uint8_t d)
{
    if (d == 0u) (*x)++;
    else if (d == 1u) (*y)++;
    else if (d == 2u) (*x)--;
    else (*y)--;
    if (*y == 9) {
        if (*x < 0) *x = (int8_t)(PAC_W - 1u);
        else if (*x >= (int8_t)PAC_W) *x = 0;
    }
}

static void pac_die(void)
{
    sfx_crash();
    if (pac.lives > 1u) {
        pac.lives--;
        pac_reset_positions();
    } else {
        pac.lives = 0u;
        pac.st = PAC_OVER;
        fm1_led_key(BTN_PLAY, 0);
        sfx_gameover();
        pac_draw();
        lcd_fill(20, 84, 200, 82, C_BLACK);
        draw_text_box(24, 90, 192, &FONT_L, "GAME OVER", RGB(255, 90, 90), 1);
        draw_text_box(24, 134, 192, &FONT_S, "PLAY: RETRY  HOME: MENU", C_WHITE, 1);
    }
}

static void pac_start(void)
{
    pac.score = 0u; pac.lives = 3u; pac.fright_t = 0u; pac.anim = 0u;
    pac.st = PAC_PLAY;
    pac.last_ms = fm1_ms;
    pac_init_maze();
    pac_reset_positions();
    fm1_led_key(BTN_PLAY, 1);
    sfx_start();
    pac_draw();
}

static void pacman_enter(void)
{
    bgm_set(0u);
    pac.st = PAC_TITLE;
    fm1_led_key(BTN_PLAY, 0);
    lcd_fill(0, 0, 240, 240, C_BLACK);
    draw_text_box(16, 42, 208, &FONT_L, "PAC-MAN", RGB(255, 230, 40), 1);
    draw_text_box(16, 94, 208, &FONT_S, "C / C#-D# / D / E: MOVE", C_WHITE, 1);
    draw_text_box(16, 118, 208, &FONT_S, "HOME: BACK TO MENU", RGB(130, 140, 160), 1);
    draw_text_box(16, 182, 208, &FONT_S, "PRESS PLAY TO START", RGB(255, 215, 70), 1);
}

static int pacman_tick(uint32_t down_btns, uint32_t down_keys, int32_t enc_turn)
{
    if (down_btns & (1u << BTN_HOME)) { fm1_led_key(BTN_PLAY, 0); return 1; }
    if (pac.st == PAC_TITLE || pac.st == PAC_OVER) {
        if ((down_btns & (1u << BTN_PLAY)) || down_keys) pac_start();
        return 0;
    }
    if (down_btns & (1u << BTN_PLAY)) {
        if (pac.st == PAC_PLAY) {
            pac.st = PAC_PAUSE;
            fm1_led_key(BTN_PLAY, 0);
            draw_text_box(50, 104, 140, &FONT_L, "PAUSE", RGB(255, 215, 70), 1);
        } else {
            pac.st = PAC_PLAY;
            fm1_led_key(BTN_PLAY, 1);
            pac.last_ms = fm1_ms;
        }
    }
    if (pac.st != PAC_PLAY) return 0;

    if (enc_turn > 0) pac.next_dir = (uint8_t)((pac.dir + 1u) & 3u);
    else if (enc_turn < 0) pac.next_dir = (uint8_t)((pac.dir + 3u) & 3u);
    if ((down_keys & MASK_KEY_E) || (down_btns & (1u << BTN_OCT_UP)))   pac.next_dir = 0u;
    if ((down_keys & MASK_KEY_D) || (down_btns & (1u << BTN_SEL)))      pac.next_dir = 1u;
    if ((down_keys & MASK_KEY_C) || (down_btns & (1u << BTN_OCT_DOWN))) pac.next_dir = 2u;
    if ((down_keys & MASK_KEY_UP) || (down_btns & (1u << BTN_ENV)))     pac.next_dir = 3u;

    if ((uint32_t)(fm1_ms - pac.last_ms) >= 145u) {
        static const int8_t DX[4] = {1, 0, -1, 0};
        static const int8_t DY[4] = {0, 1, 0, -1};
        uint32_t i;
        pac.last_ms = fm1_ms;
        pac.anim++;
        if (pac.fright_t > 0u) pac.fright_t--;

        if (pac_can_go((int8_t)(pac.px + DX[pac.next_dir]), (int8_t)(pac.py + DY[pac.next_dir]))) {
            pac.dir = pac.next_dir;
        }
        if (pac_can_go((int8_t)(pac.px + DX[pac.dir]), (int8_t)(pac.py + DY[pac.dir]))) {
            pac_move_ent(&pac.px, &pac.py, pac.dir);
        }

        if (pac.maze[pac.py][pac.px] == 2u) {
            pac.maze[pac.py][pac.px] = 0u;
            pac.dots_left--;
            pac.score += 1u;
            sfx_blip(72u + (pac.anim & 1u) * 4u, 15u);
        } else if (pac.maze[pac.py][pac.px] == 3u) {
            pac.maze[pac.py][pac.px] = 0u;
            pac.dots_left--;
            pac.score += 10u;
            pac.fright_t = 40u;
            sfx_eat();
        }
        if (pac.score > save_data.hi_pacman) { save_data.hi_pacman = pac.score; save_scores(); }

        if (pac.dots_left == 0u) {
            pac.score += 100u;
            pac_init_maze();
            pac_reset_positions();
            sfx_line();
        }

        /* Move ghosts & check collisions */
        for (i = 0u; i < 4u; i++) {
            uint8_t d, valid[4], vn = 0u;
            uint8_t rev = (uint8_t)((pac.g[i].dir + 2u) & 3u);
            if (pac.g[i].x == pac.px && pac.g[i].y == pac.py) {
                if (pac.fright_t) {
                    pac.score += 25u;
                    pac.g[i].x = 9; pac.g[i].y = 9;
                    sfx_eat();
                } else { pac_die(); if (pac.st != PAC_PLAY) return 0; }
            }
            for (d = 0u; d < 4u; d++) {
                if (d != rev && pac_can_go((int8_t)(pac.g[i].x + DX[d]), (int8_t)(pac.g[i].y + DY[d]))) {
                    valid[vn++] = d;
                }
            }
            if (vn == 0u) valid[vn++] = rev;
            pac.g[i].dir = valid[arcade_rand() % vn];
            pac_move_ent(&pac.g[i].x, &pac.g[i].y, pac.g[i].dir);

            if (pac.g[i].x == pac.px && pac.g[i].y == pac.py) {
                if (pac.fright_t) {
                    pac.score += 25u;
                    pac.g[i].x = 9; pac.g[i].y = 9;
                    sfx_eat();
                } else { pac_die(); if (pac.st != PAC_PLAY) return 0; }
            }
        }
        pac_draw();
    }
    return 0;
}
