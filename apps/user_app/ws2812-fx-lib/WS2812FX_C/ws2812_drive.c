#include "app_config.h"
#include "jiffies.h"
#include "ws2812_bsp.h"
#include "debug.h"
#include "my_effect.h"
#include "led_strand_effect.h"
#include "led_strip_drive.h"

extern void ledc_send_rgbbuf_isr(u8 index, u8 *rgbbuf, u32 led_num, u16 again_cnt);

#define WS2812_PIXEL_COUNT     USER_WS2812_SEGMENT_1_LEN
#define WS2812_BYTE_COUNT      (WS2812_PIXEL_COUNT * 3)
#define WS2812_LED_BUF_SEC     __attribute__((section(".ws2812_led_buf"), aligned(4)))

static u8 led_val_buf[WS2812_BYTE_COUNT] WS2812_LED_BUF_SEC;

void ws281x_init(void)
{
}

/* pixels_pattern第0颗是RGB PWM逻辑占位；PA8只发送紧随其后的16颗数据。 */
void ws281x_show(unsigned char *pixels_pattern, unsigned short pattern_size)
{
    u16 i;
    u8 rgb_value[3] = {0, 0, 0};
    extern void fc_driver(u8 r, u8 g, u8 b);

    if (fc_effect.rgbw_on_off_flag != DEVICE_OFF && pattern_size >= 3) {
        rgb_value[0] = (pixels_pattern[0] * fc_effect.static_mode_brightness) >> 3;
        rgb_value[1] = (pixels_pattern[1] * fc_effect.static_mode_brightness) >> 3;
        rgb_value[2] = (pixels_pattern[2] * fc_effect.static_mode_brightness) >> 3;
    }
    fc_driver(rgb_value[0], rgb_value[1], rgb_value[2]);

    if (fc_effect.led_on_off_flag == DEVICE_ON) {
        ws2812_power_on();
        if (!ws2812_power_is_enabled()) {
            return;             /* 0x12强制断电锁定期间禁止任何LEDC数据输出 */
        }

        memset(led_val_buf, 0, sizeof(led_val_buf));
        if (pattern_size >= (WS2812_BYTE_COUNT + 3)) {
            if (fc_effect.Now_state == IS_STATIC) {
                for (i = 0; i < WS2812_BYTE_COUNT; i++) {
                    led_val_buf[i] = (pixels_pattern[3 + i] * fc_effect.static_mode_brightness) >> 3;
                }
            } else {
                memcpy(led_val_buf, pixels_pattern + 3, WS2812_BYTE_COUNT);
            }
        }
        ledc_send_rgbbuf_isr(0, led_val_buf, WS2812_PIXEL_COUNT, 0);
    } else if (ws2812_power_is_enabled()) {
        /* 正常关灯先发送黑帧，再切断IO_DM供电。 */
        memset(led_val_buf, 0, sizeof(led_val_buf));
        ledc_send_rgbbuf_isr(0, led_val_buf, WS2812_PIXEL_COUNT, 0);
        ws2812_power_off();
    }
}

unsigned long HAL_GetTick(void)
{
    return jiffies_msec();
}

void run_tick_per_10ms(void)
{
}
