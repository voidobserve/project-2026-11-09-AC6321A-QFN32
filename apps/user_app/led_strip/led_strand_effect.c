/****************************
@led_strand_effect.c
适用：
产品ID: yxwh27s5
品类：幻彩户外串灯-蓝牙
协议：BLE
负责幻彩灯串效果制作
*****************************/
#include "system/includes.h"
#include "led_strand_effect.h"
#include "WS2812FX.H"
#include "ws2812fx_effect.h"
#include "Adafruit_NeoPixel.H"
#include "tuya_ble_type.h"
#include "asm/mcpwm.h"
#include "ble_trans_profile.h"
#include "btstack/le/att.h"

// #define PC_SERIAL_DEBUG

extern void printf_buf(u8 *buf, u32 len);
static void static_mode(void);
static void ls_scene_effect(void);
void fc_set_style_custom(void);
static void strand_rainbow(void);
void jump_mutil_c(void) ;
void strand_breath(void) ;
void strand_twihkle(void) ;
void strand_flow_water(void) ;
void strand_chas_light(void) ;
void strand_colorful(void) ;
void mutil_seg_grandual(void) ;
void mutil_c_grandual(void);
void ls_set_colors(uint8_t n, color_t *c);
void ls_set_colors2(uint8_t n, color_t *c);
void soft_turn_off_lights(void); //软关灯处理
void soft_turn_on_the_light(void);   //软开灯处理

extern void fb_rf433_music_on_off(u8 on_off);

fc_effect_t fc_effect;//幻彩灯串效果数据

//-------------------------------数组不能修改---------------------------//
// 和通信协议对应
u8 rgb_sequence_map[6]=
{
    NEO_RGB,
    NEO_RBG,
    NEO_GRB,
    NEO_GBR,
    NEO_BRG,
    NEO_BGR,
};

#define MAX_STATIC_N    11
// 静态效果颜色map
const u32 fc_static_map[MAX_STATIC_N] =
{
    RED,    //0
    GREEN,  //1
    BLUE,   //2
    // WHITE,  //3
    0xEEEEEE,  //3-MT4321降功率，将白色改低RGB值
    YELLOW, //4
    CYAN,   //5
    MAGENTA,//6
    PURPLE, //7
    ORANGE, //8
    PINK,   //9
    GRAY,   //10
};

//------------------------------------------------------------------------//





// 效果数据初始化
void fc_data_init(void)
{

    fc_effect.on_off_flag = DEVICE_OFF;//总开关
    fc_effect.rgbw_on_off_flag = DEVICE_OFF; //七彩灯开关
    fc_effect.led_on_off_flag = DEVICE_OFF; //幻彩灯开关
    fc_effect.led1_on_off_flag = DEVICE_OFF; //幻彩灯开关
    
    fc_effect.last_rgbw_on_off_flag = DEVICE_ON;
    fc_effect.last_led_on_off_flag = DEVICE_ON;
    fc_effect.last_led1_on_off_flag = DEVICE_ON;

    fc_effect.led_num_max = (1 + USER_WS2812_SEGMENT_1_LEN + USER_WS2812_SEGMENT_2_LEN);  //第一路幻彩 rgb+19+60个幻彩灯
    fc_effect.led_num = (1 + USER_WS2812_SEGMENT_1_LEN + USER_WS2812_SEGMENT_2_LEN);  //第一路幻彩 rgb+19+60个幻彩灯

    fc_effect.Now_state = IS_light_music;
    fc_effect.custom_index = 10;
    fc_effect.rgbw.r = 0;
    fc_effect.rgbw.g = 0;
    fc_effect.rgbw.b = 0;

    fc_effect.dream_scene.speed = 100;
    fc_effect.sequence = NEO_GRB; //幻彩这个初始才，R是255时，才亮红光
    fc_effect.b = USER_BRIGHTNESS_MAX;
    fc_effect.static_mode_brightness = 8;
    fc_effect.speed = 80;
    fc_effect.music.m = 8;
    fc_effect.music.s = 100;   //灵敏度

    fc_effect.color_i = 6; /* 旧动态循环：0~5效果，6~7音乐 */
    fc_effect.huancai_color_i = 0;
    fc_effect.rgbw_i = 0;

    /* VM布局占位字段：功能失能时也初始化，避免保存未定义数据。 */
    fc_effect.ex_white_light_power = 0;
    fc_effect.ex_white_light_level = WHITE_LIGHT_MAX_LEVEL;
}

#ifdef PC_SERIAL_DEBUG
void pc_serial_debug_next_ws2812_mode(void *priv)
{
    uart_key_handle((uint8_t[]) { 0xEA, 0x02, 0x05, 0x07, 0xFF }, 5);
    
    sys_timeout_add(NULL, pc_serial_debug_next_ws2812_mode, 5000);
}
void pc_serial_debug_switch_ws2812_power(void *priv)
{
    static uint8_t power = 0;

    power ^= 1;
    if (power == 1) {
        soft_turn_on_the_light();
    } else {
        soft_turn_off_lights();
    }
}
void pc_serial_debug_poweron(void *priv)
{
    // soft_turn_on_the_light();
    // uart_key_handle((uint8_t[]) { 0xEA, 0x02, 0x05, 0x07, 0xFF }, 5);
    // uart_key_handle((uint8_t[]) { 0xEA, 0x02, 0x05, 0x07, 0xFF }, 5);

    // sys_timeout_add(NULL, pc_serial_debug_next_ws2812_mode, 5000);

    sys_timeout_add(NULL, pc_serial_debug_switch_ws2812_power, 5000);
}
#endif

//上电初始化
void full_color_init(void)
{

    WS2812FX_init(fc_effect.led_num, fc_effect.sequence);
    WS2812FX_setBrightness(fc_effect.b);

    /* 保持旧产品的断电前总开关与灯效状态。 */
    if (fc_effect.on_off_flag == DEVICE_ON) {
        soft_turn_on_the_light();
    } else {
        soft_turn_off_lights();
    }

#ifdef PC_SERIAL_DEBUG
    sys_timeout_add(NULL, pc_serial_debug_poweron, 1000);
#endif
}




/**
 * @brief 向app反馈灯开关状态
 *
 */
void fb_led_on_off_state(void)
{
    uint8_t Send_buffer[6];
    Send_buffer[0] = 0x01;
    Send_buffer[1] = 0x01;
    Send_buffer[2] = fc_effect.on_off_flag; //
    extern void zd_fb_2_app(u8 *p, u8 len);
    zd_fb_2_app(Send_buffer, 3);


}

/**************************************************软件开机*****************************************************/
// u8 ledreset_f = 0;

void soft_turn_on_the_light(void)   //软开灯处理
{
    fc_effect.on_off_flag = DEVICE_ON;
    // fc_effect.rgbw_on_off_flag = DEVICE_ON;
    // fc_effect.led_on_off_flag = DEVICE_ON; //幻彩灯开关
    // fc_effect.led1_on_off_flag = DEVICE_ON; //幻彩灯开关
    
    // 恢复主机关机前记录的状态
    printf("rgbw on_off: %d -> %d\n", fc_effect.rgbw_on_off_flag, fc_effect.last_rgbw_on_off_flag);
    printf("led  on_off: %d -> %d\n", fc_effect.led_on_off_flag, fc_effect.last_led_on_off_flag);
    printf("led1 on_off: %d -> %d\n", fc_effect.led1_on_off_flag, fc_effect.last_led1_on_off_flag);
    fc_effect.rgbw_on_off_flag = fc_effect.last_rgbw_on_off_flag;
    fc_effect.led_on_off_flag = fc_effect.last_led_on_off_flag;
    fc_effect.led1_on_off_flag = DEVICE_OFF; /* 旧产品无第二路灯带 */

    if (fc_effect.led_on_off_flag == DEVICE_ON) {
        ws2812_power_unlock();
    }
    WS2812FX_start();

    // ledreset_f = 2;  // 在aws_rgb_mode_update()内会先跑开机灯光效果，完成后会自动调用当前的幻彩灯效果
    aws_rgb_mode_update(2);  // 更新灯光+跑开机灯光效果

    printf("soft_turn_on_the_light\n");

    if (fc_effect.led_on_off_flag == DEVICE_ON)
    {
        if (fc_effect.Now_state == IS_light_music)
        {
            fb_rf433_music_on_off(1);
        }
    }

}

ON_OFF_FLAG get_on_off_state(void)
{
    return fc_effect.on_off_flag;
}

void set_on_off_led(u8 on_off)
{
    fc_effect.on_off_flag = on_off;

    if(fc_effect.on_off_flag == DEVICE_ON)
    {
        soft_turn_on_the_light();
    }
    else
    {
        soft_turn_off_lights();
    }
}



/***************************************************软件关机*****************************************************/
void soft_turn_off_lights(void) //软关灯处理
{
    if (fc_effect.on_off_flag == DEVICE_ON)
    {   // 记录主机关机前状态
        fc_effect.last_rgbw_on_off_flag = fc_effect.rgbw_on_off_flag;
        fc_effect.last_led_on_off_flag = fc_effect.led_on_off_flag;
        fc_effect.last_led1_on_off_flag = fc_effect.led1_on_off_flag;
    }

    fc_effect.on_off_flag = DEVICE_OFF;
    fc_effect.rgbw_on_off_flag = DEVICE_OFF;
    fc_effect.led_on_off_flag = DEVICE_OFF; //幻彩灯开关
    fc_effect.led1_on_off_flag = DEVICE_OFF; //幻彩灯开关
    fb_rf433_music_on_off(0);

    printf("rgbw on_off: %d -> %d\n", fc_effect.last_rgbw_on_off_flag, fc_effect.rgbw_on_off_flag);
    printf("led  on_off: %d -> %d\n", fc_effect.last_led_on_off_flag, fc_effect.led_on_off_flag);
    printf("led1 on_off: %d -> %d\n", fc_effect.last_led1_on_off_flag, fc_effect.led1_on_off_flag);

    WS2812FX_stop();
    ws2812_power_off();
    printf("soft_turn_off_light!!\n");
}



/*************************************************** 常规的七彩灯具效果 *****************************************************/
/*静态，跳变，渐变，呼吸，频闪*/
//-----------------------------------------------------------------------------------------静态效果
// app指令，模式中的静态效果
void fc_static_effect(u8 n)
{
    fc_effect.Now_state = IS_STATIC;

   fc_effect.dream_scene.rgb[0].r = (fc_static_map[n] >> 16) & 0xff;
   fc_effect.dream_scene.rgb[0].g = (fc_static_map[n] >> 8) & 0xff;
   fc_effect.dream_scene.rgb[0].b = (fc_static_map[n] ) & 0xff;


    set_fc_effect();
}
//app指令，色盘的颜色指令
void set_static_mode(u8 r, u8 g, u8 b)
{
    fc_effect.Now_state = IS_STATIC;
    fc_effect.dream_scene.rgb[0].r = r;
    fc_effect.dream_scene.rgb[0].g = g;
    fc_effect.dream_scene.rgb[0].b = b;

    set_fc_effect();
}

/* 逻辑段0仅驱动RGB PWM；逻辑段1对应PA8的16颗WS2812。
 * 保留SEGMENT2别名仅为复用新框架中双段效果代码，其数据不再发送到硬件。 */
#define USER_SEGMENT_COUNT      2
typedef enum {
    USER_SEGMENT0_INDEX = 0,
    USER_SEGMENT1_INDEX = 1,
    USER_SEGMENT2_INDEX = USER_SEGMENT1_INDEX,
} user_segment_index_e;

#define USER_SEGMENT0_START     0
#define USER_SEGMENT0_END       (USER_SEGMENT0_START + USER_WS2812_SEGMENT_0_LEN - 1)
#define USER_SEGMENT1_START     (USER_SEGMENT0_END + 1)
#define USER_SEGMENT1_END       (USER_SEGMENT1_START + USER_WS2812_SEGMENT_1_LEN - 1)
#define USER_SEGMENT2_START     USER_SEGMENT1_START
#define USER_SEGMENT2_END       USER_SEGMENT1_END

