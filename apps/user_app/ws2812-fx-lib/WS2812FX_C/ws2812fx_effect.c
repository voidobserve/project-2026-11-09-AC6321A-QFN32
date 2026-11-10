// 利用ws2812fx库，创建效果
#include "app_config.h"
#include "WS2812FX.h"
#include "ws2812fx_tool.h"
#include "Adafruit_NeoPixel.h"
#include "led_strand_effect.h"
#include "system/includes.h"

extern  Segment* _seg;
extern  uint16_t _seg_len;
extern Segment_runtime* _seg_rt;



void clean_step(void)
{

  _seg_rt->counter_mode_step = 0;
}


/* ****************项目专属效果******************************* */
// 调整RGB顺序效果，以红绿蓝跳变
uint16_t WS2812FX_adj_rgb_sequence(void)
{
  u32 c = BLUE;
  switch(_seg_rt->counter_mode_step )
  {
    case 0:
      c = RED;
      break;
    case 1:
      c = GREEN;
      break;
    case 2:
      c = BLUE;
      break;
  }

  Adafruit_NeoPixel_fill( c, _seg->start, _seg_len);

  if(_seg_rt->counter_mode_step < 3)
  {
    _seg_rt->counter_mode_step++;
  }
  return 1000;
}

// 亮度+最大提示效果
uint16_t max_bright_tips_blue(void)
{
  if(_seg_rt->counter_mode_step == 0)
  {
    Adafruit_NeoPixel_fill( BLUE, _seg->start, _seg_len);
  }
  _seg_rt->counter_mode_step++;
  if(_seg_rt->counter_mode_step > 1)
  {
    extern void read_flash_device_status_init(void);
    read_flash_device_status_init();
    set_fc_effect();
  }
  return 100;
}

uint16_t max_bright_tips_puple(void)
{
  if(_seg_rt->counter_mode_step == 0)
  {
    Adafruit_NeoPixel_fill( PINK, _seg->start, _seg_len);
  }
  _seg_rt->counter_mode_step++;
  if(_seg_rt->counter_mode_step > 1)
  {
    extern void read_flash_device_status_init(void);
    read_flash_device_status_init();
    set_fc_effect();
  }
  return 100;
}

uint16_t power_on_effect(void)
{
  if(_seg_rt->counter_mode_step)
  {
    Adafruit_NeoPixel_fill( WHITE, _seg->start, _seg_len);
  }
  else
  {
    Adafruit_NeoPixel_fill( GRAY, _seg->start, _seg_len);
  }
  _seg_rt->counter_mode_step =!_seg_rt->counter_mode_step;
  _seg_rt->aux_param++;
  if(_seg_rt->aux_param > 6)
  {
    extern void read_flash_device_status_init(void);
    read_flash_device_status_init();
    set_fc_effect();
  }
  return (500 );
}

uint16_t power_off_effect(void)
{
  if(_seg_rt->counter_mode_step)
  {
    Adafruit_NeoPixel_fill( BLACK, _seg->start, _seg_len);
  }
  else
  {
    Adafruit_NeoPixel_fill( RED, _seg->start, _seg_len);
  }
  _seg_rt->counter_mode_step =!_seg_rt->counter_mode_step;
  _seg_rt->aux_param++;
  if(_seg_rt->aux_param > 4)
  {
    // 硬件关机
    // gpio_direction_output(IO_PORTA_08,0);
  }
  return (500 );
}

// 解绑效果
uint16_t unbind_effect(void)
{
  if(_seg_rt->counter_mode_step)
  {
    Adafruit_NeoPixel_fill( WHITE, _seg->start, _seg_len);
  }
  else
  {
    Adafruit_NeoPixel_fill( GRAY, _seg->start, _seg_len);
  }
  _seg_rt->counter_mode_step =!_seg_rt->counter_mode_step;

  return (500 );
}


//**************************************************************************************************


/*
 * Color wipe function，多种颜色流水效果
 * LEDs are turned on (color1) in sequence, then turned off (color2) in sequence.
 * if (uint8_t rev == true) then LEDs are turned off in reverse order
 * 颜色擦除功能，用color1依次点亮LED,再用color2依次擦除
 * rev = 0:两个颜色同向， =1color2反方向擦除
 * IS_REVERSE:流水方向
 */
uint16_t WS2812FX_multiColor_wipe(uint8_t is_reverse,uint8_t rev)
{
  static uint32_t color[2];

	if(_seg_rt->counter_mode_step == _seg_len)
  {
    // 一个循环后更换颜色
    color[_seg_rt->aux_param] = _seg->colors[_seg_rt->aux_param3];
    _seg_rt->aux_param++;
    _seg_rt->aux_param%=2;
    _seg_rt->aux_param3++;
    _seg_rt->aux_param3%=_seg->c_n;
  }
  if(_seg_rt->counter_mode_step == 0)
  {

    color[_seg_rt->aux_param] = _seg->colors[_seg_rt->aux_param3];
    _seg_rt->aux_param++;
    _seg_rt->aux_param%=2;
    _seg_rt->aux_param3++;
    _seg_rt->aux_param3%=_seg->c_n;
    SET_CYCLE;
  }

  if(_seg_rt->counter_mode_step < _seg_len) {
    uint32_t led_offset = _seg_rt->counter_mode_step;
    if(is_reverse) {
      WS2812FX_setPixelColor(_seg->stop - led_offset, color[0]);
    } else {
      WS2812FX_setPixelColor(_seg->start + led_offset, color[0]);
    }
  } else {
    uint32_t led_offset = _seg_rt->counter_mode_step - _seg_len;
    if((is_reverse && !rev) || (!is_reverse && rev)) {
      WS2812FX_setPixelColor(_seg->stop - led_offset, color[1]);
    } else {
      WS2812FX_setPixelColor(_seg->start + led_offset, color[1]);
    }
  }

	_seg_rt->counter_mode_step = (_seg_rt->counter_mode_step + 1) % (_seg_len * 2);

  return (_seg->speed );
}


// 多种颜色流水效果
// 正向流水，颜色同向
uint16_t WS2812FX_mode_multi_forward_same(void)
{
  return WS2812FX_multiColor_wipe(0,0) ;
}


// 多种颜色流水效果
// 反向流水，颜色同向
uint16_t WS2812FX_mode_multi_back_same(void)
{
  return WS2812FX_multiColor_wipe(1,0);
}

/*
在每一个LED颜色随机。并且淡出，淡入
Cycle a rainbow on each LED
 */
uint16_t WS2812FX_mode_fade_each_led(void) {
  if( _seg_rt->counter_mode_step == 0) {
    _seg_rt->counter_mode_step = 0x0f;
    for(uint16_t i=_seg->start; i <= _seg->stop; i++) {
      WS2812FX_setPixelColor(i, WS2812FX_color_wheel(WS2812FX_random8()));

    }
  }
  else{
    WS2812FX_fade_out();
  }
  _seg_rt->counter_mode_step++;
  return (_seg->speed / 8);
}




// 功能：颜色块跳变效果，多个颜色块组成背景,以块为单位步进做流水,
// _seg->c_n:有效颜色数量
// SIZE_OPTION：决定颜色块大小
// IS_REVERSE:0 反向流水 ；1正向流水，WS2812FX_setOptions(REVERSE)来设置

uint16_t WS2812FX_mode_single_block_scan(void)
{
  uint8_t size = fc_effect.dream_scene.seg_size;
  uint8_t j;
  uint32_t c;

  if(size > (_seg->stop - _seg->start))
    return 0;

  _seg_rt->counter_mode_step = 0;
  _seg_rt->aux_param = 0;
  while(_seg_rt->counter_mode_step  < _seg->stop)
  {
    for(j = 0; j < size; j++)
    {
      if(IS_REVERSE == 0) //反向流水
      {
        WS2812FX_setPixelColor( _seg->start + _seg_rt->counter_mode_step, _seg->colors[_seg_rt->aux_param]);
      }
      else
      {
        WS2812FX_setPixelColor( _seg->stop - _seg_rt->counter_mode_step, _seg->colors[_seg_rt->aux_param]);
      }
      _seg_rt->counter_mode_step++;
      if(_seg_rt->counter_mode_step  > _seg->stop)
      {
        break;
      }
    }
    _seg_rt->aux_param++;
    _seg_rt->aux_param %= _seg->c_n;
  }

    c = _seg->colors[0];
    // 重新开始，对颜色转盘
    for(j=1; j< _seg->c_n; j++)
    {
      // 把后面的颜色提前
      _seg->colors[j-1] = _seg->colors[j];
    }
    _seg->colors[j-1] = c;

  return _seg->speed*5;
}


// 多段颜色同时流水效果
// SIZE_OPTION：像素点大小
// SIZE_SMALL   1
// SIZE_MEDIUM  3
// SIZE_LARGE   5
// SIZE_XLARGE  7


uint16_t WS2812FX_mode_multi_block_scan(void)
{
  uint8_t size = (SIZE_OPTION << 1) + 1;
  uint8_t j;
  uint16_t i;
  if(size > (_seg->stop - _seg->start))
    return 0;

  /* 构建背景颜色 */
  if(_seg_rt->counter_mode_step == 0)
  {
    while(_seg_rt->counter_mode_step <= _seg->stop)
    {
      for(j = 0; (j < size) && (_seg_rt->counter_mode_step <= _seg->stop); j++)
      {
        WS2812FX_setPixelColor( _seg->start + _seg_rt->counter_mode_step, _seg->colors[_seg_rt->aux_param]);
        _seg_rt->counter_mode_step++;
      }
      _seg_rt->aux_param++;
      _seg_rt->aux_param %= _seg->c_n;
      // _seg_rt->aux_param %= MAX_NUM_COLORS;
    }
  }
  else
  {
    if(IS_REVERSE) //反向流水
    {
      WS2812FX_move_reverse(_seg->start, _seg->stop);
    }
    else
    {
      WS2812FX_move_forward(_seg->start, _seg->stop);
    }


  }

  return _seg->speed ;
}





/*
 * Fades the LEDs between mutil colors
 */
#if 0
uint16_t WS2812FX_mode_mutil_fade(void)
{
  uint8_t size = 1 << SIZE_OPTION;
  uint16_t j;
  uint32_t color, color1,color0;
  int lum = _seg_rt->counter_mode_step;

  size = 5; //调试用
  _seg->c_n = 3; //调试用
  if(size > (_seg->stop - _seg->start))
    return 0;

  if(lum > 255) lum = 511 - lum; // lum = 0 -> 255 -> 0
  _seg_rt->aux_param = 0;
  _seg_rt->aux_param2_ex = 0;
  // if(_seg_rt->aux_param2_ex == 0)
  {
    while(_seg_rt->aux_param2_ex  < _seg->stop)
    {
      color0 = _seg->colors[_seg_rt->aux_param];
      _seg_rt->aux_param++;
      _seg_rt->aux_param %= _seg->c_n;
      color1 = _seg->colors[_seg_rt->aux_param];
      _seg_rt->aux_param++;
      _seg_rt->aux_param %= _seg->c_n;
      color = WS2812FX_color_blend(color1, color0, lum);
      for(j = 0; j < size; j++)
      {
        WS2812FX_setPixelColor( _seg->start + _seg_rt->aux_param2_ex, \
                                color);
        _seg_rt->aux_param2_ex++;
        if(_seg_rt->aux_param2_ex  > _seg->stop)
        {
          break;
        }
      }
    }
  }




  // uint32_t color = WS2812FX_color_blend(_seg->colors[1], _seg->colors[0], lum);
  // Adafruit_NeoPixel_fill(color, _seg->start, _seg_len);

  // 此时颜色停留在color1,把color0颜色变换
  if(_seg_rt->counter_mode_step==0)
  {

  }

  _seg_rt->counter_mode_step++;

  // 此时颜色停留在color1
  if(_seg_rt->counter_mode_step > 511) {
    _seg_rt->counter_mode_step = 0;
    SET_CYCLE;
  }
  // 此时颜色停留再color0
  if(_seg_rt->counter_mode_step == 256)
  {

  }
  return (_seg->speed / 32);
}


#endif

// 多段颜色，同时在做渐变效果，每段效果把_seg->colors轮转
// SIZE_OPTION：像素点大小
// SIZE_SMALL   1
// SIZE_MEDIUM  3
// SIZE_LARGE   5
// SIZE_XLARGE  7
// WS2812FX_set_coloQty()设置颜色数量
uint16_t WS2812FX_mode_mutil_fade(void)
{
  uint8_t size =  fc_effect.dream_scene.seg_size;
  uint16_t j;
  uint8_t cnt0=0,cnt1 = 1;
  uint32_t color, color1,color0;
  static uint32_t c1[MAX_NUM_COLORS];
  int lum = _seg_rt->counter_mode_step;


  if(size > (_seg->stop - _seg->start) && size== 0)
    return 0;

  if(lum > 255) lum = 511 - lum; // lum = 0 -> 255 -> 0
  _seg_rt->aux_param = 0;
  _seg_rt->aux_param2_ex = 0;

  if(_seg_rt->aux_param3 == 0)
  {
    _seg_rt->aux_param3 = 1;
    memcpy(c1, _seg->colors ,MAX_NUM_COLORS * 4);

  }

    while(_seg_rt->aux_param2_ex  < _seg->stop)
    {
      color0 = _seg->colors[cnt0];
      cnt0++;
      cnt0 %= _seg->c_n;
      color1 = c1[cnt1];
      cnt1++;
      cnt1 %= _seg->c_n;
      color = WS2812FX_color_blend(color1, color0, lum);
      for(j = 0; j < size; j++)
      {
        WS2812FX_setPixelColor( _seg->start + _seg_rt->aux_param2_ex, \
                                color);
        _seg_rt->aux_param2_ex++;
        if(_seg_rt->aux_param2_ex  > _seg->stop)
        {
          break;
        }
      }
    }

  _seg_rt->counter_mode_step+=4;

  // 此时颜色停留在color1
  if(_seg_rt->counter_mode_step > 511) {
    _seg_rt->counter_mode_step = 0;
    // color0的颜色池左移1

    SET_CYCLE;
  }

  // 此时颜色停留在color1,把color0颜色变换,color0向左转盘
  if(_seg_rt->counter_mode_step==0)
  {
    uint32_t c_tmp;
    c_tmp = _seg->colors[0];

    memmove(_seg->colors, _seg->colors + 1, (_seg->c_n-1)*4);

    _seg->colors[_seg->c_n - 1] = c_tmp;
  }

  // 此时颜色停留再color0
  if(_seg_rt->counter_mode_step == 256)
  {
    // color1的颜色池左移1
    uint32_t c_tmp;
    c_tmp = c1[0];
    memmove(&c1[0], &c1[1], (_seg->c_n-1)*4);


    c1[_seg->c_n - 1] = c_tmp;
  }
  return (_seg->speed / 32);
}



// 多段颜色构成背景色，做呼吸效果
// SIZE_OPTION：像素点大小
// SIZE_SMALL   1
// SIZE_MEDIUM  3
// SIZE_LARGE   5
// SIZE_XLARGE  7
// WS2812FX_set_coloQty()设置颜色数量

uint16_t WS2812FX_mode_mutil_breath(void)
{
  uint8_t size = fc_effect.dream_scene.seg_size;
  uint8_t j;
  uint16_t lum = _seg_rt->aux_param3;
  uint32_t color;

  if(lum>255)
  {
    lum = 511 - lum;
  }
  if(size > (_seg->stop - _seg->start))
    return 0;
  _seg_rt->counter_mode_step = 0;
  _seg_rt->aux_param = 0;
  while(_seg_rt->counter_mode_step <= _seg->stop)
  {
    for(j = 0; (j < size) && (_seg_rt->counter_mode_step <= _seg->stop); j++)
    {
      color =  WS2812FX_color_blend( 0,_seg->colors[_seg_rt->aux_param], lum);
      WS2812FX_setPixelColor( _seg->start + _seg_rt->counter_mode_step, color);
      _seg_rt->counter_mode_step++;
    }
    _seg_rt->aux_param++;
    _seg_rt->aux_param %= _seg->c_n;
  }

  _seg_rt->aux_param3+=2;
  if(_seg_rt->aux_param3 > (512 - 5))
  {
    _seg_rt->aux_param3 = 5;
  }

  return _seg->speed;
}


// 多段颜色构成背景色，做闪烁
// SIZE_OPTION：像素点大小
// SIZE_SMALL   1
// SIZE_MEDIUM  3
// SIZE_LARGE   5
// SIZE_XLARGE  7
// WS2812FX_set_coloQty()设置颜色数量

