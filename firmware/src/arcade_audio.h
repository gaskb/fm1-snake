/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
#include <stdint.h>
#include "outrun_bgm.h"

static const uint32_t INC_C9[12] = {
    815363807u, 863847862u, 915214929u, 969636441u, 1027294024u, 1088380105u,
    1153098554u, 1221665363u, 1294309365u, 1371273005u, 1452813141u, 1539201906u};

static uint32_t note_inc(uint32_t note)
{
    if (note == 0u || note > 127u) return 0u;
    return INC_C9[note % 12u] >> (10u - note / 12u);
}

static int32_t fm_sine(uint32_t ph)
{
    int32_t x = (int32_t)ph >> 16, ax = x < 0 ? -x : x, y, ay;
    y = (x * (32768 - ax)) >> 13;
    ay = y < 0 ? -y : y;
    y += ((((y * ay) >> 15) - y) * 7373) >> 15;
    return y > 32767 ? 32767 : y < -32767 ? -32767 : y;
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
static void sfx_crash(void)    { static const uint8_t n[4] = {48u, 45u, 42u, 38u}; sfx_trigger(n, 4u, 70u); }
static void sfx_check(void)    { static const uint8_t n[4] = {76u, 79u, 83u, 88u}; sfx_trigger(n, 4u, 55u); }

/* ---- Tetris Korobeiniki BGM ---- */
typedef struct { uint8_t mel; uint8_t bass; uint8_t beats; } tet_step_t;

static const tet_step_t TETRIS_BGM[] = {
    {76, 40, 2}, {71, 52, 1}, {72, 40, 1}, {74, 52, 2}, {72, 40, 1}, {71, 52, 1},
    {69, 45, 2}, {69, 57, 1}, {72, 45, 1}, {76, 57, 2}, {74, 45, 1}, {72, 57, 1},
    {71, 44, 3}, {72, 56, 1}, {74, 44, 2}, {76, 56, 2},
    {72, 45, 2}, {69, 57, 2}, {69, 45, 2}, {0,  57, 2},
    {0,  50, 1}, {74, 50, 2}, {77, 62, 1}, {81, 50, 2}, {79, 62, 1}, {77, 50, 1},
    {76, 48, 3}, {72, 60, 1}, {76, 48, 2}, {74, 60, 1}, {72, 48, 1},
    {71, 44, 2}, {71, 56, 1}, {72, 44, 1}, {74, 40, 2}, {76, 52, 2},
    {72, 45, 2}, {69, 57, 2}, {69, 45, 2}, {0,  0,  2},
};
#define TETRIS_BGM_LEN (sizeof(TETRIS_BGM) / sizeof(TETRIS_BGM[0]))
#define BGM_EIGHTH_FRAMES (8200u)

/* 0=off, 1=Tetris, 2=Magical Sound Shower, 3=Passing Breeze, 4=Splash Wave */
static volatile uint8_t bgm_enabled = 0;
static volatile uint32_t bgm_gen = 0;
static uint32_t bgm_seen_gen = 0;

/* Tetris state */
static uint32_t tet_idx = 0, tet_rem = 0;
static uint32_t tet_ph1 = 0, tet_inc1 = 0, tet_ph2 = 0, tet_inc2 = 0;
static int32_t tet_env1 = 0, tet_env2 = 0;

/* 4-voice FM + Drums state for OutRun MIDI streams */
static uint32_t or_ev_idx = 0, or_wait_frames = 0;
static struct {
    uint32_t ph_c, ph_m, inc;
    int32_t amp, gate, mod_env;
} fm_v[4];

static uint32_t kick_ph = 0, kick_inc = 0, kick_rem = 0;
static int32_t kick_amp = 0;
static uint32_t snr_rem = 0, hat_rem = 0;
static int32_t snr_amp = 0, hat_amp = 0;
static uint32_t lfsr = 0xACE1u;

static void bgm_set(uint8_t song)
{
    bgm_enabled = song;
    bgm_gen++;
}

static void reset_bgm_state(void)
{
    uint32_t v;
    tet_idx = 0u; tet_rem = 0u;
    or_ev_idx = 0u; or_wait_frames = 0u;
    for (v = 0u; v < 4u; v++) {
        fm_v[v].gate = 0; fm_v[v].amp = 0; fm_v[v].mod_env = 0;
    }
    kick_rem = 0u; kick_amp = 0;
    snr_rem = 0u; snr_amp = 0;
    hat_rem = 0u; hat_amp = 0;
}

static void trigger_or_event(const bgm_ev_t *ev)
{
    uint8_t ch = ev->ch;
    if (ch < 4u) {
        if (ev->note == 0u || ev->vel == 0u) {
            fm_v[ch].gate = 0;
        } else {
            int32_t base_g = (ch == 2u) ? 4600 : (ch == 0u) ? 3600 : (ch == 1u) ? 2800 : 1900;
            fm_v[ch].inc = note_inc(ev->note);
            fm_v[ch].gate = (base_g * (int32_t)ev->vel) >> 7;
            fm_v[ch].mod_env = (ch == 2u) ? 22000 : (ch == 0u) ? 16000 : 11000;
        }
    } else if (ch == 4u) {
        if (ev->note == 1u) { /* Kick */
            kick_ph = 0u;
            kick_inc = 15000000u; /* ~154 Hz */
            kick_rem = 3200u;
            kick_amp = 7000;
        } else if (ev->note == 2u) { /* Snare */
            snr_rem = 2800u;
            snr_amp = 4800;
        } else if (ev->note == 3u) { /* Hi-hat */
            hat_rem = 900u;
            hat_amp = 2200;
        }
    }
}

static void app_audio(int32_t *out, uint32_t n)
{
    uint32_t i;
    uint32_t seq = sfx_seq;
    uint32_t gen = bgm_gen;

    if (gen != bgm_seen_gen) {
        bgm_seen_gen = gen;
        reset_bgm_state();
    }

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

        /* 2. BGM channel */
        if (bgm_enabled == 1u) {
            if (tet_rem == 0u) {
                const tet_step_t *st = &TETRIS_BGM[tet_idx];
                tet_inc1 = note_inc(st->mel);
                tet_inc2 = note_inc(st->bass);
                tet_rem = (uint32_t)st->beats * BGM_EIGHTH_FRAMES;
                tet_env1 = st->mel ? 3800 : 0;
                tet_env2 = st->bass ? 2800 : 0;
                tet_idx = (tet_idx + 1u) % TETRIS_BGM_LEN;
            }
            tet_rem--;
            if (tet_rem < 400u) {
                tet_env1 = (tet_env1 * 15) >> 4;
                tet_env2 = (tet_env2 * 15) >> 4;
            }
            if (tet_inc1 && tet_env1 > 0) {
                int32_t sq1 = (tet_ph1 < 0x40000000u) ? 20000 : -20000;
                tet_ph1 += tet_inc1;
                mix += (sq1 * tet_env1) >> 15;
            }
            if (tet_inc2 && tet_env2 > 0) {
                int32_t t = (int32_t)(tet_ph2 >> 16);
                int32_t tri = (t < 0 ? -t : t) - 16384;
                tet_ph2 += tet_inc2;
                mix += (tri * tet_env2) >> 14;
            }
        } else if (bgm_enabled >= 2u) {
            const bgm_ev_t *stream = (bgm_enabled == 2u) ? BGM_MAGICAL :
                                     (bgm_enabled == 3u) ? BGM_BREEZE : BGM_SPLASH;
            uint32_t slen = (bgm_enabled == 2u) ? BGM_MAGICAL_LEN :
                            (bgm_enabled == 3u) ? BGM_BREEZE_LEN : BGM_SPLASH_LEN;

            while (or_wait_frames == 0u) {
                const bgm_ev_t *ev = &stream[or_ev_idx];
                if (ev->dt > 0u) {
                    or_wait_frames = (uint32_t)ev->dt * 441u;
                    break;
                }
                trigger_or_event(ev);
                or_ev_idx++;
                if (or_ev_idx >= slen) or_ev_idx = 0u;
            }
            if (or_wait_frames > 0u) {
                or_wait_frames--;
                if (or_wait_frames == 0u) {
                    const bgm_ev_t *ev = &stream[or_ev_idx];
                    trigger_or_event(ev);
                    or_ev_idx++;
                    if (or_ev_idx >= slen) or_ev_idx = 0u;
                }
            }

            /* 4-voice 2-op FM synthesis */
            uint32_t v;
            for (v = 0u; v < 4u; v++) {
                if (fm_v[v].amp < fm_v[v].gate) {
                    fm_v[v].amp += 96;
                    if (fm_v[v].amp > fm_v[v].gate) fm_v[v].amp = fm_v[v].gate;
                } else if (fm_v[v].amp > fm_v[v].gate) {
                    fm_v[v].amp -= 14;
                    if (fm_v[v].amp < fm_v[v].gate) fm_v[v].amp = fm_v[v].gate;
                }
                if (fm_v[v].amp > 0 && fm_v[v].inc > 0u) {
                    int32_t mod, car;
                    uint32_t m_inc = (v == 2u) ? (fm_v[v].inc << 1) : fm_v[v].inc;
                    fm_v[v].ph_m += m_inc;
                    fm_v[v].ph_c += fm_v[v].inc;
                    if (fm_v[v].mod_env > 1500) fm_v[v].mod_env -= 4;
                    mod = (fm_sine(fm_v[v].ph_m) * fm_v[v].mod_env) >> 15;
                    car = fm_sine(fm_v[v].ph_c + ((uint32_t)mod << 16));
                    mix += (car * fm_v[v].amp) >> 15;
                }
            }

            /* Drums */
            lfsr = (lfsr >> 1) ^ (-(lfsr & 1u) & 0xB400u);
            if (kick_rem > 0u) {
                kick_rem--;
                kick_ph += kick_inc;
                if (kick_inc > 4500000u) kick_inc -= 3600u;
                mix += (fm_sine(kick_ph) * kick_amp) >> 15;
                kick_amp = (kick_amp * 255) >> 8;
            }
            if (snr_rem > 0u) {
                int32_t nz = (lfsr & 1u) ? 16000 : -16000;
                snr_rem--;
                mix += (nz * snr_amp) >> 15;
                snr_amp = (snr_amp * 254) >> 8;
            }
            if (hat_rem > 0u) {
                int32_t nz = (lfsr & 1u) ? 14000 : -14000;
                hat_rem--;
                mix += (nz * hat_amp) >> 15;
                hat_amp = (hat_amp * 248) >> 8;
            }
        }

        out[2u * i] += mix;
        out[2u * i + 1u] += mix;
    }
}