struct _user_led_segment_t {
    uint16_t start;
    uint16_t end;
};
struct _user_led_segment_t user_led_segments[USER_SEGMENT_COUNT] = {
    [USER_SEGMENT0_INDEX] = { USER_SEGMENT0_START, USER_SEGMENT0_END },
    [USER_SEGMENT1_INDEX] = { USER_SEGMENT1_START, USER_SEGMENT1_END },
};

static void static_mode(void)
{
    // WS2812FX_stop();
    WS2812FX_setSegment_colorOptions(
        USER_SEGMENT1_INDEX,                            // 第N段
        user_led_segments[USER_SEGMENT1_INDEX].start,   // 起始位置
        user_led_segments[USER_SEGMENT1_INDEX].end,     // 结束位置
        &WS2812FX_mode_static,                  //效果
        0,                                      //颜色，WS2812FX_setColors设置
        0,                                    //速度
        0);                                     //选项，这里像素点大小：1

    WS2812FX_setSegment_colorOptions(
        USER_SEGMENT2_INDEX,                            // 第N段
        user_led_segments[USER_SEGMENT2_INDEX].start,   // 起始位置
        user_led_segments[USER_SEGMENT2_INDEX].end,     // 结束位置
        &WS2812FX_mode_static,                  //效果
        0,                                      //颜色，WS2812FX_setColors设置
        0,                                    //速度
        0);                                     //选项，这里像素点大小：1

    WS2812FX_set_coloQty(1, fc_effect.dream_scene.c_n);
    WS2812FX_set_coloQty(2, fc_effect.dream_scene.c_n);
    ls_set_colors(1, &fc_effect.dream_scene.rgb[0]);
    ls_set_colors2(1, &fc_effect.dream_scene.rgb[0]);
    if (fc_effect.b > USER_BRIGHTNESS_MAX)
        fc_effect.b = USER_BRIGHTNESS_MAX;
    WS2812FX_setBrightness(fc_effect.b);
    WS2812FX_start();
    WS2812FX_trigger();
}

//-----------------------------------------------------------------------------------------跳变效果

// 多段颜色跳变
void jump_mutil_c(void)
{
    //WS2812FX_stop();
    WS2812FX_setSegment_colorOptions(
        USER_SEGMENT1_INDEX,                            // 第N段
        user_led_segments[USER_SEGMENT1_INDEX].start,   // 起始位置
        user_led_segments[USER_SEGMENT1_INDEX].end,     // 结束位置
        &WS2812FX_mode_single_block_scan,       //效果
        0,                                      //颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed,            //速度
        SIZE_MEDIUM);                           //选项，这里像素点大小：3

    WS2812FX_set_coloQty(1, fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);

    WS2812FX_start();
    WS2812FX_trigger();
}

//奥威斯的开机跳变效果
void aoweisi_kaiji_xiaoguo(void)
{
    uint32_t aoweisi_colors[3];
    color_t aoweisi_c[3];
    uint8_t i;
    aoweisi_c[0].r = (RED >> 16) & 0xff;
    aoweisi_c[0].g = (RED >> 8) & 0xff;
    aoweisi_c[0].b = (RED >> 0) & 0xff;
    aoweisi_c[1].r = (GREEN >> 16) & 0xff;
    aoweisi_c[1].g = (GREEN >> 8) & 0xff;
    aoweisi_c[1].b = (GREEN >> 0) & 0xff;
    aoweisi_c[2].r = (BLUE >> 16) & 0xff;
    aoweisi_c[2].g = (BLUE >> 8) & 0xff;
    aoweisi_c[2].b = (BLUE >> 0) & 0xff;
    fc_effect.dream_scene.c_n = 3;
    extern uint16_t aoweisi_kaiji_c_jump(void);
    extern uint16_t aoweisi_kaiji_c_jump2(void);
    WS2812FX_setSegment_colorOptions(
        USER_SEGMENT1_INDEX,                            // 第N段
        user_led_segments[USER_SEGMENT1_INDEX].start,   // 起始位置
        user_led_segments[USER_SEGMENT1_INDEX].end,     // 结束位置
        &aoweisi_kaiji_c_jump,       //效果
        0,                                      //颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed,            //速度
        SIZE_MEDIUM);                           //选项，这里像素点大小：3

    WS2812FX_setSegment_colorOptions(
        USER_SEGMENT2_INDEX,                            // 第N段
        user_led_segments[USER_SEGMENT2_INDEX].start,   // 起始位置
        user_led_segments[USER_SEGMENT2_INDEX].end,     // 结束位置
        &aoweisi_kaiji_c_jump2,       //效果
        0,                                      //颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed,            //速度
        SIZE_MEDIUM);                           //选项，这里像素点大小：3

    WS2812FX_set_coloQty(1, fc_effect.dream_scene.c_n);
    WS2812FX_set_coloQty(2, fc_effect.dream_scene.c_n);

    for (i = 0; i < 3; i++)
    {
        aoweisi_colors[i] = aoweisi_c[i].r << 16 | aoweisi_c[i].g << 8 | aoweisi_c[i].b;
    }

    WS2812FX_setColors(1, aoweisi_colors);
    WS2812FX_setColors(2, aoweisi_colors);

    WS2812FX_start();
    WS2812FX_trigger();
}


// 标准跳变，整体颜色跳变
void standard_jump(void)
{
    extern uint16_t WS2812FX_mutil_c_jump(void);
    //WS2812FX_stop();
    WS2812FX_setSegment_colorOptions(
        USER_SEGMENT1_INDEX,                            // 第N段
        user_led_segments[USER_SEGMENT1_INDEX].start,   // 起始位置
        user_led_segments[USER_SEGMENT1_INDEX].end,     // 结束位置
        &WS2812FX_mutil_c_jump,       //效果
        0,                                      //颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed * 50,            //速度
        0);                           //选项，这里像素点大小：3

    WS2812FX_set_coloQty(1, fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);
    WS2812FX_start();
    WS2812FX_trigger();
}

//奥威斯跳变  固定速度2秒跳一次
void aoweisi_jump(void)
{
    extern uint16_t aoweisi_mutil_c_jump(void);
    // WS2812FX_stop(); //不能加，加了会灭全部幻彩灯，客户不想要
    WS2812FX_setSegment_colorOptions(
        USER_SEGMENT1_INDEX,                            // 第N段
        user_led_segments[USER_SEGMENT1_INDEX].start,   // 起始位置
        user_led_segments[USER_SEGMENT1_INDEX].end,     // 结束位置
        &aoweisi_mutil_c_jump,       //效果
        0,                                      //颜色，WS2812FX_setColors设置
        2000,            //速度
        0);                           //选项，这里像素点大小：3

    WS2812FX_setSegment_colorOptions(
        USER_SEGMENT2_INDEX,                            // 第N段
        user_led_segments[USER_SEGMENT2_INDEX].start,   // 起始位置
        user_led_segments[USER_SEGMENT2_INDEX].end,     // 结束位置
        &aoweisi_mutil_c_jump,       //效果
        0,                                      //颜色，WS2812FX_setColors设置
        2000,            //速度
        0);                           //选项，这里像素点大小：3

    WS2812FX_set_coloQty(1, fc_effect.dream_scene.c_n);
    WS2812FX_set_coloQty(2, fc_effect.dream_scene.c_n);

    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);
    ls_set_colors2(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);
    WS2812FX_start();
    WS2812FX_trigger();
}



//-----------------------------------------------------------------------------------------渐变效果
// 多段，不同颜色渐变
void mutil_seg_grandual(void)
{
    //WS2812FX_stop();
    WS2812FX_setSegment_colorOptions(
        USER_SEGMENT1_INDEX,                            // 第N段
        user_led_segments[USER_SEGMENT1_INDEX].start,   // 起始位置
        user_led_segments[USER_SEGMENT1_INDEX].end,     // 结束位置
        &WS2812FX_mode_mutil_fade,              //效果
        0,                                      //颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed,            //速度
        SIZE_MEDIUM);                                //选项，这里像素点大小：3,反向/反向

    WS2812FX_set_coloQty(1, fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);

    WS2812FX_start();
    WS2812FX_trigger();
}

// 整条灯带渐变，支持多种颜色之间切换
// 颜色池：fc_effect.dream_scene.rgb[]
// 颜色数量fc_effect.dream_scene.c_n
void mutil_c_grandual(void)
{
    extern uint16_t WS2812FX_mutil_c_gradual(void);

    //WS2812FX_stop();
    WS2812FX_setSegment_colorOptions(
        USER_SEGMENT1_INDEX,                            // 第N段
        user_led_segments[USER_SEGMENT1_INDEX].start,   // 起始位置
        user_led_segments[USER_SEGMENT1_INDEX].end,     // 结束位置
        &WS2812FX_mutil_c_gradual,              //效果
        0,                                      //颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed,            //速度
        SIZE_MEDIUM);                           //选项，这里像素点大小：3,反向/反向

    WS2812FX_set_coloQty(1, fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);
    WS2812FX_start();
    WS2812FX_trigger();
}


void aoweisi_mutil_c_grandual(void)
{


    extern uint16_t WS2812FX_mutil_c_gradual(void);
    extern uint16_t WS2812FX_mutil_c_gradual2(void);

    WS2812FX_stop();
    WS2812FX_setSegment_colorOptions(
        USER_SEGMENT1_INDEX,                            // 第N段
        user_led_segments[USER_SEGMENT1_INDEX].start,   // 起始位置
        user_led_segments[USER_SEGMENT1_INDEX].end,     // 结束位置
        &WS2812FX_mutil_c_gradual,              //效果
        0,                                      //颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed,            //速度
        SIZE_MEDIUM);                           //选项，这里像素点大小：3,反向/反向

    WS2812FX_setSegment_colorOptions(
        USER_SEGMENT2_INDEX,                            // 第N段
        user_led_segments[USER_SEGMENT2_INDEX].start,   // 起始位置
        user_led_segments[USER_SEGMENT2_INDEX].end,     // 结束位置
        &WS2812FX_mutil_c_gradual2,              //效果
        0,                                      //颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed,            //速度
        SIZE_MEDIUM);                           //选项，这里像素点大小：3,反向/反向

    WS2812FX_set_coloQty(1, fc_effect.dream_scene.c_n);
    WS2812FX_set_coloQty(2, fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);
    ls_set_colors2(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);
    WS2812FX_start();
    WS2812FX_trigger();


}


//-----------------------------------------------------------------------------------------呼吸效果
//标准呼吸
void strand_breath(void)
{
    //WS2812FX_stop();
    WS2812FX_setSegment_colorOptions(
        USER_SEGMENT1_INDEX,                            // 第N段
        user_led_segments[USER_SEGMENT1_INDEX].start,   // 起始位置
        user_led_segments[USER_SEGMENT1_INDEX].end,     // 结束位置
        &WS2812FX_mode_mutil_breath,            //效果
        0,                                      //颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed,            //速度
        SIZE_MEDIUM);                           //选项，这里像素点大小：3

    WS2812FX_set_coloQty(1, fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);

    WS2812FX_start();
    WS2812FX_trigger();
}
//单色呼吸
void single_c_breath(void)
{
    extern uint16_t WS2812FX_mode_breath(void);
    WS2812FX_setSegment_colorOptions(
        USER_SEGMENT1_INDEX,                            // 第N段
        user_led_segments[USER_SEGMENT1_INDEX].start,   // 起始位置
        user_led_segments[USER_SEGMENT1_INDEX].end,     // 结束位置
        &WS2812FX_mode_breath,            //效果
        0,                                      //颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed,            //速度
        SIZE_MEDIUM);                           //选项，这里像素点大小：3

    WS2812FX_set_coloQty(1, fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);

    WS2812FX_start();
    WS2812FX_trigger();
}