uint16_t WS2812FX_mode_mutil_twihkle(void)
{
  uint8_t size = (SIZE_OPTION << 1) + 1;
  uint8_t j;
  // size = 5;
  // _seg->c_n = 3;
  if(size > (_seg->stop - _seg->start))
    return 0;

  _seg_rt->counter_mode_step = 0;
  _seg_rt->aux_param = 0;
  if(_seg_rt->aux_param3)
  {
      while(_seg_rt->counter_mode_step <= _seg->stop)
    {
      for(j = 0; (j < size) && (_seg_rt->counter_mode_step <= _seg->stop); j++)
      {
        WS2812FX_setPixelColor( _seg->start + _seg_rt->counter_mode_step, _seg->colors[_seg_rt->aux_param]);
        _seg_rt->counter_mode_step++;
      }
      _seg_rt->aux_param++;
      _seg_rt->aux_param %= _seg->c_n;
    }
  }
  else
  {
    Adafruit_NeoPixel_fill( BLACK, _seg->start, _seg_len);

  }

  _seg_rt->aux_param3 =!_seg_rt->aux_param3;


  return _seg->speed;
}

// 多种颜色跳变
uint16_t WS2812FX_mutil_c_jump(void)
{

  Adafruit_NeoPixel_fill(_seg->colors[_seg_rt->counter_mode_step], _seg->start, _seg_len);
  _seg_rt->counter_mode_step++;
  _seg_rt->counter_mode_step %= _seg->c_n;

  return _seg->speed;
}

extern void aws_pixels_dynamic_mode_change(u8 dir);
uint8_t poweron_anim_finished = 0;

uint16_t aoweisi_kaiji_c_jump(void)
{
//   printf("aoweisi_kaiji_c_jump step = %d, %08X",_seg_rt->counter_mode_step, _seg->colors[_seg_rt->counter_mode_step]);

  if(_seg_rt->counter_mode_step >= 3)
  {
    poweron_anim_finished = 1;
    // aws_pixels_dynamic_mode_change(CHANGE_DIR_STAND);
    // aws_pixels_static_mode_change(CHANGE_DIR_STAND);
    return 1000;
  }
  Adafruit_NeoPixel_fill(_seg->colors[_seg_rt->counter_mode_step % _seg->c_n], _seg->start, _seg_len);
  _seg_rt->counter_mode_step++;
//   _seg_rt->counter_mode_step %= _seg->c_n;

  return 300;
}

uint16_t aoweisi_kaiji_c_jump2(void)
{
//   printf("aoweisi_kaiji_c_jump2 step = %d, %08X",_seg_rt->counter_mode_step, _seg->colors[_seg_rt->counter_mode_step]);

  if(_seg_rt->counter_mode_step >= 3)
  {
    poweron_anim_finished = 1;
    // aws_pixels_dynamic_mode_change(CHANGE_DIR_STAND);
    // aws_pixels_static_mode_change(CHANGE_DIR_STAND);
    return 1000;
  }

  Adafruit_NeoPixel_fill(_seg->colors[_seg_rt->counter_mode_step % _seg->c_n], _seg->start, _seg_len);
//   if( _seg_rt->counter_mode_step < 2)
//     _seg_rt->counter_mode_step++;
    _seg_rt->counter_mode_step++;
    // _seg_rt->counter_mode_step %= _seg->c_n;

  return 300;
}


uint16_t aoweisi_mutil_c_jump(void)
{

  Adafruit_NeoPixel_fill(_seg->colors[_seg_rt->counter_mode_step], _seg->start, _seg_len);
  _seg_rt->counter_mode_step++;
  _seg_rt->counter_mode_step %= _seg->c_n;

   return _seg->speed;

}



u8 si = 0;
uint16_t rgbw_mutil_c_jump(void)
{
u32 color;
// if(_seg_rt->rgbw_counter_mode_step == 0)
// {
// color = RED;
// }
// else if(_seg_rt->rgbw_counter_mode_step == 1)
// {
//   color = GREEN;
// }
// else if(_seg_rt->rgbw_counter_mode_step == 1)
// {
//   color = BLUE;
// }
   color = _seg->colors[si];
// printf("_seg_rt->counter_mode_step=%d",si);
// printf("color=%04x",color);

  Adafruit_NeoPixel_fill(color, _seg->start, _seg_len);

  // _seg_rt->rgbw_counter_mode_step++;
  // _seg_rt->rgbw_counter_mode_step %= _seg->c_n;
si++;
si %= _seg->c_n;
  return _seg->speed;
}


// 支持多颜色频闪
uint16_t WS2812FX_mutil_strobe(void)
{
  if(_seg_rt->aux_param == 0)
  {
    Adafruit_NeoPixel_fill(_seg->colors[_seg_rt->counter_mode_step], _seg->start, _seg_len);
    _seg_rt->counter_mode_step++;
    _seg_rt->counter_mode_step %= _seg->c_n;
  }
  else
  {
    Adafruit_NeoPixel_fill(BLACK, _seg->start, _seg_len);
  }
  _seg_rt->aux_param =! _seg_rt->aux_param ;

  return _seg->speed ;
}

// 整条灯带渐变，支持多种颜色之间切换
// 颜色池：fc_effect.dream_scene.rgb[]
// 颜色数量fc_effect.dream_scene.c_n
uint16_t WS2812FX_mutil_c_gradual(void)
{
  static u8 index ;
  u32 rgb;
  static u8 pp = 0;

  int lum = _seg_rt->counter_mode_step;
  if(lum > 255) lum = 511 - lum; // lum = 0 -> 255 -> 0

  // _seg->colors[1]:目标颜色
  uint32_t color = WS2812FX_color_blend(_seg->colors[1], _seg->colors[0], lum);

  Adafruit_NeoPixel_fill(color, _seg->start, _seg_len);

  if(_seg_rt->counter_mode_step == 256)
  {
    pp = 2;
    index++;
    index %= fc_effect.dream_scene.c_n;

      rgb = ( (u32)fc_effect.dream_scene.rgb[index].r << 16 ) |
              ( (u32)fc_effect.dream_scene.rgb[index].g << 8 ) |
              ( (u32)fc_effect.dream_scene.rgb[index].b ) ;

      _seg->colors[0] = color;
      _seg->colors[1] = rgb;

  }

  _seg_rt->counter_mode_step++;
  if(_seg_rt->counter_mode_step > 511) {
    _seg_rt->counter_mode_step = 0;

    // SET_CYCLE;
  }
  return (_seg->speed /5);
}



uint16_t WS2812FX_mutil_c_gradual2(void)
{
  static u8 index2 ;
  u32 rgb2;


  int lum2 = _seg_rt->rgbw_counter_mode_step;
  if(lum2 > 255) lum2 = 511 - lum2; // lum = 0 -> 255 -> 0

  // _seg->colors[1]:目标颜色
  uint32_t color2 = WS2812FX_color_blend(_seg->colors[1], _seg->colors[0], lum2);

  Adafruit_NeoPixel_fill(color2, _seg->start, _seg_len);

  if(_seg_rt->rgbw_counter_mode_step == 256)
  {
 
    index2++;
    index2 %= fc_effect.dream_scene.c_n;

      rgb2 = ( (u32)fc_effect.dream_scene.rgb[index2].r << 16 ) |
              ( (u32)fc_effect.dream_scene.rgb[index2].g << 8 ) |
              ( (u32)fc_effect.dream_scene.rgb[index2].b ) ;

      _seg->colors[0] = color2;
      _seg->colors[1] = rgb2;

  }

  _seg_rt->rgbw_counter_mode_step++;
  if(_seg_rt->rgbw_counter_mode_step > 511) {
    _seg_rt->rgbw_counter_mode_step = 0;

    // SET_CYCLE;
  }
  return (_seg->speed/5);
}





uint16_t rgbw_mutil_c_gradual(void)
{
  static u8 index ;
  u32 rgb;
  static u8 pp = 0;

  int lum = _seg_rt->counter_mode_step;
  if(lum > 255) lum = 511 - lum; // lum = 0 -> 255 -> 0

  // _seg->colors[1]:目标颜色
  uint32_t color = WS2812FX_color_blend(_seg->colors[1], _seg->colors[0], lum);

  Adafruit_NeoPixel_fill(color, _seg->start, _seg_len);

  if(_seg_rt->counter_mode_step == 256)
  {
    pp = 2;
    index++;
    index %= fc_effect.rgbw_scene.c_n;

      rgb = ( (u32)fc_effect.rgbw_scene.rgb[index].r << 16 ) |
              ( (u32)fc_effect.rgbw_scene.rgb[index].g << 8 ) |
              ( (u32)fc_effect.rgbw_scene.rgb[index].b ) ;

      _seg->colors[0] = color;
      _seg->colors[1] = rgb;

  }

  _seg_rt->counter_mode_step += 2;
  if(_seg_rt->counter_mode_step > 511) {
    _seg_rt->counter_mode_step = 0;

    SET_CYCLE;
  }
  return (_seg->speed /5);
}





// // 整条灯带呼吸，每次呼吸切换颜色
// uint16_t mutil_c_breath(void)
// {
//   int lum = _seg_rt->counter_mode_step;
//   if(lum > 255) lum = 511 - lum; // lum = 0 -> 255 -> 0

//   uint32_t color =  WS2812FX_color_blend(0,_seg->colors[_seg_rt->aux_param], lum);
//   Adafruit_NeoPixel_fill(color, _seg->start, _seg_len);

//   _seg_rt->counter_mode_step += 1;
//   if(_seg_rt->counter_mode_step > 511) {
//     _seg_rt->counter_mode_step = 0;
//     _seg_rt->aux_param++;
//     _seg_rt->aux_param %= _seg->c_n;
//     printf("\n _seg_rt->aux_param=%d",_seg_rt->aux_param);
//     printf("\n c=%x",_seg->colors[_seg_rt->aux_param]);
//     return 3000 + _seg->speed;
//   }
//   else if(_seg_rt->counter_mode_step == 255)
//   {
//     return _seg->speed;
//   }

//   else{
//     return 20;
//   }
// }

//森木光纤灯
// rgb通道呼吸
// 按照用户需求，亮5秒，灭3秒。这个时间为最小时间单位，进行调节
// 整体呼吸，亮-》灭-》亮
uint16_t breath_rgb(void)
{
  
  int lum = _seg_rt->counter_mode_step;
  if(lum > 255) lum = 511 - lum; // lum = 0 -> 255 -> 0

  uint32_t color =  WS2812FX_color_blend(_seg->colors[1], _seg->colors[0], lum);
  Adafruit_NeoPixel_fill(color, _seg->start, _seg_len);

  _seg_rt->counter_mode_step += 1;
  if(_seg_rt->counter_mode_step > 511) {
    _seg_rt->counter_mode_step = 0;

    return 3000 + _seg->speed;
  }
  else if(_seg_rt->counter_mode_step == 255)
  {
    return _seg->speed;
  }

  else{
    return 20;
  }

}

//-------------------------------------- 声控-----------------------------------------
// ----------------------------------------------------------------------------全彩音乐效果
typedef enum {E_RISE, E_TOP}FS_ACT;

typedef struct
{
  uint8_t act;//当前动作，0：上升：1：顶部效果
  uint16_t c_pos;
  uint16_t c_pos1;
  uint16_t trg;
}music_oc_t;  //oc:open/close 开合模式

music_oc_t m_oc={
  .trg = 15
};
music_oc_t m_oc2={
  .trg = 15
};
typedef struct
{
  uint8_t act; //当前动作，0：上升：1：顶部效果
  uint16_t rise_tag; //上升的目标值
  uint16_t c_pos; //当前位置
  uint8_t fall_sp;  //下降速度
  short int top_pos;
  uint8_t top_sp; //顶端速度
  uint32_t bgc;   //背景颜色
} music_fs_t;//频谱声控

music_fs_t m_fs=
{
    // .rise_tag = 40,
    .rise_tag = 0,
    .bgc = GRAY,
    .act = E_TOP,
};
music_fs_t m_fs2=
{
    // .rise_tag = 40,
    .rise_tag = 0,
    .bgc = GRAY,
    .act = E_TOP,
};
// p:百分比
void set_music_oc_trg(u8 p)   //修改灯带上升的长度
{
    // m_oc.trg = p + 1;    //奥威斯 16个灯 9时就爆灯
    // m_oc2.trg = p * 4;   //奥威斯 16个灯 9时就爆灯
    m_oc.trg = p * USER_WS2812_SEGMENT_1_LEN / 19 + 1;
    m_oc2.trg = p * 4 * USER_WS2812_SEGMENT_2_LEN / 60;
}
// p:百分比
void set_music_fs_trg(u8 p)
{
    // m_fs.rise_tag = p;   // 100
    // m_fs2.rise_tag = p*4-3;   // 100  *4是因为原来的15，另外一个是60，是15的4倍
    m_fs.rise_tag = p * USER_WS2812_SEGMENT_1_LEN / 19;
    m_fs2.rise_tag = (p * 4 - 3) * USER_WS2812_SEGMENT_2_LEN / 60;
}




//------------------------------------------七彩声控效果
 extern u8 get_sound_result(void);
// 渐变，触发变色
uint16_t fc_music_gradual(void)
{
 
  uint32_t color = WS2812FX_color_wheel(_seg_rt->counter_mode_step);

  Adafruit_NeoPixel_fill(color, _seg->start, _seg_len);

  if(get_sound_result())
  {
    _seg_rt->counter_mode_step += 20;
  }

  _seg_rt->counter_mode_step = (_seg_rt->counter_mode_step + 1) & 0xFF;

  if(_seg_rt->counter_mode_step == 0)  SET_CYCLE;

  return (100);
}


// 呼吸，触发渐亮-》渐暗，最后黑，每次变色
uint16_t fc_music_breath(void)
{
  static uint32_t color1 ;
  if(get_sound_result())
  {
    // if(_seg_rt->counter_mode_step == 0)
    {
      color1 = WS2812FX_color_wheel( _seg_rt->aux_param);
      _seg_rt->aux_param +=5;
    }
    _seg_rt->counter_mode_step = 1;
  }

  int lum = _seg_rt->counter_mode_step;
  if(lum > 255) lum = 511 - lum; // lum = 0 -> 255 -> 0

  uint32_t color =  WS2812FX_color_blend(0, color1, lum);
  Adafruit_NeoPixel_fill(color, _seg->start, _seg_len);
  if(_seg_rt->counter_mode_step != 0)
  {
    _seg_rt->counter_mode_step += 8;
  }
  if(_seg_rt->counter_mode_step > 511) {
    _seg_rt->counter_mode_step = 0;
  }
  return 10;
}

// 定色，触发换颜色
uint16_t fc_music_static(void)
{
  uint32_t color1;
  if(get_sound_result())
  {
    // if(_seg_rt->counter_mode_step == 0)
    {
      color1 = WS2812FX_color_wheel( _seg_rt->aux_param);
      _seg_rt->aux_param +=20;
    }
    Adafruit_NeoPixel_fill(color1, _seg->start, _seg_len);
  }

  return 0xffff;
}

// 定色，触发黑->爆闪一下，每次变色
uint16_t fc_music_twinkle(void)
{
  uint32_t color1;
  if(get_sound_result())
  {
    // if(_seg_rt->counter_mode_step == 0)
    {
      color1 = WS2812FX_color_wheel( _seg_rt->aux_param);
      _seg_rt->aux_param +=20;
    }
    Adafruit_NeoPixel_fill(color1, _seg->start, _seg_len);
  }
  else
  {
    Adafruit_NeoPixel_fill(BLACK, _seg->start, _seg_len);
  }
  return 100;
}

//----------------------------------------------------------end


extern u16 get_max_sp(void);
#define MAX_RATE 17
uint8_t met_trg = 0;
// 流星发射，声音触发，可以连续发射
uint16_t music_meteor(void)
{

  static uint8_t i = 0;
  uint32_t r1, g1, b1, w1 ;
  const uint8_t rate[MAX_RATE] = {100,75,45,30,20,15,10,7,5,3,2,0,0,0,0,0,0};
  int w = (_seg->colors[0] >> 24) & 0xff;
  int r = (_seg->colors[0] >> 16) & 0xff;
  int g = (_seg->colors[0] >>  8) & 0xff;
  int b =  _seg->colors[0]        & 0xff;
  if(met_trg)
  {
    _seg->colors[0] =  WS2812FX_color_wheel(_seg_rt->counter_mode_step);
    _seg_rt->counter_mode_step = _seg_rt->counter_mode_step + 33 & 0xff;
    if(i == MAX_RATE - 1)
    {
      i = 0;
    }
    met_trg = 0;
  }

  WS2812FX_copyPixels(_seg->start+1,_seg->start, _seg_len-1);
  r1 = r * rate[i] / 100;
  g1 = g * rate[i] / 100;
  b1 = b * rate[i] / 100;
  w1 = w * rate[i] / 100;
  WS2812FX_setPixelColor_rgbw(_seg->start, r1, g1, b1, w1);


  if(i < MAX_RATE-1)
    i++;

  return (30 );
}


// 彩虹色滚动，音乐触发加速
uint16_t trg_en, sp_en = 10, en_cnt;

