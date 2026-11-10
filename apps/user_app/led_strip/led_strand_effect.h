#ifndef led_strand_effect_h
#define led_strand_effect_h
#include "cpu.h"
#include "led_strip_sys.h"
#include "WS2812FX.H" 

#include "one_wire.h"
#define MAX_SMEAR_LED_NUM 48  //最多48个灯/48段


/************************************************** 白光灯覆盖WS2812灯带 *****************************************************/
#define WHITE_LIGHT_EX_ENABLE       0  /* 旧产品无独立白灯通道，白光由RGB混色实现 */
#define WHITE_LIGHT_MAX_LEVEL       8
/************************************************** 白光灯覆盖WS2812灯带 *****************************************************/


//当前模式枚举
typedef enum
{
  ACT_TY_PAIR,          //配对效果
  ACT_CUSTOM,           //自定义效果
  IS_STATIC,            //静态模式
  IS_light_music = 27,    //音乐律动
  IS_light_scene = 56,   //炫彩情景
  IS_smear_adjust = 59  //涂抹功能


} Now_state_e;

//涂抹工具
typedef enum
{
  IS_drum = 1,  //油桶
  IS_pen = 2,   //画笔
  IS_eraser = 3 //橡皮檫
} smear_tool_e;

//方向
typedef enum
{
  IS_forward = 0, //正向
  IS_back = 16    //反向
} direction_e;

//变化方式
typedef enum
{
  MODE_MUTIL_RAINBOW = 2,           //彩虹(多段颜色)
  MODE_MUTIL_JUMP = 10,             //跳变模式(多段颜色)
  MODE_MUTIL_BRAETH = 11,           //呼吸模式(多段颜色)
  MODE_MUTIL_TWIHKLE = 12,          //闪烁模式(多段颜色)
  MODE_MUTIL_FLOW_WATER = 13,       //流水模式(多段颜色)
  MODE_CHAS_LIGHT = 14,             //追光模式
  MODE_MUTIL_COLORFUL = 15,         //炫彩模式(多段颜色)
  MODE_MUTIL_SEG_GRADUAL = 16,      //渐变模式(多段颜色)
  MODE_JUMP,                        //标准跳变
  MODE_STROBE,                      //频闪，颜色之间插入黑mode
  MODE_MUTIL_C_GRADUAL,             //多种颜色切换整条渐变
  MODE_2_C_FIX_FLOW,                //两种颜色混合流水，渐变色流水
  MODE_SINGLE_FLASH_RANDOM = 21 ,   //单点，单色随机闪现
  MODE_SEG_FLASH_RANDOM = 22,       //从的颜色池抽取颜色闪现，以段为单位，闪现位置随机
  MODE_SINGLE_METEOR = 23,          //流星效果
  MODE_SINGLE_C_BREATH = 24,        //单色呼吸
  MODE_B_G_METEOR = 25,             //带背景色流星
  MODE_OPEN = 26,                   //开幕式
  MODE_CLOSE = 27,                  //闭幕式
  MODE_DOT_RUNNING = 28,            //多个点跑马 ，点和点直接固定间隔5，支持每个点不同颜色，支持设置背景色
  MODE_DOT_RUNNING_COLLECTIONS = 29,  //跑马集合模式
  MODE_SINGLE_SUPERPOSITION = 30,         //单色堆积
  MODE_B_G_SUPERPOSITION = 31,         //带底色堆积
  MODE_MUTILE_SUPERPOSITION = 32,       //多色堆积，不灭
  MODE_BREATH_W,                    //W通道呼吸 
  MODE_GRADUAL,                     //标准渐变，彩虹颜色
  MODE_MUTIL_C_BREATH,
  MODE_COLOR_METEOR ,               //无背景色，指定颜色的流星
  MODE_JUMP_METEORR ,               //跳变效果的流星
  MODE_GRADUAL_METEOR ,             //渐变效果的流星
  MODE_RAINBOW_FLOW ,


  MODE_REBOUND_FLOW = 101,
  MODE_AOWEISI_1 = 102,
  MODE_AOWEISI_2 = 103,
  MODE_AOWEISI_3 = 104,



} change_type_e;

