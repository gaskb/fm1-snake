/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
#include "arcade_common.h"
#include "arcade_audio.h"

enum { OR_TITLE = 0, OR_PLAY = 1, OR_PAUSE = 2, OR_OVER = 3 };

#define OR_SKY0 RGB(20, 12, 55)
#define OR_SKY1 RGB(65, 25, 95)
#define OR_SKY2 RGB(150, 45, 90)
#define OR_SKY3 RGB(230, 105, 55)
#define OR_SUN  RGB(255, 210, 60)
#define OR_MTN  RGB(35, 18, 52)

#define OR_GRASS_A RGB(24, 155, 55)
#define OR_GRASS_B RGB(18, 125, 42)
#define OR_ROAD_A  RGB(55, 58, 66)
#define OR_ROAD_B  RGB(48, 50, 58)
#define OR_CURB_R  RGB(230, 45, 45)
#define OR_CURB_W  RGB(240, 240, 240)
#define OR_LANE    RGB(235, 235, 210)

#define OR_NCARS 1u

static struct {
    int32_t speed;      /* 0..280 (scaled *1 for km/h) */
    int32_t player_x;   /* -140..140 (0 = center of road) */
    int32_t steer_vis;  /* -1, 0, +1 for car tilt */
    int32_t sky_x;      /* horizon parallax */
    uint32_t pos;       /* road Z position */
    int32_t curve;      /* current road curve (-80..80) */
    int32_t target_curve;
    uint32_t seg_rem;   /* distance until next curve change */
    uint32_t next_cp;   /* next checkpoint distance */
    uint16_t time_s;
    uint16_t score;
    uint8_t st, bgm_mode, new_hi, crash_t, cp_flash;
    uint32_t last_frame_ms, last_sec_ms;

    struct {
        uint32_t z;     /* 100..2000 (0 = closest) */
        int8_t lane;    /* -1, 0, +1 */
        uint8_t col_idx;
    } cars[OR_NCARS];
} orun;

static const uint16_t OR_CAR_COLS[3] = {
    RGB(60, 150, 255),
    RGB(255, 215, 50),
    RGB(220, 230, 240)
};

static void or_draw_top(void)
{
    int32_t sx;
    char hud1[16] = "TIME 45  KMH 000";
    char hud2[14] = "SCR 0000";
    const char *trk = (orun.bgm_mode == 2u) ? "MAGICAL" :
                      (orun.bgm_mode == 3u) ? "BREEZE" :
                      (orun.bgm_mode == 4u) ? "SPLASH" : "BGM OFF";

    cv_begin(240, 120, OR_SKY0);
    cv_rect(0, 30, 240, 30, OR_SKY1);
    cv_rect(0, 60, 240, 30, OR_SKY2);
    cv_rect(0, 90, 240, 30, OR_SKY3);

    /* Retro sunset sun with scanline stripes */
    sx = 120 + ((orun.sky_x >> 2) % 240);
    if (sx < -40) sx += 240;
    if (sx > 280) sx -= 240;
    cv_rect(sx - 24, 52, 48, 36, OR_SUN);
    cv_rect(sx - 28, 60, 56, 28, OR_SUN);
    cv_rect(sx - 28, 72, 56, 2, OR_SKY2);
    cv_rect(sx - 28, 78, 56, 3, OR_SKY2);
    cv_rect(sx - 28, 84, 56, 4, OR_SKY3);

    /* Distant mountain silhouette */
    {
        int32_t i, off = orun.sky_x;
        for (i = -2; i < 8; i++) {
            int32_t mx = ((i * 48 + off) % 336) - 48;
            cv_rect(mx, 102, 36, 18, OR_MTN);
            cv_rect(mx + 8, 94, 20, 8, OR_MTN);
            cv_rect(mx + 14, 88, 8, 6, OR_MTN);
        }
    }

    /* HUD */
    fmt_num(hud1 + 5, orun.time_s, 2u);
    hud1[7] = ' ';
    fmt_num(hud1 + 13, (uint16_t)orun.speed, 3u);
    fmt_num(hud2 + 4, orun.score, 4u);
    cv_text(6, 4, &FONT_S, hud1, C_WHITE);
    cv_text(6, 22, &FONT_S, hud2, RGB(255, 215, 70));
    cv_text(168, 22, &FONT_S, trk, RGB(180, 230, 255));

    if (orun.cp_flash > 0u) {
        cv_text(52, 52, &FONT_L, "CHECKPOINT!", RGB(120, 255, 140));
    }
    cv_blit(0, 0);
}