//七个色循环，单色呼吸
void mutil_c_breath(void)
{
    extern uint16_t WS2812FX_mutil_breath(void);

    WS2812FX_setSegment_colorOptions(
        USER_SEGMENT1_INDEX,                            // 第N段
        user_led_segments[USER_SEGMENT1_INDEX].start,   // 起始位置
        user_led_segments[USER_SEGMENT1_INDEX].end,     // 结束位置
        &WS2812FX_mutil_breath,            //效果
        0,                                      //颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed / 10,            //速度
        SIZE_MEDIUM);
    //选项，这里像素点大小：3
    WS2812FX_setSegment_colorOptions(
        USER_SEGMENT2_INDEX,                            // 第N段
        user_led_segments[USER_SEGMENT2_INDEX].start,   // 起始位置
        user_led_segments[USER_SEGMENT2_INDEX].end,     // 结束位置
        &WS2812FX_mutil_breath,            //效果
        0,                                      //颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed / 10,            //速度
        SIZE_MEDIUM);                           //选项，这里像素点大小：3

    WS2812FX_set_coloQty(1, fc_effect.dream_scene.c_n);
    WS2812FX_set_coloQty(2, fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);
    ls_set_colors2(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);

    WS2812FX_start();
    WS2812FX_trigger();
}




//-----------------------------------------------------------------------------------------频闪效果
void ls_strobe(void)
{
    extern uint16_t WS2812FX_mutil_strobe(void);


    WS2812FX_setSegment_colorOptions(
        USER_SEGMENT1_INDEX,                            // 第N段
        user_led_segments[USER_SEGMENT1_INDEX].start,   // 起始位置
        user_led_segments[USER_SEGMENT1_INDEX].end,     // 结束位置
        &WS2812FX_mutil_strobe,            //效果
        0,                                      //颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed,            //速度
        SIZE_MEDIUM);
    //选项，这里像素点大小：3
    WS2812FX_setSegment_colorOptions(
        USER_SEGMENT2_INDEX,                            // 第N段
        user_led_segments[USER_SEGMENT2_INDEX].start,   // 起始位置
        user_led_segments[USER_SEGMENT2_INDEX].end,     // 结束位置
        &WS2812FX_mutil_strobe,            //效果
        0,                                      //颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed,            //速度
        SIZE_MEDIUM);                           //选项，这里像素点大小：3

    WS2812FX_set_coloQty(1, fc_effect.dream_scene.c_n);
    WS2812FX_set_coloQty(2, fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);
    ls_set_colors2(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);

    WS2812FX_start();
    WS2812FX_trigger();
}



/*************************************************** 常规的幻彩灯具效果 *****************************************************/
/*流水，过渡，跑动，开幕，闭幕，流星，流星雨，星空*/

//-----------------------------------------------------------------------------------------流水效果
void strand_flow_water(void)
{
    uint8_t option;
    // 正向
    if (fc_effect.dream_scene.direction == IS_forward)
    {
        option = SIZE_MEDIUM | 0;
    }
    else
    {
        option = SIZE_MEDIUM | REVERSE;
    }

    //WS2812FX_stop();
    WS2812FX_setSegment_colorOptions(
        USER_SEGMENT1_INDEX,                            // 第N段
        user_led_segments[USER_SEGMENT1_INDEX].start,   // 起始位置
        user_led_segments[USER_SEGMENT1_INDEX].end,     // 结束位置
        &WS2812FX_mode_multi_block_scan,        //效果
        0,                                      //颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed,            //速度
        option);                                //选项，这里像素点大小：3,反向/反向
    WS2812FX_set_coloQty(1, fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);

    WS2812FX_start();
    WS2812FX_trigger();
}

// 两种颜色混合流水效果，渐变色流水
void tow_color_fix_flow(void)
{
    extern uint16_t WS2812FX_mode_running_lights(void);

    uint8_t option;
    if (fc_effect.dream_scene.direction == IS_forward)
    {
        option = 0;
    }
    else
    {
        option = REVERSE;
    }
    // WS2812FX_stop();
    WS2812FX_setSegment_colorOptions(
        USER_SEGMENT1_INDEX,                            // 第N段
        user_led_segments[USER_SEGMENT1_INDEX].start,   // 起始位置
        user_led_segments[USER_SEGMENT1_INDEX].end,     // 结束位置
        &WS2812FX_mode_running_lights,             //效果
        0,                                      //颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed,            //速度
        option);                            //选项，这里像素点大小：1
    WS2812FX_set_coloQty(1, fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);

    WS2812FX_start();
    WS2812FX_trigger();
}

//-----------------------------------------------------------------------------------------彩虹效果
static void strand_rainbow(void)
{

    WS2812FX_setSegment_colorOptions(
        USER_SEGMENT1_INDEX,                            // 第N段
        user_led_segments[USER_SEGMENT1_INDEX].start,   // 起始位置
        user_led_segments[USER_SEGMENT1_INDEX].end,     // 结束位置
        &WS2812FX_mode_mutil_fade,               //效果
        0,                                      //颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed,            //速度
        SIZE_SMALL);                            //选项，这里像素点大小：1

    WS2812FX_set_coloQty(1, fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);

    WS2812FX_start();
    WS2812FX_trigger();
}

void rainbow_flow(void)
{

    extern uint16_t WS2812FX_mode_rainbow_cycle(void);
    WS2812FX_setSegment_colorOptions(
        USER_SEGMENT1_INDEX,                            // 第N段
        user_led_segments[USER_SEGMENT1_INDEX].start,   // 起始位置
        user_led_segments[USER_SEGMENT1_INDEX].end,     // 结束位置
        &WS2812FX_mode_rainbow_cycle,               //效果
        0,                                      //颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed,            //速度
        SIZE_MEDIUM);                            //选项，这里像素点大小：1

    WS2812FX_set_coloQty(1, fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);

    WS2812FX_start();
    WS2812FX_trigger();



}
//-----------------------------------------------------------------------------------------闪烁效果
void strand_twihkle(void)
{
    uint8_t option;
    //WS2812FX_stop();
    WS2812FX_setSegment_colorOptions(
        USER_SEGMENT1_INDEX,                            // 第N段
        user_led_segments[USER_SEGMENT1_INDEX].start,   // 起始位置
        user_led_segments[USER_SEGMENT1_INDEX].end,     // 结束位置
        &WS2812FX_mode_mutil_twihkle,           //效果
        0,                                      //颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed,            //速度
        SIZE_SMALL);                            //选项，这里像素点大小：1
    WS2812FX_set_coloQty(1, fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);

    WS2812FX_start();
    WS2812FX_trigger();

}

//------------------------------------------------------------------------------------------追光效果
void strand_chas_light(void)
{

    // 正向
    if (fc_effect.dream_scene.direction == IS_forward)
    {
        WS2812FX_setSegment_colorOptions(
            USER_SEGMENT1_INDEX,                            // 第N段
            user_led_segments[USER_SEGMENT1_INDEX].start,   // 起始位置
            user_led_segments[USER_SEGMENT1_INDEX].end,     // 结束位置
            &WS2812FX_mode_multi_forward_same,        //效果
            0,                                      //颜色，WS2812FX_setColors设置
            fc_effect.dream_scene.speed,            //速度
            0);                                     //选项
    }
    else
    {
        WS2812FX_setSegment_colorOptions(
            USER_SEGMENT1_INDEX,                            // 第N段
            user_led_segments[USER_SEGMENT1_INDEX].start,   // 起始位置
            user_led_segments[USER_SEGMENT1_INDEX].end,     // 结束位置
            &WS2812FX_mode_multi_back_same,        //效果
            0,                                      //颜色，WS2812FX_setColors设置
            fc_effect.dream_scene.speed,            //速度
            0);
    }
    WS2812FX_set_coloQty(1, fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);

    WS2812FX_start();
    WS2812FX_trigger();
}

//------------------------------------------------------------------------------------------炫彩效果
void strand_colorful(void)
{
    uint8_t option;
    //WS2812FX_stop();
    WS2812FX_setSegment_colorOptions(
        USER_SEGMENT1_INDEX,                            // 第N段
        user_led_segments[USER_SEGMENT1_INDEX].start,   // 起始位置
        user_led_segments[USER_SEGMENT1_INDEX].end,     // 结束位置
        &WS2812FX_mode_multi_block_scan,        //效果
        0,                                      //颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed,            //速度
        SIZE_SMALL);                            //选项，这里像素点大小：1
    WS2812FX_set_coloQty(1, fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);

    WS2812FX_start();
    WS2812FX_trigger();
}


//------------------------------------------------------------------------------------------星空效果
// 单点单色随机闪现
void single_c_flash_random(void)
{
    extern uint16_t WS2812FX_mode_fire_flicker_intense(void);
    WS2812FX_setSegment_colorOptions(
        USER_SEGMENT1_INDEX,                            // 第N段
        user_led_segments[USER_SEGMENT1_INDEX].start,   // 起始位置
        user_led_segments[USER_SEGMENT1_INDEX].end,     // 结束位置
        &WS2812FX_mode_fire_flicker_intense,    //效果
        0,                                      //颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed,            //速度
        0);                           //选项，这里像素点大小：3,反向/反向

    WS2812FX_set_coloQty(1, fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);
    WS2812FX_start();
    WS2812FX_trigger();
}

// 从的颜色池抽取颜色闪现，以段为单位，闪现位置随机
void seg_mutil_c_flash_random(void)
{
    extern uint16_t WS2812FX_mode_fireworks(void);
    WS2812FX_setSegment_colorOptions(
        USER_SEGMENT1_INDEX,                            // 第N段
        user_led_segments[USER_SEGMENT1_INDEX].start,   // 起始位置
        user_led_segments[USER_SEGMENT1_INDEX].end,     // 结束位置
        &WS2812FX_mode_fireworks,               //效果
        0,                                      //颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed,            //速度
        0);                           //选项，这里像素点大小：3,反向/反向

    WS2812FX_set_coloQty(1, fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);

    WS2812FX_start();
    WS2812FX_trigger();
}
//------------------------------------------------------------------------------------------流星效果
// 单色流星效果
void single_meteor(void)
{
    uint8_t option;
    if (fc_effect.dream_scene.direction == IS_forward)
    {
        option = 0;
    }
    else
    {
        option = REVERSE;
    }
    extern uint16_t WS2812FX_mode_comet(void);
    WS2812FX_setSegment_colorOptions(
        USER_SEGMENT1_INDEX,                            // 第N段
        user_led_segments[USER_SEGMENT1_INDEX].start,   // 起始位置
        user_led_segments[USER_SEGMENT1_INDEX].end,     // 结束位置
        &WS2812FX_mode_comet,                   //效果
        0,                                      //颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed,            //速度
        option);                                //选项，这里像素点大小：3,反向/反向

    WS2812FX_set_coloQty(1, fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);
    WS2812FX_start();
    WS2812FX_trigger();
}
void single_c_meteor(void)
{
    extern uint16_t meteor_signle_c(void);
    extern uint16_t meteor_signle_c2(void);
    uint8_t option;
    if (fc_effect.dream_scene.direction == IS_forward)
    {
        option = 0 | FADE_XSLOW;
    }
    else
    {
        option = REVERSE | FADE_XSLOW;
    }


    WS2812FX_setSegment_colorOptions(
        USER_SEGMENT1_INDEX,                            // 第N段
        user_led_segments[USER_SEGMENT1_INDEX].start,   // 起始位置
        user_led_segments[USER_SEGMENT1_INDEX].end,     // 结束位置
        &meteor_signle_c,                       //效果
        0,                                      //颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed,            //速度
        option);                           //选项，这里像素点大小：3

    WS2812FX_setSegment_colorOptions(
        USER_SEGMENT2_INDEX,                            // 第N段
        user_led_segments[USER_SEGMENT2_INDEX].start,   // 起始位置
        user_led_segments[USER_SEGMENT2_INDEX].end,     // 结束位置
        &meteor_signle_c2,                       //效果
        0,                                      //颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed,            //速度
        option);                           //选项，这里像素点大小：3



    WS2812FX_set_coloQty(1, fc_effect.dream_scene.c_n);
    WS2812FX_set_coloQty(2, fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);
    ls_set_colors2(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);

    WS2812FX_start();
    WS2812FX_trigger();
}
// 带有背景色的流星，每次只有一个流星，流星时间间隔3秒
void background_meteor(void)
{
    extern uint16_t meteor_bc(void);

    WS2812FX_setSegment_colorOptions(
        USER_SEGMENT1_INDEX,                            // 第N段
        user_led_segments[USER_SEGMENT1_INDEX].start,   // 起始位置
        user_led_segments[USER_SEGMENT1_INDEX].end,     // 结束位置
        &meteor_bc,                       //效果
        0,                                      //颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed,            //速度
        SIZE_MEDIUM);                           //选项，这里像素点大小：3

    WS2812FX_set_coloQty(1, fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);

    WS2812FX_start();
    WS2812FX_trigger();
}
//单个，全彩颜色的流星效果
void color_meteor(void)
{

    extern uint16_t single_color_meteor(void);
    uint8_t option;
    if (fc_effect.dream_scene.direction == IS_forward)
    {
        option = 0 | SIZE_XLARGE;
    }
    else
    {
        option = REVERSE | SIZE_XLARGE;
    }

    WS2812FX_setSegment_colorOptions(
        USER_SEGMENT1_INDEX,                            // 第N段
        user_led_segments[USER_SEGMENT1_INDEX].start,   // 起始位置
        user_led_segments[USER_SEGMENT1_INDEX].end,     // 结束位置
        &single_color_meteor,                   //效果
        0,                                      //颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed,            //速度
        option);                                //选项，这里像素点大小：3

    WS2812FX_set_coloQty(1, fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);

    WS2812FX_start();
    WS2812FX_trigger();

}

