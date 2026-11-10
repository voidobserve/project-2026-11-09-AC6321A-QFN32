#ifndef LED_STRIP_DRIVE_H
#define LED_STRIP_DRIVE_H

#include "board_ac632n_demo_cfg.h"
#include "asm/ledc.h"
#include "asm/gpio.h"

/* 旧硬件定义：一条16颗WS2812，数据脚PA8，IO_DM控制灯带电源。 */
#define LEDC_PIN                IO_PORTA_08
#define WS2812_POWER_PIN        IO_PORT_DM

typedef enum {
    OFF,
    ON,
} MIC_OFFON;

extern MIC_OFFON MIC_ENABLE;

void led_state_init(void);
void led_gpio_init(void);
void led_pwm_init(void);
void ledc_init(const struct ledc_platform_data *arg);

/* WS2812电源控制。lock_off用于兼容旧协议0x12的强制断电语义。 */
void ws2812_power_on(void);
void ws2812_power_off(void);
void ws2812_power_lock_off(void);
void ws2812_power_unlock(void);
u8 ws2812_power_is_enabled(void);

#endif