static void or_draw_palm(int32_t x, int32_t y, int32_t h)
{
    int32_t w = h / 6;
    if (w < 2) w = 2;
    if (x < -20 || x > 260) return;
    /* Trunk */
    cv_rect(x - w / 2, y - h, w, h, RGB(120, 72, 35));
    /* Crown */
    cv_rect(x - h / 3, y - h - w, (h * 2) / 3, w * 2, RGB(30, 175, 65));
    cv_rect(x - h / 2, y - h, h, w, RGB(45, 210, 80));
}

static void or_draw_player_car(int32_t tilt)
{
    int32_t cx = 120 + tilt * 3;
    int32_t cy = 96;
    uint16_t body = (orun.crash_t & 2u) ? RGB(255, 255, 255) : RGB(225, 30, 35);
    /* Shadow */
    cv_rect(cx - 24, cy + 16, 48, 5, RGB(20, 20, 25));
    /* Tires */
    cv_rect(cx - 23, cy + 8, 8, 10, C_BLACK);
    cv_rect(cx + 15, cy + 8, 8, 10, C_BLACK);
    /* Body */
    cv_rect(cx - 20, cy + 3, 40, 11, body);
    cv_rect(cx - 14, cy - 3, 28, 7, RGB(185, 20, 25));
    /* Windshield & seats */
    cv_rect(cx - 11, cy - 2, 22, 4, RGB(90, 190, 240));
    cv_rect(cx - 8, cy - 5, 5, 3, RGB(255, 210, 160)); /* Driver */
    cv_rect(cx + 3, cy - 5, 5, 3, RGB(255, 230, 110)); /* Passenger */
    /* Twin tail-lights */
    cv_rect(cx - 17, cy + 6, 7, 3, RGB(255, 190, 60));
    cv_rect(cx + 10, cy + 6, 7, 3, RGB(255, 190, 60));
}

