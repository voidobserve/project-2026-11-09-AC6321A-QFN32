
#include "stdio.h"
#include "led_strip_sys.h"


// 上电必须初始化灯具参数
// 灯具长度发生变化必须调用
void init_led_strip(led_strip_t *strip)
{
    g_led_strip.channel = strip->channel;
    g_led_strip.lenght = strip->lenght;

}


// R序列为0
// G序列为1
// B序列为2
#define _R  0
#define _G  1
#define _B  2

// 数组输入rgb_sequence_e
// BUF的元素排列按照rgb_sequence_e，不能改变
const rgb_sequence_t rgb_sequence_buf[6] =
{
    {_R, _G, _B},
    {_R, _B, _G},
    {_G, _B, _R},
    {_G, _R, _B},
    {_B, _R, _G},
    {_B, _G, _R}
};


void m_hsv_to_rgb(u8 *R, u8 *G, u8 *B, int h, int s, int v)
{
	// R,G,B from 0-255, H from 0-360, S,V from 0-1000
	int i;
	float RGB_min, RGB_max;
    int g,  r,  b;

	if(h>360)
        h=360;
    if(s>1000)
        s=1000;
    if(v>1000)
        v=1000;

	s = s / 10;
	v = v / 10;

	RGB_max = v*2.55f;
	RGB_min = RGB_max*(100 - s) / 100.0f;

	i = h / 60;
	int difs = h % 60; // factorial part of h

					   // RGB adjustment amount by hue
	float RGB_Adj = (RGB_max - RGB_min)*difs / 60.0f;

	switch (i) {
	case 0:
		r = RGB_max;
		g = RGB_min + RGB_Adj;
		b = RGB_min;
		break;
	case 1:
		r = RGB_max - RGB_Adj;
		g = RGB_max;
		b = RGB_min;
		break;
	case 2:
		r = RGB_min;
		g = RGB_max;
		b = RGB_min + RGB_Adj;
		break;
	case 3:
		r = RGB_min;
		g = RGB_max - RGB_Adj;
		b = RGB_max;
		break;
	case 4:
		r = RGB_min + RGB_Adj;
		g = RGB_min;
		b = RGB_max;
		break;
	default:		// case 5:
		r = RGB_max;
		g = RGB_min;
		b = RGB_max - RGB_Adj;
		break;
	}
    *G = g;
    *B = b;
    *R = r;

}