//跳变的流星效果
void color_jump_meteor(void)
{

    extern uint16_t jump_meteor(void);

    WS2812FX_setSegment_colorOptions(
        USER_SEGMENT1_INDEX,                            // 第N段
        user_led_segments[USER_SEGMENT1_INDEX].start,   // 起始位置
        user_led_segments[USER_SEGMENT1_INDEX].end,     // 结束位置
        &jump_meteor,                       //效果
        0,                                      //颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed,            //速度
        SIZE_MEDIUM);                           //选项，这里像素点大小：3

    WS2812FX_set_coloQty(1, fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);

    WS2812FX_start();
    WS2812FX_trigger();

}

//渐变的流星效果
void color_gradual_meteor(void)
{
    extern uint16_t gradual_meteor(void);

    WS2812FX_setSegment_colorOptions(
        USER_SEGMENT1_INDEX,                            // 第N段
        user_led_segments[USER_SEGMENT1_INDEX].start,   // 起始位置
        user_led_segments[USER_SEGMENT1_INDEX].end,     // 结束位置
        &gradual_meteor,                       //效果
        0,                                      //颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed,            //速度
        SIZE_MEDIUM);                           //选项，这里像素点大小：3

    WS2812FX_set_coloQty(1, fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);

    WS2812FX_start();
    WS2812FX_trigger();


}
//------------------------------------------------------------------------------------------开幕效果
void open_mode(void)
{
    extern uint16_t open_effect(void);
    extern uint16_t fu_open_effect(void);


    WS2812FX_setSegment_colorOptions(
        USER_SEGMENT1_INDEX,                            // 第N段
        user_led_segments[USER_SEGMENT1_INDEX].start,   // 起始位置
        user_led_segments[USER_SEGMENT1_INDEX].end,     // 结束位置
        &open_effect,                       //效果
        0,                                      //颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed,            //速度
        0);                           //选项，这里像素点大小：3

    WS2812FX_setSegment_colorOptions(
        USER_SEGMENT2_INDEX,                            // 第N段
        user_led_segments[USER_SEGMENT2_INDEX].start,   // 起始位置
        user_led_segments[USER_SEGMENT2_INDEX].end,     // 结束位置
        &fu_open_effect,                       //效果
        0,                                      //颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed,            //速度
        0);                           //选项，这里像素点大小：3



    WS2812FX_set_coloQty(1, fc_effect.dream_scene.c_n);
    WS2812FX_set_coloQty(2, fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);
    ls_set_colors2(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);

    WS2812FX_start();
    WS2812FX_trigger();
}

//------------------------------------------------------------------------------------------闭幕效果
void close_mode(void)
{
    extern uint16_t close_effect(void);



    WS2812FX_setSegment_colorOptions(
        USER_SEGMENT1_INDEX,                            // 第N段
        user_led_segments[USER_SEGMENT1_INDEX].start,   // 起始位置
        user_led_segments[USER_SEGMENT1_INDEX].end,     // 结束位置
        &close_effect,                       //效果
        0,                                      //颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed,            //速度
        0);                           //选项，这里像素点大小：3

    WS2812FX_setSegment_colorOptions(
        USER_SEGMENT2_INDEX,                            // 第N段
        user_led_segments[USER_SEGMENT2_INDEX].start,   // 起始位置
        user_led_segments[USER_SEGMENT2_INDEX].end,     // 结束位置
        &close_effect,                       //效果
        0,                                      //颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed,            //速度
        0);                           //选项，这里像素点大小：3



    WS2812FX_set_coloQty(1, fc_effect.dream_scene.c_n);
    WS2812FX_set_coloQty(2, fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);
    ls_set_colors2(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);

    WS2812FX_start();
    WS2812FX_trigger();
}
//------------------------------------------------------------------------------------------跑动效果
//多颜色跑动效果
void dot_running(void)
{
    uint8_t option;
    extern uint16_t aoweisi_dot_running1(void);
    extern uint16_t aoweisi_dot_running2(void);

    // 正向
    if (fc_effect.dream_scene.direction == IS_forward)
    {
        option = SIZE_MEDIUM | 0;
    }
    else
    {
        option = SIZE_MEDIUM | REVERSE;
    }

    WS2812FX_stop();
    WS2812FX_setSegment_colorOptions(
        USER_SEGMENT1_INDEX,                            // 第N段
        user_led_segments[USER_SEGMENT1_INDEX].start,   // 起始位置
        user_led_segments[USER_SEGMENT1_INDEX].end,     // 结束位置
        &aoweisi_dot_running1,        //效果
        0,                                      //颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed,            //速度
        option);                                //选项，这里像素点大小：3,反向/反向


    WS2812FX_setSegment_colorOptions(
        USER_SEGMENT2_INDEX,                            // 第N段
        user_led_segments[USER_SEGMENT2_INDEX].start,   // 起始位置
        user_led_segments[USER_SEGMENT2_INDEX].end,     // 结束位置
        &aoweisi_dot_running2,        //效果
        0,                                      //颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed,            //速度
        option);                                //选项，这里像素点大小：3,反向/反向


    WS2812FX_set_coloQty(1, fc_effect.dream_scene.c_n);
    WS2812FX_set_coloQty(2, fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);
    ls_set_colors2(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);

    WS2812FX_start();
    WS2812FX_trigger();
}

//奥威斯
void aoweisi_running(void)
{
    uint8_t option;
    extern uint16_t aoweisi_dot_running(void);
    fc_effect.dream_scene.direction = IS_forward;
    // 正向
    if (fc_effect.dream_scene.direction == IS_forward)
    {
        option = SIZE_MEDIUM | 0;
    }
    else
    {
        option = SIZE_MEDIUM | REVERSE;
    }

    //WS2812FX_stop();
    WS2812FX_setSegment_colorOptions(
        USER_SEGMENT1_INDEX,                            // 第N段
        user_led_segments[USER_SEGMENT1_INDEX].start,   // 起始位置
        user_led_segments[USER_SEGMENT1_INDEX].end,     // 结束位置
        &aoweisi_dot_running,        //效果
        0,                                      //颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed,            //速度
        option);                                //选项，这里像素点大小：3,反向/反向
    WS2812FX_set_coloQty(1, fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);

    WS2812FX_start();
    WS2812FX_trigger();
}


//反弹流水
void rebound_flow(void)
{

    WS2812FX_setSegment_colorOptions(
        USER_SEGMENT1_INDEX,                            // 第N段
        user_led_segments[USER_SEGMENT1_INDEX].start,   // 起始位置
        user_led_segments[USER_SEGMENT1_INDEX].end,     // 结束位置
        &WS2812FX_mode_larson_scanner,
        0,
        fc_effect.dream_scene.speed,
        FADE_SLOW);

    WS2812FX_start();
    WS2812FX_trigger();

}
/*多色叠加*/
void superposition_mutil_effect(void)
{

    uint8_t option;
    extern uint16_t multi_superposition(void);

    // 正向
    if (fc_effect.dream_scene.direction == IS_forward)
    {
        option = 0;
    }
    else
    {
        option = REVERSE;
    }

    //WS2812FX_stop();
    WS2812FX_setSegment_colorOptions(
        USER_SEGMENT1_INDEX,                            // 第N段
        user_led_segments[USER_SEGMENT1_INDEX].start,   // 起始位置
        user_led_segments[USER_SEGMENT1_INDEX].end,     // 结束位置
        &multi_superposition,        //效果
        0,                                      //颜色，WS2812FX_setColors设置
        (fc_effect.dream_scene.speed),            //速度
        option);                                //选项，这里像素点大小：3,反向/反向
    WS2812FX_set_coloQty(1, fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);

    WS2812FX_start();
    WS2812FX_trigger();

}

/*************************************************** 其他效果 *****************************************************/
// ---------------------------------------------------------------------------------------------------------涂抹效果


/************************************************** API *****************************************************/

// ---------------------------------------------------------------------------------------------------------音乐效果

//-------------------------------------------------speed
u16 get_max_sp(void)
{
    u16 s;
    s = fc_effect.led_num * 30 / 1000;
    if (s < 10) s = 10;
    return s; //每个LED30us
}


// 最慢500ms一帧,s:1-100%
void ls_set_speed(uint8_t s)
{

    fc_effect.dream_scene.speed = 500 - (500 * s / 100);
    if (fc_effect.dream_scene.speed < 10)
    {
        fc_effect.dream_scene.speed = 10;
    }

    set_fc_effect();
}

void speed_fast(void)
{
    if (fc_effect.dream_scene.speed > 100)
    {
        fc_effect.dream_scene.speed -= 100;
    }
    else
    {
        fc_effect.dream_scene.speed = 50;

    }
    ls_scene_effect();

    printf("fc_effect.dream_scene.speed  = %d", fc_effect.dream_scene.speed);
}

void speed_slow(void)
{
    if (fc_effect.dream_scene.speed < 500 - 100)
    {
        fc_effect.dream_scene.speed += 100;
    }
    else
    {
        fc_effect.dream_scene.speed = 500;

    }
    printf("fc_effect.dream_scene.speed  = %d", fc_effect.dream_scene.speed);
    ls_scene_effect();

}

// ------------------------------------------------亮度
// 0-100
void set_bright(u8 b)
{
    if (b == 0) b = 10;
    fc_effect.b = 255 * b / 100;
    if (fc_effect.b > USER_BRIGHTNESS_MAX)
        fc_effect.b = USER_BRIGHTNESS_MAX;

    WS2812FX_setBrightness(fc_effect.b);
}

void bright_plus(void)
{
    if (fc_effect.static_mode_brightness < 8)
        fc_effect.static_mode_brightness++;
    // WS2812FX_show();
    WS2812FX_trigger();

    save_user_data_area3();
    printf("\n fc_effect.b = %d", fc_effect.b);
}

void bright_sub(void)
{
    if (fc_effect.static_mode_brightness > 1)
        fc_effect.static_mode_brightness--;
    // WS2812FX_show();
    WS2812FX_trigger();

    save_user_data_area3();
    printf("\n fc_effect.b = %d", fc_effect.b);
}



