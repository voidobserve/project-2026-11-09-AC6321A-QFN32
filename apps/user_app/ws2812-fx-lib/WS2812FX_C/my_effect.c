
#include "WS2812FX.H"
#include "Adafruit_NeoPixel.H"



#define MAX_EFFECT 8
// 0:涂鸦效果，收到涂鸦指令才切换
// 1：内置效果，按键，IR等操作切换
uint8_t isWS2812 = 1;

static uint8_t mode_index = 0;
static uint8_t my_brightness;
static uint16_t my_speed = 800;

extern uint8_t get_effect_index(void);
extern void switch_effect(uint8_t effect);

void effect_speed_plus(void)
{
  my_speed -= 100;
  if((my_speed < 10)||(my_speed > 800))
    my_speed =10;

  switch_effect(get_effect_index());
}

void effect_speed_sub(void)
{
  my_speed += 100;
  if(my_speed > 800)
    my_speed = 800;

  switch_effect(get_effect_index());
}

void effect_plus(void)
{
    mode_index++;
    mode_index %= MAX_EFFECT;
}

void effect_sub(void)
{
    if(mode_index > 0)
        mode_index--;
    else{
        mode_index = MAX_EFFECT - 1;
    }
}


uint8_t get_effect_index(void)
{
    return mode_index;
}

void set_effect_index(uint8_t m)
{
    mode_index = m;
    mode_index %= MAX_EFFECT;

}

// 设置效果用涂鸦的
void set_effect_tuya(void)
{
    isWS2812 = 0;
}

// 设置效果用本地效果
void set_effect_local(void)
{
    isWS2812 = 1;
}

// 获取当前效果属于本地/涂鸦
uint8_t get_effect_type(void)
{
    return isWS2812;
}
void set_effect_type(uint8_t t)
{
    isWS2812 = t;
}

#if 0
// 随机扫描，尾部褪色：随机颜色流水(往返)，走过后留下渐暗颜色
static void random_sweep_tail_fade(void)
{
    // FX_MODE_TWINKLE_FADE
    WS2812FX_setSegment_colorOptions(1,0,28,FX_MODE_FADE_SINGLE,0,my_speed,0);
    WS2812FX_setSegment_colorOptions(0,0,28,FX_MODE_COLOR_SWEEP_RANDOM,GRAY,my_speed,0);
}

void switch_effect(uint8_t effect)
{

    WS2812FX_stop();
    // WS2812FX_setSegmentsInactive();
    WS2812FX_setSegment_colorOptions(0,0,28,FX_MODE_TWINKLE_FADE_RANDOM,GRAY,my_speed,0);

    WS2812FX_removeActiveSegment(1);
    WS2812FX_removeActiveSegment(2);
    switch (effect)
    {
    case 0:
         random_sweep_tail_fade();
        break;

    case 1:
        // WS2812FX_setMode(FX_MODE_TWINKLE_FADE_RANDOM);  //随机闪点，随机颜色，慢慢淡出
        WS2812FX_setSegment_colorOptions(0,0,9,FX_MODE_TWINKLE_FADE_RANDOM,GRAY,my_speed,FADE_SLOW);
	    WS2812FX_setSegment_colorOptions(1,10,19,FX_MODE_TWINKLE_FADE_RANDOM,GRAY,my_speed,FADE_SLOW);
        WS2812FX_setSegment_colorOptions(2,20,28,FX_MODE_TWINKLE_FADE_RANDOM,GRAY,my_speed,FADE_SLOW);
        // WS2812FX_setSegment_colorOptions(0,0,28,FX_MODE_TWINKLE_FADE_RANDOM,GRAY,my_speed,FADE_SLOW);//随机闪点，随机颜色，慢慢淡出
        break;
    case 2:
        //WS2812FX_setMode(FX_MODE_RAINBOW_CYCLE); //彩虹流水 - 随机颜色
        WS2812FX_setSegment_colorOptions(0,0,28,FX_MODE_RAINBOW_CYCLE,GRAY,my_speed,0);
        break;

    case 3:
        WS2812FX_setSegment_colorOptions(0,0,28,FX_MODE_COLOR_SWEEP_RANDOM,GRAY,my_speed,0);
        break;
    case 4:
        // 效果需要segment[1]支持
        WS2812FX_setColor_seg(1,GRAY);

        //WS2812FX_setMode(FX_MODE_FIREWORKS_RANDOM); //随机闪现一段不同颜色，再淡出
        WS2812FX_setSegment_colorOptions(0,0,28,FX_MODE_FIREWORKS_RANDOM,GRAY,my_speed,0);
        break;
    case 5:
        //WS2812FX_setMode(FX_MODE_SINGLE_DYNAMIC);  /* 每个led都是随机颜色 */
        WS2812FX_setSegment_colorOptions(0,0,28,FX_MODE_SINGLE_DYNAMIC,GRAY,my_speed,0);
        break;
    case 6:
        //WS2812FX_setMode(FX_MODE_RAINBOW); 	/* 所有LED彩虹渐变 */
        WS2812FX_setSegment_colorOptions(0,0,28,FX_MODE_RAINBOW,GRAY,my_speed,0);
        break;
    case 7:
        //WS2812FX_setMode(FX_MODE_MULTI_DYNAMIC); /*  每个LED随机颜色，并动态变化颜色 */
        WS2812FX_setSegment_colorOptions(0,0,28,FX_MODE_MULTI_DYNAMIC,GRAY,my_speed,0);
        break;
    default:
        break;
    }
    WS2812FX_start();
}


void my_effect_init(void)
{
    WS2812FX_init(29,NEO_RGB);
    WS2812FX_setBrightness(((u32)light_value.V*255) / 1000);
    WS2812FX_setSpeed(800);
    switch_effect(mode_index);
    // WS2812FX_start();
}

#endif