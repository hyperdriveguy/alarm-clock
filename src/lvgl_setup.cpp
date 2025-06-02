#include <lvgl.h>
#include <LovyanGFX.hpp>
#include <Ticker.h>
#include <Adafruit_FT6206.h>

#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 240
#define TOUCH_THRESHOLD 40

static constexpr int8_t TOUCH_SDA = 1;
static constexpr int8_t TOUCH_SCL = 2;

Adafruit_FT6206 ft6206;

class LGFX : public lgfx::LGFX_Device {
public:
    lgfx::Panel_ILI9341 panel;
    lgfx::Bus_SPI bus;
    lgfx::Light_PWM backlight;

    LGFX() {
        {
            auto cfg = bus.config();
            cfg.spi_host = SPI2_HOST;
            cfg.spi_mode = 0;
            cfg.freq_write = 80000000;
            cfg.pin_sclk = 6;
            cfg.pin_mosi = 7;
            cfg.pin_miso = 5;
            cfg.pin_dc = 8;
            // Removed cfg.pin_cs as it is not supported.
            bus.config(cfg);
            panel.setBus(&bus);
        }

        {
            auto cfg = backlight.config();
            cfg.pin_bl = 9;
            cfg.invert = false;
            cfg.freq = 5000;
            cfg.pwm_channel = 7;
            backlight.config(cfg);
            panel.setLight(&backlight);
        }

        setPanel(&panel);
        {
            auto panel_cfg = panel.config();
            panel_cfg.pin_rst = 14;
            panel_cfg.pin_cs = 18;
            panel.config(panel_cfg);
        }
    } // Added missing closing brace for constructor
};

LGFX tft;

void touch_read_cb(lv_indev_drv_t *drv, lv_indev_data_t *data) {
    if (ft6206.touched()) {
        TS_Point p = ft6206.getPoint();
        data->state = LV_INDEV_STATE_PR;
        data->point.x = SCREEN_WIDTH - p.y;
        data->point.y = p.x;
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}

void lvgl_setup() {
    lv_init();
    tft.begin();
    tft.setBrightness(128);  // Mid-brightness

    static lv_disp_draw_buf_t draw_buf;
    static lv_color_t buf[SCREEN_WIDTH * 10];
    lv_disp_draw_buf_init(&draw_buf, buf, nullptr, SCREEN_WIDTH * 10);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.flush_cb = [](lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p) {
        tft.startWrite();
        tft.setAddrWindow(area->x1, area->y1, area->x2 - area->x1 + 1, area->y2 - area->y1 + 1);
        tft.writePixels((lgfx::rgb565_t *)&color_p->full, (area->x2 - area->x1 + 1) * (area->y2 - area->y1 + 1));
        tft.endWrite();
        lv_disp_flush_ready(drv);
    };
    disp_drv.draw_buf = &draw_buf;
    disp_drv.hor_res = SCREEN_WIDTH;
    disp_drv.ver_res = SCREEN_HEIGHT;
    lv_disp_drv_register(&disp_drv);

    if (!ft6206.begin(TOUCH_THRESHOLD)) {
        Serial.println("Touch init failed!");
    } else {
        static lv_indev_drv_t indev_drv;
        lv_indev_drv_init(&indev_drv);
        indev_drv.type = LV_INDEV_TYPE_POINTER;
        indev_drv.read_cb = touch_read_cb;
        lv_indev_drv_register(&indev_drv);
    }
}
