/* SPDX-License-Identifier: GPL-3.0-only */
/* FM-1 Snake: classic Snake game for the M-VAVE FM-1 (built on Dinghy).
 *
 *   Controls:
 *     BTN_PLAY                   Start / Pause / Restart
 *     KNOB1 or SELECT encoder    Turn left / right
 *     BTN_OCT- / OCT+            Left / Right
 *     BTN_ENV (or FX) / SEL      Up / Down
 *     Keys C4/D4/E4/F4           Left / Down / Up / Right (great on fm1-live keyboard)
 *
 * Rules of the road (no FPU, no libgcc): integers only, no float, no 64-bit division.
 */
#define APP_NAME "SNAKE"
static void app_idle(void);
#define LCD_IDLE app_idle
#include "platform.c"

/* ---- Sound effects (runs in audio ISR: 44.1 kHz stereo, integers only) ---- */
static const uint32_t INC_C9[12] = {
    815363807u, 863847862u, 915214929u, 969636441u, 1027294024u, 1088380105u,
    1153098554u, 1221665363u, 1294309365u, 1371273005u, 1452813141u, 1539201906u};

#define SFX_MAX 8u
static struct {
    uint8_t notes[SFX_MAX];
    uint16_t dur_frames[SFX_MAX];
    uint8_t len;
} sfx_q;

static volatile uint32_t sfx_seq = 0;
static uint32_t sfx_seen = 0;
static uint32_t sfx_idx = 0, sfx_rem = 0, sfx_ph = 0, sfx_inc = 0;
static int32_t sfx_amp = 0, sfx_gate = 0;

static uint32_t note_inc(uint32_t note)
{
    if (note > 127u) return 0u;
    return INC_C9[note % 12u] >> (10u - note / 12u);
}

static void sfx_trigger(const uint8_t *notes, uint8_t len, uint16_t ms_each)
{
    uint32_t i;
    uint16_t frames = (uint16_t)((uint32_t)ms_each * 44u);
    if (len > SFX_MAX) len = SFX_MAX;
    for (i = 0; i < len; i++) {
        sfx_q.notes[i] = notes[i];
        sfx_q.dur_frames[i] = frames;
    }
    sfx_q.len = len;
    sfx_seq++;
}

static void sfx_eat(void)
{
    static const uint8_t n[2] = {84u, 88u}; /* C6 -> E6 */
    sfx_trigger(n, 2u, 45u);
}

static void sfx_start(void)
{
    static const uint8_t n[3] = {72u, 76u, 79u}; /* C5 -> E5 -> G5 */
    sfx_trigger(n, 3u, 65u);
}

static void sfx_gameover(void)
{
    static const uint8_t n[4] = {64u, 63u, 62u, 60u}; /* E4 -> D#4 -> D4 -> C4 */
    sfx_trigger(n, 4u, 120u);
}

static void sfx_turn(void)
{
    static const uint8_t n[1] = {96u}; /* C7 tiny tick */
    sfx_trigger(n, 1u, 14u);
}

static void app_audio(int32_t *out, uint32_t n)
{
    uint32_t i;
    uint32_t seq = sfx_seq;
    if (seq != sfx_seen) {
        sfx_seen = seq;
        sfx_idx = 0u;
        if (sfx_q.len > 0u) {
            sfx_inc = note_inc(sfx_q.notes[0]);
            sfx_rem = sfx_q.dur_frames[0];
            sfx_gate = 7500;
        }
    }
    for (i = 0; i < n; i++) {
        if (sfx_rem > 0u) {
            sfx_rem--;
            if (sfx_rem == 0u) {
                sfx_idx++;
                if (sfx_idx < sfx_q.len) {
                    sfx_inc = note_inc(sfx_q.notes[sfx_idx]);
                    sfx_rem = sfx_q.dur_frames[sfx_idx];
                    sfx_gate = 7500;
                } else {
                    sfx_gate = 0;
                }
            }
        } else {
            sfx_gate = 0;
        }

        if (sfx_amp < sfx_gate) {
            sfx_amp += 64;
            if (sfx_amp > sfx_gate) sfx_amp = sfx_gate;
        } else if (sfx_amp > sfx_gate) {
            sfx_amp -= 24;
            if (sfx_amp < sfx_gate) sfx_amp = sfx_gate;
        }

        if (sfx_amp > 0) {
            int32_t sq, s;
            sfx_ph += sfx_inc;
            /* Warm pulse/square blend */
            sq = ((int32_t)sfx_ph < 0) ? 24000 : -24000;
            s = (sq * sfx_amp) >> 15;
            out[2u * i] += s;
            out[2u * i + 1u] += s;
        }
    }
}