static void or_draw_road(void)
{
    int32_t y;
    int32_t i;

    cv_begin(240, 120, OR_GRASS_A);

    for (y = 0; y < 120; y++) {
        uint32_t z = 2400u / (uint32_t)(y + 6);
        uint32_t band = ((z + (orun.pos >> 3)) >> 4) & 1u;
        int32_t rw = 8 + (y * 96) / 119;
        int32_t inv_y = 119 - y;
        int32_t curve_dx = (orun.curve * inv_y * inv_y) >> 14;
        int32_t player_dx = (orun.player_x * y) / 119;
        int32_t cx = 120 + curve_dx - player_dx;
        int32_t cw = 2 + (y * 9) / 119;

        /* Grass stripe */
        cv_rect(0, y, 240, 1, band ? OR_GRASS_A : OR_GRASS_B);
        /* Curbs */
        cv_rect(cx - rw - cw, y, cw, 1, band ? OR_CURB_R : OR_CURB_W);
        cv_rect(cx + rw, y, cw, 1, band ? OR_CURB_R : OR_CURB_W);
        /* Asphalt */
        cv_rect(cx - rw, y, rw * 2, 1, band ? OR_ROAD_A : OR_ROAD_B);
        /* Lane dividers (3 lanes -> 2 dashed lines) */
        if (band) {
            int32_t lw = (y > 60) ? 2 : 1;
            cv_rect(cx - rw / 3, y, lw, 1, OR_LANE);
            cv_rect(cx + rw / 3, y, lw, 1, OR_LANE);
        }
    }

    /* Roadside palm trees (drawn back-to-front) */
    for (i = 3; i >= 0; i--) {
        uint32_t tz = (uint32_t)((3 - i) * 90 + 30) - ((orun.pos >> 3) % 90u);
        int32_t ty = (int32_t)(2400u / (tz + 15u)) - 6;
        if (ty > 4 && ty < 118) {
            int32_t rw = 8 + (ty * 96) / 119;
            int32_t inv_y = 119 - ty;
            int32_t curve_dx = (orun.curve * inv_y * inv_y) >> 14;
            int32_t player_dx = (orun.player_x * ty) / 119;
            int32_t cx = 120 + curve_dx - player_dx;
            int32_t h = 10 + (ty * 34) / 119;
            or_draw_palm(cx - rw - h / 2 - 4, ty, h);
            or_draw_palm(cx + rw + h / 2 + 4, ty, h);
        }
    }

    /* Traffic cars */
    for (i = 0; i < (int32_t)OR_NCARS; i++) {
        uint32_t cz = orun.cars[i].z;
        int32_t cy = (int32_t)(2400u / (cz + 15u)) - 6;
        if (cy > 4 && cy < 118) {
            int32_t rw = 8 + (cy * 96) / 119;
            int32_t inv_y = 119 - cy;
            int32_t curve_dx = (orun.curve * inv_y * inv_y) >> 14;
            int32_t player_dx = (orun.player_x * cy) / 119;
            int32_t cx = 120 + curve_dx - player_dx + (int32_t)orun.cars[i].lane * (rw * 2 / 3);
            int32_t w = 6 + (cy * 26) / 119;
            int32_t h = 4 + (cy * 16) / 119;
            cv_rect(cx - w / 2, cy - h, w, h, OR_CAR_COLS[orun.cars[i].col_idx]);
            cv_rect(cx - w / 3, cy - h + 1, (w * 2) / 3, h / 3 + 1, RGB(40, 50, 70));
            cv_rect(cx - w / 2 + 1, cy - 3, w / 4 + 1, 2, RGB(255, 50, 50));
            cv_rect(cx + w / 4, cy - 3, w / 4 + 1, 2, RGB(255, 50, 50));
        }
    }

    or_draw_player_car(orun.steer_vis);
    cv_blit(0, 120);
}

static void or_start(void)
{
    uint32_t i;
    orun.speed = 60;
    orun.player_x = 0;
    orun.steer_vis = 0;
    orun.sky_x = 0;
    orun.pos = 0u;
    orun.curve = 0;
    orun.target_curve = 0;
    orun.seg_rem = 180u;
    orun.next_cp = 600u;
    orun.time_s = 45u;
    orun.score = 0u;
    orun.new_hi = 0u;
    orun.crash_t = 0u;
    orun.cp_flash = 0u;
    orun.st = OR_PLAY;
    orun.last_frame_ms = fm1_ms;
    orun.last_sec_ms = fm1_ms;

    for (i = 0u; i < OR_NCARS; i++) {
        orun.cars[i].z = 120u + i * 110u;
        orun.cars[i].lane = (int8_t)((int32_t)(i % 3u) - 1);
        orun.cars[i].col_idx = (uint8_t)(i % 3u);
    }

    fm1_led_key(BTN_PLAY, 1);
    bgm_set(orun.bgm_mode);
    sfx_start();
    or_draw_top();
    or_draw_road();
}

static void or_gameover(void)
{
    orun.st = OR_OVER;
    bgm_set(0u);
    fm1_led_key(BTN_PLAY, 0);
    sfx_gameover();
    lcd_fill(16, 76, 208, 92, C_BLACK);
    draw_text_box(20, 82, 200, &FONT_L, "TIME OVER", RGB(255, 90, 90), 1);
    if (orun.new_hi) {
        draw_text_box(20, 122, 200, &FONT_S, "NEW HIGH SCORE!", RGB(255, 215, 70), 1);
    } else {
        char buf[16] = "SCORE: 0000";
        fmt_num(buf + 7, orun.score, 4u);
        draw_text_box(20, 122, 200, &FONT_S, buf, RGB(180, 230, 255), 1);
    }
    draw_text_box(20, 148, 200, &FONT_S, "PLAY: RETRY  HOME: MENU", C_WHITE, 1);
}

