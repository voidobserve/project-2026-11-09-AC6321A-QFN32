/* RGB PWM与单路WS2812底层驱动：引脚保持旧产品硬件定义。 */
#include "led_strip_sys.h"
#include "led_strip_drive.h"
#include "asm/adc_api.h"
#include "asm/mcpwm.h"
#include "led_strand_effect.h"

MIC_OFFON MIC_ENABLE;
static u8 ws2812_power_locked;
static u8 ws2812_power_enabled;

const struct ledc_platform_data ledc_data = {
    .index = 0,
    .port = LEDC_PIN,
    .idle_level = 0,
    .out_inv = 0,
    .bit_inv = 1,
    .t_unit = t_42ns,
    .t1h_cnt = 21,
    .t1l_cnt = 8,
    .t0h_cnt = 10,
    .t0l_cnt = 20,
    .t_rest_cnt = 20000,
    .cbfun = NULL,
};

void ws2812_power_on(void)
{
    if (ws2812_power_locked) {
        return;
    }
    gpio_direction_output(WS2812_POWER_PIN, 1);
    ws2812_power_enabled = 1;
}

void ws2812_power_off(void)
{
    gpio_direction_output(WS2812_POWER_PIN, 0);
    ws2812_power_enabled = 0;
}

void ws2812_power_lock_off(void)
{
    ws2812_power_locked = 1;
    ws2812_power_off();
}

void ws2812_power_unlock(void)
{
    ws2812_power_locked = 0;
    ws2812_power_on();
}

u8 ws2812_power_is_enabled(void)
{
    return ws2812_power_enabled && !ws2812_power_locked;
}

void led_state_init(void)
{
    ledc_init(&ledc_data);
}

void led_gpio_init(void)
{
    gpio_set_die(IO_PORTA_01, 1);
    gpio_direction_output(IO_PORTA_01, 0);
    gpio_set_die(IO_PORTA_02, 1);
    gpio_direction_output(IO_PORTA_02, 0);
    gpio_set_die(IO_PORTB_05, 1);
    gpio_direction_output(IO_PORTB_05, 0);

    gpio_set_die(WS2812_POWER_PIN, 1);
    gpio_set_pull_down(WS2812_POWER_PIN, 0);
    gpio_set_pull_up(WS2812_POWER_PIN, 0);
    ws2812_power_locked = 0;
    ws2812_power_off();
}

void led_pwm_init(void)
{
    struct pwm_platform_data pwm_p_data;

    pwm_p_data.pwm_aligned_mode = pwm_edge_aligned;
    pwm_p_data.pwm_ch_num = pwm_ch0;
    pwm_p_data.frequency = 1000;
    pwm_p_data.duty = 0;
    pwm_p_data.h_pin = IO_PORTA_01;
    pwm_p_data.l_pin = -1;
    pwm_p_data.complementary_en = 0;
    mcpwm_init(&pwm_p_data);

    pwm_p_data.pwm_ch_num = pwm_ch1;
    pwm_p_data.h_pin = IO_PORTA_02;
    mcpwm_init(&pwm_p_data);

    pwm_p_data.pwm_ch_num = pwm_ch2;
    pwm_p_data.h_pin = IO_PORTB_05;
    mcpwm_init(&pwm_p_data);
}

/* 硬件接线为GRB，保持旧工程的R/G交换关系。白光由调用方传入相等的RGB值实现。 */
void fc_driver(u8 r, u8 g, u8 b)
{
    u32 duty_r = r * 10000 / 255;
    u32 duty_g = g * 10000 / 255;
    u32 duty_b = b * 10000 / 255;

    mcpwm_set_duty(pwm_ch0, duty_g);
    mcpwm_set_duty(pwm_ch1, duty_r);
    mcpwm_set_duty(pwm_ch2, duty_b);
}