uint16_t music_energy(void)
{

  //   for(uint16_t i=0; i < _seg_len; i++) {
  //     uint32_t color = WS2812FX_color_wheel(((i * 256 / _seg_len) + _seg_rt->counter_mode_step) & 0xFF);
  //     WS2812FX_setPixelColor(_seg->stop - i, color);
  //     // r = (color >> 16) & 0xFF * index / 100;
  //     // g = (color >>  8) & 0xFF * index / 100;
  //     // b =  color        & 0xFF * index / 100;
  //     // WS2812FX_setPixelColor_rgb(_seg->stop - i, r, g, b);
  //   }


  // if(trg_en)
  // {
  //   sp_en = get_max_sp();
  //   en_cnt++;
  //   _seg_rt->counter_mode_step = (_seg_rt->counter_mode_step + 20) & 0xFF;
  //   if(en_cnt>10)
  //   {
  //     trg_en = 0;
  //   }
  // }
  // else{
  //   en_cnt = 0;
  //   sp_en = 30;
  //   _seg_rt->counter_mode_step = (_seg_rt->counter_mode_step + 1) & 0xFF;
  // }

  return (sp_en);
}

uint16_t music_star_sp;
// 采集声音的ADC值
void set_mss(uint16_t s)
{
  if(500 > s)
    music_star_sp = 500 -s;
  else music_star_sp = 10;
  if(music_star_sp < 10) music_star_sp = 10;


}
uint8_t music_s_m;//0:随机颜色；1：白色，2：蓝色
void set_music_s_m(u8 m)
{
  music_s_m = m;
}

// 星空效果，音乐触发，更多星星，闪烁速度更快
uint16_t music_star(void)
{
  uint8_t j=2;
  if(music_s_m == 0)
    _seg->colors[0]= WS2812FX_color_wheel(WS2812FX_random8());
  if(music_s_m == 1)  
    _seg->colors[0] = WHITE;
  if(music_s_m == 2)  
    _seg->colors[0] = BLUE;
  uint8_t w = (_seg->colors[0] >> 24) & 0xFF;
  uint8_t r = (_seg->colors[0] >> 16) & 0xFF;
  uint8_t g = (_seg->colors[0] >>  8) & 0xFF;
  uint8_t b = (_seg->colors[0]        & 0xFF);
  uint8_t lum = max(w, max(r, max(g, b))) / 2;

  if(trg_en)
  {
    sp_en = get_max_sp();
    _seg_rt->aux_param++;
    if(_seg_rt->aux_param >3) //加速持续时间
    {
      _seg_rt->aux_param = 0;
      trg_en = 0;

    }

  }
  else{
    sp_en = 500;
  }

  en_cnt+=10;
  if(en_cnt >= sp_en )
  // if(en_cnt >= music_star_sp)
  {
    en_cnt = 0;
    WS2812FX_fade_out();
    for(uint16_t i=0; i <= j ; i++)
    {
      int flicker = WS2812FX_random8_lim(lum);

      WS2812FX_setPixelColor_rgbw(WS2812FX_random16_lim(_seg_len), max(r - flicker, 0), max(g - flicker, 0), max(b - flicker, 0), max(w - flicker, 0));
    }
  }
  SET_CYCLE;
  return (10 );
}

// 彩虹闪烁，有声音两彩虹，没声音灭
uint16_t music_rainbow_flash(void) 
{
  uint16_t i;

  if(trg_en)
  {
    en_cnt++;
    if(en_cnt>10)
    {
      trg_en = 0;
    }
  }
  else{
    en_cnt = 0;
    Adafruit_NeoPixel_fill(GRAY, _seg->start, _seg_len);

  }

  if(trg_en)
  {
    for(i=0; i<_seg_len; i++) 
    {
      uint32_t color = WS2812FX_color_wheel(((i * 256 / _seg_len) + _seg_rt->counter_mode_step) & 0xFF);
      WS2812FX_setPixelColor( i, color);
        
    }

  }
      _seg_rt->counter_mode_step = (_seg_rt->counter_mode_step + 2) & 0xFF;
    if(_seg_rt->counter_mode_step == 0) SET_CYCLE;
  return (get_max_sp());
}


uint16_t music_multi_c_flow(void)
{
  uint8_t size = 5;
  uint8_t j, reverse;
  uint16_t i;

  if(trg_en)
  {
    // trg_en = 0;
    // if(sp_en >10)
    //   sp_en -= 10;
    // else sp_en = 10;
    sp_en = 10;
    _seg_rt->aux_param++;
    if(_seg_rt->aux_param >3) //加速持续时间
    {
      _seg_rt->aux_param = 0;
      trg_en = 0;

    }

  }
  else{
    sp_en = 500;
    // if(sp_en<500)
    //   sp_en +=10;
    // else sp_en = 500;
  }

  en_cnt+=10;
  // printf("%d",music_star_sp);
  // if(music_star_sp < 80)
  // {
  //   music_star_sp = music_star_sp/3;
  // }
  // else  if(music_star_sp < 100)
  // {
  //   music_star_sp = music_star_sp/2;
  // }
  // else if(music_star_sp < 150)
  // {
  //   music_star_sp = music_star_sp*3;
  // }
  // else if(music_star_sp < 200)
  // {
  //   music_star_sp = music_star_sp*4;
  // }
  // else
  // {
  //   music_star_sp = music_star_sp*5;

  // }

  if(en_cnt >= sp_en )
  {
    en_cnt = 0;
    /* 构建背景颜色 */
    if(_seg_rt->counter_mode_step == 0)
    {
      while(_seg_rt->counter_mode_step <= _seg->stop)
      {
        _seg->colors[0]= WS2812FX_color_wheel(WS2812FX_get_random_wheel_index(WS2812FX_random8()));
        for(j = 0; (j < size) && (_seg_rt->counter_mode_step <= _seg->stop); j++)
        {
          WS2812FX_setPixelColor( _seg->start + _seg_rt->counter_mode_step, _seg->colors[0]);
          _seg_rt->counter_mode_step++;
        }
      }
    }
    else
    {

      if(reverse) //反向流水
      {
        WS2812FX_move_reverse(_seg->start, _seg->stop);
      }
      else
      {
        WS2812FX_move_forward(_seg->start, _seg->stop);
      }
    }
  }

  return 10 ;
}


// oc:open clos,开合模式
uint16_t music_oc_2(void)
{
  uint8_t i,j;
  if(m_oc.trg > _seg_len/2)
  {
    m_oc.trg = _seg_len/2;
  }

  if(_seg_rt->aux_param==0)
  {
    m_oc.c_pos = _seg_len/2;
    m_oc.c_pos1 = _seg_len/2;
    _seg_rt->aux_param = 1;
  }

  for(i=_seg_len/2; i< m_oc.c_pos; i++)
  {
    uint32_t color = WS2812FX_color_wheel(((i * 256 / _seg_len) + _seg_rt->counter_mode_step) & 0xFF);
    WS2812FX_setPixelColor(  i, color);
		WS2812FX_setPixelColor(  _seg_len/2 - (i - _seg_len/2) -1, color);

  }
  _seg_rt->counter_mode_step = (_seg_rt->counter_mode_step + 1) & 0xFF;
    if(_seg_rt->counter_mode_step == 0) SET_CYCLE;

  if(m_oc.act == E_RISE)
  {
    if(m_oc.c_pos < _seg_len/2 + m_oc.trg)
    {
      m_oc.c_pos++;
    }
    else
    {
      m_oc.act = E_TOP;
      m_oc.trg = 0;
    }

    if(m_oc.c_pos1 > _seg_len/2 - m_oc.trg && m_oc.c_pos1 > 0)
    {
      m_oc.c_pos1--;
    }
  }
  else
  {
    WS2812FX_setPixelColor( m_oc.c_pos, 0);
    WS2812FX_setPixelColor( m_oc.c_pos1, 0);

    if(m_oc.c_pos > _seg_len/2)
    {
      m_oc.c_pos--;
    }
    if(m_oc.c_pos1<_seg_len/2)
    {
      m_oc.c_pos1++;
    }
  }

  if(m_oc.c_pos < _seg_len/2 + m_oc.trg)
  {
    m_oc.act = E_RISE;
  }

  return (get_max_sp());
}

// 两边往中间跑，开合效果
uint16_t music_2_side_oc(void)
{
uint32_t color;
  // 顺序上升是：从1 ~ 一半
  // 倒序上升是：最长 ~ 一半
  uint8_t i,j;

   if(m_oc.trg > (USER_WS2812_SEGMENT_1_LEN + 1) / 2)
  {
    m_oc.trg = (USER_WS2812_SEGMENT_1_LEN + 1) / 2;
  }
  /* 设置初值 */
  if(_seg_rt->aux_param==0)
  {
     Adafruit_NeoPixel_fill(BLACK, _seg->start, _seg_len);
    m_oc.c_pos = 1; //顺序的位置，从0开始
    m_oc.c_pos1 = _seg_len;//倒叙的位置，从最后一点开始
    _seg_rt->aux_param = 1;
  }

//  printf("\n m_oc.trg=%d",m_oc.trg);
  // printf("m_oc.c_pos = %d", m_oc.c_pos);
  // printf("m_oc.c_pos1 = %d", m_oc.c_pos1);



//填颜色
  for(i=1; i < m_oc.c_pos; i++)
  {
    // uint32_t color = WS2812FX_color_wheel(((i * 256 / _seg_len) + _seg_rt->counter_mode_step) & 0xFF);

    if(i < (3 * USER_WS2812_SEGMENT_1_LEN / 19))  
    {
      color = BLUE;

    }
    else if( i >= (3 * USER_WS2812_SEGMENT_1_LEN / 19) && i < (7 * USER_WS2812_SEGMENT_1_LEN / 19))
    {
      color =   GREEN;
    }
    else
    {
      color =   RED;
    }


    WS2812FX_setPixelColor( i, color);
    WS2812FX_setPixelColor( _seg_len - i + 1, color);

  }

    _seg_rt->counter_mode_step = (_seg_rt->counter_mode_step + 1) & 0xFF;
    if(_seg_rt->counter_mode_step == 0) SET_CYCLE;
  /* 上升处理 */
  if(m_oc.act == E_RISE)
  {
    if(m_oc.c_pos <  m_oc.trg ) //顺序上升位置，堆积上升   可以控制两端是否连接
    {
      m_oc.c_pos++;
    }
    else
    {
      m_oc.act = E_TOP;   //标记来到顶端
      m_oc.trg = 0;       //清除触发长度，为下降准备
    }

    if(m_oc.c_pos1 > _seg_len - m_oc.trg && m_oc.c_pos1 > 0) //倒序上升位置，
    {
      m_oc.c_pos1--;
    }
  }
  /* 下降处理 */
  else
  {
    WS2812FX_setPixelColor( m_oc.c_pos, 0);
    WS2812FX_setPixelColor( m_oc.c_pos1, 0);

    if(m_oc.c_pos > 1)//计算顺序下降位置
    {
      m_oc.c_pos--;
    }
    if(m_oc.c_pos1 < _seg_len )
    {
      m_oc.c_pos1++;
    }
  }

  if(m_oc.c_pos < m_oc.trg)
  {
    m_oc.act = E_RISE;
  }
  // return (get_max_sp());
  return (10);  //该速度应与acd值的获取时间一样
}



// 两边往中间跑，开合效果
uint16_t music_2_side_oc2(void)
{

 #if 1
  uint32_t color;
  // 顺序上升是：从1 ~ 一半
  // 倒序上升是：最长 ~ 一半
  uint8_t i,j;

   if(m_oc2.trg > (USER_WS2812_SEGMENT_2_LEN + 1) / 2)
  {
    m_oc2.trg = (USER_WS2812_SEGMENT_2_LEN + 1) / 2;
  }
  /* 设置初值 */
  if(_seg_rt->aux_param==0)
  {
    Adafruit_NeoPixel_fill(BLACK, _seg->start, _seg_len);
    m_oc2.c_pos = 1; //顺序的位置，从0开始
    m_oc2.c_pos1 = _seg_len;//倒叙的位置，从最后一点开始
    _seg_rt->aux_param = 1;
  }

//填颜色
  for(i=1; i < m_oc2.c_pos; i++)
  {

    if(i < (6 * USER_WS2812_SEGMENT_2_LEN / 60))  
    {
      color = BLUE;

    }
    else if( i >= (6 * USER_WS2812_SEGMENT_2_LEN / 60) && i < (24 * USER_WS2812_SEGMENT_2_LEN / 60))
    {
      color =   GREEN;
    }
    else
    {
      color =   RED;
    }

    //需要快跳过第一段的灯带数量
    WS2812FX_setPixelColor( i+_seg->start, color);
    WS2812FX_setPixelColor( _seg_len - (i-_seg->start) + 1, color);

  }

    _seg_rt->counter_mode_step = (_seg_rt->counter_mode_step + 1) & 0xFF;
    if(_seg_rt->counter_mode_step == 0) SET_CYCLE;
  /* 上升处理 */
  if(m_oc2.act == E_RISE)
  {
    if(m_oc2.c_pos <  m_oc2.trg ) //顺序上升位置，堆积上升   可以控制两端是否连接
    {
      m_oc2.c_pos +=1;
    }
    else
    {
      m_oc2.act = E_TOP;   //标记来到顶端
      m_oc2.trg = 0;       //清除触发长度，为下降准备
    }

    if(m_oc2.c_pos1 > _seg_len - m_oc2.trg && m_oc2.c_pos1 > 0) //倒序上升位置，
    {
      m_oc2.c_pos1 -= 1;
    }
  }
  /* 下降处理 */
  else
  {
    WS2812FX_setPixelColor( m_oc2.c_pos+_seg->start, 0);
    WS2812FX_setPixelColor( m_oc2.c_pos1+_seg->start, 0);

    if(m_oc2.c_pos > 1)//计算顺序下降位置
    {
      m_oc2.c_pos--;
    }
    if(m_oc2.c_pos1 < _seg_len )
    {
      m_oc2.c_pos1++;
    }
  }

  if(m_oc2.c_pos < m_oc2.trg)
  {
    m_oc2.act = E_RISE;
  }
#endif
  return (10);  //该速度应与acd值的获取时间一样
}


uint16_t sp;


// 频谱，生长效果，到达最高位置，顶点变成白色，慢慢下降
uint16_t music_fs(void) {
  uint8_t i;
  uint32_t color_top , color;
  if(m_fs.rise_tag > _seg_len) m_fs.rise_tag = _seg_len;
  
  for(i=0; i< m_fs.c_pos; i++) //上升点亮灯带
  {
    color = WS2812FX_color_wheel(((i * 256 / _seg_len) + _seg_rt->counter_mode_step) & 0xFF);
    WS2812FX_setPixelColor( i, color);
    // if(m_fs.top_pos<i)    //点亮顶端白点
    // WS2812FX_setPixelColor( i+1, WHITE);

  }
  _seg_rt->counter_mode_step = (_seg_rt->counter_mode_step + 1) & 0xFF;
  if(_seg_rt->counter_mode_step == 0) SET_CYCLE;

  if(m_fs.act == E_RISE)
  {
    if(m_fs.c_pos < m_fs.rise_tag - 1) //-1是留一个点位置给顶端显示白点
    {
      m_fs.c_pos++;
      
    }
    else//到达顶端
    {
      m_fs.act = E_TOP;
      if(m_fs.c_pos > m_fs.top_pos)  //刷新顶端值
      {
        m_fs.top_pos = m_fs.c_pos  ;
        color_top = color;
      }
      m_fs.rise_tag = 0;
    }
    // voice_c%=voice_trg;

  }
  else
  {
    if(m_fs.fall_sp >= 3)
    {
      m_fs.fall_sp=0;
			if(m_fs.c_pos > 0) m_fs.c_pos--;
      WS2812FX_setPixelColor( m_fs.c_pos, 0);
    }
    m_fs.fall_sp++;
  }

  if(m_fs.rise_tag >= m_fs.c_pos && m_fs.rise_tag >0)
  {
    m_fs.act = E_RISE;
  }
  else
  {
    m_fs.act = E_TOP;
  }

  if(m_fs.top_pos >= m_fs.c_pos)
  {
    if(m_fs.top_sp>=10)
    {
      WS2812FX_setPixelColor( m_fs.top_pos+2, BLACK);
      WS2812FX_setPixelColor( m_fs.top_pos+1, color_top);
      WS2812FX_setPixelColor( m_fs.top_pos, color_top);
      m_fs.top_sp=0;
      m_fs.top_pos--;
    }
    m_fs.top_sp++;
  }

  return (get_max_sp());
}