/* *********************************样式 */
// 自定义样式
void fc_set_style_custom(void)
{
    fc_effect.Now_state = ACT_CUSTOM;
}

// ------------------------------------------------RGB顺序
// s:0-5
void set_rgb_sequence(u8 s)
{
    if (s < 6)
    {
        fc_effect.sequence = rgb_sequence_map[s];
        //WS2812FX_stop();
        WS2812FX_init(fc_effect.led_num, fc_effect.sequence);
        fc_effect.custom_index = 2;       //调整RGB顺序效果
        fc_set_style_custom();  //自定义效果
        set_fc_effect();
    }
}


// 设置各段颜色
// n颜色数量
// c颜色池指针
void ls_set_colors(uint8_t n, color_t *c)
{
    uint32_t colors[MAX_NUM_COLORS];
    uint8_t i;
    for (i = 0; i < n; i++)
    {
        colors[i] = c[i].r << 16 | c[i].g << 8 | c[i].b;
    }
    WS2812FX_setColors(1, colors);

}
void ls_set_colors2(uint8_t n, color_t *c)
{
    uint32_t colors[MAX_NUM_COLORS];
    uint8_t i;
    for (i = 0; i < n; i++)
    {
        colors[i] = c[i].r << 16 | c[i].g << 8 | c[i].b;
    }
    WS2812FX_setColors(2, colors);

}
void ls_set_colors0(uint8_t n, color_t *c)
{
    uint32_t colors[MAX_NUM_COLORS];
    uint8_t i;
    for (i = 0; i < n; i++)
    {
        colors[i] = c[i].r << 16 | c[i].g << 8 | c[i].b;
    }
    WS2812FX_setColors(0, colors);
}


// 设置fc_effect.dream_scene.rgb的颜色池
// n:0-MAX_NUM_COLORS
// c:WS2812FX颜色系，R<<16,G<<8,B在低8位
void ls_set_color(uint8_t n, uint32_t c)
{
    if (n < MAX_NUM_COLORS)
    {
        fc_effect.dream_scene.rgb[n].r = (c >> 16) & 0xff;
        fc_effect.dream_scene.rgb[n].g = (c >> 8) & 0xff;
        fc_effect.dream_scene.rgb[n].b = c & 0xff;
    }
}

// ------------------------------------------------灯带长度
void set_ls_lenght(u16 l)
{
    if (l > 2048)
        l = 2048;
    if (l > fc_effect.led_num_max)
        l = fc_effect.led_num_max;
    fc_effect.led_num = l;
    WS2812FX_stop();
    WS2812FX_init(fc_effect.led_num, fc_effect.sequence);
    WS2812FX_start();
    set_fc_effect();
    WS2812FX_trigger();

}

/*-------------------------------------------声控----------------------------------------*/
void music_mode_plus(void)
{
    fc_effect.music.m++;
    fc_effect.music.m %= 12;
    fc_effect.Now_state = IS_light_music;
    set_fc_effect();
}

void music_mode_sub(void)
{
    if (fc_effect.music.m > 0)
        fc_effect.music.m--;
    else fc_effect.music.m = 11;
    fc_effect.Now_state = IS_light_music;
    set_fc_effect();
}

void set_music_mode(u8 m)
{
    fb_rf433_music_on_off(1);
    fc_effect.music.m = m;
    fc_effect.Now_state = IS_light_music;
    set_fc_effect();
}

void set_music_sensitive(u8 s)
{
    fc_effect.music.s = s;
}

void set_music_type(u8 ty)
{
    fc_effect.music.m_type = ty;
}

//-------------------------------------------------------------------流星
// 时间递减
// sub:减数，ms
//放在了while循环，10ms减一次
// fc_effect.meteor_period = 8;//默认8秒  周期值
// fc_effect.period_cnt = fc_effect.meteor_period*1000;  //ms,运行时的计数器 8000ms
void meteor_period_sub(void)
{
    if (fc_effect.period_cnt > 10)
    {
        fc_effect.period_cnt -= 10;
    }
    else
    {
        fc_effect.period_cnt = 0;   //计数器清零
        if (fc_effect.mode_cycle)    //模式循环完成，更新
        {
            fc_effect.period_cnt = fc_effect.meteor_period * 1000;
            fc_effect.mode_cycle = 0;
        }
    }
}

//清空计时
//作用：使用数组实现流星效果的函数，在切换的流星效果时，立即切换
void clean_period_cnt(void)
{
    fc_effect.period_cnt = 0;
}
// 0:计时完成
// 1：计时中
u8 get_effect_p(void)
{
    if (fc_effect.period_cnt > 0) return 1;
    else return 0;
}

//app指令
const u8 app_cmd_collections[12][30] = {
    //三色跳变  速度固定 2s跳一下
    {0x04,0x02,0x7},//三色跳变

    //呼吸  7  //只有一个呼吸，整段 速度可调
    {0x04,0x02,0x0B},

    //渐变    //只有一个整段渐变  速度可调
    {0x04,0x02,0x0A},

    //流星雨  速度可调
    {0x38,0x00,0x00,0x0A,0x17,0x00,0x05,0x03,0xFF,0x00,0x00,0x00,0xFF,0x00,0x00,0x00,0xFF},  //11  三色流星雨
    {0x38,0x00,0x00,0x0A,0x17,0x00,0x05,0x06,0xFF,0x00,0x00,0x00,0xFF,0x00,0x00,0x00,0xFF,0xFF,0x00,0xFF,0x00,0xFF,0xFF,0xFF,0xFF,0x00},  //12 七色流星雨

    //标准跑动  //需要一个  速度可调
    {0x38,0x00,0x00,0x19,0x1C,0x00,0x05,0x07,0xFF,0x00,0x00,0x00,0xFF,0x00,0x00,0x00,0xFF,0xFF,0x00,0xFF,0x00,0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x00,0x00},//34 红绿蓝青紫白 跑动

    {0x04,0x02,0x15},
    {0x04,0x02,0x16},
    {0x04,0x02,0x17},
    {0x04,0x02,0x18},
    {0x04,0x02,0x19},
    {0x04,0x02,0x1a},
};

const u8 yaokong_ctrl[7][3] = {
    //基本模式 7 一般是七彩灯
    {0x04,0x02,0x0},//红色   0
    {0x04,0x02,0x2},//绿色   1
    {0x04,0x02,0x1},//蓝色   2
    {0x04,0x02,0x3},//青色   3
    {0x04,0x02,0x4},//黄色   4
    {0x04,0x02,0x5},//紫色   5
    {0x04,0x02,0x6},//白色   6
};

// 幻彩灯静态模式切换
void aws_pixels_static_mode_change(u8 dir)
{
    u32 yaokong_color = BLACK;
    
    if (fc_effect.led_on_off_flag == DEVICE_OFF && fc_effect.led1_on_off_flag == DEVICE_OFF)
        return;

    if (fc_effect.Now_state == IS_STATIC)
    {
        if (dir == CHANGE_DIR_FORWARD)
        {
            fc_effect.huancai_color_i++;
            fc_effect.huancai_color_i %= 12;
        }
        else if (dir == CHANGE_DIR_BACKWARD)
        {
            if (fc_effect.huancai_color_i > 0)
                fc_effect.huancai_color_i--;
            else
                fc_effect.huancai_color_i = 12 - 1;
        }
        else if (dir == CHANGE_DIR_STAND)
        {
            fc_effect.huancai_color_i %= 12;
        }
    }
    else
    {
        if (dir == CHANGE_DIR_STAND)
        {
            return;
        }
    }
    
    switch (fc_effect.huancai_color_i)
    {
    default:
        fc_effect.huancai_color_i = 0;
    case 0:
        yaokong_color = RED;
        break;
    case 1:
        yaokong_color = GREEN;
        break;
    case 2:
        yaokong_color = BLUE;
        break;
    case 3:
        yaokong_color = CYAN;
        break;
    case 4:
        yaokong_color = YELLOW;
        break;
    case 5:
        yaokong_color = PURPLE; /* 保持旧工程第6项紫色 */
        break;
    case 6:
        yaokong_color = WHITE;  /* 旧工程白色：RGB三色全亮 */
        break;
    case 7:
        yaokong_color = 0xE324E6;  //粉红
        break;
    case 8:
        yaokong_color = 0xFF6100;  //橙色
        break;
    case 9:
        yaokong_color = 0xFBB00F;  //蛋壳色
        break;
    case 10:
        yaokong_color = 0x0BFF1C;  //青绿色
        break;
    case 11:
        yaokong_color = 0x17E6F3;  //天蓝色
        break;
    }

    fc_effect.Now_state = IS_STATIC;

    fc_effect.dream_scene.rgb[0].r = (yaokong_color >> 16) & 0xff;
    fc_effect.dream_scene.rgb[0].g = (yaokong_color >> 8) & 0xff;
    fc_effect.dream_scene.rgb[0].b = (yaokong_color) & 0xff;

    set_fc_effect();
    printf("fc_effect.huancai_color_i = %d", fc_effect.huancai_color_i);
}

// 设置fc_effect.rgbw_scene.rgb的颜色池
// n:0-MAX_NUM_COLORS
// c:WS2812FX颜色系，R<<16,G<<8,B在低8位

void rgbw_set_mutil_color(uint8_t n, uint32_t c)
{
    if (n < MAX_NUM_COLORS)
    {
        fc_effect.rgbw_scene.rgb[n].r = (c >> 16) & 0xff;
        fc_effect.rgbw_scene.rgb[n].g = (c >> 8) & 0xff;
        fc_effect.rgbw_scene.rgb[n].b = c & 0xff;
    }
}

#if 1
// 旧产品RGB按键效果兼容：RGB三路PWM，无独立W通道。
static void rgbw_app_effect_collections(u8 key_v)
{
    if (key_v == 0)//红
    {
        fc_effect.b = USER_BRIGHTNESS_MAX;
        WS2812FX_setBrightness(fc_effect.b);
        fc_effect.rgbw_on_off_flag = DEVICE_ON;
        fc_effect.rgbw.r = 255;
        fc_effect.rgbw.g = 0;
        fc_effect.rgbw.b = 0;

        fc_effect.custom_index = 10;  //静态
    }
    else if (key_v == 1)//绿
    {
        fc_effect.b = USER_BRIGHTNESS_MAX;
        WS2812FX_setBrightness(fc_effect.b);
        fc_effect.rgbw.r = 0;
        fc_effect.rgbw.g = 255;
        fc_effect.rgbw.b = 0;

        fc_effect.custom_index = 10;
    }
    else if (key_v == 2)//蓝
    {
        fc_effect.b = USER_BRIGHTNESS_MAX;
        WS2812FX_setBrightness(fc_effect.b);
        fc_effect.rgbw.r = 0;
        fc_effect.rgbw.g = 0;
        fc_effect.rgbw.b = 255;

        fc_effect.custom_index = 10;
    }
    else if (key_v == 3)//黄
    {
        fc_effect.b = USER_BRIGHTNESS_MAX;
        WS2812FX_setBrightness(fc_effect.b);
        fc_effect.rgbw.r = 255;
        fc_effect.rgbw.g = 255;
        fc_effect.rgbw.b = 0;

        fc_effect.custom_index = 10;
    }
    else if (key_v == 4)//青
    {
        fc_effect.b = USER_BRIGHTNESS_MAX;
        WS2812FX_setBrightness(fc_effect.b);
        fc_effect.rgbw.r = 0;
        fc_effect.rgbw.g = 255;
        fc_effect.rgbw.b = 255;

        fc_effect.custom_index = 10;
    }
    else if (key_v == 5)//紫
    {
        fc_effect.b = USER_BRIGHTNESS_MAX;
        WS2812FX_setBrightness(fc_effect.b);
        fc_effect.rgbw.r = 255;
        fc_effect.rgbw.g = 0;
        fc_effect.rgbw.b = 255;

        fc_effect.custom_index = 10;
    }
    else if (key_v == 6)//白光最亮
    {
        fc_effect.rgbw.r = 255;
        fc_effect.rgbw.g = 255;
        fc_effect.rgbw.b = 255;

        fc_effect.b = USER_BRIGHTNESS_MAX;

        WS2812FX_setBrightness(fc_effect.b);
        fc_effect.custom_index = 10;
    }
    else if (key_v == 7) // 旧W=50%：RGB混白
    {
        fc_effect.rgbw.r = 128;
        fc_effect.rgbw.g = 128;
        fc_effect.rgbw.b = 128;
        fc_effect.custom_index = 10;
    }
    else if (key_v == 8) // 旧W=25%：RGB混白
    {
        fc_effect.rgbw.r = 64;
        fc_effect.rgbw.g = 64;
        fc_effect.rgbw.b = 64;
        fc_effect.custom_index = 10;
    }
    else if (key_v == 9)  //七色渐变
    {
        fc_effect.b = USER_BRIGHTNESS_MAX;
        WS2812FX_setBrightness(fc_effect.b);
        rgbw_set_mutil_color(0, RED);
        rgbw_set_mutil_color(1, GREEN);
        rgbw_set_mutil_color(2, BLUE);
        rgbw_set_mutil_color(3, YELLOW);
        rgbw_set_mutil_color(4, CYAN);
        rgbw_set_mutil_color(5, MAGENTA);
        rgbw_set_mutil_color(6, WHITE);

        fc_effect.rgbw_scene.c_n = 7;
        fc_effect.custom_index = 12;
    }
    else if (key_v == 10)  //三色跳变
    {
        fc_effect.b = USER_BRIGHTNESS_MAX;
        WS2812FX_setBrightness(fc_effect.b);
        rgbw_set_mutil_color(0, RED);
        rgbw_set_mutil_color(1, GREEN);
        rgbw_set_mutil_color(2, BLUE);

        fc_effect.rgbw_scene.c_n = 3;
        fc_effect.custom_index = 11;
    }
    else if (key_v == 11)//关RGBW
    {
        fc_effect.rgbw_on_off_flag = DEVICE_OFF;
    }

    aws_rgb_mode_update(0);
}

