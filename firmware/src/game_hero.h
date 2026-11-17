/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
#include "arcade_common.h"
#include "arcade_audio.h"

enum { HR_TITLE = 0, HR_PLAY = 1, HR_PAUSE = 2, HR_OVER = 3 };

#define HR_MAX_NOTES 24u
#define HR_HIT_Y     192
#define HR_FALL_SPEED 3   /* px per 25ms frame (~120 px/sec -> ~1.5s fall) */

/* Map chromatic pitch class (0=C .. 11=B) to white-key index (0=C .. 6=B) */
static const uint8_t CHROM_TO_WHITE[12] = {0, 0, 1, 1, 2, 3, 3, 4, 4, 5, 5, 6};
static const uint8_t IS_BLACK[12]       = {0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0};
static const char *const WHITE_LBL[7]   = {"C", "D", "E", "F", "G", "A", "B"};

static const uint16_t HR_LANE_COLS[12] = {
    RGB(255, 80, 80),   RGB(255, 130, 50),  RGB(255, 210, 50),  RGB(180, 240, 60),
    RGB(70, 235, 100),  RGB(60, 220, 200),  RGB(60, 170, 255),  RGB(90, 120, 255),
    RGB(160, 90, 255),  RGB(230, 80, 240),  RGB(255, 80, 170),  RGB(255, 120, 120)
};

static struct {
    uint8_t st, high_mode, song_idx;
    uint16_t score, combo, hits, misses;
    uint8_t judge; /* 0=none, 1=PERFECT, 2=GOOD, 3=MISS */
    uint8_t judge_t;
    uint32_t last_ms, last_spawn_ms;

    uint32_t stream_idx;
    uint32_t acc_dt_ms;

    struct {
        int16_t y;
        uint8_t lane;
        uint8_t active;
        uint8_t is_black;
    } notes[HR_MAX_NOTES];

    uint8_t lane_flash[12];
} hr;

static void hr_draw_title(void)
{
    const char *sname = (hr.song_idx == 0u) ? "MAGICAL SOUND SHOWER" :
                        (hr.song_idx == 1u) ? "PASSING BREEZE" : "SPLASH WAVE";
    const char *mname = hr.high_mode ? "HIGH (12 KEYS + #/b)" : "BASE (7 WHITE KEYS)";

    bgm_set(0u);
    fm1_led_key(BTN_PLAY, 0);
    lcd_fill(0, 0, 240, 240, C_BLACK);
    draw_text_box(12, 22, 216, &FONT_L, "SYNTH HERO", RGB(255, 90, 210), 1);
    draw_text_box(12, 64, 216, &FONT_S, "SONG (KNOB1):", RGB(140, 160, 190), 1);
    draw_text_box(12, 84, 216, &FONT_S, sname, RGB(255, 215, 70), 1);
    draw_text_box(12, 114, 216, &FONT_S, "MODE (C / E):", RGB(140, 160, 190), 1);
    draw_text_box(12, 134, 216, &FONT_S, mname, RGB(110, 255, 140), 1);
    draw_text_box(12, 166, 216, &FONT_S, "HOME: BACK TO MENU", RGB(130, 140, 160), 1);
    draw_text_box(12, 196, 216, &FONT_S, "PRESS PLAY TO ROCK!", RGB(255, 255, 255), 1);
}

static void hr_spawn_from_midi(uint32_t dt_ms)
{
    const bgm_ev_t *stream = (hr.song_idx == 0u) ? BGM_MAGICAL :
                             (hr.song_idx == 1u) ? BGM_BREEZE : BGM_SPLASH;
    uint32_t slen = (hr.song_idx == 0u) ? BGM_MAGICAL_LEN :
                    (hr.song_idx == 1u) ? BGM_BREEZE_LEN : BGM_SPLASH_LEN;

    hr.acc_dt_ms += dt_ms;
    while (hr.acc_dt_ms >= (uint32_t)stream[hr.stream_idx].dt * 10u) {
        const bgm_ev_t *ev = &stream[hr.stream_idx];
        hr.acc_dt_ms -= (uint32_t)ev->dt * 10u;
        if (ev->ch == 0u && ev->note > 0u && ev->vel > 0u) {
            /* Spawn falling note if spacing >= 160 ms so it's playable */
            if ((uint32_t)(fm1_ms - hr.last_spawn_ms) >= 180u) {
                uint32_t pc = ev->note % 12u;
                uint8_t lane = hr.high_mode ? (uint8_t)pc : CHROM_TO_WHITE[pc];
                uint32_t i;
                for (i = 0u; i < HR_MAX_NOTES; i++) {
                    if (!hr.notes[i].active) {
                        hr.notes[i].active = 1u;
                        hr.notes[i].y = 26;
                        hr.notes[i].lane = lane;
                        hr.notes[i].is_black = hr.high_mode ? IS_BLACK[pc] : 0u;
                        hr.last_spawn_ms = fm1_ms;
                        break;
                    }
                }
            }
        }
        hr.stream_idx++;
        if (hr.stream_idx >= slen) hr.stream_idx = 0u;
    }
}

