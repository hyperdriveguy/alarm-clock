#include <lvgl.h>
#include "ntp_clock64.h"

extern NTPClock64 rtclock;

lv_obj_t *clock_label;

void updateClockDisplay() {
    ClockDateTime now = rtclock.nowSplit();
    char buf[16];
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d", now.hour, now.minute, now.second);
    lv_label_set_text(clock_label, buf);
}

void createClockFace() {
    clock_label = lv_label_create(lv_scr_act());
    lv_obj_align(clock_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_font(clock_label, LV_FONT_DEFAULT, LV_PART_MAIN);
    updateClockDisplay();
}