static void app_idle(void)
{
    /* No heavy MIDI handling needed during DMA; keep watchdog/USB happy */
}

/* ---- Game constants & state ---- */
#define GRID_W 19u
#define GRID_H 17u
#define CELL   12u
#define BOARD_X 6
#define BOARD_Y 28
#define MAX_LEN (GRID_W * GRID_H)

#define C_BG     RGB(10, 14, 12)
#define C_BORDER RGB(50, 140, 70)
#define C_HEAD   RGB(110, 255, 120)
#define C_BODY   RGB(30, 185, 65)
#define C_FOOD   RGB(255, 65, 65)
#define C_LEAF   RGB(100, 230, 90)
#define C_HUD    RGB(180, 255, 190)
#define C_GOLD   RGB(255, 215, 70)
#define C_RED       RGB(255, 90, 90)
#define C_SNAKE_DIM RGB(130, 150, 135)

enum { DIR_RIGHT = 0, DIR_DOWN = 1, DIR_LEFT = 2, DIR_UP = 3 };
enum { ST_TITLE = 0, ST_PLAY = 1, ST_PAUSE = 2, ST_OVER = 3 };

static struct {
    uint16_t hi_score;
    uint16_t reserved;
} save_data = {0u, 0u};

static uint8_t snake_x[MAX_LEN];
static uint8_t snake_y[MAX_LEN];
static uint16_t snake_len;
static uint16_t score;
static uint8_t dir;
static uint8_t dir_q[2];
static uint8_t dir_q_n;
static uint8_t food_x, food_y;
static uint8_t state;
static uint8_t new_hi;
static uint32_t snake_rng = 0x12345678u;

static uint32_t snake_rand(void)
{
    uint32_t x = snake_rng ^ (fm1_ms & 0xFFu);
    if (x == 0u) x = 0xCAFEBABEu;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    snake_rng = x;
    return x;
}

static void fmt_num3(char *dst, uint16_t v)
{
    if (v > 999u) v = 999u;
    dst[0] = (char)('0' + (v / 100u) % 10u);
    dst[1] = (char)('0' + (v / 10u) % 10u);
    dst[2] = (char)('0' + v % 10u);
    dst[3] = 0;
}

static void draw_hud(void)
{
    char s1[12] = "SCORE 000";
    char s2[10] = "HI 000";
    fmt_num3(s1 + 6, score);
    fmt_num3(s2 + 3, save_data.hi_score);
    lcd_fill(0, 0, 240, 24, C_BLACK);
    draw_text_box(6, 4, 120, &FONT_S, s1, C_HUD, 0);
    draw_text_box(126, 4, 108, &FONT_S, s2, C_GOLD, 2);
}

static void draw_cell(uint8_t gx, uint8_t gy, uint16_t col)
{
    int32_t px = BOARD_X + (int32_t)gx * (int32_t)CELL;
    int32_t py = BOARD_Y + (int32_t)gy * (int32_t)CELL;
    lcd_fill(px + 1, py + 1, (int32_t)CELL - 2, (int32_t)CELL - 2, col);
}