// RGBW效果的切换
void rf433_rgbw_change_mode(void)
{
    fc_effect.rgbw_i++;
    fc_effect.rgbw_i %= 12; /* 旧工程RGB效果：6色、3档白光、渐变、跳变、关闭 */

    rgbw_app_effect_collections(fc_effect.rgbw_i);
}
#endif

void legacy_rgb_mode_next(void)
{
    rf433_rgbw_change_mode();
}

//------------------------------------------------------遥控

extern uint8_t flash_flag;
extern uint8_t flash_flag_1;

enum 
{
    AWS_PIXELS_MODE_START = 0,
    AWS_PIXELS_MODE_3COLORS_JUMP = AWS_PIXELS_MODE_START,
    AWS_PIXELS_MODE_7COLORS_BREATHING,
    AWS_PIXELS_MODE_7COLORS_GRADUAL,
    AWS_PIXELS_MODE_3COLORS_METEOR,
    AWS_PIXELS_MODE_7COLORS_METEOR,
    AWS_PIXELS_MODE_6COLORS_RUNNING,
    AWS_PIXELS_MODE_7COLORS_FLASH,
    AWS_PIXELS_MODE_SP1,
    AWS_PIXELS_MODE_SP2,
    AWS_PIXELS_MODE_SP3,
    AWS_PIXELS_MODE_SP4,
    AWS_PIXELS_MODE_SP5,
    AWS_PIXELS_MODE_MUSIC1,
    AWS_PIXELS_MODE_MUSIC2,

    AWS_PIXELS_MODE_MAX,
    AWS_PIXELS_MODE_LAST_ONE = AWS_PIXELS_MODE_MAX - 1,
};

// 幻彩灯动态模式切换
void aws_pixels_dynamic_mode_change(u8 dir)
{
    if (fc_effect.led_on_off_flag == DEVICE_OFF && fc_effect.led1_on_off_flag == DEVICE_OFF)
        return;

    if (fc_effect.Now_state == IS_light_scene || fc_effect.Now_state == IS_light_music)
    {
        if (dir == CHANGE_DIR_FORWARD)
        {
            fc_effect.color_i++;
            if (fc_effect.color_i > AWS_PIXELS_MODE_LAST_ONE)
                fc_effect.color_i = AWS_PIXELS_MODE_START;
        }
        else if (dir == CHANGE_DIR_BACKWARD)
        {
            if (fc_effect.color_i > AWS_PIXELS_MODE_START)
                fc_effect.color_i--;
            else
                fc_effect.color_i = AWS_PIXELS_MODE_LAST_ONE;
        }
        else if (dir == CHANGE_DIR_STAND)
        {
            fc_effect.color_i = fc_effect.color_i;
        }
    }
    else
    {
        if (dir == CHANGE_DIR_STAND)
        {
            return;
        }
    }

    printf("aws_pixels_dynamic_mode_change, color_i = %d", fc_effect.color_i);

    switch (fc_effect.color_i)
    {
    case 0:  // 三色跳变  速度固定 2s跳一下
        ls_set_color(0, RED);
        ls_set_color(1, GREEN);
        ls_set_color(2, BLUE);
        fc_effect.dream_scene.change_type = MODE_JUMP;
        fc_effect.dream_scene.c_n = 3;
        fc_effect.Now_state = IS_light_scene;
        break;

    case 1:  // 奥威斯调用 七色循环呼吸
        ls_set_color(0, RED);
        ls_set_color(1, GREEN);
        ls_set_color(2, BLUE);
        ls_set_color(3, YELLOW);
        ls_set_color(4, CYAN);
        ls_set_color(5, PURPLE);
        ls_set_color(6, WHITE);
        fc_effect.dream_scene.change_type = MODE_MUTIL_C_BREATH;
        fc_effect.dream_scene.c_n = 7;
        fc_effect.Now_state = IS_light_scene;
        break;

    case 2:  // 奥威斯调用 七彩渐变
        ls_set_color(0, RED);
        ls_set_color(1, GREEN);
        ls_set_color(2, BLUE);
        ls_set_color(3, YELLOW);
        ls_set_color(4, CYAN);
        ls_set_color(5, PURPLE);
        ls_set_color(6, WHITE);
        fc_effect.dream_scene.change_type = MODE_MUTIL_C_GRADUAL;
        fc_effect.dream_scene.c_n = 7;
        fc_effect.Now_state = IS_light_scene;
        break;

    case 3:  // 三色流星雨
        fc_effect.Now_state = IS_light_scene;
        fc_effect.dream_scene.change_type = MODE_SINGLE_METEOR;   // 变化类型、模式
        fc_effect.dream_scene.direction = 0x00;     // 方向
        fc_effect.dream_scene.seg_size = 0x05;      // 段大小
        fc_effect.dream_scene.c_n = 0x03;           // 颜色数量
        fc_effect.dream_scene.rgb[0].r = 0xFF;
        fc_effect.dream_scene.rgb[0].g = 0x00;
        fc_effect.dream_scene.rgb[0].b = 0x00;
        fc_effect.dream_scene.rgb[1].r = 0x00;
        fc_effect.dream_scene.rgb[1].g = 0xFF;
        fc_effect.dream_scene.rgb[1].b = 0x00;
        fc_effect.dream_scene.rgb[2].r = 0x00;
        fc_effect.dream_scene.rgb[2].g = 0x00;
        fc_effect.dream_scene.rgb[2].b = 0xFF;
        break;

    case 4:  // 七色流星雨
        fc_effect.Now_state = IS_light_scene;
        fc_effect.dream_scene.change_type = MODE_SINGLE_METEOR;   // 变化类型、模式
        fc_effect.dream_scene.direction = 0x00;     // 方向
        fc_effect.dream_scene.seg_size = 0x05;      // 段大小
        fc_effect.dream_scene.c_n = 0x06;           // 颜色数量
        fc_effect.dream_scene.rgb[0].r = 0xFF;
        fc_effect.dream_scene.rgb[0].g = 0x00;
        fc_effect.dream_scene.rgb[0].b = 0x00;
        fc_effect.dream_scene.rgb[1].r = 0x00;
        fc_effect.dream_scene.rgb[1].g = 0xFF;
        fc_effect.dream_scene.rgb[1].b = 0x00;
        fc_effect.dream_scene.rgb[2].r = 0x00;
        fc_effect.dream_scene.rgb[2].g = 0x00;
        fc_effect.dream_scene.rgb[2].b = 0xFF;
        fc_effect.dream_scene.rgb[3].r = 0xFF;
        fc_effect.dream_scene.rgb[3].g = 0x00;
        fc_effect.dream_scene.rgb[3].b = 0xFF;
        fc_effect.dream_scene.rgb[4].r = 0x00;
        fc_effect.dream_scene.rgb[4].g = 0xFF;
        fc_effect.dream_scene.rgb[4].b = 0xFF;
        fc_effect.dream_scene.rgb[5].r = 0xFF;
        fc_effect.dream_scene.rgb[5].g = 0xFF;
        fc_effect.dream_scene.rgb[5].b = 0x00;
        break;

    case 5:  // 红绿蓝青紫白 跑动
        fc_effect.Now_state = IS_light_scene;
        fc_effect.dream_scene.change_type = MODE_DOT_RUNNING;   // 变化类型、模式
        fc_effect.dream_scene.direction = 0x00;     // 方向
        fc_effect.dream_scene.seg_size = 0x05;      // 段大小
        fc_effect.dream_scene.c_n = 0x07;           // 颜色数量
        fc_effect.dream_scene.rgb[0].r = 0xFF;
        fc_effect.dream_scene.rgb[0].g = 0x00;
        fc_effect.dream_scene.rgb[0].b = 0x00;
        fc_effect.dream_scene.rgb[1].r = 0x00;
        fc_effect.dream_scene.rgb[1].g = 0xFF;
        fc_effect.dream_scene.rgb[1].b = 0x00;
        fc_effect.dream_scene.rgb[2].r = 0x00;
        fc_effect.dream_scene.rgb[2].g = 0x00;
        fc_effect.dream_scene.rgb[2].b = 0xFF;
        fc_effect.dream_scene.rgb[3].r = 0xFF;
        fc_effect.dream_scene.rgb[3].g = 0x00;
        fc_effect.dream_scene.rgb[3].b = 0xFF;
        fc_effect.dream_scene.rgb[4].r = 0x00;
        fc_effect.dream_scene.rgb[4].g = 0xFF;
        fc_effect.dream_scene.rgb[4].b = 0xFF;
        fc_effect.dream_scene.rgb[5].r = 0xFF;
        fc_effect.dream_scene.rgb[5].g = 0xFF;
        fc_effect.dream_scene.rgb[5].b = 0x00;
        fc_effect.dream_scene.rgb[6].r = 0x00;
        fc_effect.dream_scene.rgb[6].g = 0x00;
        fc_effect.dream_scene.rgb[6].b = 0x00;
        break;

    case 6:  // 七色频闪
        ls_set_color(0, BLUE);
        ls_set_color(1, GREEN);
        ls_set_color(2, RED);
        ls_set_color(3, WHITE);
        ls_set_color(4, YELLOW);
        ls_set_color(5, CYAN);
        ls_set_color(6, PURPLE);
        fc_effect.dream_scene.change_type = MODE_STROBE;
        fc_effect.dream_scene.c_n = 7;
        fc_effect.Now_state = IS_light_scene;
        break;

    case 7:  // 奥威斯4321特殊
        ls_set_color(0, RED);
        ls_set_color(1, GREEN);
        ls_set_color(2, BLUE);
        fc_effect.dream_scene.change_type = MODE_OPEN;
        fc_effect.dream_scene.c_n = 3;
        fc_effect.Now_state = IS_light_scene;
        break;

    case 8:  // 奥威斯4321特殊
        ls_set_color(0, RED);
        ls_set_color(1, GREEN);
        ls_set_color(2, BLUE);
        fc_effect.dream_scene.change_type = MODE_CLOSE;
        fc_effect.dream_scene.c_n = 3;
        fc_effect.Now_state = IS_light_scene;
        break;

    case 9:  //奥威斯4321特殊
        ls_set_color(0, RED);
        ls_set_color(1, GREEN);
        ls_set_color(2, BLUE);
        ls_set_color(3, YELLOW);
        ls_set_color(4, CYAN);
        ls_set_color(5, MAGENTA);
        ls_set_color(6, WHITE);
        fc_effect.dream_scene.c_n = 7;
        fc_effect.dream_scene.change_type = MODE_AOWEISI_1;
        fc_effect.Now_state = IS_light_scene;
        break;

    case 10:  // 奥威斯4321特殊
        ls_set_color(0, RED);
        ls_set_color(1, GREEN);
        ls_set_color(2, BLUE);
        ls_set_color(3, YELLOW);
        ls_set_color(4, CYAN);
        ls_set_color(5, MAGENTA);
        ls_set_color(6, WHITE);
        fc_effect.dream_scene.c_n = 7;
        fc_effect.dream_scene.change_type = MODE_AOWEISI_2;
        fc_effect.Now_state = IS_light_scene;
        break;

    case 11:  // 奥威斯4321特殊   
        ls_set_color(0, RED);
        ls_set_color(1, GREEN);
        ls_set_color(2, BLUE);
        ls_set_color(3, YELLOW);
        ls_set_color(4, CYAN);
        ls_set_color(5, MAGENTA);
        ls_set_color(6, WHITE);
        fc_effect.dream_scene.c_n = 7;
        fc_effect.dream_scene.change_type = MODE_AOWEISI_3;
        fc_effect.Now_state = IS_light_scene;
        break;

    case 12:  // 音乐模式1
        fc_effect.music.m = 8;
        fc_effect.Now_state = IS_light_music;
        break;

    case 13:  // 音乐模式2
        fc_effect.music.m = 3;
        fc_effect.Now_state = IS_light_music;
        break;

    default:
        return;
    }

    if (fc_effect.Now_state != IS_light_music)  // fc_effect.Now_state 等于 IS_light_music 时，会发送music_on出去，所以在这里不需要发送music_on出去
        fb_rf433_music_on_off(0);
    set_fc_effect();
    save_user_data_area3();
}