static void hr_draw_frame(void)
{
    uint32_t nlanes = hr.high_mode ? 12u : 7u;
    int32_t lw = hr.high_mode ? 19 : 32;
    int32_t x0 = hr.high_mode ? 6 : 8;
    uint32_t i;
    char hud1[14] = "SCR 0000";
    char hud2[12] = "CMB x000";

    /* Top half: HUD + falling notes */
    cv_begin(240, 120, RGB(10, 12, 22));
    for (i = 0u; i <= nlanes; i++) {
        cv_rect(x0 + (int32_t)i * lw, 24, 1, 96, RGB(32, 38, 58));
    }
    for (i = 0u; i < HR_MAX_NOTES; i++) {
        if (!hr.notes[i].active || hr.notes[i].y >= 120) continue;
        int32_t nx = x0 + (int32_t)hr.notes[i].lane * lw + 2;
        int32_t ny = hr.notes[i].y;
        uint16_t col = hr.high_mode ? HR_LANE_COLS[hr.notes[i].lane] : HR_LANE_COLS[hr.notes[i].lane * 2u % 12u];
        cv_rect(nx, ny, lw - 3, 10, col);
        if (hr.notes[i].is_black) cv_rect(nx + 2, ny + 2, lw - 7, 6, C_BLACK);
    }

    fmt_num(hud1 + 4, hr.score, 4u);
    fmt_num(hud2 + 5, hr.combo, 3u);
    cv_rect(0, 0, 240, 22, C_BLACK);
    cv_text(6, 3, &FONT_S, hud1, C_WHITE);
    cv_text(154, 3, &FONT_S, hud2, RGB(255, 215, 70));

    if (hr.judge_t > 0u) {
        const char *jt = (hr.judge == 1u) ? "PERFECT!" : (hr.judge == 2u) ? "GOOD" : "MISS";
        uint16_t jc = (hr.judge == 1u) ? RGB(110, 255, 130) : (hr.judge == 2u) ? RGB(255, 220, 70) : RGB(255, 80, 80);
        cv_text(80, 50, &FONT_S, jt, jc);
    }
    cv_blit(0, 0);

    /* Bottom half (y = 120..239): notes + hit line at y=72 (screen 192) + key labels */
    cv_begin(240, 120, RGB(10, 12, 22));
    for (i = 0u; i <= nlanes; i++) {
        cv_rect(x0 + (int32_t)i * lw, 0, 1, 76, RGB(32, 38, 58));
    }
    /* Hit line */
    cv_rect(x0, 72, (int32_t)nlanes * lw, 4, RGB(240, 240, 255));

    for (i = 0u; i < HR_MAX_NOTES; i++) {
        if (!hr.notes[i].active || hr.notes[i].y < 110) continue;
        int32_t nx = x0 + (int32_t)hr.notes[i].lane * lw + 2;
        int32_t ny = hr.notes[i].y - 120;
        uint16_t col = hr.high_mode ? HR_LANE_COLS[hr.notes[i].lane] : HR_LANE_COLS[hr.notes[i].lane * 2u % 12u];
        if (ny >= -10 && ny < 86) {
            cv_rect(nx, ny, lw - 3, 10, col);
            if (hr.notes[i].is_black) cv_rect(nx + 2, ny + 2, lw - 7, 6, C_BLACK);
        }
    }

    /* Key bar at bottom */
    for (i = 0u; i < nlanes; i++) {
        int32_t kx = x0 + (int32_t)i * lw + 1;
        uint16_t col = hr.lane_flash[i] ? RGB(255, 255, 150) :
                       (hr.high_mode && IS_BLACK[i]) ? RGB(35, 40, 55) : RGB(210, 215, 225);
        cv_rect(kx, 80, lw - 2, 36, col);
        if (!hr.high_mode) {
            cv_text(kx + 10, 90, &FONT_S, WHITE_LBL[i], C_BLACK);
        }
    }
    cv_blit(0, 120);
}