static void clear_cell(uint8_t gx, uint8_t gy)
{
    int32_t px = BOARD_X + (int32_t)gx * (int32_t)CELL;
    int32_t py = BOARD_Y + (int32_t)gy * (int32_t)CELL;
    lcd_fill(px, py, (int32_t)CELL, (int32_t)CELL, C_BG);
}

static void draw_head(uint8_t gx, uint8_t gy, uint8_t d)
{
    int32_t px = BOARD_X + (int32_t)gx * (int32_t)CELL;
    int32_t py = BOARD_Y + (int32_t)gy * (int32_t)CELL;
    lcd_fill(px + 1, py + 1, (int32_t)CELL - 2, (int32_t)CELL - 2, C_HEAD);
    /* Tiny eyes oriented in movement direction */
    if (d == DIR_RIGHT) {
        lcd_fill(px + 8, py + 3, 2, 2, C_BLACK);
        lcd_fill(px + 8, py + 7, 2, 2, C_BLACK);
    } else if (d == DIR_LEFT) {
        lcd_fill(px + 2, py + 3, 2, 2, C_BLACK);
        lcd_fill(px + 2, py + 7, 2, 2, C_BLACK);
    } else if (d == DIR_UP) {
        lcd_fill(px + 3, py + 2, 2, 2, C_BLACK);
        lcd_fill(px + 7, py + 2, 2, 2, C_BLACK);
    } else {
        lcd_fill(px + 3, py + 8, 2, 2, C_BLACK);
        lcd_fill(px + 7, py + 8, 2, 2, C_BLACK);
    }
}

static void draw_food(uint8_t gx, uint8_t gy)
{
    int32_t px = BOARD_X + (int32_t)gx * (int32_t)CELL;
    int32_t py = BOARD_Y + (int32_t)gy * (int32_t)CELL;
    lcd_fill(px + 2, py + 3, 8, 7, C_FOOD);
    lcd_fill(px + 5, py + 1, 3, 2, C_LEAF);
}

static void draw_board_frame(void)
{
    lcd_fill(0, 24, 240, 216, C_BLACK);
    lcd_fill(BOARD_X - 2, BOARD_Y - 2, (int32_t)(GRID_W * CELL) + 4, (int32_t)(GRID_H * CELL) + 4, C_BORDER);
    lcd_fill(BOARD_X, BOARD_Y, (int32_t)(GRID_W * CELL), (int32_t)(GRID_H * CELL), C_BG);
}

static void spawn_food(void)
{
    uint32_t tries = 0u;
    for (;;) {
        uint8_t fx = (uint8_t)(snake_rand() % GRID_W);
        uint8_t fy = (uint8_t)(snake_rand() % GRID_H);
        uint16_t i;
        uint8_t hit = 0u;
        for (i = 0u; i < snake_len; i++) {
            if (snake_x[i] == fx && snake_y[i] == fy) {
                hit = 1u;
                break;
            }
        }
        if (!hit || ++tries > 500u) {
            food_x = fx;
            food_y = fy;
            break;
        }
    }
}

static void draw_full_snake(void)
{
    uint16_t i;
    for (i = 1u; i < snake_len; i++) {
        draw_cell(snake_x[i], snake_y[i], C_BODY);
    }
    draw_head(snake_x[0], snake_y[0], dir);
    draw_food(food_x, food_y);
}

static void draw_title_screen(void)
{
    draw_hud();
    draw_board_frame();
    draw_text_box(16, 56, 208, &FONT_L, "SNAKE", C_HEAD, 1);
    draw_text_box(16, 102, 208, &FONT_S, "FM-1 EDITION  v" APP_VER, C_SNAKE_DIM, 1);
    draw_text_box(16, 134, 208, &FONT_S, "KNOB1 / KEYS: MOVE", C_WHITE, 1);
    draw_text_box(16, 156, 208, &FONT_S, "OCT-/+ ENV/SEL: DPAD", C_WHITE, 1);
    draw_text_box(16, 190, 208, &FONT_S, "PRESS PLAY TO START", C_GOLD, 1);
}