/* 旧协议的WS2812模式循环固定为6个动态效果+2个音乐效果。
 * 新工程的额外效果仍可保留，但不会由旧UART/433循环进入。 */
static void legacy_ws_mode_apply(void)
{
    fc_effect.color_i %= 8;
    if (fc_effect.color_i < 6) {
        fc_effect.Now_state = IS_light_scene;
        aws_pixels_dynamic_mode_change(CHANGE_DIR_STAND);
        return;
    }

    fc_effect.Now_state = IS_light_music;
    fc_effect.music.m = (fc_effect.color_i == 6) ? 8 : 3;
    fb_rf433_music_on_off(1);
    set_fc_effect();
    save_user_data_area3();
}

void legacy_ws_mode_next(void)
{
    fc_effect.color_i = (fc_effect.color_i + 1) % 8;
    legacy_ws_mode_apply();
}

//恢复灯光的默认状态
void rf433_reset_mode(void)
{
    printf("rf433_reset_mode");
    // ledreset_f = 0;
    fc_effect.rgbw.r = 0;
    fc_effect.rgbw.g = 0;
    fc_effect.rgbw.b = 0;
    fc_effect.Now_state = IS_light_music;
    fc_effect.custom_index = 10;
    fc_effect.dream_scene.speed = 100;
    fc_effect.speed = 80;
    fc_effect.music.m = 8;
    fc_effect.music.s = 100;   //灵敏度
    fc_effect.b = USER_BRIGHTNESS_MAX;
    fc_effect.color_i = 6;
    fc_effect.ex_white_light_power = 0; /* 仅清理VM占位状态，不启用白灯功能 */
    fc_effect.ex_white_light_level = WHITE_LIGHT_MAX_LEVEL;
    WS2812FX_setBrightness(fc_effect.b);
    set_fc_effect();
}

//单键亮度调整
void rf433_bright_adjust(void)
{
    static u8 b_f = 1;
    if (b_f)  //减少亮度
    {
        bright_sub();
        if (fc_effect.static_mode_brightness <= 1)
        {
            b_f = 0;
        }
    }
    else  //增加亮度
    {
        bright_plus();
        if (fc_effect.static_mode_brightness >= 8)
        {
            b_f = 1;
        }
    }
}


//单键速度调整
void rf433_speed_adjust(void)
{
    static u8 speed_f = 1;
    if (speed_f)  //减少速度
    {
        speed_slow();

        if (fc_effect.dream_scene.speed >= 500)
        {
            speed_f = 0;
        }
    }
    else  //增加速度
    {
        speed_fast();

        if (fc_effect.dream_scene.speed <= 50)
        {
            speed_f = 1;
        }
    }

    ls_scene_effect();
}

//rgbw灯开关
void rf433_rgbw_switch(void)
{
    if (fc_effect.rgbw_on_off_flag == DEVICE_ON)
    {
        fc_effect.rgbw_on_off_flag = DEVICE_OFF;
        fc_effect.last_rgbw_on_off_flag = DEVICE_OFF;
    }
    else
    {
        fc_effect.rgbw_on_off_flag = DEVICE_ON;
        fc_effect.last_rgbw_on_off_flag = DEVICE_ON;
    }
}

//幻彩灯开关
void rf433_color_switch(void)
{
    if (fc_effect.led_on_off_flag == DEVICE_ON)
    {
        fc_effect.led_on_off_flag = DEVICE_OFF;
        fc_effect.led1_on_off_flag = DEVICE_OFF;
        fc_effect.last_led_on_off_flag = DEVICE_OFF;
        fc_effect.last_led1_on_off_flag = DEVICE_OFF;
        fb_rf433_music_on_off(0);
    }
    else
    {
        fc_effect.led_on_off_flag = DEVICE_ON;
        fc_effect.led1_on_off_flag = DEVICE_OFF;
        fc_effect.last_led_on_off_flag = DEVICE_ON;
        fc_effect.last_led1_on_off_flag = DEVICE_OFF;
        ws2812_power_unlock();

        legacy_ws_mode_apply();
        aws_pixels_static_mode_change(CHANGE_DIR_STAND);
    }
}

/*************************************************** 自定义效果调度 *****************************************************/
void aws_rgb_mode_update(uint8_t power_sw_flag)
{
    //自定义内置效果，范围从10开始
    if (get_on_off_state())
    {
        if (fc_effect.custom_index == 10)   //静态颜色
        {
            WS2812FX_setSegment_colorOptions(           //设置一段颜色的效果
                USER_SEGMENT0_INDEX,                            // 第N段
                user_led_segments[USER_SEGMENT0_INDEX].start,   // 起始位置
                user_led_segments[USER_SEGMENT0_INDEX].end,     // 结束位置
                &WS2812FX_mode_static,                  //效果
                0,                                      //颜色，WS2812FX_setColors设置
                0,                                      //速度
                0);                                     //选项，这里像素点大小：1
            WS2812FX_set_coloQty(0, fc_effect.rgbw_scene.c_n);  // 设置颜色数量  0：第0段   fc_effect.dream_scene.c_n  颜色数量，一个颜色包含（RGB）
            ls_set_colors0(1, &fc_effect.rgbw);   //1:1个颜色    &fc_effect.rgb 这个颜色是什么色
            WS2812FX_start();
        }
        else if (fc_effect.custom_index == 11)  //跳变
        {
            WS2812FX_setSegment_colorOptions(
                USER_SEGMENT0_INDEX,                            // 第N段
                user_led_segments[USER_SEGMENT0_INDEX].start,   // 起始位置
                user_led_segments[USER_SEGMENT0_INDEX].end,     // 结束位置
                &rgbw_mutil_c_jump,       //效果
                0,                                      //颜色，WS2812FX_setColors设置
                500,            //速度
                0);                           //选项，这里像素点大小：3

            WS2812FX_set_coloQty(0, fc_effect.rgbw_scene.c_n);
            ls_set_colors0(fc_effect.rgbw_scene.c_n, &fc_effect.rgbw_scene.rgb);
            WS2812FX_start();
        }
        else if (fc_effect.custom_index == 12) //
        {
            WS2812FX_setSegment_colorOptions(
                USER_SEGMENT0_INDEX,                            // 第N段
                user_led_segments[USER_SEGMENT0_INDEX].start,   // 起始位置
                user_led_segments[USER_SEGMENT0_INDEX].end,     // 结束位置
                &rgbw_mutil_c_gradual,              //效果
                0,                                      //颜色，WS2812FX_setColors设置
                10,            //速度
                SIZE_MEDIUM);                           //选项，这里像素点大小：3,反向/反向

            WS2812FX_set_coloQty(0, fc_effect.rgbw_scene.c_n);
            ls_set_colors0(fc_effect.rgbw_scene.c_n, &fc_effect.rgbw_scene.rgb);
            WS2812FX_start();
        }
        else if (fc_effect.custom_index == 13) //单色呼吸
        {
            WS2812FX_setSegment_colorOptions(
                USER_SEGMENT0_INDEX,                            // 第N段
                user_led_segments[USER_SEGMENT0_INDEX].start,   // 起始位置
                user_led_segments[USER_SEGMENT0_INDEX].end,     // 结束位置
                &WS2812FX_mode_breath,            //效果
                0,                                      //颜色，WS2812FX_setColors设置
                fc_effect.dream_scene.speed,            //速度
                SIZE_MEDIUM);                           //选项，这里像素点大小：3

            WS2812FX_set_coloQty(0, fc_effect.dream_scene.c_n);
            ls_set_colors0(fc_effect.rgbw_scene.c_n, &fc_effect.rgbw_scene.rgb);
            WS2812FX_start();
        }
        else if (fc_effect.custom_index == 14) //两色渐变
        {
            WS2812FX_setSegment_colorOptions(
                USER_SEGMENT0_INDEX,                            // 第N段
                user_led_segments[USER_SEGMENT0_INDEX].start,   // 起始位置
                user_led_segments[USER_SEGMENT0_INDEX].end,     // 结束位置
                &WS2812FX_mutil_c_gradual,              //效果
                0,                                      //颜色，WS2812FX_setColors设置
                fc_effect.dream_scene.speed,            //速度
                SIZE_MEDIUM);                           //选项，这里像素点大小：3,反向/反向

            WS2812FX_set_coloQty(0, fc_effect.dream_scene.c_n);
            ls_set_colors0(fc_effect.rgbw_scene.c_n, &fc_effect.rgbw_scene.rgb);
            WS2812FX_start();
        }
        else if (fc_effect.custom_index == 15) //标准频闪
        {
            WS2812FX_setSegment_colorOptions(
                USER_SEGMENT0_INDEX,                            // 第N段
                user_led_segments[USER_SEGMENT0_INDEX].start,   // 起始位置
                user_led_segments[USER_SEGMENT0_INDEX].end,     // 结束位置
                &WS2812FX_mutil_strobe,       //效果
                0,                                      //颜色，WS2812FX_setColors设置
                fc_effect.dream_scene.speed,            //速度
                0);                           //选项，这里像素点大小：3

            WS2812FX_set_coloQty(0, fc_effect.dream_scene.c_n);
            ls_set_colors0(fc_effect.rgbw_scene.c_n, &fc_effect.rgbw_scene.rgb);
            WS2812FX_start();
        }

        // 幻彩是开机
        if (power_sw_flag == 2 && fc_effect.led_on_off_flag == DEVICE_ON)
        {
            // ledreset_f = 0;
            set_bright(100);
            aoweisi_kaiji_xiaoguo();
        }
        if (power_sw_flag == 1 && fc_effect.led_on_off_flag == DEVICE_ON)
        {
            // ledreset_f = 0;
            set_music_mode(8);
            fc_effect.color_i = 12;
        }
    }
}