static void hr_start(void)
{
    uint32_t i;
    hr.score = 0u; hr.combo = 0u; hr.hits = 0u; hr.misses = 0u;
    hr.judge = 0u; hr.judge_t = 0u;
    hr.stream_idx = 0u; hr.acc_dt_ms = 0u;
    hr.last_ms = fm1_ms; hr.last_spawn_ms = 0u;
    hr.st = HR_PLAY;
    for (i = 0u; i < HR_MAX_NOTES; i++) hr.notes[i].active = 0u;
    for (i = 0u; i < 12u; i++) hr.lane_flash[i] = 0u;

    fm1_led_key(BTN_PLAY, 1);
    bgm_set((uint8_t)(2u + hr.song_idx));
    hr_draw_frame();
}

static void hr_hit_lane(uint8_t lane)
{
    uint32_t i;
    int32_t best_d = 999;
    int32_t best_i = -1;
    hr.lane_flash[lane] = 4u;

    for (i = 0u; i < HR_MAX_NOTES; i++) {
        int32_t d;
        if (!hr.notes[i].active || hr.notes[i].lane != lane) continue;
        d = hr.notes[i].y - HR_HIT_Y;
        if (d < 0) d = -d;
        if (d < best_d) { best_d = d; best_i = (int32_t)i; }
    }

    if (best_i >= 0 && best_d <= 30) {
        hr.notes[best_i].active = 0u;
        hr.hits++;
        hr.combo++;
        hr.judge = (best_d <= 14) ? 1u : 2u;
        hr.judge_t = 12u;
        hr.score = (uint16_t)(hr.score + (hr.judge == 1u ? 20u : 10u) + (hr.combo > 30u ? 30u : hr.combo));
        if (hr.score > save_data.hi_hero) {
            save_data.hi_hero = hr.score;
            save_scores();
        }
    } else {
        hr.combo = 0u;
        hr.judge = 3u;
        hr.judge_t = 10u;
    }
}

static void hero_enter(void)
{
    hr.st = HR_TITLE;
    hr.high_mode = 0u;
    hr.song_idx = 0u;
    hr_draw_title();
}

static int hero_tick(uint32_t down_btns, uint32_t down_keys, int32_t enc_turn)
{
    uint32_t i;
    if (down_btns & (1u << BTN_HOME)) {
        bgm_set(0u);
        fm1_led_key(BTN_PLAY, 0);
        return 1;
    }

    if (hr.st == HR_TITLE) {
        if (enc_turn != 0 || (down_keys & (MASK_KEY_UP | MASK_KEY_D))) {
            hr.song_idx = (uint8_t)((hr.song_idx + (enc_turn < 0 ? 2u : 1u)) % 3u);
            sfx_turn();
            hr_draw_title();
        } else if ((down_keys & (MASK_KEY_C | MASK_KEY_E)) || (down_btns & ((1u << BTN_OCT_DOWN) | (1u << BTN_OCT_UP)))) {
            hr.high_mode ^= 1u;
            sfx_turn();
            hr_draw_title();
        } else if (down_btns & (1u << BTN_PLAY)) {
            hr_start();
        }
        return 0;
    }

    if (down_btns & (1u << BTN_PLAY)) {
        if (hr.st == HR_PLAY) {
            hr.st = HR_PAUSE;
            bgm_set(0u);
            fm1_led_key(BTN_PLAY, 0);
            draw_text_box(50, 104, 140, &FONT_L, "PAUSE", RGB(255, 215, 70), 1);
        } else {
            hr.st = HR_PLAY;
            bgm_set((uint8_t)(2u + hr.song_idx));
            fm1_led_key(BTN_PLAY, 1);
            hr.last_ms = fm1_ms;
        }
    }
    if (hr.st != HR_PLAY) return 0;

    /* Check pressed piano keys (key 0 = F3 -> MIDI 53) */
    for (i = 0u; i < 27u; i++) {
        if (down_keys >> i & 1u) {
            uint32_t pc = (53u + i) % 12u;
            uint8_t lane = hr.high_mode ? (uint8_t)pc : CHROM_TO_WHITE[pc];
            hr_hit_lane(lane);
        }
    }

    if ((uint32_t)(fm1_ms - hr.last_ms) >= 25u) {
        uint32_t dt = fm1_ms - hr.last_ms;
        if (dt > 60u) dt = 60u;
        hr.last_ms = fm1_ms;

        hr_spawn_from_midi(dt);

        for (i = 0u; i < 12u; i++) if (hr.lane_flash[i]) hr.lane_flash[i]--;
        if (hr.judge_t) hr.judge_t--;

        for (i = 0u; i < HR_MAX_NOTES; i++) {
            if (!hr.notes[i].active) continue;
            hr.notes[i].y += HR_FALL_SPEED;
            if (hr.notes[i].y > HR_HIT_Y + 28) {
                hr.notes[i].active = 0u;
                hr.misses++;
                hr.combo = 0u;
                hr.judge = 3u;
                hr.judge_t = 10u;
            }
        }
        hr_draw_frame();
    }
    return 0;
}
