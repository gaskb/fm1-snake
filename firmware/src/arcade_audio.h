/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
#include <stdint.h>

static const uint32_t INC_C9[12] = {
    815363807u, 863847862u, 915214929u, 969636441u, 1027294024u, 1088380105u,
    1153098554u, 1221665363u, 1294309365u, 1371273005u, 1452813141u, 1539201906u};

static uint32_t note_inc(uint32_t note)
{
    if (note == 0u || note > 127u) return 0u;
    return INC_C9[note % 12u] >> (10u - note / 12u);
}

/* ---- SFX channel ---- */
#define SFX_MAX 8u
static struct {
    uint8_t notes[SFX_MAX];
    uint16_t dur_frames[SFX_MAX];
    uint8_t len;
} sfx_q;

static volatile uint32_t sfx_seq = 0;
static uint32_t sfx_seen = 0, sfx_idx = 0, sfx_rem = 0, sfx_ph = 0, sfx_inc = 0;
static int32_t sfx_amp = 0, sfx_gate = 0;

static void sfx_trigger(const uint8_t *notes, uint8_t len, uint16_t ms_each)
{
    uint32_t i;
    uint16_t frames = (uint16_t)((uint32_t)ms_each * 44u);
    if (len > SFX_MAX) len = SFX_MAX;
    for (i = 0u; i < len; i++) {
        sfx_q.notes[i] = notes[i];
        sfx_q.dur_frames[i] = frames;
    }
    sfx_q.len = len;
    sfx_seq++;
}

static void sfx_blip(uint8_t note, uint16_t ms) { sfx_trigger(&note, 1u, ms); }
static void sfx_eat(void)      { static const uint8_t n[2] = {84u, 88u}; sfx_trigger(n, 2u, 45u); }
static void sfx_start(void)    { static const uint8_t n[3] = {72u, 76u, 79u}; sfx_trigger(n, 3u, 60u); }
static void sfx_gameover(void) { static const uint8_t n[4] = {64u, 63u, 62u, 60u}; sfx_trigger(n, 4u, 120u); }
static void sfx_line(void)     { static const uint8_t n[4] = {72u, 76u, 79u, 84u}; sfx_trigger(n, 4u, 45u); }
static void sfx_turn(void)     { sfx_blip(96u, 14u); }
static void sfx_paddle(void)   { sfx_blip(72u, 25u); }
static void sfx_brick(void)    { sfx_blip(84u, 30u); }
static void sfx_wall(void)     { sfx_blip(60u, 18u); }

/* ---- Tetris Korobeiniki BGM (melody + bass) ---- */
typedef struct { uint8_t mel; uint8_t bass; uint8_t beats; } bgm_step_t;

/* 1 beat = 1 eighth-note (~190 ms at ~158 BPM) */
static const bgm_step_t TETRIS_BGM[] = {
    /* Bar 1: E5 . B4 C5 | D5 . C5 B4 */
    {76, 40, 2}, {71, 52, 1}, {72, 40, 1}, {74, 52, 2}, {72, 40, 1}, {71, 52, 1},
    /* Bar 2: A4 . A4 C5 | E5 . D5 C5 */
    {69, 45, 2}, {69, 57, 1}, {72, 45, 1}, {76, 57, 2}, {74, 45, 1}, {72, 57, 1},
    /* Bar 3: B4 . . C5 | D5 . E5 . */
    {71, 44, 3}, {72, 56, 1}, {74, 44, 2}, {76, 56, 2},
    /* Bar 4: C5 . A4 . | A4 . . . */
    {72, 45, 2}, {69, 57, 2}, {69, 45, 2}, {0,  57, 2},
    /* Bar 5: . D5 . F5 | A5 . G5 F5 */
    {0,  50, 1}, {74, 50, 2}, {77, 62, 1}, {81, 50, 2}, {79, 62, 1}, {77, 50, 1},
    /* Bar 6: E5 . . C5 | E5 . D5 C5 */
    {76, 48, 3}, {72, 60, 1}, {76, 48, 2}, {74, 60, 1}, {72, 48, 1},
    /* Bar 7: B4 . B4 C5 | D5 . E5 . */
    {71, 44, 2}, {71, 56, 1}, {72, 44, 1}, {74, 40, 2}, {76, 52, 2},
    /* Bar 8: C5 . A4 . | A4 . . . */
    {72, 45, 2}, {69, 57, 2}, {69, 45, 2}, {0,  0,  2},
};
#define TETRIS_BGM_LEN (sizeof(TETRIS_BGM) / sizeof(TETRIS_BGM[0]))
#define BGM_EIGHTH_FRAMES (8200u)

static volatile uint8_t bgm_enabled = 0;
static uint32_t bgm_idx = 0, bgm_rem = 0;
static uint32_t bgm_ph1 = 0, bgm_inc1 = 0, bgm_ph2 = 0, bgm_inc2 = 0;
static int32_t bgm_env1 = 0, bgm_env2 = 0;

static void bgm_set(uint8_t on)
{
    bgm_enabled = on;
    if (on) {
        bgm_idx = 0u;
        bgm_rem = 0u;
    }
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

    for (i = 0u; i < n; i++) {
        int32_t mix = 0;

        /* 1. SFX channel */
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
        if (sfx_amp < sfx_gate) { sfx_amp += 64; if (sfx_amp > sfx_gate) sfx_amp = sfx_gate; }
        else if (sfx_amp > sfx_gate) { sfx_amp -= 24; if (sfx_amp < sfx_gate) sfx_amp = sfx_gate; }
        if (sfx_amp > 0 && sfx_inc > 0u) {
            int32_t sq;
            sfx_ph += sfx_inc;
            sq = ((int32_t)sfx_ph < 0) ? 22000 : -22000;
            mix += (sq * sfx_amp) >> 15;
        }

        /* 2. BGM channel (Tetris Korobeiniki) */
        if (bgm_enabled) {
            if (bgm_rem == 0u) {
                const bgm_step_t *st = &TETRIS_BGM[bgm_idx];
                bgm_inc1 = note_inc(st->mel);
                bgm_inc2 = note_inc(st->bass);
                bgm_rem = (uint32_t)st->beats * BGM_EIGHTH_FRAMES;
                bgm_env1 = st->mel ? 3800 : 0;
                bgm_env2 = st->bass ? 2800 : 0;
                bgm_idx = (bgm_idx + 1u) % TETRIS_BGM_LEN;
            }
            bgm_rem--;
            if (bgm_rem < 400u) {
                bgm_env1 = (bgm_env1 * 15) >> 4;
                bgm_env2 = (bgm_env2 * 15) >> 4;
            }
            if (bgm_inc1 && bgm_env1 > 0) {
                /* 25% pulse duty for crisp NES-like lead */
                int32_t sq1 = (bgm_ph1 < 0x40000000u) ? 20000 : -20000;
                bgm_ph1 += bgm_inc1;
                mix += (sq1 * bgm_env1) >> 15;
            }
            if (bgm_inc2 && bgm_env2 > 0) {
                /* Triangle-like bass */
                int32_t t = (int32_t)(bgm_ph2 >> 16);
                int32_t tri = (t < 0 ? -t : t) - 16384;
                bgm_ph2 += bgm_inc2;
                mix += (tri * bgm_env2) >> 14;
            }
        }

        out[2u * i] += mix;
        out[2u * i + 1u] += mix;
    }
}