static void or_step(uint32_t keys_held, uint32_t btns_held, int32_t enc_turn)
{
    uint32_t i;
    int32_t gas = (keys_held & (MASK_KEY_UP | MASK_KEY_F)) || (btns_held & (1u << BTN_ENV));
    int32_t brake = (keys_held & MASK_KEY_D) || (btns_held & (1u << BTN_SEL));
    int32_t left = (keys_held & MASK_KEY_C) || (btns_held & (1u << BTN_OCT_DOWN));
    int32_t right = (keys_held & MASK_KEY_E) || (btns_held & (1u << BTN_OCT_UP));

    /* Speed & off-road drag */
    if (orun.crash_t > 0u) {
        orun.crash_t--;
        orun.speed -= 8;
    } else if (gas) {
        orun.speed += 5;
    } else if (brake) {
        orun.speed -= 10;
    } else {
        orun.speed -= 2;
    }

    if (orun.player_x < -98 || orun.player_x > 98) {
        if (orun.speed > 110) orun.speed -= 6;
    }
    if (orun.speed < 0) orun.speed = 0;
    if (orun.speed > 280) orun.speed = 280;

    /* Steering */
    orun.steer_vis = 0;
    if (enc_turn != 0) {
        orun.player_x += enc_turn * 10;
        orun.steer_vis = (enc_turn > 0) ? 1 : -1;
    }
    if (orun.speed > 0) {
        if (left)  { orun.player_x -= 6; orun.steer_vis = -1; }
        if (right) { orun.player_x += 6; orun.steer_vis = 1; }
        /* Centrifugal pull on curves */
        orun.player_x -= (orun.curve * orun.speed) >> 11;
    }
    if (orun.player_x < -145) orun.player_x = -145;
    if (orun.player_x > 145)  orun.player_x = 145;

    /* Advance road */
    orun.pos += (uint32_t)orun.speed;
    orun.sky_x -= (orun.curve * orun.speed) >> 11;

    /* Curve segments */
    if (orun.seg_rem <= (uint32_t)(orun.speed >> 4)) {
        int32_t r = (int32_t)(arcade_rand() % 5u) - 2; /* -2..+2 */
        orun.target_curve = r * 35;
        orun.seg_rem = 120u + (arcade_rand() % 180u);
    } else {
        orun.seg_rem -= (uint32_t)(orun.speed >> 4);
    }
    if (orun.curve < orun.target_curve) orun.curve += 2;
    else if (orun.curve > orun.target_curve) orun.curve -= 2;

    /* Distance score & checkpoints */
    orun.score = (uint16_t)(orun.pos >> 7);
    if (orun.score > save_data.hi_outrun) {
        save_data.hi_outrun = orun.score;
        orun.new_hi = 1u;
    }
    if (orun.score >= orun.next_cp) {
        orun.next_cp += 500u;
        orun.time_s = (uint16_t)(orun.time_s + 15u);
        if (orun.time_s > 99u) orun.time_s = 99u;
        orun.cp_flash = 25u;
        sfx_check();
    }
    if (orun.cp_flash > 0u) orun.cp_flash--;

    /* Traffic cars */
    for (i = 0u; i < OR_NCARS; i++) {
        int32_t dz = (orun.speed - 95) >> 4;
        int32_t nz = (int32_t)orun.cars[i].z - dz;
        if (nz < 8) {
            orun.cars[i].z = 260u + (arcade_rand() % 140u);
            orun.cars[i].lane = (int8_t)((int32_t)(arcade_rand() % 3u) - 1);
            orun.cars[i].col_idx = (uint8_t)(arcade_rand() % 3u);
        } else if (nz > 420) {
            orun.cars[i].z = 400u;
        } else {
            orun.cars[i].z = (uint32_t)nz;
        }

        /* Collision near camera */
        if (orun.crash_t == 0u && orun.cars[i].z >= 10u && orun.cars[i].z <= 22u) {
            int32_t car_x = (int32_t)orun.cars[i].lane * 64;
            int32_t diff = orun.player_x - car_x;
            if (diff < 0) diff = -diff;
            if (diff < 34) {
                orun.crash_t = 14u;
                orun.speed = 40;
                orun.cars[i].z = 320u;
                sfx_crash();
            }
        }
    }

    /* Countdown timer */
    if ((uint32_t)(fm1_ms - orun.last_sec_ms) >= 1000u) {
        orun.last_sec_ms += 1000u;
        if (orun.time_s > 0u) orun.time_s--;
        if (orun.time_s == 0u) {
            save_scores();
            or_draw_top();
            or_gameover();
            return;
        }
    }

    or_draw_top();
    or_draw_road();
}