void aoweisi_effect_1(void)
{
    WS2812FX_stop();
    WS2812FX_setSegment_colorOptions(
        USER_SEGMENT1_INDEX,                            // 第N段
        user_led_segments[USER_SEGMENT1_INDEX].start,   // 起始位置
        user_led_segments[USER_SEGMENT1_INDEX].end,     // 结束位置
        &double_bounce_flow,        //效果
        0,                                      //颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed,            //速度
        FADE_XFAST);                                //选项，这里像素点大小：3,反向/反向


    WS2812FX_setSegment_colorOptions(
        USER_SEGMENT2_INDEX,                            // 第N段
        user_led_segments[USER_SEGMENT2_INDEX].start,   // 起始位置
        user_led_segments[USER_SEGMENT2_INDEX].end,     // 结束位置
        &fu_double_bounce_flow,        //效果
        0,                                      //颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed,            //速度
        FADE_SLOW);                                //选项，这里像素点大小：3,反向/反向

    WS2812FX_set_coloQty(1, fc_effect.dream_scene.c_n);
    WS2812FX_set_coloQty(2, fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);
    ls_set_colors2(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);

    WS2812FX_start();
    WS2812FX_trigger();
}

void aoweisi_effect_2(void)
{
    WS2812FX_stop();
    WS2812FX_setSegment_colorOptions(
        USER_SEGMENT1_INDEX,                            // 第N段
        user_led_segments[USER_SEGMENT1_INDEX].start,   // 起始位置
        user_led_segments[USER_SEGMENT1_INDEX].end,     // 结束位置
        &longmeteor_signle_c,        //效果
        0,                                      //颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed,            //速度
        0);                                //选项，这里像素点大小：3,反向/反向


    WS2812FX_setSegment_colorOptions(
        USER_SEGMENT2_INDEX,                            // 第N段
        user_led_segments[USER_SEGMENT2_INDEX].start,   // 起始位置
        user_led_segments[USER_SEGMENT2_INDEX].end,     // 结束位置
        &f_longmeteor_signle_c,        //效果
        0,                                      //颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed,            //速度
        0);                                //选项，这里像素点大小：3,反向/反向

    WS2812FX_set_coloQty(1, fc_effect.dream_scene.c_n);
    WS2812FX_set_coloQty(2, fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);
    ls_set_colors2(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);

    WS2812FX_start();
    WS2812FX_trigger();
}

void aoweisi_effect_3(void)
{
    flash_flag = 1;
    flash_flag_1 = 1;
    WS2812FX_stop();
    WS2812FX_setSegment_colorOptions(
        USER_SEGMENT1_INDEX,                            // 第N段
        user_led_segments[USER_SEGMENT1_INDEX].start,   // 起始位置
        user_led_segments[USER_SEGMENT1_INDEX].end,     // 结束位置
        &longmeteor_signle_blink,        //效果
        0,                                      //颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed,            //速度
        0);                                //选项，这里像素点大小：3,反向/反向

    WS2812FX_setSegment_colorOptions(
        USER_SEGMENT2_INDEX,                            // 第N段
        user_led_segments[USER_SEGMENT2_INDEX].start,   // 起始位置
        user_led_segments[USER_SEGMENT2_INDEX].end,     // 结束位置
        &f_longmeteor_signle_blink,        //效果
        0,                                      //颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed,            //速度
        0);                                //选项，这里像素点大小：3,反向/反向

    WS2812FX_set_coloQty(1, fc_effect.dream_scene.c_n);
    WS2812FX_set_coloQty(2, fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);
    ls_set_colors2(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);

    WS2812FX_start();
    WS2812FX_trigger();
}

/*************************************************** 情景效果调度 *****************************************************/
static void ls_scene_effect(void)
{
    switch (fc_effect.dream_scene.change_type)
    {
    case MODE_MUTIL_RAINBOW:      //彩虹
        strand_rainbow();
        break;
    case MODE_MUTIL_JUMP://跳变模式
        aoweisi_kaiji_xiaoguo();
        break;
    case MODE_MUTIL_BRAETH://呼吸模式
        strand_breath();
        break;
    case MODE_MUTIL_TWIHKLE://闪烁模式
        strand_twihkle();
        break;
    case MODE_MUTIL_FLOW_WATER://流水模式
        strand_flow_water();
        break;
    case MODE_CHAS_LIGHT://追光模式
        strand_chas_light();
        break;
    case MODE_MUTIL_COLORFUL://炫彩模式
        strand_colorful();
        break;
    case MODE_MUTIL_SEG_GRADUAL://渐变模式
        mutil_seg_grandual();
        break;
    case MODE_JUMP:     //标准跳变
        aoweisi_jump();
        break;
    case MODE_STROBE:   //标准频闪
        ls_strobe();
        break;
    case MODE_MUTIL_C_GRADUAL:  //多段同时渐变
        aoweisi_mutil_c_grandual();
        break;
    case MODE_2_C_FIX_FLOW:      //两种颜色混色流水
        tow_color_fix_flow();
        break;
    case MODE_SINGLE_FLASH_RANDOM:
        single_c_flash_random();
        break;
    case MODE_SEG_FLASH_RANDOM:
        seg_mutil_c_flash_random();
        break;
    case MODE_SINGLE_C_BREATH:   //单色呼吸
        single_c_breath();
        break;
    case MODE_SINGLE_METEOR:
        single_c_meteor();     //单流星
        break;
    case MODE_B_G_METEOR:
        background_meteor();
        break;
    case MODE_OPEN:
        open_mode();    //开幕效果
        break;
    case MODE_CLOSE:
        close_mode();     //闭幕效果
        break;
    case MODE_DOT_RUNNING:     //跑动效果
        dot_running();   //奥威斯调用
        break;
    case MODE_COLOR_METEOR:   //无背景颜色的流星
        color_meteor();
        break;
    case MODE_JUMP_METEORR:   //跳变效果的流星
        color_jump_meteor();
        break;
    case MODE_GRADUAL_METEOR:   //渐变效果的流星
        color_gradual_meteor();
        break;
    case MODE_RAINBOW_FLOW:
        rainbow_flow();
        break;
    case MODE_MUTIL_C_BREATH:
        mutil_c_breath();  //多色循环呼吸  奥威斯调用
        break;
    case MODE_REBOUND_FLOW: //反弹流水
        rebound_flow();
        break;
    case MODE_MUTILE_SUPERPOSITION:
        superposition_mutil_effect();
        break;
    case MODE_AOWEISI_1:
        aoweisi_effect_1();
        break;
    case MODE_AOWEISI_2:
        aoweisi_effect_2();
        break;
    case MODE_AOWEISI_3:
        aoweisi_effect_3();
        break;

    default:
        break;
    }
}

/*************************************************** 声控效果调度 *****************************************************/
void music_effect(void)
{
    extern uint16_t music_meteor(void);

    //频谱
    extern uint16_t music_fs(void);
    extern uint16_t music_fs_bc(void);
    extern uint16_t music_fs_green_blue(void);
    extern uint16_t music_fs_green_blue2(void);

    // 节奏
    extern uint16_t music_2_side_oc(void);
    extern uint16_t music_2_side_oc2(void);
    extern uint16_t music_oc_2(void);
    extern uint16_t music_rainbow_flash(void);

    // 滚动
    extern uint16_t music_energy(void);
    extern uint16_t music_multi_c_flow(void);
    extern uint16_t music_meteor(void);

    // 能量
    extern uint16_t music_star(void); //七彩

    extern void set_music_s_m(u8 m);

    void *p = set_music_s_m;
    void *p2 = set_music_s_m;
    switch (fc_effect.music.m)
    {
    case 0: //能量1
        set_music_s_m(0);
        p = &music_star;
        break;
    case 1: //能量2
        set_music_s_m(1);
        p = &music_star;
        break;
    case 2: //能量3
        set_music_s_m(2);
        p = &music_star;
        break;

    case 3://节奏1
        p = &music_2_side_oc;
        p2 = &music_2_side_oc2;
        break;

    case 4://节奏2
        p = &music_oc_2;
        break;

    case 5://节奏3
        p = &music_rainbow_flash;
        break;

    case 6://频谱1
        p = &music_fs;
        break;
    case 7://频谱2
        p = &music_fs_bc;
        break;
    case 8://频谱3
        p = &music_fs_green_blue;
        p2 = &music_fs_green_blue2;
        break;

    case 9://滚动1
        p = &music_energy;
        break;
    case 10://滚动2
        p = &music_multi_c_flow;
        break;
    case 11://滚动3
        p = &music_meteor;
        break;
    }

    WS2812FX_setSegment_colorOptions(
        USER_SEGMENT1_INDEX,                            // 第N段
        user_led_segments[USER_SEGMENT1_INDEX].start,   // 起始位置
        user_led_segments[USER_SEGMENT1_INDEX].end,     // 结束位置
        p,              //效果
        WHITE,                      //颜色，WS2812FX_setColors设置
        50,                        //速度
        SIZE_MEDIUM | FADE_XSLOW);               //选项，这里像素点大小：3,反向/反向
    // WS2812FX_start();

    WS2812FX_setSegment_colorOptions(
        USER_SEGMENT2_INDEX,                            // 第N段
        user_led_segments[USER_SEGMENT2_INDEX].start,   // 起始位置
        user_led_segments[USER_SEGMENT2_INDEX].end,     // 结束位置
        p2,              //效果
        WHITE,                      //颜色，WS2812FX_setColors设置
        50,                        //速度
        SIZE_MEDIUM | FADE_XSLOW);               //选项，这里像素点大小：3,反向/反向
    WS2812FX_start();
    WS2812FX_trigger();
}

/************************************************** 总效果调度函数 *****************************************************/
u8 music_flag = 1;

void set_fc_effect(void)
{
    if (fc_effect.on_off_flag == DEVICE_ON)
    {
        switch (fc_effect.Now_state)
        {
        case IS_light_scene:
            printf("当前效果状态: 炫彩情景");
            break;
        case ACT_TY_PAIR:
            printf("当前效果状态: 配对效果");
            break;
        case ACT_CUSTOM:
            printf("当前效果状态: 自定义效果");
            break;
        case IS_light_music:
            printf("当前效果状态: 音乐律动");
            break;
        case IS_smear_adjust:
            printf("当前效果状态: 涂抹功能");
            break;
        case IS_STATIC:
            printf("当前效果状态: 静态模式");
            break;
        default:
            printf("当前效果状态: 其他");
            break;
        }
        
        switch (fc_effect.Now_state)
        {
        case IS_light_scene:
            music_flag = 0;
            clean_period_cnt(); //计时清零，切换效果时立即执行效果
            set_bright(100);
            ls_scene_effect();
            /* code */
            break;
        case ACT_TY_PAIR:
            // 配对完成，要恢复fc_effect.Now_state
            break;
        case ACT_CUSTOM:
            /* code */
            break;
        case IS_light_music:
            music_flag = 1;
            set_bright(100);
            fb_rf433_music_on_off(1);
            /* code */
            music_effect();
            break;
        case IS_smear_adjust:
            break;
        case IS_STATIC:
            music_flag = 0;
            static_mode();
            break;

        default:
            break;
        }

    }
}


/************************************************** 白光灯覆盖WS2812灯带 *****************************************************/
#if WHITE_LIGHT_EX_ENABLE

void user_ex_white_light_set_power(uint8_t power)
{
    fc_effect.ex_white_light_power = power;
}

void user_ex_white_light_switch_power(void)
{
    fc_effect.ex_white_light_power ^= 1;
}

void user_ex_white_light_set_level(uint32_t level)
{
    fc_effect.ex_white_light_level = level;
}

uint8_t user_ex_white_light_get_power(void)
{
    return fc_effect.ex_white_light_power;
}

uint32_t user_ex_white_light_get_level(void)
{
    return fc_effect.ex_white_light_level;
}
#endif
