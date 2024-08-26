/* FPC1020 Touch sensor driver
 *
 * Copyright (c) 2014 Fingerprint Cards AB <tech@fingerprints.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License Version 2
 * as published by the Free Software Foundation.
 */

#include "fpc1020_navlib.h"
#include "fpc1020_common.h"
#define I32_MAX      (0x7fffffff)                // Max positive value representable as 32-bit signed integer 
#define ABS(X)       (((X) < 0) ? -(X) : (X))    // Fast absolute value for 32-bit integers

#define FPC1150_CH                      36
#define FPC1150_CHP                    36
#define FPC1150_CW                     12

#define FPC1021_CH                      64
#define FPC1021_CW                      64
#define MIN_DIFF_THRESHOLD      10000

// Threshold of how different the best match may be, if the best match is more different than this value then we consider the movement to be nonexistant
#define NORM_MIN_DIFF_THRESHOLD 20
#define NORM_MIN_DIFF_THRESHOLD_1150 50

static int fpc1021_calculate_diff(const u8* p_curr, const u8* p_prev, int cmp_width, int cmp_height, int diff_limit)
{
    int x, y;
    int diff = 0;

    for (y = 0; y < cmp_height; y++)
    {
        for (x = 0; x < cmp_width; x++)
        {
            int i = x + y * nav_para.nav_img_w;
            diff += ABS(p_curr[i] - p_prev[i]);
        }

        // Not good enough, abandon early
        if (diff > diff_limit)
        {
            return I32_MAX;
        }
    }

    return diff;
}

static int fpc1150_calculate_diff(const u8* p_curr, const u8* p_prev, int cmp_width, int cmp_height) {
    int x, y;
    int diff = 0;

    for (y = 0; y < cmp_height; y++) {
        for (x = 0; x < cmp_width; x++) {
            int i = x + y * nav_para.nav_img_w;
            diff += ABS(p_curr[i] - p_prev[i]);
        }
    }

    return diff;
}

// TODO There is a problem with this algorithm where it returns movement when the finger enters/leaves the sensor
void fpc1021_get_movement(const u8* p_curr, const u8* p_prev, int* p_dx, int* p_dy)
{
    int x, y;
    int comp_w = nav_para.nav_img_w - (FPC1021_CW / nav_para.nav_img_col_mask);
    int comp_h = nav_para.nav_img_h - (FPC1021_CH / nav_para.nav_img_row_skip);
    int min_diff = I32_MAX;

    // Default vector
    *p_dx = 0;
    *p_dy = 0;

    // Calculate translation vector
    for (y = -(FPC1021_CH / nav_para.nav_img_row_skip); y <= (FPC1021_CH / nav_para.nav_img_row_skip); ++y)
    {
        for (x = -(FPC1021_CW / nav_para.nav_img_col_mask); x <= (FPC1021_CW / nav_para.nav_img_col_mask); ++x)
        {
            int diff;

            diff = fpc1021_calculate_diff(
                       p_curr + ((x + (FPC1021_CW / nav_para.nav_img_col_mask)) / 2) + ((y + (FPC1021_CH / nav_para.nav_img_row_skip)) / 2) * nav_para.nav_img_w,
                       p_prev + (((FPC1021_CW / nav_para.nav_img_col_mask) - x) / 2) + (((FPC1021_CH / nav_para.nav_img_row_skip) - y) / 2) * nav_para.nav_img_h,
                       comp_w,
                       comp_h,
                       min_diff);
            if (diff < min_diff)
            {
                min_diff = diff;
                *p_dx = x;
                *p_dy = y;
            }
        }
    }

    if ((min_diff / (comp_w * comp_h)) > NORM_MIN_DIFF_THRESHOLD)
    {
        *p_dx = 0;
        *p_dy = 0;
    }

    // Account for masked columns and skipped rows
    *p_dx *= nav_para.nav_img_col_mask;
    *p_dy *= nav_para.nav_img_row_skip;
}

#define MAX_DX    (32/4)
#define MAX_DY    (64/4)

static int fpc1155_calculate_diff(const u8* p_curr, const u8* p_prev, int cmp_width, int cmp_height, int diff_limit)
{
    int x, y;
    int diff = 0;

    for (y = 0; y < cmp_height; y++)
    {
        for (x = 0; x < cmp_width; x++) {
            int i = x + y * nav_para.nav_img_w;
            diff += ABS(p_curr[i] - p_prev[i]);
        }

        // Not good enough, abandon early
        if (diff > diff_limit)
        {
            return I32_MAX;
        }
    }

    return diff;
}


