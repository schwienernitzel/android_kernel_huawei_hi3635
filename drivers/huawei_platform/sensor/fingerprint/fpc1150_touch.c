/* FPC1020 Touch sensor driver
 *
 * Copyright (c) 2013,2014 Fingerprint Cards AB <tech@fingerprints.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License Version 2
 * as published by the Free Software Foundation.
 */

//#define DEBUG
//#define DEBUG_TIME
#include <linux/input.h>
#include <linux/delay.h>
#include <linux/time.h>
#include "fpc1020_common.h"
#include "fpc1020_input.h"
#include "fpc1020_capture.h"
#include "fpc1020_debug.h"
#include "fpc1020_navlib.h"

#include <huawei_platform/log/hw_log.h>

#define HWLOG_TAG             FingerPrint

enum
{
    DEBUG_LEVEL = 1U<<0,
    DEBUG_NAV_DBG = 1U<<1,
    DEBUG_NAV_TOUCH_DBG = 1U<<2,
    DEBUG_NAV_DPAD_DBG = 1U<<3,
};


HWLOG_REGIST();
#define FP_LOG_INFO(x...)        _hwlog_info(HWLOG_TAG, ##x)
#define FP_LOG_ERR(x...)        _hwlog_err(HWLOG_TAG, ##x)
#define FP_LOG_DEBUG(x...) \
do { \
    if (fp_debug_mask & DEBUG_LEVEL ) \
        _hwlog_info(HWLOG_TAG, ##x); \
}while(0)

#define FP_LOG_NAV_DBG(x...) \
do { \
    if (fp_debug_mask & DEBUG_NAV_DBG ) \
        _hwlog_info(HWLOG_TAG, ##x); \
}while(0)

#define FP_LOG_NAV_TOUCH_DBG(x...) \
do { \
    if (fp_debug_mask & DEBUG_NAV_TOUCH_DBG ) \
        _hwlog_info(HWLOG_TAG, ##x); \
}while(0)

#define FP_LOG_NAV_DPAD_DBG(x...) \
do { \
    if (fp_debug_mask & DEBUG_NAV_DPAD_DBG ) \
        _hwlog_info(HWLOG_TAG, ##x); \
}while(0)


u8 fp_debug_mask = 0;

/*@ single_double_tap_cond = change_count + image_count
* at least one of single tap, (result > 2)to make sure our result is robust.
*/
static u8 single_double_tap_cond = 0;
//the sys file is sys/module/fpc1020/parameters/debug_mask, write 15 can open all log.
module_param_named(debug_mask, fp_debug_mask, uint, S_IWUSR | S_IRUGO);


/* -------------------------------------------------------------------- */
/* function prototypes                            */
/* -------------------------------------------------------------------- */
#ifdef CONFIG_INPUT_FPC1020_NAV
static int fpc1150_write_nav_setup(fpc1020_data_t* fpc1020);

static int capture_nav_image(fpc1020_data_t* fpc1020);

/* -------------------------------------------------------------------- */
/* driver constants                            */
/* -------------------------------------------------------------------- */

#define FPC1150_INPUT_POLL_INTERVAL (8000u)
#define FPC1150_CLICK_INTERVAL        (10000u)
#define THRESHOLD_DURATION_CLICK       300/*click threshold*/
#define THRESHOLD_DURATION_FAST_CLICK  70
#define THRESHOLD_DURATION_HOLD        150/*long press threshold*/
#define THRESHOLD_DURATION_DTAP        500
#define THRESHOLD_DURATION_TAP        1500
#define FLOAT_MAX 100
#define FPC1020_NAV_ALGO_SUBZONES    2
#define DEVICE_WIDTH 1080
#define DEVICE_HEIGHT 1920

enum
{
    FNGR_ST_NONE = 0,
    FNGR_ST_DETECTED, /*finger has been detected*/
    FNGR_ST_LOST, /*finger has leave*/
    FNGR_ST_TAP, /*click once with pad*/
    FNGR_ST_HOLD, /*long press*/
    FNGR_ST_MOVING, /*moving: left right up dowm */
    FNGR_ST_L_HOLD, /*Mouse wheel event, don't use by fpc1150*/
    FNGR_ST_DOUBLE_TAP, /*click twice*/
};

enum
{
    FPC1150_INPUTMODE_TRACKPAD     = 0,    //trackpad(navi) event report
    FPC1150_INPUTMODE_MOUSE        = 1,    //mouse event report
    FPC1150_INPUTMODE_TOUCH        = 2,    //touch event report
    FPC1150_INPUTMODE_DPAD         = 3,    //dpad event report
};

/*!@ Finger print sensor FPC1150, Position Table based on subarea compution,
@ 208*80 = C * R
*/
struct fpc1150_ptable_t {
    short x;
    short y;
};

/* -------------------------------------------------------------------- */
/* function definitions                            */
/* -------------------------------------------------------------------- */

static int get_finger_position(int *pCoordX,
                               int *pCoordY,
                               int subarea_bits)
{

    int xThr = 0;
    int yThr = 0;
    int error = 0;
    int xMapTable[16] = {0, 0, -3, -3, 1, 1, -3, -3, 3, 3, -1, -1, 4, 4, 0, 0};
    int yMapTable[16] = {0, 0, 2, 2, 1, 1, 3, 3, 1, 1, 3, 3, 2, 2, 4, 4};

    const int MaxValueX = 10;
    const int MaxValueY = 10;

    int fingerPresenceB0 = subarea_bits & 0xffu;
    int fingerPresenceB1 = (subarea_bits >> 8) & 0xffu;

    int xG = 0;
    int yG = 0;
    int norm = 0;

    // First the x direction
    xG += xMapTable[fingerPresenceB0 & 0x0f];
    xG += xMapTable[(fingerPresenceB0 >> 4) & 0x0f];
    xG += xMapTable[fingerPresenceB1 & 0x0f];

    // Then the y direction
    yG += yMapTable[fingerPresenceB0 & 0x0f];
    //Find the number of subare containing finger
    norm += yG;
    // Then the y direction for the negative part
    yG -= yMapTable[fingerPresenceB1 & 0x0f];

    //Find the number of subare containing finger
    norm += yMapTable[(fingerPresenceB0 >> 4) & 0x0f];
    norm += yMapTable[fingerPresenceB1 & 0x0f];

    if (norm > 0)
    {
        // 3 is the maximum value of the weighting coefficient
        xG = (10 * xG / (norm * 3));
        yG = (10 * yG / norm);

        //Avoid small movement to be reported
        if (abs(xG) < xThr)
        {
            xG = 0;
        }
        //Avoid small movement to be reported
        if (abs(yG) < yThr)
        {
            yG = 0;
        }
    }
    else
    {
        xG = 0;
        yG = 0;
    }

    // Check validity range of the center of gravity
    if (abs(xG) > MaxValueX)
    {
        error = 1;
    }
    if (abs(yG) > MaxValueY)
    {
        error = 1;
    }
    if (subarea_bits == 0x111 || subarea_bits == 0x011 || subarea_bits == 0x110 || subarea_bits == 0x101
            || subarea_bits == 0x100 || subarea_bits == 0x010) {
        xG = -10;
    }
    *pCoordX = xG;
    *pCoordY = yG;

    return error;
}

static int get_finger_iposition(int *pCoordX,
                                int *pCoordY,
                                int subarea_bits)
{

    int error = 0;
    int i = 0;
    int xG = 0;
    int yG = 0;
    int avaible_subarea_cnt = 0;/*store avaible subarea count*/
    int subarea_tmp = 0;
    int subarea_sum_x_axis = 0;
    int subarea_sum_y_axis = 0;

    struct fpc1150_ptable_t ptable_xy[] = {
        {5, 3}, {19, 3}, {33, 3}, {47, 3},
        {5, 9}, {19, 9}, {33, 9}, {47, 9},
        {5, 17}, {19, 17}, {33, 17}, {47, 17}
    };
    int avalibe_postion_arr[12] = {0};
    subarea_tmp = subarea_bits;

    memset(avalibe_postion_arr, 0, 12);
    for (i = 0; i < 12; i++)
    {
        /* From MSB --> LSB to get every avaible bit */
        if ((subarea_tmp >> (11 - i))&0x1 == 0x01) {
            avalibe_postion_arr[11 - i] = 1;
            avaible_subarea_cnt++;
        }
    }

    if (avaible_subarea_cnt > 0) {
        for (i = 0; i < 12; i++) {
            if (avalibe_postion_arr[i] == 1) {
                subarea_sum_x_axis += ptable_xy[i].x;
                subarea_sum_y_axis += ptable_xy[i].y;
            }
        }

        xG = subarea_sum_x_axis / avaible_subarea_cnt;
        yG = subarea_sum_y_axis / avaible_subarea_cnt;
    }
    FP_LOG_INFO("[FPC]  subarea, Avg xG:%d, yG:%d, avaible_subarea_cnt:%d, sensor_reg_zone:0x%x\n ", xG , yG, avaible_subarea_cnt, subarea_bits);

    *pCoordX = xG;
    *pCoordY = yG;

    return error;
}

static void capture_post_processing(u8 *p_img)
{
    int x;
    int y;
    for (y = 0; y < nav_para.nav_img_h; y++) {
        for (x = 0; x < nav_para.nav_img_w; x++) {
            p_img[x+y*nav_para.nav_img_w] = p_img[(x*2)+y*(nav_para.nav_img_w*2)];
        }
    }
}

/*waite 40ms for finger down */
int fpc1020_capture_nav_wait_finger_down(fpc1020_data_t* fpc1020)
{
    int error = 0;
    error = fpc1020_nav_wait_finger_present(fpc1020);
    fpc1020_read_irq(fpc1020, true);
    return (error >= 0) ? 0 : error;
}

void init_enhanced_navi_setting(fpc1020_data_t* fpc1020)//
{
    switch (fpc1020->nav.input_mode)
    {
    case FPC1150_INPUTMODE_TRACKPAD:
        fpc1020->nav.p_sensitivity_key = 160;
        fpc1020->nav.p_multiplier_x = 120;
        fpc1020->nav.p_multiplier_y = 120;
        fpc1020->nav.multiplier_key_accel = 1;
        fpc1020->nav.threshold_key_accel = 70;
        break;
    case FPC1150_INPUTMODE_DPAD:
        fpc1020->nav.p_sensitivity_key = 200;
        fpc1020->nav.p_multiplier_x = 600;
        fpc1020->nav.p_multiplier_y = 400;
        fpc1020->nav.multiplier_key_accel = 1;
        fpc1020->nav.threshold_key_accel = 40;
        fpc1020->nav.threshold_key_start = 10;
        fpc1020->nav.threshold_key_start_up = 60;
        fpc1020->nav.threshold_key_start_down = 60;
        fpc1020->nav.threshold_key_start_left = 60;
        fpc1020->nav.threshold_key_start_right = 60;
        break;
    case FPC1150_INPUTMODE_TOUCH:
        fpc1020->nav.p_sensitivity_key = 160;
        fpc1020->nav.p_multiplier_x = 700;
        fpc1020->nav.p_multiplier_y = 2000;
        fpc1020->nav.multiplier_key_accel = 1;
        break;
    default:
        break;
    }
}

static void dpad_report_key(fpc1020_data_t* fpc1020, int key, int status)
{
    input_report_key(fpc1020->touch_pad_dev, key, status);
    input_sync(fpc1020->touch_pad_dev);
    input_report_key(fpc1020->touch_pad_dev, key, 0);
    input_sync(fpc1020->touch_pad_dev);

    switch (key)
    {
    case KEY_UP:
        FP_LOG_INFO("[FPC/DPAD] report keycode is up\n");
        break;
    case KEY_DOWN:
        FP_LOG_INFO("[FPC/DPAD] report keycode is down\n");
        break;
    case KEY_LEFT:
        FP_LOG_INFO("[FPC/DPAD] report keycode is left\n");
        break;
    case KEY_RIGHT:
        FP_LOG_INFO("[FPC/DPAD] report keycode is right\n");
        break;
    case KEY_ENTER:
        FP_LOG_INFO("[FPC/DPAD] report keycode is enter\n");
        break;
    case KEY_DELETE:
        FP_LOG_INFO("[FPC/DPAD] report keycode is delete\n");
        break;
    default:
        break;
    }
}

/*--------------------------------------------------------*/
static void dispatch_dpad_event(fpc1020_data_t* fpc1020, int x, int y, int finger_status)
{
    int abs_x, abs_y;
    int key_code = -1;//remember the key code index in fpc1020->nav.nav_dir_key array.
    int sumy_x= 0;
    int touchx_down_up = 0;
    int touchy_down_up = 0;

    switch (finger_status)
    {
    case FNGR_ST_DETECTED:
        fpc1020->nav.sum_x = 0;
        fpc1020->nav.sum_y = 0;
        break;
    case FNGR_ST_LOST:
        fpc1020->nav.sum_x = 0;/*clean this for next event*/
        fpc1020->nav.sum_y = 0;
        fpc1020->nav.throw_event = 0;
        fpc1020->nav.throw_event_move = 0;
        FP_LOG_NAV_DPAD_DBG("[FPC/DPAD] report finger up last\n");
        break;
    case  FNGR_ST_TAP:
        if (fpc1020->nav.throw_event == 0) {
            FP_LOG_NAV_DPAD_DBG("[FPC/DPAD] report single click\n");
        }
        break;
    case FNGR_ST_DOUBLE_TAP:
    {
        //dpad_report_key(fpc1020, KEY_DELETE, 1);
        fpc1020->nav.tap_status = -1;
        FP_LOG_NAV_DPAD_DBG("[FPC/DPAD] no report double click\n");
    }
    fpc1020->nav.tap_status = -1;
    break;

    case FNGR_ST_HOLD:
        if (fpc1020->nav.throw_event_move == 0)
        {
            dpad_report_key(fpc1020, KEY_ENTER, 1);
            mdelay(30);
            FP_LOG_INFO("[FPC/DPAD] report long press\n");
            fpc1020->nav.throw_event = 1;
        }
        fpc1020->nav.tap_status = -1;
        break;

    case FNGR_ST_MOVING:
        FP_LOG_NAV_DPAD_DBG("[FPC/DPAD] %s: report raw axis x=%d, y=%d\n", __func__, x, y);
        // Correct axis factor
        x = x * fpc1020->nav.p_multiplier_x / FLOAT_MAX;
        y = y * fpc1020->nav.p_multiplier_y / FLOAT_MAX;

        // Adjust Sensitivity
        FP_LOG_NAV_DPAD_DBG("[FPC/DPAD] %s: multiplied axis x=%d, y=%d, pre_sum x:%d, y:%d\n",
                            __func__, x, y, fpc1020->nav.sum_x, fpc1020->nav.sum_y);

            fpc1020->nav.sum_x += x;   // sum_x only use here
            fpc1020->nav.sum_y += y;
            abs_x = fpc1020->nav.sum_x > 0 ? fpc1020->nav.sum_x : -fpc1020->nav.sum_x;
            abs_y = fpc1020->nav.sum_y > 0 ? fpc1020->nav.sum_y : -fpc1020->nav.sum_y;
            FP_LOG_INFO("[FPC/DPAD] Navigation: report active=%d,axis[%d,%d:%d,%d],abs[%d,%d],ysum[x:%d,y:%d],imgs=%d,changes=%d, diff=%d\n",
                    fpc1020->nav.throw_event,fpc1020->nav.touch_down_x,fpc1020->nav.touch_down_y,
                    fpc1020->nav.touch_up_x,fpc1020->nav.touch_up_y,abs_x, abs_y, fpc1020->nav.sum_x, fpc1020->nav.sum_y,
                    fpc1020->nav.nav_imgs, fpc1020->nav.touch_change_count, ABS(ABS(fpc1020->nav.sumy)-ABS(fpc1020->nav.sumx)));

        if(fpc1020->nav.throw_event == 1)
        {
            break;//has been report key.
        }

        sumy_x = ABS(fpc1020->nav.sumy-fpc1020->nav.sumx);
        touchx_down_up = fpc1020->nav.touch_down_x-fpc1020->nav.touch_up_x;
        touchy_down_up = fpc1020->nav.touch_down_y-fpc1020->nav.touch_up_y;

        /* Add get_movement position and get sum of total point to limite Real movement, not just fake click*/
        //if(abs_x > abs_y)//left right
        if ((abs(touchx_down_up) > abs(touchy_down_up))&&(abs_x > abs_y))
        {
            FP_LOG_INFO("[FPC/DPAD] Navigation_0: nav.sum_x:%d,, nav.sum_y:%d,touchx_down_up:%d, touchy_down_up:%d, throw_event:%d, nav_imgs:%d, touch_change_count:%d\n",
                        fpc1020->nav.sum_x, fpc1020->nav.sum_y, touchx_down_up, touchy_down_up,
                        fpc1020->nav.throw_event,   fpc1020->nav.nav_imgs, fpc1020->nav.touch_change_count);
            if (fpc1020->nav.sum_x > fpc1020->nav.threshold_key_start_left
                    /*&& sumy_x>7 */ &&
                    ( (touchx_down_up > 9) )/*subarea on change*/
                    /* && ABS(touchy_down_up)<12*/)
            {
                key_code = NAV_DIR_LEFT;
                FP_LOG_NAV_DPAD_DBG("[FPC/DPAD] prepare report is key left, sumy_x:%d, touchx_down_up:%d\n", sumy_x, touchx_down_up);
            }
            else if ( fpc1020->nav.sum_x < -fpc1020->nav.threshold_key_start_right
                     &&( (touchx_down_up < -9)  )/*subarea on change*/)
            {
                FP_LOG_NAV_DPAD_DBG("[FPC/DPAD]  prepare report is key right, sumy_x:%d, touchx_down_up:%d\n", sumy_x, touchx_down_up);
                key_code = NAV_DIR_RIGHT;
            }
        }
        else if (((abs(touchx_down_up) <= abs(touchy_down_up)))&&(abs_x <= abs_y))
        {
            FP_LOG_INFO("[FPC/DPAD] Navigation_1: nav.sum_x:%d,, nav.sum_y:%d,touchx_down_up:%d, touchy_down_up:%d, throw_event:%d, nav_imgs:%d, touch_change_count:%d\n",
                        fpc1020->nav.sum_x, fpc1020->nav.sum_y, touchx_down_up, touchy_down_up,
                        fpc1020->nav.throw_event, fpc1020->nav.nav_imgs, fpc1020->nav.touch_change_count);

            if ((fpc1020->nav.sum_y < -fpc1020->nav.threshold_key_start_up)
                &&(touchy_down_up < -7))
            {
                FP_LOG_NAV_DPAD_DBG("[FPC/DPAD] prepare report is key up\n");
            if( fpc1020->nav.nav_dir_enable & (1<<NAV_DIR_UP))
                    key_code = NAV_DIR_UP;
            }
            else if ( (fpc1020->nav.sum_y > fpc1020->nav.threshold_key_start_down)
                &&(touchy_down_up > 7))
            {
                FP_LOG_NAV_DPAD_DBG("[FPC/DPAD] prepare report is key down\n");
            if( fpc1020->nav.nav_dir_enable & (1<<NAV_DIR_DOWN))
                    key_code = NAV_DIR_DOWN;
            }
        }
        else {
            FP_LOG_INFO("[FPC/DPAD] Navigation_3: finger_status:%d, moving,nav.sum_x:%d,, nav.sum_y:%d,touchx_down_up:%d, touchy_down_up:%d, throw_event:%d, nav_imgs:%d, touch_change_count:%d\n",
                        finger_status, fpc1020->nav.sum_x, fpc1020->nav.sum_y, touchx_down_up, touchy_down_up,
                        fpc1020->nav.throw_event,   fpc1020->nav.nav_imgs, fpc1020->nav.touch_change_count);
        }

        //report key value
        if (key_code >= NAV_DIR_UP && key_code <= NAV_DIR_RIGHT)
        {
                    if(fpc1020->nav.nav_dir_enable & (1<<key_code))
                    {
                        fpc1020->nav.key_code = key_code;
                        dpad_report_key(fpc1020, fpc1020->nav.nav_dir_key[key_code], 1);
                    }
                    //has key report, so set and clean some variable
                    fpc1020->nav.sum_x = 0;
                    fpc1020->nav.sum_y = 0;
                    fpc1020->nav.tap_status = -1;
                    fpc1020->nav.throw_event = 1;
                    fpc1020->nav.throw_event_move = 1;
        }
        break;
    default:
        break;
    }/*switch (finger_status)*/
}


static void dispatch_trackpad_event(fpc1020_data_t* fpc1020, int x, int y, int finger_status)
{
    int abs_x, abs_y;
    int sign_x, sign_y;

    if (finger_status == FNGR_ST_TAP)
    {
        input_report_key(fpc1020->input_dev, KEY_ENTER, 1);
        input_sync(fpc1020->input_dev);
        input_report_key(fpc1020->input_dev, KEY_ENTER, 0);
        input_sync(fpc1020->input_dev);
        return;
    }

    sign_x = x > 0 ? 1 : -1;
    sign_y = y > 0 ? 1 : -1;
    abs_x = x * sign_x;
    abs_y = y * sign_y;

    abs_x = x > 0 ? x : -x;
    abs_y = y > 0 ? y : -y;

    if (abs_x > fpc1020->nav.threshold_key_accel)
        x = (fpc1020->nav.threshold_key_accel
             + (abs_x - fpc1020->nav.threshold_key_accel) * fpc1020->nav.multiplier_key_accel) * sign_x;
    if (abs_y > fpc1020->nav.threshold_key_accel)
        y = (fpc1020->nav.threshold_key_accel
             + (abs_y - fpc1020->nav.threshold_key_accel) * fpc1020->nav.multiplier_key_accel) * sign_y;

    // Correct axis factor
    x = x * fpc1020->nav.p_multiplier_x / FLOAT_MAX;
    y = y * fpc1020->nav.p_multiplier_y / FLOAT_MAX;

    // Adjust Sensitivity
    x = x * fpc1020->nav.p_sensitivity_key / FLOAT_MAX;
    y = y * fpc1020->nav.p_sensitivity_key / FLOAT_MAX;

    input_report_rel(fpc1020->input_dev, REL_X, x);
    input_report_rel(fpc1020->input_dev, REL_Y, y);

    input_sync(fpc1020->input_dev);
}


/* -------------------------------------------------------------------- */
static void dispatch_touch_event(fpc1020_data_t* fpc1020, int x, int y, int finger_status)
{
    int sign_x, sign_y;
    int abs_x, abs_y;

    switch (finger_status)
    {
    case FNGR_ST_DETECTED:
    case FNGR_ST_TAP:
        break;

    case FNGR_ST_LOST:
        FP_LOG_NAV_TOUCH_DBG("[FPC/TOUCH] finger lost, nav_sum_x=%d,nav_sum_y=%d, filer=%d\n",
                             fpc1020->nav.sum_x, fpc1020->nav.sum_y, fpc1020->nav.throw_event);

        if (fpc1020->nav.sum_y > 17)
        {
            fpc1020->nav.move_direction = SLIDE_DOWN;
        }
        else if (fpc1020->nav.sum_y < -1)
        {
            fpc1020->nav.move_direction = SLIDE_UP;
        }
        if (fpc1020->nav.throw_event != 1)
        {
            if (fpc1020->nav.move_direction == SLIDE_DOWN)/*move down*/
            {
                FP_LOG_NAV_TOUCH_DBG("[FPC/TOUCH] report key down\n");
                dpad_report_key(fpc1020, KEY_DOWN, 1);
                fpc1020->nav.filter_key = 1;
            }
            else if (fpc1020->nav.move_direction == SLIDE_UP)
            {
                dpad_report_key(fpc1020, KEY_UP, 0);
                fpc1020->nav.filter_key = 1;
                FP_LOG_NAV_TOUCH_DBG("[FPC/TOUCH] report key up\n");
            }
        }
        fpc1020->nav.throw_event = 0;
        fpc1020->nav.move_direction = 0; /*1: slide down, 1: slide up, 0: init*/
        fpc1020->nav.sum_y = 0;
        fpc1020->nav.sum_x = 0;
        break;
    case FNGR_ST_DOUBLE_TAP:
        dpad_report_key(fpc1020, KEY_DELETE, 1);
        fpc1020->nav.throw_event = 1;
        mdelay(50);
        FP_LOG_NAV_TOUCH_DBG("[FPC/TOUCH] report double click\n");
        break;

    case FNGR_ST_HOLD:
        dpad_report_key(fpc1020, KEY_ENTER, 1);
        fpc1020->nav.throw_event = 1;
        mdelay(50);
        FP_LOG_NAV_TOUCH_DBG("[FPC/TOUCH] report long press\n");
        break;

    case FNGR_ST_MOVING:
        sign_x = x > 0 ? 1 : -1;
        sign_y = y > 0 ? 1 : -1; //reverse direction
        abs_x = x > 0 ? x : -x;
        abs_y = y > 0 ? y : -y;
        if (abs_x > fpc1020->nav.threshold_key_accel)
            x = (fpc1020->nav.threshold_key_accel
                 + ( abs_x - fpc1020->nav.threshold_key_accel) * fpc1020->nav.multiplier_key_accel) * sign_x;
        if (abs_y > fpc1020->nav.threshold_key_accel)
            y = (fpc1020->nav.threshold_key_accel
                 + (abs_y - fpc1020->nav.threshold_key_accel) * fpc1020->nav.multiplier_key_accel) * sign_y;
        x = x * fpc1020->nav.p_multiplier_x / FLOAT_MAX;
        y = y * fpc1020->nav.p_multiplier_y / FLOAT_MAX;
        x = x * fpc1020->nav.p_sensitivity_key / FLOAT_MAX;
        y = y * fpc1020->nav.p_sensitivity_key / FLOAT_MAX;

        FP_LOG_NAV_TOUCH_DBG("[FPC/TOUCH] %s:touch moving sum x,y(%d, %d)/sum_x,sum_y(%d,%d)\n",
                             __func__, x, y, fpc1020->nav.sum_x, fpc1020->nav.sum_y);

        fpc1020->nav.sum_x += x;
        fpc1020->nav.sum_y += y;
        break;

    default:
        FP_LOG_NAV_TOUCH_DBG("[FPC/TOUCH] : undefined gesture events\n");
        break;
    }
}


/* -------------------------------------------------------------------- */
static void process_navi_event(fpc1020_data_t* fpc1020, int dx, int dy, int finger_status)
{
    const int THRESHOLD_RANGE_TAP = 500000;
    const int THRESHOLD_RANGE_MIN_TAP = 200;
    /* const int THRESHOLD_RANGE_DOUBLE_TAP = 1600;*/
    const int THRESHOLD_RANGE_DOUBLE_TAP = 4500;
    //const unsigned long THRESHOLD_DURATION_TAP = 3000;//350;
    int filtered_finger_status = finger_status;
    static int deviation_x = 0;
    static int deviation_y = 0;
    int deviation;
    int deviation_x_ext;
    int deviation_y_ext;
    static unsigned long tick_down = 0;
    unsigned long tick_curr = jiffies * 1000 / HZ;
    unsigned long duration = 0;

    FP_LOG_INFO("[FPC] %s: finger_status:%d, dx=%d, dy=%d, tick_down:%d, tick_curr:%d \n",
                __func__, finger_status, dx, dy, tick_down, tick_curr);

    if (finger_status == FNGR_ST_DETECTED)
    {
        tick_down = tick_curr;
        fpc1020->nav.touch_change_x = 0;
        fpc1020->nav.touch_change_y = 0;
    }

    if (tick_down > 0)
    {
        duration = tick_curr - tick_down;

        deviation_x += dx;
        deviation_y += dy;
        deviation = deviation_x * deviation_x + deviation_y * deviation_y;

        FP_LOG_INFO("[FPC] %s: report click deviation[%d,%d]=%d, duration=%d, finger status=%d, throw =%d, double interval%d, zones=%d, maxzones=%d\n",
                    __func__, deviation_x, deviation_y, deviation, duration, fpc1020->nav.tap_status, fpc1020->nav.throw_event,
                    tick_curr - fpc1020->nav.tap_start,fpc1020->nav.last_zones, fpc1020->nav.max_zones);

        if (deviation > THRESHOLD_RANGE_TAP)
        {
            deviation_x = 0;
            deviation_y = 0;
            tick_down = 0;
            fpc1020->nav.tap_status = -1;

            FP_LOG_NAV_DBG("[FPC] %s:throw the events\n", __func__);

            if (duration > THRESHOLD_DURATION_TAP)//1500 1.5s
            {
                FP_LOG_NAV_DBG("[FPC] %s: prepare long press because of outside\n", __func__);
                filtered_finger_status = FNGR_ST_HOLD;// FNGR_ST_L_HOLD;
            }
        }
        else
        {
            if (duration < THRESHOLD_DURATION_TAP)
            {
                deviation_x_ext = ABS(fpc1020->nav.touch_down_x-fpc1020->nav.touch_up_x);
                deviation_y_ext = ABS(fpc1020->nav.touch_down_y-fpc1020->nav.touch_up_y);
                if ( finger_status == FNGR_ST_LOST )
                {
                    if (fpc1020->diag.tap_enable == 1 && fpc1020->nav.max_zones > 2 /*&& fpc1020->nav.nav_imgs > 3 */)
                    {
                        FP_LOG_NAV_DBG("[FPC]_click,d report changes=%d, abs=[%d,%d], throw=%d, imgs=%d\n",
                                       fpc1020->nav.touch_change_count,deviation_x_ext, deviation_y_ext,
                                       fpc1020->nav.throw_event, fpc1020->nav.nav_imgs);
                        if (fpc1020->nav.tap_status == FNGR_ST_TAP
                                && tick_curr - fpc1020->nav.tap_start <= THRESHOLD_DURATION_DTAP
                                && ((deviation <= THRESHOLD_RANGE_DOUBLE_TAP )|| fpc1020->nav.touch_change_count == 0 )
                                &&(deviation_x_ext < 14 && deviation_y_ext < 7)
                                && fpc1020->nav.throw_event == 0
                                &&duration > 20)
                        {
                            if (single_double_tap_cond > 2 || (fpc1020->nav.touch_change_count + fpc1020->nav.nav_imgs) > 2)
                            {
                                fpc1020->nav.tap_status = FNGR_ST_DOUBLE_TAP;
                                filtered_finger_status = FNGR_ST_DOUBLE_TAP;
                                //fpc1020->nav.detect_zones = 0;

                                FP_LOG_NAV_DBG("[FPC] %s:prepare report double click\n", __func__);

                                FP_LOG_NAV_DBG("[FPC] %s:prepare report double click, duration:%d\n",
                                               __func__, duration);

                            }
                            else
                            {
                                filtered_finger_status = FNGR_ST_LOST;
                                FP_LOG_NAV_DBG("[FPC] %s:Can't report Fake double click\n", __func__);
                            }

                            single_double_tap_cond = 0;
                        }
                        /*deviation_x_ext < 20 && deviation_y_ext < 20*/
                        else if (((((deviation <= THRESHOLD_RANGE_DOUBLE_TAP)&& deviation_x_ext < 10 && deviation_y_ext < 7) ||
                                   ((deviation_x_ext<=4&& deviation_y_ext <= 8)&&ABS(fpc1020->nav.max_zones-fpc1020->nav.last_zones)<=2) || ((deviation_y_ext<=1)&&deviation_x_ext<=1))/*case1*/
                                  && (duration < THRESHOLD_DURATION_CLICK)
                                  && (duration >= 20)
                                  && fpc1020->nav.throw_event == 0 && fpc1020->nav.touch_change_count < 10)
                                 /* ||*/ /*case 2*/ /*(fpc1020->nav.last_zones>1&&fpc1020->nav.throw_event == 0
                                  && duration < THRESHOLD_DURATION_FAST_CLICK && deviation == 0&&deviation_y_ext<7)*/)
                        {
                            FP_LOG_NAV_DBG("[FPC] report touch changes count x:%d, y:%d, touch[%d, %d; %d,%d]\n",
                                           fpc1020->nav.touch_change_x, fpc1020->nav.touch_change_y, fpc1020->nav.touch_down_x,
                                           fpc1020->nav.touch_down_y, fpc1020->nav.touch_up_x, fpc1020->nav.touch_up_y);
                            if (  (fpc1020->nav.touch_change_x <= 3)&&(fpc1020->nav.touch_change_y <= 3)
                                    && (((fpc1020->nav.touch_down_x > 39)&& (fpc1020->nav.touch_up_x == 47)) ||
                                        ((fpc1020->nav.touch_down_x == 47)&& (fpc1020->nav.touch_up_x > 39)) ||
                                        ((fpc1020->nav.touch_down_x == 5)&&(fpc1020->nav.touch_up_x < 13)) ||
                                        ((fpc1020->nav.touch_down_x < 13)&&(fpc1020->nav.touch_up_x == 5))
                                       ))
                                FP_LOG_NAV_DBG("[FPC] report invalid touch area\n");
                            else {
                                filtered_finger_status = FNGR_ST_TAP;
                                fpc1020->nav.tap_status = FNGR_ST_TAP;
                                fpc1020->nav.tap_start = tick_curr;
                                single_double_tap_cond = fpc1020->nav.touch_change_count + fpc1020->nav.nav_imgs;
                                FP_LOG_NAV_DBG("[FPC] %s:prepare report single click, duration:%d, single_double_tap_cond:%d\n", __func__,duration,  single_double_tap_cond);
                            }
                        }
                        else
                        {
                            FP_LOG_NAV_DBG("[FPC] %s: still report finger lost 1, cond_a:%d, cond_b:%d \n", __func__,
                                           (((deviation <= THRESHOLD_RANGE_DOUBLE_TAP)&& deviation_x_ext < 14 && deviation_y_ext < 7) ||
                                            ((deviation_x_ext<=4&& deviation_y_ext <= 8)&&ABS(fpc1020->nav.max_zones-fpc1020->nav.last_zones)<=2) || ((deviation_y_ext<=1)&&deviation_x_ext<=1)),
                                           (duration < THRESHOLD_DURATION_CLICK)&& (duration > 20)&& fpc1020->nav.throw_event == 0 && fpc1020->nav.touch_change_count < 10);

                            filtered_finger_status = FNGR_ST_LOST;
                        }
                        //tick_down = 0;
                    }
                    else /*fpc1020->nav.last_zones > 1*/
                    {
                        FP_LOG_NAV_DBG("[FPC] %s: still report finger lost 2\n", __func__);
                        filtered_finger_status = FNGR_ST_LOST;
                    }
                    deviation_x = 0;
                    deviation_y = 0;
                    fpc1020->nav.touch_change_count = 0;
                    fpc1020->nav.touch_change_x = 0;
                    fpc1020->nav.touch_change_y = 0;
                }
                else if ((deviation <= THRESHOLD_RANGE_MIN_TAP ||
                          ((dy == fpc1020->nav.move_pre_y)&& (dx==fpc1020->nav.move_pre_x)))
                         && duration > THRESHOLD_DURATION_HOLD)
                {
                    FP_LOG_NAV_DBG("[FPC] %s:inside  prepare report long press\n", __func__);
                    //if (deviation < THRESHOLD_RANGE_MIN_TAP)
                    filtered_finger_status = FNGR_ST_HOLD;// FNGR_ST_L_HOLD;
                    fpc1020->nav.tap_status = -1;
                    tick_down = 0;
                    deviation_x = 0;
                    deviation_y = 0;
                }
            }
            else if ((deviation <= THRESHOLD_RANGE_MIN_TAP ||
                      ((dy == fpc1020->nav.move_pre_y)&& (dx==fpc1020->nav.move_pre_x)))
                     && duration > THRESHOLD_DURATION_HOLD)
            {
                FP_LOG_NAV_DBG("[FPC] %s: prepare report long press\n", __func__);
                //if (deviation < THRESHOLD_RANGE_MIN_TAP)
                filtered_finger_status = FNGR_ST_HOLD;// FNGR_ST_L_HOLD;
                fpc1020->nav.tap_status = -1;
                tick_down = 0;
                deviation_x = 0;
                deviation_y = 0;
            }
            else
                FP_LOG_NAV_DBG("[FPC] ++++: tap_status:%d, [pre_dx:%d, pre_dy:%d], [dx:%d,dy:%d], deviation:%d,duration:%d, \n", fpc1020->nav.tap_status, fpc1020->nav.move_pre_x, fpc1020->nav.move_pre_y, dx, dy, deviation, duration);
            fpc1020->nav.move_pre_x = dx;
            fpc1020->nav.move_pre_y = dy;
        }

    }

    switch (fpc1020->nav.input_mode)
    {
    case FPC1150_INPUTMODE_TRACKPAD :
        dispatch_trackpad_event(fpc1020, dy, dx, filtered_finger_status);
        break;
    case FPC1150_INPUTMODE_DPAD:
        dispatch_dpad_event(fpc1020, dy, dx, filtered_finger_status);
        break;
    case FPC1150_INPUTMODE_TOUCH:
        dispatch_touch_event(fpc1020, dy, dx, filtered_finger_status);
        break;
    default:
        pr_info("[FPC]: undefined input mode\n");
        break;
    }
}

/* -------------------------------------------------------------------- */
int  fpc1020_input_init(fpc1020_data_t* fpc1020)
{
    int error = 0;
    int nav_image_width = nav_para.nav_img_w;
    int nav_image_height = nav_para.nav_img_h;

    if ((fpc1020->chip.type != FPC1020_CHIP_1020A)
            && (fpc1020->chip.type != FPC1020_CHIP_1021A)
            && (fpc1020->chip.type != FPC1020_CHIP_1021B)
            && (fpc1020->chip.type != FPC1020_CHIP_1021F)
            && (fpc1020->chip.type != FPC1020_CHIP_1150A)
            && (fpc1020->chip.type != FPC1020_CHIP_1150B)
            && (fpc1020->chip.type != FPC1020_CHIP_1150F))
    {
        dev_err(&fpc1020->spi->dev, "%s, chip not supported (%s)\n",
                __func__,
                fpc1020_hw_id_text(fpc1020));

        return -EINVAL;
    }

    dev_dbg(&fpc1020->spi->dev, "%s\n", __func__);

    fpc1020->input_dev = input_allocate_device();

    if (!fpc1020->input_dev)
    {
        dev_err(&fpc1020->spi->dev, "Input_allocate_device failed.\n");
        error  = -ENOMEM;
    }

    if (!error)
    {
        fpc1020->input_dev->name = FPC1020_TOUCH_PAD_DEV_NAME;

        /* Set event bits according to what events we are generating */
        set_bit(EV_KEY, fpc1020->input_dev->evbit);
        set_bit(EV_REL, fpc1020->input_dev->evbit);
        input_set_capability(fpc1020->input_dev, EV_REL, REL_X);
        input_set_capability(fpc1020->input_dev, EV_REL, REL_Y);
        //input_set_capability(fpc1020->input_dev, EV_KEY, BTN_MOUSE);
        input_set_capability(fpc1020->input_dev, EV_KEY, KEY_ENTER);
#if defined (SUPPORT_DOUBLE_TAP)
        input_set_capability(fpc1020->input_dev, EV_KEY, KEY_VOLUMEUP);
        input_set_capability(fpc1020->input_dev, EV_KEY, KEY_VOLUMEDOWN);
#endif
        input_set_capability(fpc1020->input_dev, EV_KEY, KEY_DELETE);
        input_set_capability(fpc1020->input_dev, EV_KEY, KEY_SLEEP);

        //set_bit(FPC1020_KEY_FINGER_PRESENT, fpc1020->input_dev->keybit);

        /* Register the input device */
        error = input_register_device(fpc1020->input_dev);
        if (error)
        {
            dev_err(&fpc1020->spi->dev, "Input_register_device failed.\n");
            input_free_device(fpc1020->input_dev);
            fpc1020->input_dev = NULL;
        }
    }

    fpc1020->touch_pad_dev = input_allocate_device();
    if (!fpc1020->touch_pad_dev)
    {
        dev_err(&fpc1020->spi->dev, "Input_allocate_device failed.\n");
        error  = -ENOMEM;
    }

    if (!error)
    {
        fpc1020->touch_pad_dev->name = FPC1020_INPUT_NAME;

        /* Set event bits according to what events we are generating */
        set_bit(EV_KEY, fpc1020->touch_pad_dev->evbit);
        set_bit(EV_ABS, fpc1020->touch_pad_dev->evbit);
        set_bit(BTN_TOUCH, fpc1020->touch_pad_dev->keybit);
        set_bit(ABS_X, fpc1020->touch_pad_dev->absbit);
        set_bit(ABS_Y, fpc1020->touch_pad_dev->absbit);
        set_bit(ABS_Z, fpc1020->touch_pad_dev->absbit);
        /*dpad*/
        /*double tap demo*/
        set_bit(KEY_UP, fpc1020->touch_pad_dev->keybit);
        set_bit(KEY_RIGHT, fpc1020->touch_pad_dev->keybit);
        set_bit(KEY_LEFT, fpc1020->touch_pad_dev->keybit);
        set_bit(KEY_DOWN, fpc1020->touch_pad_dev->keybit);
        set_bit(KEY_BACK, fpc1020->touch_pad_dev->keybit);
        set_bit(KEY_EXIT, fpc1020->touch_pad_dev->keybit);
        set_bit(KEY_DELETE, fpc1020->touch_pad_dev->keybit);
        input_set_capability(fpc1020->touch_pad_dev, EV_KEY, KEY_ENTER);
        input_set_capability(fpc1020->touch_pad_dev, EV_KEY, KEY_BACK);
        input_set_capability(fpc1020->touch_pad_dev, EV_KEY, KEY_EXIT);
        input_set_capability(fpc1020->touch_pad_dev, EV_KEY, KEY_DELETE);
        input_set_abs_params(fpc1020->touch_pad_dev, ABS_X, 0, DEVICE_WIDTH, 0, 0);
        input_set_abs_params(fpc1020->touch_pad_dev, ABS_Y, 0, DEVICE_HEIGHT, 0, 0);

        /* Register the input device */
        error = input_register_device(fpc1020->touch_pad_dev);

        if (error)
        {
            dev_err(&fpc1020->spi->dev, "Input_register_device failed.\n");
            input_free_device(fpc1020->touch_pad_dev);
            fpc1020->touch_pad_dev = NULL;
        }
    }

    if (!error)
    {
        if (fpc1020->chip.type == FPC1020_CHIP_1150A
                || fpc1020->chip.type == FPC1020_CHIP_1150B
                || fpc1020->chip.type == FPC1020_CHIP_1150F)
        {
            nav_image_width = nav_para.nav_img_h;
            nav_image_height = nav_para.nav_img_w;
        }
        /* sub area setup */
        fpc1020->nav.image_nav_row_start = ((fpc1020->chip.pixel_rows - nav_image_height) / 2);
        fpc1020->nav.image_nav_row_count = nav_image_height;
        fpc1020->nav.image_nav_col_start = ((fpc1020->chip.pixel_columns - nav_image_width) / 2) / fpc1020->chip.adc_group_size;
        fpc1020->nav.image_nav_col_groups = (nav_image_width + fpc1020->chip.adc_group_size - 1) / fpc1020->chip.adc_group_size;

        fpc1020->nav.input_mode = FPC1150_INPUTMODE_DPAD;
        init_enhanced_navi_setting(fpc1020);
        fpc1020->nav.throw_event = 0;
    }
    return error;
}


/* -------------------------------------------------------------------- */
void  fpc1020_input_destroy(fpc1020_data_t* fpc1020)
{
    dev_dbg(&fpc1020->spi->dev, "%s\n", __func__);

    if (fpc1020->input_dev != NULL)
    {
        input_free_device(fpc1020->input_dev);
    }
    if (fpc1020->touch_pad_dev != NULL)
    {
        input_free_device(fpc1020->touch_pad_dev);
    }
}

/* -------------------------------------------------------------------- */
void fpc1020_input_enable(fpc1020_data_t* fpc1020, bool enabled)
{
    dev_dbg(&fpc1020->spi->dev, "%s\n", __func__);

    fpc1020->nav.enabled = enabled;

    return ;
}

int fpc1020_subzrea_zones_sum(int zones)
{
    u8 count = 0;
    u16 mask = FPC1020_FINGER_DETECT_ZONE_MASK;
    if (zones < 0)
        return zones;
    else {
        zones &= mask;
        while (zones && mask) {
            count += (zones & 1) ? 1 : 0;
            zones >>= 1;
            mask >>= 1;
        }
    }
    // dev_dbg(&fpc1020->spi->dev, "%s %d zones\n", __func__, count);
    return (int)count;
}

/* -------------------------------------------------------------------- */
int fpc1020_input_task(fpc1020_data_t* fpc1020)
{
    int sub;
    int touch_x;
    int touch_y;
    u8 zones;
    u32 abs_x;
    u32 abs_y;
    //bool isReverse = false;
    int dx = 0;
    int dy = 0;
    int sumX = 0;
    int sumY = 0;
    int error = 0;
    //unsigned char* prevBuffer = NULL;
    //unsigned char* curBuffer = NULL;
    unsigned long diffTime = 0;

    dev_dbg(&fpc1020->spi->dev, "%s\n", __func__);
    error = fpc1150_write_nav_setup(fpc1020);
    if (fpc1020->diag.result || (fpc1020->finger_status == FPC1020_FINGER_DOWN)) {
        error = fpc1020_capture_wait_finger_up(fpc1020);
        if (0 == error)
            fpc1020->finger_status = FPC1020_FINGER_UP;
        dev_info(&fpc1020->spi->dev, "%s, finger_status = %d\n", __func__,fpc1020->finger_status);
    }

    while (!fpc1020->worker.stop_request &&
            fpc1020->nav.enabled && (error >= 0))
    {

        error = fpc1020_capture_nav_wait_finger_down(fpc1020);

        if (error < 0)
        {
            break;
        }

        fpc1020->nav.time = jiffies;
        error = fpc1020_check_finger_present_sum(fpc1020, &sub);
        /*get_finger_position(&touch_x, &touch_y, sub);*/
        get_finger_iposition(&touch_x, &touch_y, sub);
        if (sub == 0)
        {
            dev_info(&fpc1020->spi->dev, "%s,[%d], first invaild subarea:%d, continue!", __func__, __LINE__, sub);
            continue;
        } else {
            fpc1020->nav.touch_down_x = touch_x;
            fpc1020->nav.touch_down_y = touch_y;
        }
        error = fpc1150_write_nav_setup(fpc1020);
        if (error < 0)
        {
            break;
        }
        error = fpc1020_check_finger_present_raw(fpc1020);
        process_navi_event(fpc1020, 0, 0, FNGR_ST_DETECTED);//

        //fpc1020->nav.click_start = jiffies;
        zones = fpc1020_subzrea_zones_sum(error);
        FP_LOG_NAV_DBG("[FPC] down touch, first check subarea: sub=0x%x, x=%d, y=%d\n", sub, touch_x, touch_y);
        fpc1020->nav.detect_zones = zones;
        fpc1020->nav.last_zones = zones;

        fpc1020->nav.max_zones =0;

        error = capture_nav_image(fpc1020);
        if (error < 0)
        {
            break;
        }

        memcpy(fpc1020->prev_img_buf, fpc1020->huge_buffer, nav_para.nav_img_size);

        while (!fpc1020->worker.stop_request && (error >= 0))
        {
            error = fpc1020_check_finger_present_sum(fpc1020, &sub);
            if (error < fpc1020->setup.capture_finger_up_threshold + 1)
            {
                FP_LOG_NAV_DBG("[FPC] prepare report finger up, first, last_zones=%d, error:%d\n", fpc1020->nav.last_zones, error);
                process_navi_event(fpc1020, 0, 0, FNGR_ST_LOST);
                //fpc1020->nav.throw_event = 0;
                sumX = 0;
                sumY = 0;
                fpc1020->nav.time = 0;
                //isReverse = false;
                //fpc1020->nav.throw_event = 0;
                fpc1020->nav.nav_imgs = 0;
                //fpc1020->nav.touch_change_count = 0;
                fpc1020->nav.sumx  = 0;
                fpc1020->nav.sumy = 0;
                //dpad_report_key(fpc1020, fpc1020->nav.nav_dir_key[fpc1020->nav.key_code], 0);
                break;
            }
            if (error > fpc1020->nav.max_zones)
                fpc1020->nav.max_zones = error;

            fpc1020->nav.last_zones = error;
            fpc1020->nav.touch_pre_x = touch_x;
            fpc1020->nav.touch_pre_y = touch_y;
            /*get_finger_position(&touch_x, &touch_y, sub);*/
         get_finger_iposition(&touch_x, &touch_y, sub);
         if (sub == 0)
         {
             dev_info(&fpc1020->spi->dev, "%s,[%d], second invaild subarea:%d, continue!", __func__, __LINE__, sub);
         continue;
         } else {
            if (fpc1020->nav.touch_pre_x != touch_x || fpc1020->nav.touch_pre_y != touch_y)
                fpc1020->nav.touch_change_count++;
            if (fpc1020->nav.touch_pre_x != touch_x)  fpc1020->nav.touch_change_x++;
            if (fpc1020->nav.touch_pre_y != touch_y)   fpc1020->nav.touch_change_y++;
            fpc1020->nav.touch_up_x = touch_x;
            fpc1020->nav.touch_up_y = touch_y;
            FP_LOG_NAV_DBG("[FPC] second check subarea:  sub=0x%x, touch x=%d, y=%d\n",sub,touch_x, touch_y);//////////////
         }
            error = capture_nav_image(fpc1020);/*Get the capture image*/
            fpc1020->nav.nav_imgs++;
            if (error < 0)
            {
                break;
            }

            memcpy(fpc1020->cur_img_buf, fpc1020->huge_buffer, nav_para.nav_img_w * nav_para.nav_img_h);

            error = fpc1020_check_finger_present_sum(fpc1020, &sub);
            fpc1020->nav.detect_zones = error;
            if (error < fpc1020->setup.capture_finger_up_threshold + 1) /*subarea is 0, so finger is leave*/
            {
                FP_LOG_NAV_DBG("[FPC] prepare report finger up, second, last zones=%d, error:%d\n", fpc1020->nav.last_zones, error);
                process_navi_event(fpc1020, 0, 0, FNGR_ST_LOST);
                //fpc1020->nav.throw_event = 0;
                sumX = 0;
                sumY = 0;
                fpc1020->nav.time = 0;
                //isReverse = false;
                fpc1020->nav.nav_imgs = 0;
                fpc1020->nav.sumx = 0;
                fpc1020->nav.sumy = 0;
                //dpad_report_key(fpc1020, fpc1020->nav.nav_dir_key[fpc1020->nav.key_code], 0);
                break;
            }
            fpc1020->nav.touch_pre_x = touch_x;//////////////////15
            fpc1020->nav.touch_pre_y = touch_y;
            /*get_finger_position(&touch_x, &touch_y, sub);*/
            get_finger_iposition(&touch_x, &touch_y, sub);
            if (sub == 0)
            {
                dev_info(&fpc1020->spi->dev, "%s,[%d], third invaild subarea:%d, continue!", __func__, __LINE__, sub);
                continue;
            } else {
            fpc1020->nav.touch_up_x = touch_x;
            fpc1020->nav.touch_up_y = touch_y;
            if (fpc1020->nav.touch_pre_x != touch_x || fpc1020->nav.touch_pre_y != touch_y)
                  fpc1020->nav.touch_change_count++; 
            FP_LOG_NAV_DBG("[FPC] touch third check subarea:  sub=0x%x, x=%d, y=%d, touch_change_count:%d\n", sub, touch_x, touch_y,  fpc1020->nav.touch_change_count);
            }
            /*calculate the image, get the dx and dy*/
            /*printk("[FPC] Before calc, ts:%ul\n", jiffies * 1000 / HZ);*/
            fpc1150_get_movement(fpc1020->prev_img_buf, fpc1020->cur_img_buf, &dx, &dy);
            /*printk("[FPC] After calc, ts:%ul\n", jiffies * 1000 / HZ);*/

            memcpy(fpc1020->prev_img_buf, fpc1020->cur_img_buf, nav_para.nav_img_size);

            FP_LOG_NAV_DBG("[FPC] report get_movement dx=%d, dy=%d...original, zones=%d\n", dx, dy, fpc1020->nav.detect_zones);
#if 0
            if ((dx != 0 || dy != 0)&&fpc1020->nav.sumx == 0 && fpc1020->nav.sumy == 0 /*&& fpc1020->nav.nav_imgs > 5*/) {//??
                fpc1020->nav.sumx += dx;
                fpc1020->nav.sumy += dy;
                dx = 0;
                dy = 0;
            }
            if (dx !=0 || dy != 0) {
                fpc1020->nav.sumx += dx;
                fpc1020->nav.sumy += dy;
            }

            /*workaround to filter data*/
            abs_x = dx > 0 ? dx:-dx;
            abs_y = dy > 0 ? dy:-dy;

            //if (dx != 0 && dy != 0 && fpc1020->nav.detect_zones <= 3) {
            //fpc1020->nav.poll_filter++;
            //}
            //fpc1020->nav.poll_counter++;
            if ((dx!=0 || dy!=0) && fpc1020->nav.detect_zones < FPC1020_NAV_ALGO_SUBZONES)
            {
                FP_LOG_NAV_DBG("[FPC] report get movement 1 reset@%d\n",__LINE__);
                dx = 0;
                dy = 0;
            }
            /* if ((fpc1020->nav.detect_zones == FPC1020_NAV_ALGO_SUBZONES || fpc1020->nav.detect_zones == FPC1020_NAV_ALGO_SUBZONES+1)
                 &&(abs_x>10 && abs_y>10)) {//3//reset
                 dx = 0;
                 dy = 0;
                 FP_LOG_NAV_DBG("[FPC] report get movement 2 reset@%d\n",__LINE__);
             }*/
            if (abs_x != 0 && abs_y != 0 && abs_x == abs_y)
            {
                FP_LOG_NAV_DBG("[FPC] report get movement 3 reset@%d\n",__LINE__);
                dx = 0;
                dy = 0;
            }

            if ((abs_x != 0 || abs_y != 0) && fpc1020->nav.nav_imgs <= 2)
            {
                FP_LOG_NAV_DBG("[FPC] report get movement 4 reset@%d\n",__LINE__);
                dx = 0;
                dy = 0;
            }
#endif

            //sumX += dx;
            //sumY += dy;

            FP_LOG_NAV_DBG("[FPC] report get_movement dx=%d, dy=%d, raw sumx=%d, raw sumy=%d, imgs=%d\n",
                           dx, dy, fpc1020->nav.sumx, fpc1020->nav.sumy, fpc1020->nav.nav_imgs);


            //dx = 0;
            //dy = 0;
            diffTime = abs(jiffies - fpc1020->nav.time);
            if (diffTime > 0)
            {
                diffTime = diffTime * 1000000 / HZ;

                if (diffTime >= FPC1150_INPUT_POLL_INTERVAL)
                {

                    process_navi_event(fpc1020, dx, dy, FNGR_ST_MOVING);
                    //dev_info(&fpc1020->spi->dev, "[INFO] nav finger moving. sumX = %d, sumY = %d\n", sumX, sumY);
                    sumX = 0;
                    sumY = 0;

                    fpc1020->nav.time = jiffies;
                }
            }
        }
    }

    if (error < 0)
    {
        dev_err(&fpc1020->spi->dev,
                "%s %s (%d)\n",
                __func__,
                (error == -EINTR) ? "TERMINATED" : "FAILED", error);
    }
    atomic_set(&fpc1020->taskstate, fp_UNINIT);
    fpc1020->nav.enabled = false;
    return error;
}

static int fpc1150_write_nav_setup(fpc1020_data_t* fpc1020)
{
    const int mux = 2;
    int error = 0;
    u16 temp_u16;
    u32 temp_u32;
    u8 temp_u8;
    fpc1020_reg_access_t reg;

    dev_err(&fpc1020->spi->dev, "%s %d\n", __func__, mux);

    error = fpc1020_wake_up(fpc1020);
    if (error)
    {
        goto out;
    }

    error = fpc1020_nav_write_sensor_setup(fpc1020);
    if (error)
    {
        goto out;
    }

    /*this value is set by test, can't be change*/
#if 0
    temp_u16 = 2;
    temp_u16 <<= 8;
    temp_u16 |= 15;
#else
    temp_u16 = 4;
    temp_u16 <<= 8;
    temp_u16 |= 10;
#endif

    FPC1020_MK_REG_WRITE(reg, FPC102X_REG_ADC_SHIFT_GAIN, &temp_u16);
    error = fpc1020_reg_access(fpc1020, &reg);
    if (error)
    {
        goto out;
    }

    //temp_u16 = fpc1020->setup.pxl_ctrl[mux];
    temp_u16 = 0x0F0E;
    FPC1020_MK_REG_WRITE(reg, FPC102X_REG_PXL_CTRL, &temp_u16);
    error = fpc1020_reg_access(fpc1020, &reg);
    if (error)
    {
        goto out;
    }

    error = fpc1020_capture_set_crop(fpc1020,
                                     nav_para.nav_img_crop_x/ fpc1020->chip.adc_group_size,
                                     nav_para.nav_img_crop_w / fpc1020->chip.adc_group_size,
                                     nav_para.nav_img_crop_y, nav_para.nav_img_crop_h);
    if (error)
        return error;

    // Setup skipping of rows
    switch (nav_para.nav_img_row_skip) {
    case 1:
        temp_u32 = 0;
        break;
    case 2:
        temp_u32 = 1;
        break;
    case 4:
        temp_u32 = 2;
        break;
    case 8:
        temp_u32 = 3;
        break;
    default:
        error = -EINVAL;
        break;
    }
    if (error)
        return error;

    temp_u32 <<= 8;

    // Setup masking of columns
    switch (nav_para.nav_img_col_mask) {
    case 1:
        temp_u32 |= 0xff;
        break;
    case 2:
        temp_u32 |= 0xcc;
        break;
    case 4:
        temp_u32 |= 0x88;
        break;
    case 8:
        temp_u32 |= 0x80;
        break;
    default:
        error = -EINVAL;
        break;
    }
    if (error)
        return error;

    //temp_u32 <<= 8;
    //temp_u32 |= 0xFF;
    temp_u32 <<= 8;
    temp_u32 |= 0; // No multisampling
    FPC1020_MK_REG_WRITE(reg, FPC102X_REG_IMG_SMPL_SETUP, &temp_u32);
    error = fpc1020_reg_access(fpc1020, &reg);
    if (error)
        return error;

    temp_u8 = 0x04;
    FPC1020_MK_REG_WRITE(reg, FPC1150_REG_FNGR_DET_THRES, &temp_u8);
    error = fpc1020_reg_access(fpc1020, &reg);
    if (error)
        return error;

    dev_err(&fpc1020->spi->dev, "%s, (%d, %d, %d, %d)\n", __func__,
            fpc1020->nav.image_nav_col_start, fpc1020->nav.image_nav_col_groups,
            fpc1020->nav.image_nav_row_start, fpc1020->nav.image_nav_row_count);

out:
    return error;
}


/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
void fpc1020_rotate_image(fpc1020_data_t* fpc1020,
                          int sensor_width,
                          int sensor_height)
{
    u8* rotateBuffer;
    u8* curr_buffer;
    int dx = 0;
    int dy = 0;
    size_t image_size_bytes = sensor_width * sensor_height;

    curr_buffer = fpc1020->huge_buffer;
    rotateBuffer = kzalloc(image_size_bytes * sizeof(u8), GFP_KERNEL);
    for (dx = 0; dx < sensor_width; dx++)
    {
        for (dy = 0; dy < sensor_height; dy++)
        {
            rotateBuffer[dx + (dy * sensor_width)] = curr_buffer[(sensor_width - dx - 1) * sensor_height + dy];
        }
    }
    memcpy(curr_buffer, &rotateBuffer[0], image_size_bytes);
    kfree(rotateBuffer);
}


static int fpc1020_check_image(fpc1020_data_t* fpc1020,
                               int sensor_width,
                               int sensor_height)
{
    u8* curr_buffer;
    u32 count;
    int dx;
    int dy;

    curr_buffer = fpc1020->huge_buffer;
    count = 0;
    for (dx = 0; dx < sensor_width; dx++)
    {
        for (dy = 0; dy < sensor_height; dy++)
        {
            if (curr_buffer[dx + (dy * sensor_width)] == 255)
                count ++;
        }
    }
    return count;
}


/* -------------------------------------------------------------------- */
static int capture_nav_image(fpc1020_data_t* fpc1020)
{
    int error = 0;


    error = fpc1020_capture_buffer(fpc1020,
                                   fpc1020->huge_buffer,
                                   0,
                                   nav_para.nav_img_size);
    //printk("[FPC] capture_nav_image report grey count =%d\n",
    //        fpc1020_check_image(fpc1020,nav_para.nav_img_w,nav_para.nav_img_h));

    return error;
}

#endif