static void outrun_enter(void)
{
    bgm_set(0u);
    orun.st = OR_TITLE;
    orun.bgm_mode = 2u; /* Magical Sound Shower by default */
    fm1_led_key(BTN_PLAY, 0);
    lcd_fill(0, 0, 240, 240, C_BLACK);
    draw_text_box(12, 28, 216, &FONT_L, "OUTRUN", RGB(255, 70, 90), 1);
    draw_text_box(12, 68, 216, &FONT_S, "MAGICAL SOUND SHOWER", RGB(255, 215, 70), 1);
    draw_text_box(12, 102, 216, &FONT_S, "C#-D# / F: GAS   D: BRAKE", C_WHITE, 1);
    draw_text_box(12, 124, 216, &FONT_S, "KNOB1 / C-E: STEER", C_WHITE, 1);
    draw_text_box(12, 146, 216, &FONT_S, "FX: RADIO  HOME: MENU", RGB(160, 200, 255), 1);
    draw_text_box(12, 192, 216, &FONT_S, "PRESS PLAY TO DRIVE!", RGB(120, 255, 140), 1);
}

static int outrun_tick(uint32_t down_btns, uint32_t down_keys, uint32_t keys_held, uint32_t btns_held, int32_t enc_turn)
{
    if (down_btns & (1u << BTN_HOME)) {
        bgm_set(0u);
        fm1_led_key(BTN_PLAY, 0);
        save_scores();
        return 1;
    }

    if (down_btns & (1u << BTN_FX)) {
        if (orun.bgm_mode == 2u) orun.bgm_mode = 3u;
        else if (orun.bgm_mode == 3u) orun.bgm_mode = 4u;
        else if (orun.bgm_mode == 4u) orun.bgm_mode = 0u;
        else orun.bgm_mode = 2u;
        if (orun.st == OR_PLAY) bgm_set(orun.bgm_mode);
    }

    if (orun.st == OR_TITLE || orun.st == OR_OVER) {
        if ((down_btns & (1u << BTN_PLAY)) || down_keys) or_start();
        return 0;
    }

    if (down_btns & (1u << BTN_PLAY)) {
        if (orun.st == OR_PLAY) {
            orun.st = OR_PAUSE;
            bgm_set(0u);
            fm1_led_key(BTN_PLAY, 0);
            draw_text_box(50, 104, 140, &FONT_L, "PAUSE", RGB(255, 215, 70), 1);
        } else {
            orun.st = OR_PLAY;
            bgm_set(orun.bgm_mode);
            fm1_led_key(BTN_PLAY, 1);
            orun.last_frame_ms = fm1_ms;
            orun.last_sec_ms = fm1_ms;
        }
    }
    if (orun.st != OR_PLAY) return 0;

    if ((uint32_t)(fm1_ms - orun.last_frame_ms) >= 30u) {
        orun.last_frame_ms = fm1_ms;
        or_step(keys_held, btns_held, enc_turn);
    }
    return 0;
}
