/*----------------------------------------------------------------------------*/
/* This file sets up LVGL; we leave most defaults. Only adjust what’s needed. */
/*----------------------------------------------------------------------------*/
#ifndef LV_CONF_H
#define LV_CONF_H

/* Disable console logs (for flash savings) */
#define LV_LOG_LEVEL    LV_LOG_LEVEL_ERROR

/* Input device settings: only one touchpad */
#define LV_INDEV_DEF_READ_PERIOD 30
#define LV_INDEV_DEF_DRAG_LIMIT   10

/* Use Monochrome 1BIT buffer for small RAM footprint */
#define LV_COLOR_DEPTH     16
#define LV_COLOR_16_SWAP   0

/* Display resolution */
#define LV_HOR_RES_MAX     240
#define LV_VER_RES_MAX     320

// This include is broken.
// #include <lv_conf_checker.h>
#endif