// 频谱，生长效果，背景色可调
uint16_t music_fs_bc(void) {
  uint16_t i;
  if(m_fs.rise_tag > _seg_len) m_fs.rise_tag = _seg_len;
  Adafruit_NeoPixel_fill(m_fs.bgc, _seg->start, _seg_len);

  for(i=0; i< m_fs.c_pos; i++) //上升点亮灯带
  {
    uint32_t color = WS2812FX_color_wheel(((i * 256 / _seg_len) + _seg_rt->counter_mode_step) & 0xFF);
    WS2812FX_setPixelColor( i, color);

  }
  _seg_rt->counter_mode_step = (_seg_rt->counter_mode_step + 1) & 0xFF;
  if(_seg_rt->counter_mode_step == 0) SET_CYCLE;



  if(m_fs.act == E_RISE)
  {
    if(m_fs.c_pos < m_fs.rise_tag ) //-1是留一个点位置给顶端显示白点
    {
      m_fs.c_pos++;

    }
    else//到达顶端
    {
      m_fs.act = E_TOP;
      if(m_fs.rise_tag > m_fs.top_pos)  //刷新顶端值
      {
        m_fs.top_pos = m_fs.rise_tag -1 ;
      }
      m_fs.rise_tag = 0;
    }
    // voice_c%=voice_trg;

  }
  else
  {
    if(m_fs.fall_sp >= 3)
    {
      m_fs.fall_sp=0;
			if(m_fs.c_pos > 0) m_fs.c_pos--;
      // WS2812FX_setPixelColor( m_fs.c_pos, m_fs.bgc);  //下降填充背景颜色
    }
    m_fs.fall_sp++;
  }

  if(m_fs.rise_tag  > m_fs.c_pos && m_fs.rise_tag >0)
  {
    m_fs.act = E_RISE;
  }
  else
  {
    m_fs.act = E_TOP;
  }


  return (get_max_sp());
}

extern u8 close_music;
uint16_t music_fs_green_blue(void)
{
    uint16_t i;
    uint8_t size = 1 << SIZE_OPTION;
    uint8_t param;
    param = _seg_len;
    uint32_t color;

#if 0
    static uint8_t curr_level = 0;
    if (curr_level < m_fs.rise_tag)
        curr_level++;
    else if (curr_level > m_fs.rise_tag)
        curr_level--;
    
    Adafruit_NeoPixel_fill(BLACK, _seg->start, _seg_len);

    color = BLUE;
    for (uint8_t index = 0; index < _seg_len; index++)
    {
        if (curr_level > index)
            WS2812FX_setPixelColor(_seg->start + index, color);
    }

    return 10;
#endif
    
    uint8_t sineIncr = max(1, (256 / param) * size);

    if (m_fs.rise_tag > _seg_len)
        m_fs.rise_tag = _seg_len;
    Adafruit_NeoPixel_fill(BLACK, _seg->start, _seg_len);

    if (close_music)
        m_fs.c_pos = 0;

    for (i = 0; i < m_fs.c_pos; i++) //上升点亮灯带
    {
        int lum = (int)Adafruit_NeoPixel_sine8(((i + _seg_rt->counter_mode_step) * sineIncr));

        if (i < (4 * USER_WS2812_SEGMENT_1_LEN / 19))
        {
            color = WS2812FX_color_blend(BLUE, BLUE, lum);
        }
        else if (i >= (4 * USER_WS2812_SEGMENT_1_LEN / 19) && i < (13 * USER_WS2812_SEGMENT_1_LEN / 19))
        {
            color = WS2812FX_color_blend(GREEN, GREEN, lum);
        }
        else
        {
            color = WS2812FX_color_blend(RED, RED, lum);
        }

        // if(IS_REVERSE)
        {
            WS2812FX_setPixelColor(_seg->start + i, color);
        }
        // else
        // {
        //   WS2812FX_setPixelColor(_seg->stop - i,  color);
        // }
    }

    _seg_rt->counter_mode_step = (_seg_rt->counter_mode_step + 1) & 0x1FF;
    if (_seg_rt->counter_mode_step == 0) SET_CYCLE;

    if (m_fs.act == E_RISE)
    {
        if (m_fs.c_pos < m_fs.rise_tag)
        {   // -1是留一个点位置给顶端显示白点
            m_fs.c_pos++;
        }
        else
        {   // 到达顶端
            m_fs.act = E_TOP;
            if (m_fs.rise_tag > m_fs.top_pos)  //刷新顶端值
            {
                m_fs.top_pos = m_fs.rise_tag - 1;
            }
            m_fs.rise_tag = 0;
        }
    }
    else
    {
        if (m_fs.c_pos > 1)
            m_fs.c_pos -= 1;
    }

    if (m_fs.rise_tag > m_fs.c_pos && m_fs.rise_tag > 0)
    {
        m_fs.act = E_RISE;
    }
    else
    {
        m_fs.act = E_TOP;
    }

    return 10;
}

uint16_t music_fs_green_blue2(void)
{
    uint16_t i;
    uint8_t size = 1 << SIZE_OPTION;
    uint8_t param;
    param = _seg_len;
    uint32_t color;
    uint8_t sineIncr = max(1, (256 / param) * size);

    if (m_fs2.rise_tag > _seg_len)
        m_fs2.rise_tag = _seg_len;
    Adafruit_NeoPixel_fill(BLACK, _seg->start, _seg_len);

    if (close_music)
        m_fs2.c_pos = 0;

    for (i = 0; i < m_fs2.c_pos; i++) //上升点亮灯带
    {
        int lum = (int)Adafruit_NeoPixel_sine8(((i + _seg_rt->counter_mode_step) * sineIncr));

        if (i < (16 * USER_WS2812_SEGMENT_2_LEN / 60))
        {
            color = WS2812FX_color_blend(BLUE, BLUE, lum);
        }
        else if (i >= (16 * USER_WS2812_SEGMENT_2_LEN / 60) && i < (52 * USER_WS2812_SEGMENT_2_LEN / 60))
        {
            color = WS2812FX_color_blend(GREEN, GREEN, lum);
        }
        else
        {
            color = WS2812FX_color_blend(RED, RED, lum);
        }
        WS2812FX_setPixelColor(_seg->start + i, color);
    }

    _seg_rt->counter_mode_step = (_seg_rt->counter_mode_step + 1) & 0x1FF;
    if (_seg_rt->counter_mode_step == 0) SET_CYCLE;

    if (m_fs2.act == E_RISE)
    {
        if (m_fs2.c_pos < m_fs2.rise_tag) //-1是留一个点位置给顶端显示白点
        {
            m_fs2.c_pos += 2;
        }
        else//到达顶端
        {
            m_fs2.act = E_TOP;
            if (m_fs2.rise_tag > m_fs2.top_pos)  //刷新顶端值
            {
                m_fs2.top_pos = m_fs2.rise_tag - 2;
            }
            m_fs2.rise_tag = 0;
        }
    }
    else
    {
        if (m_fs2.c_pos > 2) m_fs2.c_pos -= 2;
    }

    if (m_fs2.rise_tag > m_fs2.c_pos && m_fs2.rise_tag > 0)
    {
        m_fs2.act = E_RISE;
    }
    else
    {
        m_fs2.act = E_TOP;
    }

    return 10;
}

// 渐亮，适用纯色效果
// c:目标颜色
// f：完成标志
uint32_t fade_in(uint32_t c, uint8_t *f)
{
  static float i=1;
  uint8_t r,g,b;
  r = c >> 16;
  g = c >> 8;
  b = c >> 0;

  r = r * (i) / 255;
  g = g * (i) / 255;
  b = b * (i) / 255;
  if(i < 128)
  i=i*2;
  else i=i*1.2;
  *f = 0;
  if(i >= 255)
  {
    i=1;
    *f = 1;
     return c;
  } 
  
  c = r<<16 | g<<8 | b;
  return c;
}

uint32_t fu_fade_in(uint32_t c, uint8_t *f)
{
  static float i1=1;
  uint8_t fu_r,fu_g,fu_b;
  fu_r = c >> 16;
  fu_g = c >> 8;
  fu_b = c >> 0;

  fu_r = fu_r * (i1) / 255;
  fu_g = fu_g * (i1) / 255;
  fu_b = fu_b * (i1) / 255;
  if(i1 < 128)
  i1=i1*2;
  else i1=i1*1.2;
  *f = 0;
  if(i1 >= 255)
  {
    i1=1;
    *f = 1;
     return c;
  } 
  
  c = fu_r<<16 | fu_g<<8 | fu_b;
  return c;
}

/* 开幕式 ,支持多种颜色
1、慢慢渐亮到目标颜色
2、拉开帷幕
3、完成开幕后是全黑的
4、开始新的开幕，重复从“1”开始
*/
uint16_t open_effect(void)
{
  uint8_t f;
  if(_seg_rt->counter_mode_step==0)
  {
    Adafruit_NeoPixel_fill( fade_in(_seg->colors[_seg_rt->aux_param], &f), _seg->start, _seg_len);
    if(f==0)return (_seg->speed );
  } 


  WS2812FX_setPixelColor( _seg->stop - _seg_rt->counter_mode_step, BLACK);
  WS2812FX_setPixelColor( _seg->start + _seg_rt->counter_mode_step, BLACK);



  _seg_rt->counter_mode_step++;
  _seg_rt->counter_mode_step %= _seg_len/2 -1;
  if(_seg_rt->counter_mode_step==0)
  {
    _seg_rt->aux_param++;
    _seg_rt->aux_param %= _seg->c_n;

  }
  return (_seg->speed );
}
uint16_t fu_open_effect(void)
{
  uint8_t f1;
  if(_seg_rt->counter_mode_step==0)
  {
    Adafruit_NeoPixel_fill( fu_fade_in(_seg->colors[_seg_rt->aux_param], &f1), _seg->start, _seg_len);
    if(f1==0)return (_seg->speed );
  } 


  WS2812FX_setPixelColor( _seg->stop  - _seg_rt->counter_mode_step, BLACK);
  WS2812FX_setPixelColor( _seg->start + _seg_rt->counter_mode_step, BLACK);



  _seg_rt->counter_mode_step++;
  _seg_rt->counter_mode_step %= _seg_len/2;
  if(_seg_rt->counter_mode_step==0)
  {
    _seg_rt->aux_param++;
    _seg_rt->aux_param %= _seg->c_n;

  }
  return (_seg->speed );
}

/* 闭幕式 ,支持多种颜色*/
uint16_t close_effect(void)
{
  if(_seg_rt->counter_mode_step==0)
  {
    _seg_rt->aux_param3++;
    if(_seg_rt->aux_param3 < 9 )
    {
      
      WS2812FX_fade_out_targetColor( BLACK);
      return (_seg->speed );
    }
    _seg_rt->aux_param3 = 0;
    _seg_rt->aux_param++;
    _seg_rt->aux_param %= _seg->c_n;
  }
  WS2812FX_setPixelColor( _seg->start + _seg_rt->counter_mode_step, _seg->colors[_seg_rt->aux_param]);
  WS2812FX_setPixelColor( _seg->stop - _seg_rt->counter_mode_step - 1, _seg->colors[_seg_rt->aux_param] );

  _seg_rt->counter_mode_step++;
  _seg_rt->counter_mode_step %= _seg_len/2;


  return (_seg->speed );
}


/* 多个点跑马 ，点和点直接固定间隔5，支持每个点不同颜色，支持设置背景色*/
// _seg->colors[0] 马仔的颜色
// _seg->colors[1] 最后一个颜色为背景色
uint16_t multi_dot_running(void)
{


  uint32_t color;

  uint16_t i;
  for(i=0; i< _seg_len; i++)
  {
    if(_seg_rt->aux_param3 == 0)
    {
      // if(_seg_rt->counter_mode_step % 1==0)
      {
        // _seg_rt->counter_mode_step = 0;
        _seg_rt->aux_param3 = 1;
      }
      /* 0-_seg->c_n-2 是前景色 */
      color = _seg->colors[_seg_rt->aux_param2_ex];
      _seg_rt->aux_param2_ex++;
      _seg_rt->aux_param2_ex %= _seg->c_n-1;
      // color = WS2812FX_color_wheel(_seg_rt->aux_param2_ex+=70);//随机颜色
    }
    else
    {
      /* 构造背景色 */
      if(_seg_rt->counter_mode_step %= 5)
      {
        // _seg_rt->counter_mode_step = 0;

        _seg_rt->aux_param3 = 0;
      }
      /* 最后一个颜色是背景色 */
      color = _seg->colors[_seg->c_n-1];
    }

    if(IS_REVERSE) //反向
    {
      WS2812FX_setPixelColor(_seg->start + i, color);
    }
    else
    {
      WS2812FX_setPixelColor(_seg->stop - i, color);
    }
    _seg_rt->counter_mode_step = (_seg_rt->counter_mode_step + 1) % _seg_len;
  }
  _seg_rt->counter_mode_step = (_seg_rt->counter_mode_step + 1) % _seg_len;
  return (_seg->speed );
}


uint16_t aoweisi_dot_running1(void)
{
    uint32_t color;

    uint16_t i;
    for (i = 0; i < _seg_len; i++)
    {
        if (_seg_rt->aux_param3 == 0)
        {
            _seg_rt->aux_param3 = 1;
            /* 0-_seg->c_n-2 是前景色 */

            color = _seg->colors[_seg_rt->aux_param2_ex];
            // _seg_rt->aux_param2_ex++;
            // _seg_rt->aux_param2_ex %= _seg->c_n-1;

        }
        else
        {
          /* 构造背景色 */
            if (_seg_rt->counter_mode_step %= 5)
            {
                _seg_rt->aux_param3 = 0;
            }
            /* 最后一个颜色是背景色 */
            color = _seg->colors[_seg->c_n - 1];
        }

        if (IS_REVERSE) //反向
        {
            WS2812FX_setPixelColor(_seg->start + i, color);
        }
        else
        {
            WS2812FX_setPixelColor(_seg->stop - i, color);
        }
        _seg_rt->counter_mode_step = (_seg_rt->counter_mode_step + 1) % _seg_len;
    }
    _seg_rt->counter_mode_step = (_seg_rt->counter_mode_step + 1) % _seg_len;
    // printf("_seg_rt->counter_mode_step =%d",_seg_rt->counter_mode_step);
    if (_seg_rt->counter_mode_step >= 6)
    {
        _seg_rt->aux_param++;
        if (_seg_rt->aux_param == 3)
        {
            _seg_rt->aux_param = 0;
            _seg_rt->aux_param2_ex++;
            _seg_rt->aux_param2_ex %= _seg->c_n - 1;
        }
    }
    return (_seg->speed);
}


extern const u8 huancai2_num;
uint16_t aoweisi_dot_running2(void)
{
    uint32_t color;
    uint16_t i;
    for (i = 0; i < _seg_len; i++)
    {
        if (_seg_rt->aux_param3 == 0)
        {
            _seg_rt->aux_param3 = 1;
            /* 0-_seg->c_n-2 是前景色 */

            color = _seg->colors[_seg_rt->aux_param2_ex];
        }
        else
        {
          /* 构造背景色 */
            if (_seg_rt->rgbw_counter_mode_step %= 5)
            {
                _seg_rt->aux_param3 = 0;
            }
            /* 最后一个颜色是背景色 */
            color = _seg->colors[_seg->c_n - 1];
        }

        if (IS_REVERSE) //反向
        {
            WS2812FX_setPixelColor(_seg->start + i, color);
        }
        else
        {
            WS2812FX_setPixelColor(_seg->stop - i, color);
        }
        _seg_rt->rgbw_counter_mode_step = (_seg_rt->rgbw_counter_mode_step + 1) % _seg_len;
    }
    _seg_rt->rgbw_counter_mode_step = (_seg_rt->rgbw_counter_mode_step + 1) % _seg_len;

    if (_seg_rt->rgbw_counter_mode_step >= 6)
    {
        _seg_rt->aux_param++;
        if (_seg_rt->aux_param == 3)
        {
            _seg_rt->aux_param = 0;
            _seg_rt->aux_param2_ex++;
            _seg_rt->aux_param2_ex %= _seg->c_n - 1;
        }
    }
    return (_seg->speed);
}