#if 1
void fpc1150_get_movement(const u8* p_curr, const u8* p_prev, int* p_dx, int* p_dy) 
{
    int x, y;
    int comp_w =  nav_para.nav_img_w - MAX_DX;
    int comp_h =  nav_para.nav_img_h - MAX_DY;
    int min_diff = I32_MAX;
    // Default vector
    *p_dx = 0;
    *p_dy = 0;

    // Calculate translation vector
    for (y = -MAX_DY; y <= MAX_DY; ++y)
    {
        for (x = -MAX_DX; x <= MAX_DX; ++x)
        {

        int diff;

            diff = fpc1155_calculate_diff(
                p_curr + ((x + MAX_DX) / 2) + ((y + MAX_DY) / 2) *  nav_para.nav_img_w,
                p_prev + ((MAX_DX - x) / 2) + ((MAX_DY - y) / 2) * nav_para.nav_img_w,
                comp_w,
                comp_h,
                min_diff);
            if (diff < min_diff)
            {
                min_diff = diff;
                *p_dx = x;
                *p_dy = y;
            }
        }
    }

    if ((min_diff / (comp_w * comp_h)) > NORM_MIN_DIFF_THRESHOLD_1150)
    {
        *p_dx = 0;
        *p_dy = 0;
    }

    // Account for masked columns and skipped rows
    *p_dx *= nav_para.nav_img_col_mask;
    *p_dy *= nav_para.nav_img_row_skip;

}
#else
#define C_H (36 / FPC1150_NAV_IMG_ROW_SKIP)

#define C_HP (36 / FPC1150_NAV_IMG_ROW_SKIP)

#define C_W 12
#define MIN_DIFF_THRESHOLD_2 10000
static int calculate_diff(const u8* p_curr, const u8* p_prev, int cmp_width, int cmp_height)
{
        int x, y;
        int diff = 0;
        for (y = 0; y < cmp_height; y++) {
                for (x = 0; x < cmp_width; x++) {
                        int i = x + y * nav_para.nav_img_w;
                        diff += ABS(p_curr[i] - p_prev[i]);
                }
        }
        return diff;

}
void fpc1150_get_movement(const u8* p_curr, const u8* p_prev, int* p_dx, int* p_dy)
{

        int x, y;

        int compare_width  = nav_para.nav_img_w - C_W;
        int compare_height = nav_para.nav_img_h - 2 * C_HP;
        int min_diff = I32_MAX;
        printk(" fpc1150_get_movement w:%d, h:%d\n", nav_para.nav_img_w, nav_para.nav_img_h);
        // Move pointers to after padding
        p_curr += C_HP * nav_para.nav_img_w;
        p_prev += C_HP * nav_para.nav_img_w;
        // Default vector
        *p_dx = 0;
        *p_dy = 0;
    // Calculate translation vector
        for (y = 0; y <= C_H; ++y) {
                for (x = 0; x <= C_W; ++x) {
                        int diff;
                        diff = calculate_diff(p_prev, p_curr + x + y*nav_para.nav_img_w, compare_width, compare_height);
                        if (diff < min_diff) {
                                min_diff = diff;
                                *p_dx = x;
                                *p_dy = y;
                        }
                        diff = calculate_diff(p_curr, p_prev + x + y*nav_para.nav_img_w, compare_width, compare_height);
                        if (diff < min_diff) {
                                min_diff = diff;
                                *p_dx = -x;
                                *p_dy = -y;
                        }
                        diff = calculate_diff(p_prev, p_curr + x - y*nav_para.nav_img_w, compare_width, compare_height);
                        if (diff < min_diff) {
                                min_diff = diff;
                                *p_dx = x;
                                *p_dy = -y;
                        }
                        diff = calculate_diff(p_curr, p_prev + x - y*nav_para.nav_img_w, compare_width, compare_height);
                        if (diff < min_diff) {
                                min_diff = diff;
                                *p_dx = -x;
                                *p_dy = y;
                        }
                }
        }
        if (min_diff > MIN_DIFF_THRESHOLD_2) {
                *p_dx = 0;
                *p_dy = 0;
        }
        // Account for skipped rows
    *p_dx *= nav_para.nav_img_col_mask;
    *p_dy *= nav_para.nav_img_row_skip;
}
#endif
