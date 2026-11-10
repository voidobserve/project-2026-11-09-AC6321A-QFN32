#ifndef led_strip_voice_h
#define led_strip_voice_h

#include "led_strand_effect.h"

/* ============================================================================
 * 音乐频谱处理相关函数声明
 * ============================================================================ */

// 基础函数
int mic_adc_init(void);
u16 check_mic_adc(void);
u8 get_sound_result(void);
u8 get_meteor_result(void);
u8 sound_handle(void);

// 新增高级频谱算法接口函数
#if USE_ADVANCED_SPECTRUM_ALGORITHM || defined(__cplusplus)
void spectrum_set_auto_gain(u8 enable);
void spectrum_set_noise_gate(u16 threshold);
void spectrum_get_status(u16 *current, u16 *filtered, u16 *display, u16 *peak);
void spectrum_reset(void);
#endif

// Audio Reactive Library 接口函数
#if USE_AUDIO_REACTIVE_LIB || defined(__cplusplus)
void ar_lib_set_sensitivity(u8 sensitivity);
void ar_lib_set_noise_gate(u8 threshold);
int ar_lib_get_status(char *stats_buffer, int buffer_size);
void ar_lib_reset(void);
void ar_lib_set_debug(u8 enabled);
#endif

#endif