static void draw_gameover_screen(void)
{
    lcd_fill(20, 74, 200, 108, C_BLACK);
    draw_text_box(24, 82, 192, &FONT_L, "GAME OVER", C_RED, 1);
    if (new_hi) {
        draw_text_box(24, 124, 192, &FONT_S, "NEW HIGH SCORE!", C_GOLD, 1);
    } else {
        char buf[16] = "SCORE: 000";
        fmt_num3(buf + 7, score);
        draw_text_box(24, 124, 192, &FONT_S, buf, C_HUD, 1);
    }
    draw_text_box(24, 154, 192, &FONT_S, "PRESS PLAY TO RETRY", C_WHITE, 1);
}

static void queue_dir(uint8_t next_d)
{
    uint8_t ref = dir_q_n > 0u ? dir_q[dir_q_n - 1u] : dir;
    if (next_d == ref) return;
    if (((next_d ^ ref) & 1u) == 0u) return; /* ignore 180-deg reversal */
    if (dir_q_n < 2u) {
        dir_q[dir_q_n++] = next_d;
    }
}

static void start_game(void)
{
    snake_len = 3u;
    snake_x[0] = 9u; snake_y[0] = 8u;
    snake_x[1] = 8u; snake_y[1] = 8u;
    snake_x[2] = 7u; snake_y[2] = 8u;
    dir = DIR_RIGHT;
    dir_q_n = 0u;
    score = 0u;
    new_hi = 0u;
    state = ST_PLAY;
    spawn_food();
    draw_hud();
    draw_board_frame();
    draw_full_snake();
    fm1_led_key(BTN_PLAY, 1);
    sfx_start();
}

static uint32_t step_interval_ms(void)
{
    uint32_t ms = 165u;
    uint32_t dec = (uint32_t)score * 3u;
    if (dec > 95u) dec = 95u;
    return ms - dec;
}

static void step_snake(void)
{
    int32_t nx, ny;
    uint16_t i;
    uint8_t ate = 0u;
    uint8_t old_tail_x = snake_x[snake_len - 1u];
    uint8_t old_tail_y = snake_y[snake_len - 1u];

    if (dir_q_n > 0u) {
        dir = dir_q[0];
        if (dir_q_n == 2u) dir_q[0] = dir_q[1];
        dir_q_n--;
    }

    nx = (int32_t)snake_x[0];
    ny = (int32_t)snake_y[0];
    if (dir == DIR_RIGHT) nx++;
    else if (dir == DIR_LEFT) nx--;
    else if (dir == DIR_DOWN) ny++;
    else ny--;

    /* Wall collision */
    if (nx < 0 || nx >= (int32_t)GRID_W || ny < 0 || ny >= (int32_t)GRID_H) {
        state = ST_OVER;
        fm1_led_key(BTN_PLAY, 0);
        sfx_gameover();
        draw_gameover_screen();
        return;
    }

    ate = ((uint8_t)nx == food_x && (uint8_t)ny == food_y) ? 1u : 0u;

    /* Self collision (tail moves away unless eating) */
    uint16_t check_len = ate ? snake_len : (snake_len - 1u);
    for (i = 0u; i < check_len; i++) {
        if (snake_x[i] == (uint8_t)nx && snake_y[i] == (uint8_t)ny) {
            state = ST_OVER;
            fm1_led_key(BTN_PLAY, 0);
            sfx_gameover();
            draw_gameover_screen();
            return;
        }
    }

    /* Advance body */
    if (ate && snake_len < MAX_LEN) {
        snake_len++;
    }
    for (i = snake_len - 1u; i > 0u; i--) {
        snake_x[i] = snake_x[i - 1u];
        snake_y[i] = snake_y[i - 1u];
    }
    snake_x[0] = (uint8_t)nx;
    snake_y[0] = (uint8_t)ny;

    /* Incremental redraw */
    if (!ate) {
        clear_cell(old_tail_x, old_tail_y);
    }
    draw_cell(snake_x[1], snake_y[1], C_BODY);
    draw_head(snake_x[0], snake_y[0], dir);

    if (ate) {
        score++;
        if (score > save_data.hi_score) {
            save_data.hi_score = score;
            new_hi = 1u;
            settings_save(&save_data, sizeof save_data);
        }
        draw_hud();
        spawn_food();
        draw_food(food_x, food_y);
        sfx_eat();
    }
}