//奥维斯的三个客制效果 10个灯，3个灯跑动
uint16_t aoweisi_dot_running(void)
{

// printf("_seg_rt->counter_mode_step = %d",_seg_rt->counter_mode_step);
static qiehuan_color = 0;
  if(_seg_rt->counter_mode_step == 3 && _seg->c_n > 2)
  {
        qiehuan_color++;
        if(qiehuan_color == 2)
        {
          qiehuan_color = 0;
          _seg_rt->aux_param++;
          _seg_rt->aux_param %= _seg->c_n;
        }
     
  }

  int w1 = (_seg->colors[_seg_rt->aux_param] >> 24) & 0xff;
  int r1 = (_seg->colors[_seg_rt->aux_param] >> 16) & 0xff;
  int g1 = (_seg->colors[_seg_rt->aux_param] >>  8) & 0xff;
  int b1 =  _seg->colors[_seg_rt->aux_param]        & 0xff;


if(_seg_rt->counter_mode_step >= 3)
{
  _seg_rt->counter_mode_step = 0;
   Adafruit_NeoPixel_fill(BLACK, _seg->start, _seg_len);
   WS2812FX_setPixelColor_rgbw(_seg->start + _seg_rt->counter_mode_step, r1, g1, b1, w1);
    WS2812FX_setPixelColor_rgbw(_seg->start + _seg_rt->counter_mode_step + 3, r1, g1, b1, w1);
    WS2812FX_setPixelColor_rgbw(_seg->start +  _seg_rt->counter_mode_step +7, r1, g1, b1, w1);
}


    WS2812FX_setPixelColor_rgbw(_seg->start + _seg_rt->counter_mode_step, r1, g1, b1, w1);
    WS2812FX_setPixelColor_rgbw(_seg->start + _seg_rt->counter_mode_step + 3, r1, g1, b1, w1);
    WS2812FX_setPixelColor_rgbw(_seg->start +  _seg_rt->counter_mode_step +7, r1, g1, b1, w1);

if(_seg_rt->counter_mode_step >0)
{
    WS2812FX_setPixelColor_rgbw(_seg->start + _seg_rt->counter_mode_step - 1,      0, 0, 0,0);
    WS2812FX_setPixelColor_rgbw(_seg->start + _seg_rt->counter_mode_step + 3 - 1, 0, 0, 0,0);
    WS2812FX_setPixelColor_rgbw(_seg->start +  _seg_rt->counter_mode_step +7 - 1, 0, 0, 0,0);
}
 
  
  _seg_rt->counter_mode_step++;

  
  return (_seg->speed );




}




//流星长度
#define MAX_RATE 12
// 单色流水效果,流星雨  循环
uint16_t meteor_signle_c(void)
{
    uint16_t index;
    static uint8_t i = 0;
    uint32_t r1, g1, b1, w1;
    // const uint8_t rate[MAX_RATE] = {100,75,45,30,20,15,10,7,5,3,2,0,0,0,0,0,0};
    const uint8_t rate[MAX_RATE] = { 100,75,45,15,7,0,0,0,0,0,0,0,0,0,0,0,0 };

    // _seg_rt->aux_param3++;
    // if(_seg_rt->aux_param3 < 9 )
    // {
    //   i = 0;
    //   // _seg->options |= FADE_XSLOW;
    //   /* 渐灭当前灯 */
    //   WS2812FX_fade_out_targetColor( BLACK);
    //   return (_seg->speed );
    // }

    int w = (_seg->colors[_seg_rt->aux_param] >> 24) & 0xff;
    int r = (_seg->colors[_seg_rt->aux_param] >> 16) & 0xff;
    int g = (_seg->colors[_seg_rt->aux_param] >> 8) & 0xff;
    int b = _seg->colors[_seg_rt->aux_param] & 0xff;
    if (IS_REVERSE) //反向
    {
        WS2812FX_copyPixels(_seg->start, _seg->start + 1, _seg_len - 1);
    }
    else
    {
        WS2812FX_copyPixels(_seg->start + 1, _seg->start, _seg_len - 1);
    }


    // if(_seg_rt->counter_mode_step == 0)
    // for( index = _seg->start; index < _seg->stop; index++)
    // index = _seg_rt->counter_mode_step;
    {
        r1 = r * rate[i] / 100;
        g1 = g * rate[i] / 100;
        b1 = b * rate[i] / 100;
        w1 = w * rate[i] / 100;

        if (IS_REVERSE) //反向
        {
            WS2812FX_setPixelColor_rgbw(_seg->stop, r1, g1, b1, w1);
        }
        else
        {
            WS2812FX_setPixelColor_rgbw(_seg->start, r1, g1, b1, w1);
        }

        i++;
        i %= MAX_RATE;
        if (i == 0)
        {
            _seg_rt->aux_param++;
            _seg_rt->aux_param %= _seg->c_n;
        }
        // if(i>0)
        // i--;
        // else
        // i = MAX_RATE-1;
    }
    // _seg_rt->counter_mode_step++;
    // _seg_rt->counter_mode_step %= _seg_len;

    return (_seg->speed);
}

uint16_t meteor_signle_c2(void)
{
  uint16_t index;
  static uint8_t i = 0;
  uint32_t r1, g1, b1, w1 ;
  const uint8_t rate[MAX_RATE] = {100,75,45,15,7,0,0,0,0,0,0,0,0,0,0,0,0};
  // _seg_rt->aux_param3++;
  // if(_seg_rt->aux_param3 < 9 )
  // {
  //   i = 0;
  //   // _seg->options |= FADE_XSLOW;
  //   /* 渐灭当前灯 */
  //   WS2812FX_fade_out_targetColor( BLACK);
  //   return (_seg->speed );
  // }
  
  int w = (_seg->colors[_seg_rt->aux_param2_ex] >> 24) & 0xff;
  int r = (_seg->colors[_seg_rt->aux_param2_ex] >> 16) & 0xff;
  int g = (_seg->colors[_seg_rt->aux_param2_ex] >>  8) & 0xff;
  int b =  _seg->colors[_seg_rt->aux_param2_ex]        & 0xff;

    WS2812FX_copyPixels(_seg->start+1,_seg->start, _seg_len-1);
  
  

    r1 = r * rate[i] / 100;
    g1 = g * rate[i] / 100;
    b1 = b * rate[i] / 100;
    w1 = w * rate[i] / 100;

    WS2812FX_setPixelColor_rgbw(_seg->start, r1, g1, b1, w1);
    
    
    i++;
    i %= MAX_RATE;
    if(i == 0)
    {
      _seg_rt->aux_param2_ex++;
      _seg_rt->aux_param2_ex %= _seg->c_n;
    }


  return (_seg->speed );
}
//流星长度
#define m_MAX_RATE 12
// _seg->colors[0] 流星颜色
// _seg->colors[1]背景色
// 带有背景色的流星，每次只有一个流星，流星时间间隔3秒
uint16_t meteor_bc(void)
{

  static uint8_t i = 0;
  uint32_t r1, g1, b1, w1 ;

  const uint8_t rate[m_MAX_RATE] = {100,70,45,30,20,15,10,7,5,4,3,2};
  // const uint8_t rate[meteor_len] = {100,60,45,30,20,15,10,7,5,4};  //亮度值 0-100
  int w = (_seg->colors[0] >> 24) & 0xff;
  int r = (_seg->colors[0] >> 16) & 0xff;
  int g = (_seg->colors[0] >>  8) & 0xff;
  int b =  _seg->colors[0]        & 0xff;
  if(_seg_rt->aux_param == 0 )
  {
    i = 0;
    _seg_rt->aux_param = 1;
    Adafruit_NeoPixel_fill(_seg->colors[1], _seg->start, _seg_len);
  }

  WS2812FX_copyPixels(_seg->start+1,_seg->start, _seg_len-1);

  r1 = r * rate[i] / 100;
  g1 = g * rate[i] / 100;
  b1 = b * rate[i] / 100;
  w1 = w * rate[i] / 100;

  if(i >= m_MAX_RATE)
  {
    r1 = _seg->colors[1] >> 16;
    g1 = _seg->colors[1] >> 8;
    b1 = _seg->colors[1] >> 0;
  }
  WS2812FX_setPixelColor_rgbw(_seg->start, r1, g1, b1, w1);
  if(i<m_MAX_RATE)
    i++;

  _seg_rt->counter_mode_step++;
  _seg_rt->counter_mode_step %= _seg_len-1+m_MAX_RATE*2;

  if(_seg_rt->counter_mode_step == 0)
  {
    i = 0;
  }

  return (_seg->speed );
}







//单色流星
/**
 * @brief 
 * 
 * @return uint16_t 
 */
uint16_t single_color_meteor(void)
{

  static uint8_t i = 0;
  uint8_t size = 1 << SIZE_OPTION;
  uint8_t meteor_len = 12;
  if(size == 1 )
  {
    meteor_len = 3;
  }
  else if(size == 2)
  {
    meteor_len = 5;
  }
  else if(size == 4)
  {
    meteor_len = 6;
  }
  else if(size == 8)
  {
    meteor_len = 12;
  }
  if(_seg_rt->counter_mode_step == 0)   //该判断放在这里，解决效果切换时，立即切换
  {
    i = 0;
    SET_CYCLE;
    // fc_effect.mode_cycle = 1;
  } 
  if( (get_effect_p() == 1) && (fc_effect.mode_cycle == 1) )  //计时中 && 完成一个循环
  {
    return (_seg->speed );
  } 
  uint32_t r1 = 0, g1 = 0, b1 = 0, w1 = 0;  //必须初始化
  int w = 0, r = 0, g = 0, c = 0, b = 0;    //必须初始化
  const uint8_t rate[MAX_RATE] = {100,60,45,30,20,15,10,7,5,4,1,1};  //亮度值 0-100
  
  //灯的颜色
  w = (_seg->colors[0] >> 24) & 0xff;
  r = (_seg->colors[0] >> 16) & 0xff;
  g = (_seg->colors[0] >>  8) & 0xff;
  b =  _seg->colors[0]        & 0xff;

   if(IS_REVERSE) //反向流水
    WS2812FX_move_reverse(_seg->start, _seg->stop);
  else
    WS2812FX_move_forward(_seg->start, _seg->stop); 

  if(i < meteor_len)   //避免数组溢出
  {
    r1 = r * rate[i] / 100;
    g1 = g * rate[i] / 100;
    b1 = b * rate[i] / 100;
    w1 = w * rate[i] / 100;
    i++;
  }

  if(IS_REVERSE) //反向流水
    WS2812FX_setPixelColor_rgbw(_seg->stop, r1, g1, b1, w1);   //反向
  else
    WS2812FX_setPixelColor_rgbw(_seg->start, r1, g1, b1, w1);

  _seg_rt->counter_mode_step++;
  _seg_rt->counter_mode_step %= _seg_len + meteor_len ;

  if(_seg_rt->counter_mode_step == 0)
    fc_effect.mode_cycle = 1;

  return (_seg->speed );

}

//跳变流星
uint16_t jump_meteor(void)
{
 static uint8_t i = 0;
 if(_seg_rt->counter_mode_step == 0 )
  {
    i = 0;
    SET_CYCLE;
    // fc_effect.mode_cycle = 1;
    
  }

//-------------------- 周期 ---------------------------------------------
  if( (get_effect_p() == 1) && (fc_effect.mode_cycle == 1) )  //计时中 && 完成一个循环
  {
    return (_seg->speed );
  } 
  // uint8_t size = (SIZE_OPTION << 1) + 1;
 
  uint32_t r1 = 0, g1 = 0, b1 = 0, w1 = 0;  //必须初始化
  int w = 0, r = 0, g = 0, c = 0, b = 0;    //必须初始化

  // const uint8_t rate[MAX_RATE] = {100,50,40,30,20,10};  //亮度值 0-100
  const uint8_t rate[MAX_RATE] = {100,60,45,30,20,15,10,7,5,4};  //亮度值 0-100

  uint8_t n;

//---------------------- 效果过程 --------------------------------------
  w = (_seg->colors[_seg_rt->aux_param] >> 24) & 0xff;
  r = (_seg->colors[_seg_rt->aux_param] >> 16) & 0xff;
  g = (_seg->colors[_seg_rt->aux_param] >>  8) & 0xff;
  b =  _seg->colors[_seg_rt->aux_param]        & 0xff;

  WS2812FX_copyPixels(_seg->start+1,_seg->start, _seg_len-1);  //颜色平移

//调节亮度
  if( i < MAX_RATE) //避免数组溢出
  {
    r1 = r * rate[i] / 100;
    g1 = g * rate[i] / 100;
    b1 = b * rate[i] / 100;
    w1 = w * rate[i] / 100;
    i++;
  }

  WS2812FX_setPixelColor_rgbw(_seg->start, r1, g1, b1, w1);  //颜色值
  //整段变色
  // n = _seg_rt->counter_mode_step;
  // if((n % (fc_effect.led_num / _seg->c_n) )== 0)  //6是流星的长度，根据流星长度而变
  n =  _seg_rt->counter_mode_step % (3);
  // n =  _seg_rt->counter_mode_step % (fc_effect.led_num / _seg->c_n);
  if(n == 0)
  {
    _seg_rt->aux_param++;
    _seg_rt->aux_param %= _seg->c_n; //颜色数量
 
    for(uint8_t m = 0,i = 0; m < MAX_RATE; m++,i++)
    {
      w = (_seg->colors[_seg_rt->aux_param] >> 24) & 0xff;
      r = (_seg->colors[_seg_rt->aux_param] >> 16) & 0xff;
      g = (_seg->colors[_seg_rt->aux_param] >>  8) & 0xff;
      b =  _seg->colors[_seg_rt->aux_param]        & 0xff;

      r1 = r * rate[i] / 100;
      g1 = g * rate[i] / 100;
      b1 = b * rate[i] / 100;
      w1 = w * rate[i] / 100;
      // if(_seg_rt->counter_mode_step < 15)
      WS2812FX_setPixelColor_rgbw(_seg_rt->counter_mode_step - m, r1, g1, b1, w1);  //颜色值
    }
  }

   
//----------------- 结束条件 ------------------------------------------
  
  _seg_rt->counter_mode_step++;
  _seg_rt->counter_mode_step %= _seg_len  + MAX_RATE;
  if(_seg_rt->counter_mode_step == 0)
    fc_effect.mode_cycle = 1;
 
  return (_seg->speed );

}

//渐变流星
int gadual_step = 0;
uint32_t get_color_ad(uint8_t ad_step)
{

  static uint8_t index ;
  static uint32_t c0,c1;
  int lum = gadual_step;
  uint32_t color ;
  if(lum > 255) lum = 511 - lum; // lum = 0 -> 255 -> 0
  if(_seg_rt->aux_param==0)
  {
    _seg_rt->aux_param = 1;
    index = 0;
    c1 = _seg->colors[index]; 
		index++;
    c0 = _seg->colors[index];
  }
  // c1目标颜色
  color = WS2812FX_color_blend(c1, c0, lum);
  if(gadual_step == 256)
  {
   
    index++;
    index %= _seg->c_n; //颜色池的颜色数量
    c1 = _seg->colors[index]; 
 

  }
  gadual_step += ad_step ;  //修改渐变速度
  if(gadual_step > 511) {

   
    gadual_step = 0;
    index++;
    index %= _seg->c_n; //颜色池的颜色数量
    c0 = _seg->colors[index];
  }
  return color;
}


//使用子函数思路
//流星一部分， 获取渐变颜色一部分
uint16_t gradual_meteor(void)
{
 static uint8_t i = 0;
  if(_seg_rt->counter_mode_step == 0 )
  {
    i = 0;
    SET_CYCLE;
    // fc_effect.mode_cycle = 1;
    // gadual_step = 0; //新的周期
  }

  if( (get_effect_p() == 1) && (fc_effect.mode_cycle == 1) )  //计时中 && 完成一个循环
  {
    return (_seg->speed );
  } 

 
  uint32_t r1 = 0, g1 = 0, b1 = 0, w1 = 0;  //必须初始化
  int w = 0, r = 0, g = 0, c = 0, b = 0;    //必须初始化
  // const uint8_t rate[MAX_RATE] = {100,50,40,30,20,10};  //亮度值 0-100
  const uint8_t rate[MAX_RATE] = {100,60,45,30,20,15,10,7,5,4};  //亮度值 0-100

  uint32_t color;
  //获取颜色
  color = get_color_ad(20);
  w = (color >> 24) & 0xff;
  r = (color >> 16) & 0xff;
  g = (color >>  8) & 0xff;
  b =  color        & 0xff;

    WS2812FX_copyPixels(_seg->start+1,_seg->start, _seg_len-1);  //颜色平移  控制流星移动

  //调节亮度
  if( i < MAX_RATE) //避免数组溢出
  {
    r1 = r * rate[i] / 100;
    g1 = g * rate[i] / 100;
    b1 = b * rate[i] / 100;
    w1 = w * rate[i] / 100;
    i++;
  }
  
    WS2812FX_setPixelColor_rgbw(_seg->start, r1, g1, b1, w1);  //颜色值
   if(_seg_rt->counter_mode_step > 5)
  //  if(_seg_rt->counter_mode_step > (fc_effect.led_num / _seg->c_n))
   {
      for(int n = 0 ,i = 0; n < MAX_RATE;n++, i++) //i和n都是流星的长度
      {
        // color = get_color_ad(5);
        w = (color >> 24) & 0xff;
        r = (color >> 16) & 0xff;
        g = (color >>  8) & 0xff;
        b =  color        & 0xff;
        
        r1 = r * rate[i] / 100;
        g1 = g * rate[i] / 100;
        b1 = b * rate[i] / 100;
        w1 = w * rate[i] / 100;
        WS2812FX_setPixelColor_rgbw(_seg_rt->counter_mode_step - n , r1, g1, b1, w1);  //颜色值
      }
   }

 //控制灯的流动长度
  _seg_rt->counter_mode_step++;
  _seg_rt->counter_mode_step %= _seg_len  + MAX_RATE;

  if(_seg_rt->counter_mode_step == 0)
    fc_effect.mode_cycle = 1;

  return (_seg->speed );

}










