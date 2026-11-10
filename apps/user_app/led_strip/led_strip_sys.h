#ifndef led_srtip_sys_h
#define led_srtip_sys_h


// 控灯系统属性

#include "cpu.h"


/******************************************************************系统cfg******************************************************************/
/*定义数据总线大端/小端，二选一*/
#define SYS_LITTLE_END  0
#define SYS_BIG_END     1
// 定义协议大小端
#define PROTOCOL_END    SYS_BIG_END
#define SYS_BUS_END     SYS_LITTLE_END

#if (PROTOCOL_END == SYS_BUS_END)
#define __SWP16(n) n
#define __SWP32(n) n
#endif
#if (PROTOCOL_END != SYS_BUS_END)
#define __SWP16(n)  ((((u16)(n) & 0xff00) >> 8) | (((u16)(n) & 0x00FF) << 8))
#define __SWP32(n)  (((u32)(n) & 0xff000000 >> 24) | \
                    ((u32)(n) & 0x00ff0000 >> 8) | \
                    ((u32)(n) & 0x0000ff00 << 8) | \
                    ((u32)(n) & 0x000000ff << 24))

#endif


typedef enum
{
    _RGB,
    _RBG,
    _GBR,
    _GRB,
    _BRG,
    _BGR
} rgb_sequence_e;


/******************************************************************灯具配置 cfg******************************************************************/
/* 定义灯珠颜色，五选1*/
// #define LED_SRTIP_R
// #define LED_SRTIP_RG
#define LED_STRIP_RGB
// #define LED_STRIP_RGBW
// #define LED_STRIP_RGBCW

/* 定义灯珠通道,取值1~5*/
#define LED_STRIP_CH  3

#define MAX_DUAN_OF_PAINT   20      //涂抹模式最多支持20段,根据涂鸦平台定义
#define RGB_SEQUENCE        _GRB    //定义RGB通道顺序，根据实际定义
#define Disp_buf_Len        1100    //显存大小，决定了可用灯带的长度，3*350=1050，最多1050个灯珠，350组灯

// #define USER_BRIGHTNESS_MAX      (150)   // 本来255，改为150最大，用于限制最大功率
#define USER_BRIGHTNESS_MAX      (112)

/******************************************************************common*****************************************************************/

typedef struct
{
    u8 r;
    u8 g;
    u8 b;
}rgb_sequence_t;

typedef struct _HSV_COLOUR_DAT
{
    u32 h_val;
    u32 s_val;
    u32 v_val;
}HSV_COLOUR_DAT;

typedef struct _LED_COLOUR_DAT
{
    u32 r_val;
    u32 g_val;
    u32 b_val;
}LED_COLOUR_DAT;

typedef struct
{
    u16 h_val;                      //h的数值，色相 0~360
    u16 s_val;                      //s的数值，饱和度 0~360
    u16 v_val;                      //v的数值，亮度0~1000
}hsv_t;


typedef struct
{
#ifdef LED_STRIP_R
    u8 r;
#endif
#ifdef LED_STRIP_RG
    u8 r;
    u8 g;
#endif
#ifdef LED_STRIP_RGB
    u8 r;
    u8 g;
    u8 b;
#endif
#ifdef LED_STRIP_RGBW
    u8 r;
    u8 g;
    u8 b;
    u8 w;
#endif
#ifdef LED_STRIP_RGBCW
    u8 r;
    u8 g;
    u8 b;
    u8 c;
    u8 w;
#endif
}color_t;


typedef enum
{
    DEVICE_OFF,     //关机
    DEVICE_ON,      //开机
}ON_OFF_FLAG;

//灯具属性
typedef struct
{
    u16 lenght;     //灯带长度
    u8  channel;    //通道
}led_strip_t;
extern led_strip_t g_led_strip;
extern u8 on_off_flag;
extern u32 LED_LEDGTH;
extern const rgb_sequence_t rgb_sequence_buf[6];

void init_led_strip(led_strip_t *strip);


#endif
