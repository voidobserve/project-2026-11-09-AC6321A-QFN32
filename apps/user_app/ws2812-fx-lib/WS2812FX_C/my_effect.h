#ifndef my_effect_h
#define my_effect_h

#include <stdint.h>

typedef enum
{
    TYPE_TUYA,  //涂鸦效果
    TYPE_LOCAL, //本地效果
};




void effect_plus(void);
void effect_sub(void);
uint8_t get_effect_index(void);
void set_effect_index(uint8_t m);
void switch_effect(uint8_t effect);
void my_effect_init(void);

// 设置效果用涂鸦的
void set_effect_tuya(void);

// 设置效果用本地效果
void set_effect_local(void);

// 获取当前效果属于本地/涂鸦
uint8_t get_effect_type(void);
void set_effect_type(uint8_t t);

#endif