/* -------------------------------ws2812fx自带效果----------------------------------- */
/* #define	FX_MODE_STATIC			0		//静态-无闪烁。只是普通的老式静电灯。
#define	FX_MODE_BLINK			1		//眨眼-正常眨眼。50%的开/关时间。
#define	FX_MODE_BREATH			2		//呼吸-进行众所周知的i-设备的“备用呼吸”。固定速度。
#define	FX_MODE_COLOR_WIPE			3		//颜色擦除-在每个LED灯亮起后点亮所有LED灯。然后按顺序关闭它们。重复
#define	FX_MODE_COLOR_WIPE_INV			4		//颜色擦除反转-与颜色擦除相同，只是交换开/关颜色。
#define	FX_MODE_COLOR_WIPE_REV			5		//颜色擦除反转-在每个LED点亮后点亮所有LED。然后按相反的顺序关闭它们。重复
#define	FX_MODE_COLOR_WIPE_REV_INV			6		//颜色擦除反转-与颜色擦除反转相同，除了交换开/关颜色。
#define	FX_MODE_COLOR_WIPE_RANDOM			7		//颜色擦除随机-将所有LED依次切换为随机颜色。然后用另一种颜色重新开始。
#define	FX_MODE_RANDOM_COLOR			8		//随机颜色-以一种随机颜色点亮所有LED。然后将它们切换到下一个随机颜色。
#define	FX_MODE_SINGLE_DYNAMIC			9		//单一动态-以随机颜色点亮每个LED。将一个随机LED逐个更改为随机颜色。
#define	FX_MODE_MULTI_DYNAMIC			10		//多动态-以随机颜色点亮每个LED。将所有LED同时更改为新的随机颜色。
#define	FX_MODE_RAINBOW			11		//彩虹-通过彩虹同时循环所有LED。
#define	FX_MODE_RAINBOW_CYCLE			12		//彩虹循环-在整个LED串上循环彩虹。
#define	FX_MODE_SCAN			13		//扫描-来回运行单个像素。
#define	FX_MODE_DUAL_SCAN			14		//双扫描-在相反方向来回运行两个像素。
#define	FX_MODE_FADE			15		//淡入淡出-使LED灯再次淡入淡出。
#define	FX_MODE_THEATER_CHASE			16		//剧场追逐——剧场风格的爬行灯。灵感来自Adafruit的例子。
#define	FX_MODE_THEATER_CHASE_RAINBOW			17		//剧院追逐彩虹-剧院风格的彩虹效果爬行灯。灵感来自Adafruit的例子。
#define	FX_MODE_RUNNING_LIGHTS			18		//运行灯光-运行灯光效果与平滑正弦过渡。
#define	FX_MODE_TWINKLE			19		//闪烁-闪烁几个LED灯，重置，重复。
#define	FX_MODE_TWINKLE_RANDOM			20		//闪烁随机-以随机颜色闪烁多个LED，打开、重置、重复。
#define	FX_MODE_TWINKLE_FADE			21		//闪烁淡出-闪烁几个LED，淡出。
#define	FX_MODE_TWINKLE_FADE_RANDOM			22		//闪烁淡出随机-以随机颜色闪烁多个LED，然后淡出。
#define	FX_MODE_SPARKLE			23		//闪烁-每次闪烁一个LED。
#define	FX_MODE_FLASH_SPARKLE			24		//闪烁-以选定颜色点亮所有LED。随机闪烁单个白色像素。
#define	FX_MODE_HYPER_SPARKLE			25		//超级火花-像闪光火花。用更多的闪光。
#define	FX_MODE_STROBE			26		//频闪-经典的频闪效果。
#define	FX_MODE_STROBE_RAINBOW			27		//频闪彩虹-经典的频闪效果。骑自行车穿越彩虹。
#define	FX_MODE_MULTI_STROBE			28		//多重选通-具有不同选通计数和暂停的选通效果，由速度设置控制。
#define	FX_MODE_BLINK_RAINBOW			29		//闪烁彩虹-经典的闪烁效果。骑自行车穿越彩虹。
#define	FX_MODE_CHASE_WHITE			30		//追逐白色——白色上的颜色。
#define	FX_MODE_CHASE_COLOR			31		//追逐颜色-白色在颜色上奔跑。
#define	FX_MODE_CHASE_RANDOM			32		//Chase Random-白色跑步，然后是随机颜色。
#define	FX_MODE_CHASE_RAINBOW			33		//追逐彩虹——白色在彩虹上奔跑。
#define	FX_MODE_CHASE_FLASH			34		//Chase Flash（追逐闪光）-在彩色屏幕上运行的白色闪光。
#define	FX_MODE_CHASE_FLASH_RANDOM			35		//Chase Flash Random（随机闪烁）：白色闪烁，然后是随机颜色。
#define	FX_MODE_CHASE_RAINBOW_WHITE			36		//追逐彩虹白色-彩虹在白色上奔跑。
#define	FX_MODE_CHASE_BLACKOUT			37		//Chase Blackout-黑色在颜色上运行。
#define	FX_MODE_CHASE_BLACKOUT_RAINBOW			38		//追逐遮光彩虹-黑色在彩虹上奔跑。
#define	FX_MODE_COLOR_SWEEP_RANDOM			39		//颜色扫描随机-从条带开始和结束交替引入的随机颜色。
#define	FX_MODE_RUNNING_COLOR			40		//运行颜色-交替运行颜色/白色像素。
#define	FX_MODE_RUNNING_RED_BLUE			41		//运行红蓝-交替运行红/蓝像素。
#define	FX_MODE_RUNNING_RANDOM			42		//随机运行-随机彩色像素运行。
#define	FX_MODE_LARSON_SCANNER			43		//拉森扫描仪-K.I.T.T。
#define	FX_MODE_COMET			44		//彗星——从一端发射彗星。
#define	FX_MODE_FIREWORKS			45		//烟花-烟花火花。
#define	FX_MODE_FIREWORKS_RANDOM			46		//烟花随机-随机彩色烟花火花。
#define	FX_MODE_MERRY_CHRISTMAS			47		//圣诞快乐-绿色/红色像素交替运行。
#define	FX_MODE_FIRE_FLICKER			48		//火焰闪烁-火焰闪烁效果。就像在刺骨的风中。
#define	FX_MODE_FIRE_FLICKER_SOFT			49		//火焰闪烁（软）-火焰闪烁效果。跑得更慢/更柔和。
#define	FX_MODE_FIRE_FLICKER_INTENSE			50		//火焰闪烁（强烈）-火焰闪烁效果。更多颜色范围。
#define	FX_MODE_CIRCUS_COMBUSTUS			51		//Circus Combustitus-交替运行白/红/黑像素。
#define	FX_MODE_HALLOWEEN			52		//万圣节-交替运行橙色/紫色像素。
#define	FX_MODE_BICOLOR_CHASE			53		//双色追逐-背景色上运行的两个LED。
#define	FX_MODE_TRICOLOR_CHASE			54		//三色追逐-交替运行三色像素。
#define	FX_MODE_CUSTOM			55  // keep this for backward compatiblity		//闪烁福克斯-灯光随机淡入淡出。
#define	FX_MODE_CUSTOM_0			55  // custom modes need to go at the end		//通过63。自定义-最多八个用户创建的自定义效果。 */
/*
 * Random flickering.
 */
uint16_t WS2812FX_mode_fire_flicker(void) {
  return WS2812FX_fire_flicker(3);
}

/*
* Random flickering, less intensity.
*/
uint16_t WS2812FX_mode_fire_flicker_soft(void) {
  return WS2812FX_fire_flicker(6);
}

/*
* Random flickering, more intensity.
*/
uint16_t WS2812FX_mode_fire_flicker_intense(void) {
  return WS2812FX_fire_flicker(1);
}


/*
 * Random colored firework sparks.
 */
uint16_t WS2812FX_mode_fireworks_random(void) {
  return WS2812FX_fireworks(WS2812FX_color_wheel(WS2812FX_random8()));
}


/*
 * Firework sparks.
 */
uint16_t WS2812FX_mode_fireworks(void) {
  uint32_t color = BLACK;
  do { // randomly choose a non-BLACK color from the colors array
    color = _seg->colors[WS2812FX_random8_lim(MAX_NUM_COLORS)];
  } while (color == BLACK);
  return WS2812FX_fireworks(color);
}

/*
 * Firing comets from one end.
 */
uint16_t WS2812FX_mode_comet(void) {
  WS2812FX_fade_out();

  if(IS_REVERSE) {
    WS2812FX_setPixelColor(_seg->stop - _seg_rt->counter_mode_step, _seg->colors[0]);
  } else {
    WS2812FX_setPixelColor(_seg->start + _seg_rt->counter_mode_step, _seg->colors[0]);
  }

  _seg_rt->counter_mode_step = (_seg_rt->counter_mode_step + 1) % _seg_len;
  if(_seg_rt->counter_mode_step == 0) SET_CYCLE;

  return (_seg->speed / _seg_len);
}


/*
 * K.I.T.T.
 */
uint16_t WS2812FX_mode_larson_scanner(void) {




    static u32 color = RED;
    _seg->options = FADE_XFAST;
    WS2812FX_fade_out();
   if( _seg_rt->aux_param == 0)
   {
      color = RED;
   }
   else if( _seg_rt->aux_param == 1)
   {
      color = GREEN;
   }
   else if( _seg_rt->aux_param == 2)
   {
      color = BLUE;
   }
    else if( _seg_rt->aux_param == 3)
   {
      color = YELLOW;
   }
    else if( _seg_rt->aux_param == 4)
   {
      color = CYAN;
   }
    else if( _seg_rt->aux_param == 5)
   {
      color = PURPLE;
   }
    else if( _seg_rt->aux_param == 6)
   {
      color = WHITE;
   }




  if(_seg_rt->counter_mode_step < _seg_len) {
  
      WS2812FX_setPixelColor(_seg->start + _seg_rt->counter_mode_step, color);
    
  } else {
    uint16_t index = (_seg_len * 2) - _seg_rt->counter_mode_step - 2;
  
    WS2812FX_setPixelColor(_seg->start + index, color);
    
  }

  _seg_rt->counter_mode_step++;
  if(_seg_rt->counter_mode_step >= (uint16_t)((_seg_len * 2) - 2)) {
    _seg_rt->counter_mode_step = 0;
  _seg_rt->aux_param++;
 
 _seg_rt->aux_param %= 7;

    SET_CYCLE;
  }

  // return (_seg->speed/2);
  return (_seg->speed / (_seg_len));














  // WS2812FX_fade_out();

  // if(_seg_rt->counter_mode_step < _seg_len) {
  //   if(IS_REVERSE) {
  //     WS2812FX_setPixelColor(_seg->stop - _seg_rt->counter_mode_step, _seg->colors[0]);
  //   } else {
  //     WS2812FX_setPixelColor(_seg->start + _seg_rt->counter_mode_step, _seg->colors[0]);
  //   }
  // } else {
  //   uint16_t index = (_seg_len * 2) - _seg_rt->counter_mode_step - 2;
  //   if(IS_REVERSE) {
  //     WS2812FX_setPixelColor(_seg->stop - index, _seg->colors[0]);
  //   } else {
  //     WS2812FX_setPixelColor(_seg->start + index, _seg->colors[0]);
  //   }
  // }

  // _seg_rt->counter_mode_step++;
  // if(_seg_rt->counter_mode_step >= (uint16_t)((_seg_len * 2) - 2)) {
  //   _seg_rt->counter_mode_step = 0;
  //   SET_CYCLE;
  // }

  // return (_seg->speed / (_seg_len * 2));
}


/*
 * Random colored pixels running.
 */
uint16_t WS2812FX_mode_running_random(void) {
  uint8_t size = 2 << SIZE_OPTION;
  if((_seg_rt->counter_mode_step) % size == 0) {
    _seg_rt->aux_param = WS2812FX_get_random_wheel_index(_seg_rt->aux_param);
  }

  uint32_t color = WS2812FX_color_wheel(_seg_rt->aux_param);

  return WS2812FX_running(color, color);
}


/*
 * Alternating color/white pixels running.
 */
uint16_t WS2812FX_mode_running_color(void) {
  return WS2812FX_running(_seg->colors[0], _seg->colors[1]);
}


/*
 * Alternating red/blue pixels running.
 */
uint16_t WS2812FX_mode_running_red_blue(void) {
  return WS2812FX_running(RED, BLUE);
}


/*
 * Alternating red/green pixels running.
 */
uint16_t WS2812FX_mode_merry_christmas(void) {
  return WS2812FX_running(RED, GREEN);
}

/*
 * Alternating orange/purple pixels running.
 */
uint16_t WS2812FX_mode_halloween(void) {
  return WS2812FX_running(PURPLE, ORANGE);
}

/*
 * White flashes running on _color.
 */
uint16_t WS2812FX_mode_chase_flash(void) {
  return WS2812FX_chase_flash(_seg->colors[0], WHITE);
}


/*
 * White flashes running, followed by random color.
 */
uint16_t WS2812FX_mode_chase_flash_random(void) {
  return WS2812FX_chase_flash(WS2812FX_color_wheel(_seg_rt->aux_param), WHITE);
}



/*
 * White running on rainbow.
 白色点流水效果，背景为彩虹色，才会说也跟随白色点变换花样
 */
uint16_t WS2812FX_mode_chase_rainbow(void) {
  uint8_t color_sep = 256 / _seg_len;
  uint8_t color_index = _seg_rt->counter_mode_call & 0xFF;
  uint32_t color = WS2812FX_color_wheel(((_seg_rt->counter_mode_step * color_sep) + color_index) & 0xFF);

  return WS2812FX_chase(color, WHITE, WHITE);
}


/*
 * Black running on rainbow.
 */
uint16_t WS2812FX_mode_chase_blackout_rainbow(void) {
  uint8_t color_sep = 256 / _seg_len;
  uint8_t color_index = _seg_rt->counter_mode_call & 0xFF;
  uint32_t color = WS2812FX_color_wheel(((_seg_rt->counter_mode_step * color_sep) + color_index) & 0xFF);

  return WS2812FX_chase(color, BLACK, BLACK);
}


/*
 * White running followed by random color.
 */
uint16_t WS2812FX_mode_chase_random(void) {
  if(_seg_rt->counter_mode_step == 0) {
    _seg_rt->aux_param = WS2812FX_get_random_wheel_index(_seg_rt->aux_param);
  }
  return WS2812FX_chase(WS2812FX_color_wheel(_seg_rt->aux_param), WHITE, WHITE);
}


/*
 * Rainbow running on white.
 */
uint16_t WS2812FX_mode_chase_rainbow_white(void) {
  uint16_t n = _seg_rt->counter_mode_step;
  uint16_t m = (_seg_rt->counter_mode_step + 1) % _seg_len;
  uint32_t color2 = WS2812FX_color_wheel(((n * 256 / _seg_len) + (_seg_rt->counter_mode_call & 0xFF)) & 0xFF);
  uint32_t color3 = WS2812FX_color_wheel(((m * 256 / _seg_len) + (_seg_rt->counter_mode_call & 0xFF)) & 0xFF);

  return WS2812FX_chase(WHITE, color2, color3);
}

/*
 * Bicolor chase mode
 */
uint16_t WS2812FX_mode_bicolor_chase(void) {
  return WS2812FX_chase(_seg->colors[0], _seg->colors[1], _seg->colors[2]);
}


/*
 * White running on _color.
 */
uint16_t WS2812FX_mode_chase_color(void) {
  return WS2812FX_chase(_seg->colors[0], WHITE, WHITE);
}


/*
 * Black running on _color.
 */
uint16_t WS2812FX_mode_chase_blackout(void) {
  return WS2812FX_chase(_seg->colors[0], BLACK, BLACK);
}


/*
 * _color running on white.
 */
uint16_t WS2812FX_mode_chase_white(void) {
  return WS2812FX_chase(WHITE, _seg->colors[0], _seg->colors[0]);
}


/*
 * Strobe effect with different strobe count and pause, controlled by speed.
  颜色爆闪
 */
uint16_t WS2812FX_mode_multi_strobe(void) {
  Adafruit_NeoPixel_fill(_seg->colors[1], _seg->start, _seg_len);

  uint16_t delay = 200 + ((9 - (_seg->speed % 10)) * 100);
  uint16_t count = 2 * ((_seg->speed / 100) + 1);
  if(_seg_rt->counter_mode_step < count) {
    if((_seg_rt->counter_mode_step & 1) == 0) {
      Adafruit_NeoPixel_fill(_seg->colors[0], _seg->start, _seg_len);
      delay = 20;
    } else {
      delay = 50;
    }
  }

  _seg_rt->counter_mode_step = (_seg_rt->counter_mode_step + 1) % (count + 1);
  if(_seg_rt->counter_mode_step == 0) SET_CYCLE;
  return delay;
}

/*
 * Blinks one LED at a time.
 * Inspired by www.tweaking4all.com/hardware/arduino/arduino-led-strip-effects/
 */