#pragma pack (1)
/*----------------------------静态模式结构体----------------------------------*/
  

/*----------------------------幻彩情景结构体----------------------------------*/
typedef struct
{
  change_type_e change_type;  //变化类型、模式
  direction_e direction; 
  unsigned char seg_size;     //段大小
  unsigned char c_n;          //颜色数量
  color_t rgb[MAX_NUM_COLORS];
  unsigned short speed;       //由档位决定 
} dream_scene_t;


/*----------------------------音乐律动灵敏度结构体----------------------------------*/
typedef struct
{
  unsigned char m;  //模式
  unsigned char s;  //灵敏度
  unsigned char m_type;  //区分音乐的模式，手机麦或者外麦
}music_t;

/*----------------------------幻彩灯串效果大结构体----------------------------------*/
typedef struct
{
  unsigned char on_off_flag;            //开关状态
  unsigned char rgbw_on_off_flag : 4;   //RGBW开关状态
  unsigned char led_on_off_flag : 4;    //幻彩开关状态
  unsigned char led1_on_off_flag : 4;
  unsigned char last_rgbw_on_off_flag : 4;  //RGBW开关状态
  unsigned char last_led_on_off_flag : 4;   //幻彩开关状态
  unsigned char last_led1_on_off_flag : 4;

  unsigned short led_num;       //灯点数

  color_t rgbw;                 //七彩灯静态
  unsigned char b;              //亮度
  unsigned char speed;          //8档
  unsigned char sequence;       //RGB通道顺序
  Now_state_e Now_state;        //当前运行模式
  Now_state_e dream_Now_state;   //幻彩当前运行模式

  unsigned char color_i;
  unsigned char huancai_color_i;
  unsigned char rgbw_i;
  dream_scene_t dream_scene;    //幻彩情景
  dream_scene_t rgbw_scene;    //七彩情景
 

  u8 music_mode;
  music_t music;                //音乐
  u8 custom_index;               //自定义模式
 
  unsigned char mode_cycle;     //1:模式完成一个循环。0：正在跑，和meteor_period搭配用   流星
  u16 period_cnt;               //ms,运行时的计数器   流星
  unsigned char meteor_period;  //周期值，单位秒   流星

  u8 static_mode_brightness;
  unsigned short led_num_max;   //灯点数最大限制

  /* 独立白灯功能已失能；字段必须保留以兼容既有VM中fc_effect_t的布局。 */
  u8 ex_white_light_power;
  u8 ex_white_light_level;

} fc_effect_t;

#pragma pack ()

extern fc_effect_t fc_effect;

void effect_smear_adjust_updata(smear_tool_e tool, hsv_t *colour,unsigned short *led_place);

void set_fc_effect(void);

void full_color_init(void);
void soft_turn_off_lights(void); //软关灯处理
void soft_turn_on_the_light(void) ;  //软开灯处理
void ls_set_speed(uint8_t s);
void ls_set_color(uint8_t n, uint32_t c);
void fc_static_effect(u8 n);
void ls_set_colors(uint8_t n, color_t *c);

void aws_rgb_mode_update(uint8_t power_sw_flag);
void legacy_rgb_mode_next(void);
void legacy_ws_mode_next(void);


/************************************************** 白光灯覆盖WS2812灯带 *****************************************************/
void user_ex_white_light_set_power(uint8_t power);
void user_ex_white_light_switch_power(void);
void user_ex_white_light_set_level(uint32_t level);
uint8_t user_ex_white_light_get_power(void);
uint32_t user_ex_white_light_get_level(void);

void uart_update_current_white_light_level(u8 level);
void uart_update_current_white_light_power(u8 power);
/************************************************** 白光灯覆盖WS2812灯带 *****************************************************/

#endif