static void app_main(void)
{
    uint32_t prev_keys = 0u;
    uint32_t last_step_ms = 0u;

    if (settings_load(&save_data, sizeof save_data) || save_data.hi_score > 999u) {
        save_data.hi_score = 0u;
    }

    state = ST_TITLE;
    draw_title_screen();

    for (;;) {
        uint32_t keys, down_keys, released_btns = 0u, down_btns;
        int32_t turn;

        platform_poll();

        down_btns = fm1_input_edges(&released_btns);
        keys = fm1_in.notes;
        down_keys = keys & ~prev_keys;
        prev_keys = keys;

        turn = fm1_enc_take(ENC_KNOB1) + fm1_enc_take(ENC_SELECT);

        if (state == ST_TITLE || state == ST_OVER) {
            if ((down_btns & (1u << BTN_PLAY)) || down_keys) {
                start_game();
                last_step_ms = fm1_ms;
            }
            continue;
        }

        if (down_btns & (1u << BTN_PLAY)) {
            if (state == ST_PLAY) {
                state = ST_PAUSE;
                fm1_led_key(BTN_PLAY, 0);
                draw_text_box(50, 114, 140, &FONT_L, "PAUSE", C_GOLD, 1);
            } else if (state == ST_PAUSE) {
                state = ST_PLAY;
                fm1_led_key(BTN_PLAY, 1);
                draw_board_frame();
                draw_full_snake();
                last_step_ms = fm1_ms;
            }
        }

        if (state != ST_PLAY) continue;

        /* Relative turning via encoder */
        if (turn > 0) {
            uint8_t ref = dir_q_n > 0u ? dir_q[dir_q_n - 1u] : dir;
            queue_dir((uint8_t)((ref + 1u) & 3u));
            sfx_turn();
        } else if (turn < 0) {
            uint8_t ref = dir_q_n > 0u ? dir_q[dir_q_n - 1u] : dir;
            queue_dir((uint8_t)((ref + 3u) & 3u));
            sfx_turn();
        }

        /* D-pad buttons */
        if (down_btns & (1u << BTN_OCT_DOWN)) queue_dir(DIR_LEFT);
        if (down_btns & (1u << BTN_OCT_UP))   queue_dir(DIR_RIGHT);
        if (down_btns & ((1u << BTN_ENV) | (1u << BTN_FX)))  queue_dir(DIR_UP);
        if (down_btns & ((1u << BTN_SEL) | (1u << BTN_LFO))) queue_dir(DIR_DOWN);

        /* Keyboard keys: F3/G3/A3/B3 (0,2,4,6) and C4/D4/E4/F4 (7,9,11,12) */
        if (down_keys & ((1u << 0) | (1u << 7)))  queue_dir(DIR_LEFT);
        if (down_keys & ((1u << 2) | (1u << 9)))  queue_dir(DIR_DOWN);
        if (down_keys & ((1u << 4) | (1u << 11))) queue_dir(DIR_UP);
        if (down_keys & ((1u << 6) | (1u << 12))) queue_dir(DIR_RIGHT);

        if ((uint32_t)(fm1_ms - last_step_ms) >= step_interval_ms()) {
            last_step_ms = fm1_ms;
            step_snake();
        }
    }
}