uint16_t WS2812FX_mode_sparkle(void) {
  return WS2812FX_sparkle(_seg->colors[1], _seg->colors[0]);
}


/*
 * Lights all LEDs in the color. Flashes white pixels randomly.
 * Inspired by www.tweaking4all.com/hardware/arduino/arduino-led-strip-effects/
 */
uint16_t WS2812FX_mode_flash_sparkle(void) {
  return WS2812FX_sparkle(_seg->colors[0], WHITE);
}


/*
 * Like flash sparkle. With more flash.
 * Inspired by www.tweaking4all.com/hardware/arduino/arduino-led-strip-effects/
 */
uint16_t WS2812FX_mode_hyper_sparkle(void) {
  Adafruit_NeoPixel_fill(_seg->colors[0], _seg->start, _seg_len);

  uint8_t size = 1 << SIZE_OPTION;
  for(uint8_t i=0; i<8; i++) {
    Adafruit_NeoPixel_fill(WHITE, _seg->start + WS2812FX_random16_lim(_seg_len - size), size);
  }

  SET_CYCLE;
  return (_seg->speed / 32);
}


/*
 * Blink several LEDs on, fading out.
 */
uint16_t WS2812FX_mode_twinkle_fade(void) {
  return WS2812FX_twinkle_fade(_seg->colors[0]);
}


/*
 * Blink several LEDs in random colors on, fading out.
 */
uint16_t WS2812FX_mode_twinkle_fade_random(void) {
  return WS2812FX_twinkle_fade(WS2812FX_color_wheel(WS2812FX_random8()));
}


// 所有LED当前颜色淡出，弹出最终颜色为_seg->colors[1]
uint16_t WS2812FX_mode_fade_single(void)
{

  WS2812FX_fade_out();
  return (_seg->speed / 8);
}

/*
 * Blink several LEDs on, reset, repeat.
 * Inspired by www.tweaking4all.com/hardware/arduino/arduino-led-strip-effects/
 */
uint16_t WS2812FX_mode_twinkle(void) {
  return WS2812FX_twinkle(_seg->colors[0], _seg->colors[1]);
}

/*
 * Blink several LEDs in random colors on, reset, repeat.
 * Inspired by www.tweaking4all.com/hardware/arduino/arduino-led-strip-effects/
 */
uint16_t WS2812FX_mode_twinkle_random(void) {
  return WS2812FX_twinkle(WS2812FX_color_wheel(WS2812FX_random8()), _seg->colors[1]);
}


/*
 * Theatre-style crawling lights with rainbow effect.
 * Inspired by the Adafruit examples.
 */
uint16_t WS2812FX_mode_theater_chase_rainbow(void) {
  _seg_rt->counter_mode_step = (_seg_rt->counter_mode_step + 1) & 0xFF;
  uint32_t color = WS2812FX_color_wheel(_seg_rt->counter_mode_step);
  return WS2812FX_tricolor_chase(color, _seg->colors[1], _seg->colors[1]);
}


/*
 * Running lights effect with smooth sine transition.
 */
uint16_t WS2812FX_mode_running_lights(void) {
  uint8_t size = 1 << SIZE_OPTION;
  uint8_t param;
  param = _seg_len;
  if(_seg_len > 64)
  {
    param = 64;
  }

  uint8_t sineIncr = max(1, (256 / param) * size);
  for(uint16_t i=0; i < _seg_len; i++) {
    int lum = (int)Adafruit_NeoPixel_sine8(((i + _seg_rt->counter_mode_step) * sineIncr));
    uint32_t color = WS2812FX_color_blend(_seg->colors[0], _seg->colors[1], lum);
    if(IS_REVERSE) {
      WS2812FX_setPixelColor(_seg->start + i, color);
    } else {
      WS2812FX_setPixelColor(_seg->stop - i,  color);
    }
  }
  _seg_rt->counter_mode_step = (_seg_rt->counter_mode_step + 1) % 256;
  if(_seg_rt->counter_mode_step == 0) SET_CYCLE;
  return (_seg->speed );
}

//奥威斯客制效果
uint16_t aoweisi_mode_running_lights(void) {
  uint8_t size = 1 << SIZE_OPTION;
  uint8_t param;
  static u8 j = 0;
  uint32_t color;
  param = _seg_len;
  if(_seg_len > 64)
  {
    param = 64;
  }

  uint8_t sineIncr = max(1, (256 / param) * size);

  for(uint16_t i=0; i < _seg_len; i++) {
    int lum = (int)Adafruit_NeoPixel_sine8(((i + _seg_rt->counter_mode_step) * sineIncr));
    color = WS2812FX_color_blend(_seg->colors[0], _seg->colors[1], lum);

  }

    if(IS_REVERSE) {
      WS2812FX_setPixelColor(_seg->start + j, color);
    } else {
      WS2812FX_setPixelColor(_seg->stop - j,  color);
    }
    
    if(j < _seg_len)
    {
      j++;

    }
    else
    {
      j = 0;
    }
  _seg_rt->counter_mode_step = (_seg_rt->counter_mode_step + 1) % 256;
  if(_seg_rt->counter_mode_step == 0) SET_CYCLE;
  return (_seg->speed );
}



/*
 * Tricolor chase mode
 */
uint16_t WS2812FX_mode_tricolor_chase(void) {
  return WS2812FX_tricolor_chase(_seg->colors[0], _seg->colors[1], _seg->colors[2]);
}


/*
 * Alternating white/red/black pixels running.
 */
uint16_t WS2812FX_mode_circus_combustus(void) {
  return WS2812FX_tricolor_chase(RED, WHITE, BLACK);
}


/*
 * Theatre-style crawling lights.
 * Inspired by the Adafruit examples.
 */
uint16_t WS2812FX_mode_theater_chase(void) {
  return WS2812FX_tricolor_chase(_seg->colors[0], _seg->colors[1], _seg->colors[1]);
}

/*
 * Cycles a rainbow over the entire string of LEDs.
 彩虹渐变颜色流水效果
 */
uint16_t WS2812FX_mode_rainbow_cycle(void) {
  for(uint16_t i=0; i < _seg_len; i++) {
	  uint32_t color = WS2812FX_color_wheel(((i * 256 / _seg_len) + _seg_rt->counter_mode_step) & 0xFF);
    WS2812FX_setPixelColor(_seg->stop - i, color);
  }

  _seg_rt->counter_mode_step = (_seg_rt->counter_mode_step + 1) & 0xFF;

  if(_seg_rt->counter_mode_step == 0) SET_CYCLE;

  return (_seg->speed / 256);
}

/*
 * Cycles all LEDs at once through a rainbow.
 整体渐变
 */
uint16_t WS2812FX_mode_rainbow(void) {
  uint32_t color = WS2812FX_color_wheel(_seg_rt->counter_mode_step);
  Adafruit_NeoPixel_fill(color, _seg->start, _seg_len);

  _seg_rt->counter_mode_step = (_seg_rt->counter_mode_step + 1) & 0xFF;

  if(_seg_rt->counter_mode_step == 0)  SET_CYCLE;

  return (_seg->speed / 10);
}

/*
 * Runs a block of pixels back and forth.
 来回运动像素块
 */
uint16_t WS2812FX_mode_scan(void) {
  return WS2812FX_scan(_seg->colors[0], _seg->colors[1], false);
}


/*
 * Runs two blocks of pixels back and forth in opposite directions.
 */
uint16_t WS2812FX_mode_dual_scan(void) {
  return WS2812FX_scan(_seg->colors[0], _seg->colors[1], true);
}


/*
 * Fades the LEDs between two colors
 */
uint16_t WS2812FX_mode_fade(void) {
  int lum = _seg_rt->counter_mode_step;
  if(lum > 255) lum = 511 - lum; // lum = 0 -> 255 -> 0

  uint32_t color = WS2812FX_color_blend(_seg->colors[1], _seg->colors[0], lum);
  Adafruit_NeoPixel_fill(color, _seg->start, _seg_len);

  _seg_rt->counter_mode_step += 4;
  if(_seg_rt->counter_mode_step > 511) {
    _seg_rt->counter_mode_step = 0;
    SET_CYCLE;
  }
  return (_seg->speed / 128);
}

/*
 * Does the "standby-breathing" of well known i-Devices. Fixed Speed.
 * Use mode "fade" if you like to have something similar with a different speed.
 * _seg->colors[1]，和_seg->colors[0]渐变，若_seg->colors[1]为很色就是呼吸功能
 * lum最小值决定两种颜色混合最小比例。典型值15，若为红色呼吸，LED最暗到15
 */
uint16_t WS2812FX_mode_breath(void) {
  int lum = _seg_rt->counter_mode_step;
  if(lum > 255) lum = 511 - lum; // lum = 15 -> 255 -> 15

  // uint16_t delay;
  // if(lum == 15) delay = 970; // 970 pause before each breath
  // else if(lum <=  25) delay = 38; // 19
  // else if(lum <=  50) delay = 36; // 18
  // else if(lum <=  75) delay = 28; // 14
  // else if(lum <= 100) delay = 20; // 10
  // else if(lum <= 125) delay = 14; // 7
  // else if(lum <= 150) delay = 11; // 5
  // else delay = 10; // 4

  uint32_t color =  WS2812FX_color_blend(_seg->colors[1], _seg->colors[0], lum);
  Adafruit_NeoPixel_fill(color, _seg->start, _seg_len);
   if(_seg_rt->counter_mode_step < 35)
  {
    _seg_rt->counter_mode_step += 1;
  }
  else
  _seg_rt->counter_mode_step += 2;    //不能修改+2，否则呼吸有明显的不流畅
  if(_seg_rt->counter_mode_step > (512-5)) {
    _seg_rt->counter_mode_step = 5;
    SET_CYCLE;
  }
return _seg->speed;
}




uint16_t WS2812FX_mutil_breath(void)
{
  u8 i= 0;

  int lum = _seg_rt->counter_mode_step;
  // if(lum < 512){

    if(lum > 255) lum = 511 - lum; // lum = 5 -> 255 -> 5
  // }else {
  //   lum = 5;
  // }
 
//  开始颜色 ，转去的颜色
  // uint32_t color =  WS2812FX_color_blend(_seg->colors[_seg_rt->aux_param ],0, lum);
  uint32_t color =  WS2812FX_color_blend(0,_seg->colors[_seg_rt->aux_param ], lum);
// printf("color=%04x",color);
  Adafruit_NeoPixel_fill(color, _seg->start, _seg_len);


  if(_seg_rt->counter_mode_step < 50)
  {
    _seg_rt->counter_mode_step += 1;
     i = 0;
  }
  else if (_seg_rt->counter_mode_step > (512- 50)){

      _seg_rt->counter_mode_step += 1;
      i = 0;
  }
  else{

    i = 1;
      _seg_rt->counter_mode_step += 1;    //不能修改+2，否则呼吸有明显的不流畅
  }

  // if(_seg_rt->counter_mode_step > (512-5) + 20) {
  if(_seg_rt->counter_mode_step > (512-5)) {
    _seg_rt->counter_mode_step = 5;
    _seg_rt->aux_param++;
    if(_seg_rt->aux_param == 7)_seg_rt->aux_param = 0;
    SET_CYCLE;
  }

  if(i){
      return _seg->speed;
  }else{
    return (_seg->speed * 3 / 2);
  }
  

}


/*
 * Lights every LED in a random color. Changes all LED at the same time
 * to new random colors.
 * 每次以随机颜色变换所有LED
 */
uint16_t WS2812FX_mode_multi_dynamic(void) {
  for(uint16_t i=_seg->start; i <= _seg->stop; i++) {
    WS2812FX_setPixelColor(i, WS2812FX_color_wheel(WS2812FX_random8()));
  }
  SET_CYCLE;
  return _seg->speed;
}



/*
 * Lights all LEDs in one random color up. Then switches them
 * to the next random color.
 * 彩虹跳变
 */
uint16_t WS2812FX_mode_random_color(void) {
  _seg_rt->aux_param = WS2812FX_get_random_wheel_index(_seg_rt->aux_param); // aux_param will store our random color wheel index
  uint32_t color = WS2812FX_color_wheel(_seg_rt->aux_param);
  Adafruit_NeoPixel_fill(color, _seg->start, _seg_len);
  SET_CYCLE;
  return _seg->speed;
}


/*
 * Lights every LED in a random color. Changes one random LED after the other
 * to another random color.
 * 以随机颜色点亮每个LED。依次更改一个随机LED
 * 另一种随机颜色。
 */
uint16_t WS2812FX_mode_single_dynamic(void) {
  if(_seg_rt->counter_mode_call == 0) {
    for(uint16_t i=_seg->start; i <= _seg->stop; i++) {
      WS2812FX_setPixelColor(i, WS2812FX_color_wheel(WS2812FX_random8()));
    }
  }

  WS2812FX_setPixelColor(_seg->start + WS2812FX_random16_lim(_seg_len), WS2812FX_color_wheel(WS2812FX_random8()));
  SET_CYCLE;
  return _seg->speed;
}


/*
 * Turns all LEDs after each other to a random color.
 * Then starts over with another color.
 * 彩虹随机颜色依次流水
 */
uint16_t WS2812FX_mode_color_wipe_random(void) {
  if(_seg_rt->counter_mode_step % _seg_len == 0) { // aux_param will store our random color wheel index
    _seg_rt->aux_param = WS2812FX_get_random_wheel_index(_seg_rt->aux_param);
  }
  uint32_t color = WS2812FX_color_wheel(_seg_rt->aux_param);
  return WS2812FX_color_wipe(color, color, false) * 2;
}


/*
 * Random color introduced alternating from start and end of strip.
 彩虹颜色往返流水，每次到达起点/终点，变换颜色
 */
uint16_t WS2812FX_mode_color_sweep_random(void) {
  if(_seg_rt->counter_mode_step % _seg_len == 0) { // aux_param will store our random color wheel index
    _seg_rt->aux_param = WS2812FX_get_random_wheel_index(_seg_rt->aux_param);
  }
  uint32_t color = WS2812FX_color_wheel(_seg_rt->aux_param);
  return WS2812FX_color_wipe(color, color, true) * 2;
}


/*
 * Lights all LEDs one after another.
 */
uint16_t WS2812FX_mode_color_wipe(void) {
  return WS2812FX_color_wipe(_seg->colors[0], _seg->colors[1], false);
}

uint16_t WS2812FX_mode_color_wipe_inv(void) {
  return WS2812FX_color_wipe(_seg->colors[1], _seg->colors[0], false);
}

uint16_t WS2812FX_mode_color_wipe_rev(void) {
  return WS2812FX_color_wipe(_seg->colors[0], _seg->colors[1], true);
}

uint16_t WS2812FX_mode_color_wipe_rev_inv(void) {
  return WS2812FX_color_wipe(_seg->colors[1], _seg->colors[0], true);
}


/*
 * Normal blinking. 50% on/off time.
 */
uint16_t WS2812FX_mode_blink(void) {
  return WS2812FX_blink(_seg->colors[0], _seg->colors[1], false);
}


/*
 * Classic Blink effect. Cycling through the rainbow.
 彩虹颜色和_seg->colors[1]交替闪烁，彩虹颜色一直在变换
 */
uint16_t WS2812FX_mode_blink_rainbow(void) {
  return WS2812FX_blink(WS2812FX_color_wheel(_seg_rt->counter_mode_call & 0xFF), _seg->colors[1], false);
}


/*
 * Classic Strobe effect.
 两个颜色爆闪，_seg->colors[1]下突然爆闪一下_seg->colors[0]，
_seg->colors[0]时间很多眼睛都没察觉
 */
uint16_t WS2812FX_mode_strobe(void) {
  return WS2812FX_blink(_seg->colors[0], _seg->colors[1], true);
}


/*
 * Classic Strobe effect. Cycling through the rainbow.
 彩虹色爆闪
 */
uint16_t WS2812FX_mode_strobe_rainbow(void) {
  return WS2812FX_blink(WS2812FX_color_wheel(_seg_rt->counter_mode_call & 0xFF), _seg->colors[1], true);
}

/*
 * No blinking. Just plain old static light.
 */
uint16_t WS2812FX_mode_static(void)
{
  /* 渐变到其他静态颜色*/
  // _seg->colors[1] = _seg->colors[0];
  // WS2812FX_fade_out();
  // WS2812FX_fade_out_targetColor(_seg->colors[0]);
   Adafruit_NeoPixel_fill(_seg->colors[0], _seg->start, _seg_len);
  SET_CYCLE;
  return _seg->speed;
}



// 提示效果,白光闪烁
uint16_t white_tips(void)
{
  if(_seg_rt->counter_mode_step)
  {
    Adafruit_NeoPixel_fill( GRAY, _seg->start, _seg_len);
  }
  else
  {
    Adafruit_NeoPixel_fill( 0, _seg->start, _seg_len);
  }
  _seg_rt->counter_mode_step =!_seg_rt->counter_mode_step;
  _seg_rt->aux_param++;
  if(_seg_rt->aux_param > 3)
  {
    extern void read_flash_device_status_init(void);
    read_flash_device_status_init();
    set_fc_effect();
  }
  return (300);
}




uint16_t multi_superposition(void)
{
  int j;
  int w = 0, r = 0, g = 0, c = 0, b = 0;    //必须初始化
  int bf_w = 0, bf_r = 0, bf_g = 0, bf_b = 0;
  static uint8_t ranks_n = 0;   //记录已经堆积的高度
  static uint8_t full_screen = 0;
  static uint8_t before_color = 0;
  w = (_seg->colors[_seg_rt->aux_param3] >> 24) & 0xff;
  r = (_seg->colors[_seg_rt->aux_param3] >> 16) & 0xff;
  g = (_seg->colors[_seg_rt->aux_param3] >>  8) & 0xff;
  b =  _seg->colors[_seg_rt->aux_param3]        & 0xff;

  if(_seg_rt->aux_param == 0)
  {
    bf_w = 0;
    bf_r = 0;
    bf_g = 0;
    bf_b = 0;
    before_color = 0;
  }
  else
  {
    if( _seg_rt->aux_param3 == 0)
    {
      before_color = _seg->c_n - 1;
    }
    else
    {
      before_color = _seg_rt->aux_param3 - 1;
    }

    bf_w = (_seg->colors[before_color] >> 24) & 0xff;
    bf_r = (_seg->colors[before_color] >> 16) & 0xff;
    bf_g = (_seg->colors[before_color] >>  8) & 0xff;
    bf_b =  _seg->colors[before_color]        & 0xff;
  }

   if(IS_REVERSE) //反向
   {

    for(j = 0; j < _seg_len; j++)  //行
    {

      if(j < _seg_rt->counter_mode_step)
        WS2812FX_setPixelColor_rgbw(_seg_len - j-1, bf_r, bf_g, bf_b, bf_w);
      else if(j == _seg_rt->counter_mode_step)
        WS2812FX_setPixelColor_rgbw(_seg_len - j-1, r, g, b, w);
    
    }
    _seg_rt->counter_mode_step++; 
    if(_seg_rt->counter_mode_step >= (_seg_len - ranks_n))
    {
       
      // if(full_screen)
      // {
      //   ranks_n=0;  
      //   full_screen=0;
      //   Adafruit_NeoPixel_fill(BLACK, _seg->start, _seg_len);  //逐个灭

      // }
      ranks_n++;
      if(ranks_n >= _seg_len)
      {
          ranks_n=0;
          _seg_rt->aux_param = 1;
          Adafruit_NeoPixel_fill(_seg->colors[ _seg_rt->aux_param3], _seg->start, _seg_len);  

          // full_screen=1;
          _seg_rt->aux_param3++;
          _seg_rt->aux_param3 %= _seg->c_n;
      }
      _seg_rt->counter_mode_step=0;
    }



   }
   else
   {
      //实现跑动
      for(j = 0; j < _seg_len; j++)  
      {

        if(j < _seg_rt->counter_mode_step)
          WS2812FX_setPixelColor_rgbw(j, bf_r, bf_g, bf_b, bf_w);
        else if(j == _seg_rt->counter_mode_step)
          WS2812FX_setPixelColor_rgbw(j, r, g, b, w);
      
      }
      _seg_rt->counter_mode_step++;
      if( _seg_rt->counter_mode_step >= (_seg_len - ranks_n))
      {
          
        // if(full_screen)
        // {
        //   ranks_n=0;  
        //   full_screen=0;
        //   Adafruit_NeoPixel_fill(BLACK, _seg->start, _seg_len);  //逐个灭

        // }
        ranks_n++;
        if(ranks_n >= _seg_len)  //实现堆积
        {
          ranks_n=0;
            // full_screen=1;
             _seg_rt->aux_param = 1;
          Adafruit_NeoPixel_fill(_seg->colors[ _seg_rt->aux_param3], _seg->start, _seg_len);  

          _seg_rt->aux_param3++;
          _seg_rt->aux_param3 %= _seg->c_n;
        }
        _seg_rt->counter_mode_step = 0;
      }



   }



   return (_seg->speed /4);
}

extern void self_fade_out();
uint16_t double_bounce_flow(void)
{ 


  self_fade_out();
  if(_seg_rt->counter_mode_step < (_seg_len /2 +1)) {

    //出
    WS2812FX_setPixelColor(_seg->stop - _seg_rt->counter_mode_step, _seg->colors[_seg_rt->aux_param]);
    WS2812FX_setPixelColor(_seg->start + _seg_rt->counter_mode_step, _seg->colors[_seg_rt->aux_param]);

  } else {

    //回
   
    uint16_t index = (_seg_len ) - _seg_rt->counter_mode_step - 2;

    WS2812FX_setPixelColor(_seg->stop - index, _seg->colors[_seg_rt->aux_param]);
    WS2812FX_setPixelColor(_seg->start + index, _seg->colors[_seg_rt->aux_param]);


  }

  _seg_rt->counter_mode_step++;

  if(_seg_rt->counter_mode_step >= (uint16_t)((_seg_len ) - 2)) {

    _seg_rt->counter_mode_step = 0;

    _seg_rt->aux_param++;
    _seg_rt->aux_param %= _seg->c_n;



    SET_CYCLE;
  }

  return (_seg->speed/2);

}

uint16_t fu_double_bounce_flow(void)
{


  self_fade_out();
  if(_seg_rt->counter_mode_step < _seg_len /2) {

    //出
    WS2812FX_setPixelColor(_seg->stop - _seg_rt->counter_mode_step, _seg->colors[_seg_rt->aux_param]);
    WS2812FX_setPixelColor(_seg->start + _seg_rt->counter_mode_step, _seg->colors[_seg_rt->aux_param]);

  } else {

    //回
   
    uint16_t index = (_seg_len ) - _seg_rt->counter_mode_step - 2;

    WS2812FX_setPixelColor(_seg->stop - index, _seg->colors[_seg_rt->aux_param]);
    WS2812FX_setPixelColor(_seg->start + index, _seg->colors[_seg_rt->aux_param]);


  }

  _seg_rt->counter_mode_step++;

  if(_seg_rt->counter_mode_step >= (uint16_t)((_seg_len ) - 2)) {

    _seg_rt->counter_mode_step = 0;

    _seg_rt->aux_param++;
    _seg_rt->aux_param %= _seg->c_n;



    SET_CYCLE;
  }

  return (_seg->speed/10);

}

uint16_t longmeteor_signle_c(void)
{

  static uint8_t i = 0;
  uint32_t r1, g1, b1, w1 ;
  const uint8_t rate[20] = {100,100,100,100,100,100,100,100,100,100,
                            100,  100,  0,  0,  0,  0,  0,  0,  0,  0};
  
  int w = (_seg->colors[_seg_rt->aux_param] >> 24) & 0xff;
  int r = (_seg->colors[_seg_rt->aux_param] >> 16) & 0xff;
  int g = (_seg->colors[_seg_rt->aux_param] >>  8) & 0xff;
  int b =  _seg->colors[_seg_rt->aux_param]        & 0xff;
  if(IS_REVERSE) //反向
  {
    WS2812FX_copyPixels(_seg->start,_seg->start+1, _seg_len-1);
  }
  else
  {
    WS2812FX_copyPixels(_seg->start+1,_seg->start, _seg_len-1);
  }
  
    r1 = r * rate[i] / 100;
    g1 = g * rate[i] / 100;
    b1 = b * rate[i] / 100;
    w1 = w * rate[i] / 100;

    if(IS_REVERSE) //反向
    {
      WS2812FX_setPixelColor_rgbw(_seg->stop, r1, g1, b1, w1);
    }
    else
    {
      WS2812FX_setPixelColor_rgbw(_seg->start, r1, g1, b1, w1);
    }
    
    i++;
    i %= 20;
    if(i == 0)
    {
      _seg_rt->aux_param++;
      _seg_rt->aux_param %= _seg->c_n;
    }


  return (_seg->speed /2);


}

uint16_t f_longmeteor_signle_c(void)
{

  static uint8_t i = 0;
  uint32_t r1, g1, b1, w1 ;
  const uint8_t rate[50] = {100,100,100,100,100,100,100,100,100,100,
                            100,100,100,100,100,100,100,100,100,100,
                            100,100,100,100,100,100,100,100,100,100,
                            100,100,100,100,100,100,100,100,100,100,
                            0,  0,  0,  0,  0,  0,  0,  0,  0,  0};
  
  int w = (_seg->colors[_seg_rt->aux_param] >> 24) & 0xff;
  int r = (_seg->colors[_seg_rt->aux_param] >> 16) & 0xff;
  int g = (_seg->colors[_seg_rt->aux_param] >>  8) & 0xff;
  int b =  _seg->colors[_seg_rt->aux_param]        & 0xff;
  if(IS_REVERSE) //反向
  {
    WS2812FX_copyPixels(_seg->start,_seg->start+1, _seg_len-1);
  }
  else
  {
    WS2812FX_copyPixels(_seg->start+1,_seg->start, _seg_len-1);
  }
  
    r1 = r * rate[i] / 100;
    g1 = g * rate[i] / 100;
    b1 = b * rate[i] / 100;
    w1 = w * rate[i] / 100;

    if(IS_REVERSE) //反向
    {
      WS2812FX_setPixelColor_rgbw(_seg->stop, r1, g1, b1, w1);
    }
    else
    {
      WS2812FX_setPixelColor_rgbw(_seg->start, r1, g1, b1, w1);
    }
    
    i++;
    i %= 50;
    if(i == 0)
    {
      _seg_rt->aux_param++;
      _seg_rt->aux_param %= _seg->c_n;
    }


  return (_seg->speed /2);


}

uint8_t flash_flag = 1;
uint8_t flash_flag_1 = 1;
uint16_t longmeteor_signle_blink(void)
{

    
    static uint8_t buff[USER_WS2812_SEGMENT_1_LEN * 4];  // 24个点，每个点4个字节，需要根据实际情况调整，大小=灯数量x（RGB：3 或者 RGBW：4）

    static uint8_t i = 0;
    uint32_t r1, g1, b1, w1;
    const uint8_t rate[20] = {100,100,100,100,100,100,100,100,100,100,
                            100,  100,  0,  0,  0,  0,  0,  0,  0,  0};

    int w = (_seg->colors[_seg_rt->aux_param] >> 24) & 0xff;
    int r = (_seg->colors[_seg_rt->aux_param] >> 16) & 0xff;
    int g = (_seg->colors[_seg_rt->aux_param] >> 8) & 0xff;
    int b = _seg->colors[_seg_rt->aux_param] & 0xff;

    if (flash_flag == 1)
    {   // 亮时照常推进1格
      if (IS_REVERSE) //反向
      {
        WS2812FX_copyPixels(_seg->start, _seg->start + 1, _seg_len - 1);
      }
      else
      {
        WS2812FX_copyPixels(_seg->start + 1, _seg->start, _seg_len - 1);
      }
    }
    else
    {   // 灭时从buff中取数据再推进一格
      uint8_t *pixels1 = Adafruit_NeoPixel_getPixels(); //获取
      uint8_t bytesPerPixel = WS2812FX_getNumBytesPerPixel(); // 3=RGB, 4=RGBW
 

      if (IS_REVERSE) //反向
      {
        memcpy(pixels1 + (_seg->start * bytesPerPixel), &buff[1], (_seg_len - 1) * bytesPerPixel);
        // memcpy(pixels1 + (_seg->start * 3), &buff[1], (_seg_len - 1) * 3);
      }
      else
      {
        memcpy(pixels1 + ((_seg->start + 1) * bytesPerPixel), &buff[0], (_seg_len - 1) * bytesPerPixel);
        // memcpy(pixels1 + ((_seg->start + 1) * 3), &buff[0], (_seg_len - 1) * 3);
      }
    }

    r1 = r * rate[i] / 100;
    g1 = g * rate[i] / 100;
    b1 = b * rate[i] / 100;
    w1 = w * rate[i] / 100;

    if (IS_REVERSE) //反向
    {
      WS2812FX_setPixelColor_rgbw(_seg->stop, r1, g1, b1, w1);
    }
    else
    {
      WS2812FX_setPixelColor_rgbw(_seg->start, r1, g1, b1, w1);
    }

    {   // 复制计算后的数据到buff
      memcpy(buff, Adafruit_NeoPixel_getPixels() + (_seg->start * 3), _seg_len * WS2812FX_getNumBytesPerPixel());
      // memcpy(buff, Adafruit_NeoPixel_getPixels() + (_seg->start * 3), _seg_len *3);
    }

    // 计算亮灭时机87
    static uint16_t flash_count = 0;
    flash_count++;
    if (flash_count == 1)  // 经过30次调用后，灭
    {
      flash_flag = 0;
    }
    else if (flash_count == 2) // 经过35次调用后，亮
    {
      flash_flag = 1;
      flash_count = 0;
    }

    if (flash_flag == 0)
    {
      Adafruit_NeoPixel_fill(BLACK, _seg->start, _seg_len);
    }


    i++;
    i %= 20;
    if (i == 0)
    {
        _seg_rt->aux_param++;
        _seg_rt->aux_param %= _seg->c_n;
    }
    return (_seg->speed / 2);
}


uint16_t f_longmeteor_signle_blink(void)
{

    
    static uint8_t buff[USER_WS2812_SEGMENT_2_LEN * 4];  // 24个点，每个点4个字节，需要根据实际情况调整，大小=灯数量x（RGB：3 或者 RGBW：4）

    static uint8_t i = 0;
    uint32_t r1, g1, b1, w1;
    const uint8_t rate[50] = { 100,100,100,100,100,100,100,100,100,100,
                              100,100,100,100,100,100,100,100,100,100,
                              100,100,100,100,100,100,100,100,100,100,
                              100,100,100,100,100,100,100,100,100,100,
                              0,  0,  0,  0,  0,  0,  0,  0,  0,  0 };

    int w = (_seg->colors[_seg_rt->aux_param] >> 24) & 0xff;
    int r = (_seg->colors[_seg_rt->aux_param] >> 16) & 0xff;
    int g = (_seg->colors[_seg_rt->aux_param] >> 8) & 0xff;
    int b = _seg->colors[_seg_rt->aux_param] & 0xff;

    if (flash_flag_1 == 1)
    {   // 亮时照常推进1格
      if (IS_REVERSE) //反向
      {
        WS2812FX_copyPixels(_seg->start, _seg->start + 1, _seg_len - 1);
      }
      else
      {
        WS2812FX_copyPixels(_seg->start + 1, _seg->start, _seg_len - 1);
      }
    }
    else
    {   // 灭时从buff中取数据再推进一格
      uint8_t *pixels1 = Adafruit_NeoPixel_getPixels(); //获取
      uint8_t bytesPerPixel = WS2812FX_getNumBytesPerPixel(); // 3=RGB, 4=RGBW

      if (IS_REVERSE) //反向
      {
        memcpy(pixels1 + (_seg->start * bytesPerPixel), &buff[1], (_seg_len - 1) * bytesPerPixel);
        // memcpy(pixels + (_seg->start * 3), &buff[1], (_seg_len - 1) * 3);
      }
      else
      {
        memcpy(pixels1 + ((_seg->start + 1) * bytesPerPixel), &buff[0], (_seg_len - 1) * bytesPerPixel);
        // memcpy(pixels + ((_seg->start + 1) * 3), &buff[0], (_seg_len - 1) * 3);
      }
    }

    r1 = r * rate[i] / 100;
    g1 = g * rate[i] / 100;
    b1 = b * rate[i] / 100;
    w1 = w * rate[i] / 100;

    if (IS_REVERSE) //反向
    {
      WS2812FX_setPixelColor_rgbw(_seg->stop, r1, g1, b1, w1);
    }
    else
    {
      WS2812FX_setPixelColor_rgbw(_seg->start, r1, g1, b1, w1);
    }

    {   // 复制计算后的数据到buff
      memcpy(buff, Adafruit_NeoPixel_getPixels() + (_seg->start * 3), _seg_len * WS2812FX_getNumBytesPerPixel());
      // memcpy(buff, Adafruit_NeoPixel_getPixels() + (_seg->start * 3), _seg_len * 3);
    }

    // 计算亮灭时机
    static uint16_t flash_count = 0;
    flash_count++;
    if (flash_count == 1)  // 经过30次调用后，灭
    {
      flash_flag_1 = 0;
    }
    else if (flash_count == 2) // 经过35次调用后，亮
    {
      flash_flag_1 = 1;
      flash_count = 0;
    }

    if (flash_flag_1 == 0)
    {
      Adafruit_NeoPixel_fill(BLACK, _seg->start, _seg_len);
    }


    i++;
    i %= 50;
    if (i == 0)
    {
        _seg_rt->aux_param++;
        _seg_rt->aux_param %= _seg->c_n;
    }
    return (_seg->speed / 2);
}
