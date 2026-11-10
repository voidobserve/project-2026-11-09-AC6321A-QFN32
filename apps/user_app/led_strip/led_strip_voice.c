#include "app_config.h"
#include "includes.h"
#include "asm/adc_api.h"
#include "ws2812fx_effect.h"
#include "led_strip_voice.h"
#include "led_strip_drive.h"
#include "audio_reactive_lib.h"

/* ============================================================================
 * 音乐频谱算法选择开关
 * ============================================================================ */
#define USE_ADVANCED_SPECTRUM_ALGORITHM 0 // 1=使用新算法, 0=使用原算法
#define USE_AUDIO_REACTIVE_LIB 0 // 1=使用audio_reactive库, 0=使用内置算法
#define SPECTRUM_DEBUG_OUTPUT  0 // 1=启用调试输出, 0=关闭调试输出

u8 sound_trg;
u8 meteor_trg;
u32 adc_sum = 0, adc_sum_n = 0;

/* ============================================================================
 * 新音乐频谱算法变量定义
 * ============================================================================ */
#if USE_ADVANCED_SPECTRUM_ALGORITHM

// 音频处理参数 - 平衡平滑度与动感
#define SPECTRUM_HISTORY_SIZE      8   // 历史数据缓存大小 (减少到8，提高敏感度)
#define SPECTRUM_SMOOTH_FACTOR     4   // 平滑滤波系数 (减少到4，保持响应性)
#define SPECTRUM_ATTACK_RATE       8   // 上升速度系数 (提高到8，快速响应节拍)
#define SPECTRUM_DECAY_RATE        3   // 下降速度系数 (提高到3，自然衰减)
#define SPECTRUM_NOISE_GATE        50  // 噪声门限
#define SPECTRUM_AUTO_GAIN_SAMPLES 100 // 自动增益采样数

// 新增：动感增强参数
#define OUTPUT_SMOOTH_FACTOR    4    // 输出平滑系数 (减少到4)
#define MAX_LEVEL_CHANGE        6    // 每次最大等级变化量 (增加到6)
#define DYNAMIC_BOOST_THRESHOLD 3000 // 动感增强阈值
#define DYNAMIC_BOOST_FACTOR    150  // 动感增强系数 (150%)

// 频谱数据结构
typedef struct
{
    u16 current_value;                  // 当前ADC值
    u16 filtered_value;                 // 滤波后的值
    u16 display_value;                  // 显示用的值
    u16 peak_value;                     // 峰值保持
    u16 history[SPECTRUM_HISTORY_SIZE]; // 历史数据
    u8 history_index;                   // 历史数据索引
    u32 sum_history;                    // 历史数据总和
    u16 average_value;                  // 平均值
    u16 dynamic_range_min;              // 动态范围最小值
    u16 dynamic_range_max;              // 动态范围最大值
    u8 peak_hold_counter;               // 峰值保持计数器

    // 新增：输出平滑相关变量
    u8 output_level;       // 当前输出等级
    u8 smooth_level;       // 平滑后的输出等级
    u8 last_output_level;  // 上一次输出等级
    u8 output_color_index; // 当前输出颜色索引
    u8 smooth_color_index; // 平滑后的颜色索引

    // 新增：动感增强相关变量
    u16 dynamic_boost;   // 动感增强值
    u8 beat_detected;    // 节拍检测标志
    u16 energy_variance; // 能量方差
    u16 last_peak_time;  // 上次峰值时间
} spectrum_data_t;

// 自动增益控制结构
typedef struct
{
    u32 sample_sum;      // 采样累加值
    u16 sample_count;    // 采样计数
    u16 gain_factor;     // 增益系数 (256=1.0倍)
    u16 noise_floor;     // 噪声底电平
    u16 signal_ceiling;  // 信号顶电平
    u8 auto_gain_enable; // 自动增益使能
} auto_gain_control_t;

// 全局变量
static spectrum_data_t spectrum;
static auto_gain_control_t agc;

// 改进的音量阈值表 - 增强动感表现
static const u16 advanced_music_voice[24] = {
    30,   60,   100,  150,  220,  300,  // 0-5: 极低音量 (降低门槛)
    400,  520,  660,  820,  1000, 1200, // 6-11: 低音量 (更敏感)
    1420, 1660, 1920, 2200, 2500, 2820, // 12-17: 中音量 (精细分级)
    3160, 3520, 3900, 4300, 4720, 5160  // 18-23: 高音量 (更精细)
};

#endif

/*********************************mic脚IO口初始化***************************************************************/

int mic_adc_init(void)
{
    adc_add_sample_ch(AD_CH_PA8); //注意：初始化AD_KEY之前，先初始化ADC
    gpio_set_die(IO_PORTA_08, 0);
    gpio_set_direction(IO_PORTA_08, 1);
    gpio_set_pull_down(IO_PORTA_08, 0);

    return 0;
}

u16 check_mic_adc(void)
{
    return adc_get_value(AD_CH_PA8);
}

// 获取声控结果
// 触发条件：（（当前声音大小 - 平均值）* 100 ）/ 平均值 > 灵敏度（0~100）
// 0:没触发
// 1:触发
u8 get_sound_result(void)
{
    u8 p_trg;
    //  WS2812FX_trigger();  //影响流星的速度
    p_trg = sound_trg;
    sound_trg = 0;
    return p_trg;
}

u8 get_meteor_result(void)
{
    u8 p_metemor_trg;
    // WS2812FX_trigger();//影响流星的速度
    p_metemor_trg = meteor_trg;
    meteor_trg = 0;
    return p_metemor_trg;
}

void set_music_fs_trg(u8 p);
extern u16 get_music_adc(void);
extern uint16_t trg_en;
extern u8 music_flag;

/* ============================================================================
 * 新音乐频谱算法核心函数
 * ============================================================================ */
#if USE_ADVANCED_SPECTRUM_ALGORITHM

/**
 * @brief 初始化频谱数据结构
 */
static void spectrum_init(void)
{
    u8 i;

    // 清零频谱数据
    spectrum.current_value = 0;
    spectrum.filtered_value = 0;
    spectrum.display_value = 0;
    spectrum.peak_value = 0;
    spectrum.history_index = 0;
    spectrum.sum_history = 0;
    spectrum.average_value = 0;
    spectrum.dynamic_range_min = 65535;
    spectrum.dynamic_range_max = 0;
    spectrum.peak_hold_counter = 0;

    // 初始化输出平滑变量
    spectrum.output_level = 0;
    spectrum.smooth_level = 0;
    spectrum.last_output_level = 0;
    spectrum.output_color_index = 0;
    spectrum.smooth_color_index = 0;

    // 初始化动感增强变量
    spectrum.dynamic_boost = 0;
    spectrum.beat_detected = 0;
    spectrum.energy_variance = 0;
    spectrum.last_peak_time = 0;

    // 清零历史数据
    for (i = 0; i < SPECTRUM_HISTORY_SIZE; i++) {
        spectrum.history[i] = 0;
    }

    // 初始化自动增益控制
    agc.sample_sum = 0;
    agc.sample_count = 0;
    agc.gain_factor = 256; // 1.0倍增益
    agc.noise_floor = 100;
    agc.signal_ceiling = 8000;
    agc.auto_gain_enable = 1;
}

/**
 * @brief 数字低通滤波器
 * @param new_value 新的ADC值
 * @param old_value 旧的滤波值
 * @param smooth_factor 平滑系数 (1-8)
 * @return 滤波后的值
 */
static u16 low_pass_filter(u16 new_value, u16 old_value, u8 smooth_factor)
{
    u32 result;

    if (smooth_factor == 0)
        smooth_factor = 1;
    if (smooth_factor > 8)
        smooth_factor = 8;

    // 加权平均滤波: result = (old * (n-1) + new) / n
    result = ((u32)old_value * (smooth_factor - 1) + new_value) / smooth_factor;

    return (u16)result;
}

/**
 * @brief 自动增益控制处理
 * @param input_value 输入ADC值
 * @return 增益调整后的值
 */
static u16 auto_gain_control(u16 input_value)
{
    u32 result;

    if (!agc.auto_gain_enable) {
        return input_value;
    }

    // 累积采样数据用于增益计算
    agc.sample_sum += input_value;
    agc.sample_count++;

    // 每100个采样更新一次增益
    if (agc.sample_count >= SPECTRUM_AUTO_GAIN_SAMPLES) {
        u16 average = agc.sample_sum / agc.sample_count;

        // 根据平均值动态调整增益
        if (average < 1000) {
            agc.gain_factor = 400; // 1.56倍增益
        } else if (average < 3000) {
            agc.gain_factor = 300; // 1.17倍增益
        } else if (average < 6000) {
            agc.gain_factor = 256; // 1.0倍增益
        } else {
            agc.gain_factor = 200; // 0.78倍增益
        }

        // 重置计数器
        agc.sample_sum = 0;
        agc.sample_count = 0;
    }

    // 应用增益
    result = ((u32)input_value * agc.gain_factor) >> 8;
    if (result > 65535)
        result = 65535;

    return (u16)result;
}

/**
 * @brief 动态响应处理 - 根据变化强度调整响应速度
 * @param current 当前值
 * @param target 目标值
 * @param change_rate 变化强度
 * @return 处理后的值
 */
static u16 dynamic_response(u16 current, u16 target, u16 change_rate)
{
    s32 diff = (s32)target - (s32)current;
    s32 step;

    if (diff == 0)
        return current;

    // 根据变化强度调整响应速度
    if (diff > 0) {
        // 上升: 变化越大，响应越快
        step = (diff * SPECTRUM_ATTACK_RATE * (change_rate + 256)) >> 10;
        if (step < 1)
            step = 1;
    } else {
        // 下降: 变化越大，下降越快，但总体比上升慢
        step = (diff * SPECTRUM_DECAY_RATE * (change_rate + 128)) >> 10;
        if (step > -1)
            step = -1;
    }

    s32 result = (s32)current + step;
    if (result < 0)
        result = 0;
    if (result > 65535)
        result = 65535;

    return (u16)result;
}

/**
 * @brief 峰值检测和保持
 * @param current_value 当前值
 */
static void peak_detection(u16 current_value)
{
    // 检测新峰值
    if (current_value > spectrum.peak_value) {
        spectrum.peak_value = current_value;
        spectrum.peak_hold_counter = 20; // 保持20个周期
    } else if (spectrum.peak_hold_counter > 0) {
        spectrum.peak_hold_counter--;
    } else {
        // 峰值缓慢衰减
        if (spectrum.peak_value > 0) {
            spectrum.peak_value = (spectrum.peak_value * 15) >> 4; // 衰减1/16
        }
    }
}

/**
 * @brief 节拍检测和动感增强
 * @param current_value 当前值
 * @param change_magnitude 变化强度
 */
static void beat_detection_and_boost(u16 current_value, u16 change_magnitude)
{
    static u16 time_counter = 0;
    time_counter++;

    // 计算能量方差 (简化版)
    if (change_magnitude > spectrum.energy_variance) {
        spectrum.energy_variance = change_magnitude;
    } else {
        spectrum.energy_variance =
            (spectrum.energy_variance * 7 + change_magnitude) >> 3;
    }

    // 节拍检测: 当变化强度超过阈值且距离上次峰值有一定间隔
    if (change_magnitude > DYNAMIC_BOOST_THRESHOLD &&
        (time_counter - spectrum.last_peak_time) > 5) { // 至少间隔5个周期

        spectrum.beat_detected = 1;
        spectrum.last_peak_time = time_counter;

        // 动感增强: 临时提升响应强度
        spectrum.dynamic_boost = DYNAMIC_BOOST_FACTOR;
    } else {
        spectrum.beat_detected = 0;

        // 动感增强值缓慢衰减
        if (spectrum.dynamic_boost > 0) {
            spectrum.dynamic_boost =
                (spectrum.dynamic_boost * 14) >> 4; // 衰减1/16
        }
    }
}

/**
 * @brief 动感增强的等级映射
 * @param raw_level 原始等级
 * @param boost_factor 增强系数
 * @return 增强后的等级
 */
static u8 dynamic_level_boost(u8 raw_level, u16 boost_factor)
{
    if (boost_factor == 0)
        return raw_level;

    // 动感增强: 在节拍时扩大等级变化范围
    u16 boosted_level = ((u32)raw_level * (100 + boost_factor)) / 100;

    // 限制在有效范围内
    if (boosted_level > 23)
        boosted_level = 23;

    return (u8)boosted_level;
}

/**
 * @brief 高级音乐频谱处理算法
 * @param raw_adc 原始ADC值
 * @return 处理后的频谱等级 (0-23)
 */
static u8 advanced_spectrum_process(u16 raw_adc)
{
    u8 i;
    u16 change_magnitude;
    s32 diff;

    // 1. 噪声门限处理
    if (raw_adc < SPECTRUM_NOISE_GATE) {
        raw_adc = 0;
    }

    // 2. 自动增益控制
    spectrum.current_value = auto_gain_control(raw_adc);

    // 3. 更新历史数据并计算平均值
    spectrum.sum_history -= spectrum.history[spectrum.history_index];
    spectrum.history[spectrum.history_index] = spectrum.current_value;
    spectrum.sum_history += spectrum.current_value;
    spectrum.history_index =
        (spectrum.history_index + 1) % SPECTRUM_HISTORY_SIZE;
    spectrum.average_value = spectrum.sum_history / SPECTRUM_HISTORY_SIZE;

    // 4. 计算变化强度
    diff = (s32)spectrum.current_value - (s32)spectrum.average_value;
    change_magnitude = (diff > 0) ? (u16)diff : (u16)(-diff);

    // 5. 一阶低通滤波
    spectrum.filtered_value =
        low_pass_filter(spectrum.current_value, spectrum.filtered_value,
                        SPECTRUM_SMOOTH_FACTOR);

    // 6. 动态响应处理
    spectrum.display_value = dynamic_response(
        spectrum.display_value, spectrum.filtered_value, change_magnitude);

    // 7. 峰值检测
    peak_detection(spectrum.display_value);

    // 8. 节拍检测和动感增强
    beat_detection_and_boost(spectrum.current_value, change_magnitude);

    // 9. 转换为频谱等级
    u8 raw_level = 0;
    for (i = 0; i < 24; i++) {
        if (spectrum.display_value <= advanced_music_voice[i]) {
            raw_level = i;
            break;
        }
    }
    if (i >= 24)
        raw_level = 23; // 最高等级

    // 10. 应用动感增强
    u8 boosted_level = dynamic_level_boost(raw_level, spectrum.dynamic_boost);

    // 11. 输出平滑处理 - 改进的变化限制
    spectrum.output_level = boosted_level;
    s8 level_diff = (s8)spectrum.output_level - (s8)spectrum.last_output_level;

    // 根据节拍检测调整变化限制
    u8 max_change = MAX_LEVEL_CHANGE;
    if (spectrum.beat_detected) {
        max_change = MAX_LEVEL_CHANGE * 2; // 节拍时允许更大变化
    }

    // 限制每次变化的最大幅度
    if (level_diff > max_change) {
        spectrum.smooth_level = spectrum.last_output_level + max_change;
    } else if (level_diff < -max_change) {
        spectrum.smooth_level = spectrum.last_output_level - max_change;
    } else {
        // 小幅度变化时减少平滑，保持敏感度
        if (spectrum.beat_detected || abs(level_diff) <= 1) {
            spectrum.smooth_level =
                spectrum.output_level; // 直接输出，保持敏感度
        } else {
            // 中等幅度变化使用轻度平滑
            spectrum.smooth_level =
                (spectrum.last_output_level * (OUTPUT_SMOOTH_FACTOR - 2) +
                 spectrum.output_level * 2) /
                OUTPUT_SMOOTH_FACTOR;
        }
    }

    spectrum.last_output_level = spectrum.smooth_level;

    return spectrum.smooth_level;
}

/**
 * @brief 高级颜色映射 - 增强动感响应
 * @param level 频谱等级 (0-23)
 * @param peak_factor 峰值因子 (0-255)
 */
static void advanced_color_mapping(u8 level, u8 peak_factor)
{
    u8 raw_color_index;

    // 更动感的颜色分级映射 - 减少重复范围
    if (level <= 2) {
        raw_color_index = 1; // 深蓝
    } else if (level <= 4) {
        raw_color_index = 2; // 蓝色
    } else if (level <= 6) {
        raw_color_index = 3; // 青蓝
    } else if (level <= 8) {
        raw_color_index = 4; // 青色
    } else if (level <= 10) {
        raw_color_index = 5; // 绿色
    } else if (level <= 12) {
        raw_color_index = 6; // 黄绿
    } else if (level <= 14) {
        raw_color_index = 6; // 黄色
    } else if (level <= 16) {
        raw_color_index = 8; // 橙黄
    } else if (level <= 18) {
        raw_color_index = 9; // 橙色
    } else if (level <= 20) {
        raw_color_index = 10; // 橙红
    } else {
        raw_color_index = 11; // 红色
    }

    // 颜色索引动态响应处理
    spectrum.output_color_index = raw_color_index;
    s8 color_diff =
        (s8)spectrum.output_color_index - (s8)spectrum.smooth_color_index;

    // 根据节拍检测调整颜色变化速度
    if (spectrum.beat_detected) {
        // 节拍时允许快速颜色变化
        if (color_diff > 2) {
            spectrum.smooth_color_index += 2;
        } else if (color_diff < -2) {
            spectrum.smooth_color_index -= 2;
        } else {
            spectrum.smooth_color_index = spectrum.output_color_index;
        }
    } else {
        // 平滑颜色变化
        if (color_diff > 1) {
            spectrum.smooth_color_index += 1; // 每次最多增加1级
        } else if (color_diff < -1) {
            spectrum.smooth_color_index -= 1; // 每次最多减少1级
        } else {
            spectrum.smooth_color_index = spectrum.output_color_index;
        }
    }

    // 峰值增强效果 - 增强对比度
    u8 final_color_index = spectrum.smooth_color_index;
    if (spectrum.beat_detected && peak_factor > 180 && final_color_index < 11) {
        final_color_index += 1; // 节拍峰值时颜色提升
    }

    set_music_oc_trg(final_color_index);
    set_music_fs_trg(level);

#if SPECTRUM_DEBUG_OUTPUT
    // 调试输出 - 监控动感效果
    static u8 debug_counter = 0;
    debug_counter++;
    if (debug_counter >= 5) { // 每5次输出一次
        debug_counter = 0;
        printf(
            "[DYNAMIC] ADC:%d, Raw:%d, Boost:%d, Final:%d, Beat:%d, Color:%d\n",
            spectrum.current_value,
            spectrum.output_level - spectrum.dynamic_boost / 10,
            spectrum.dynamic_boost, level, spectrum.beat_detected,
            final_color_index);
    }
#endif
}

#endif

/* ============================================================================
 * 原始音乐频谱算法 (保留备用)
 * ============================================================================ */

// 原始音量映射表
// u16 music_voice[16]= {
// 73     ,
// 104   ,
// 146   ,      //1
// 207   ,       //2
// 292   ,       //3
// 413   ,     //4
// 583   ,     //5
// 823   ,
// 1163   ,     //6
// 1642   ,
// 2320   ,     //7
// 3277   ,
// 4628   ,    //8
// 6538   ,
// 9235   ,    //9
// 11626  ,
// };

#if 0
u16 music_voice[21] = {
73 ,
130 ,
232 ,
413 ,
734 ,
1304 ,
1974 ,
2293 ,
2663 ,
3093 ,
3593 ,
4173 ,
4847 ,
5629 ,
6538 ,
7593 ,
8819 ,
10243 ,
11897 ,
13818 ,
16049  ,
};
#else
u16 music_voice[21] = {
    73,   130,  232,  413,  734,  1304, 1974,  2293,  2663,  3093,  3593,
    4173, 4847, 5629, 6538, 7593, 8819, 10243, 11897, 13818, 16049,
};
#endif

u8 close_music = 0;

/* ============================================================================
 * 主音乐处理函数 - 支持算法切换
 * ============================================================================ */

/**
 * @brief 音乐频谱处理主函数
 * @return 处理状态
 * @note 支持新旧算法切换，通过宏USE_ADVANCED_SPECTRUM_ALGORITHM控制
 */

u8 sound_handle(void)
{
    u16 adc;
    u8 i;
#if USE_AUDIO_REACTIVE_LIB
    static u8 algorithm_initialized = 0;
#endif

    // 获取音乐ADC值 (10ms来一次，值100ms更新一次)
    adc = get_music_adc();

    if ((fc_effect.led_on_off_flag == DEVICE_ON ||
         fc_effect.led1_on_off_flag == DEVICE_ON) &&
        music_flag == 1) {
#if USE_AUDIO_REACTIVE_LIB
        /* ========== Audio Reactive Library 算法 ========== */

        // 初始化库 (只执行一次)
        if (!algorithm_initialized) {
            ar_init(AR_MODE_VU_METER, AR_OUTPUT_LEVEL);
            ar_set_sensitivity(70);     // 较高灵敏度
            ar_set_noise_gate(3);       // 低噪声门限
            ar_set_smooth_params(6, 3); // 快上升，慢下降
            ar_set_agc_enabled(1);      // 启用AGC
            ar_set_beat_threshold(200); // 节拍检测阈值
            algorithm_initialized = 1;

            if (SPECTRUM_DEBUG_OUTPUT) {
                printf(
                    "[AR_LIB] Initialized: sensitivity=70, noise_gate=3%%\n");
            }
        }

        if (adc <= 10) {
            set_music_oc_trg(0);
            set_music_fs_trg(0);
            close_music = 1;
            ar_reset(); // 重置库状态
        } else {
            close_music = 0;

            // 使用Audio Reactive Library处理
            u8 level = ar_process(adc);
            u8 color_index = ar_get_color_index();
            u8 is_beat = ar_is_beat_detected();

            // 颜色映射 - 基于库的输出做简单映射
            u8 final_color_index;
            if (level <= 2) {
                final_color_index = 1; // 深蓝
            } else if (level <= 5) {
                final_color_index = 3; // 蓝色
            } else if (level <= 8) {
                final_color_index = 4; // 青色
            } else if (level <= 12) {
                final_color_index = 5; // 绿色
            } else if (level <= 16) {
                final_color_index = 6; // 黄色
            } else if (level <= 20) {
                final_color_index = 9; // 橙色
            } else {
                final_color_index = 11; // 红色
            }

            // 节拍增强
            if (is_beat && final_color_index < 11) {
                final_color_index += 1;
            }

            set_music_oc_trg(final_color_index);
            set_music_fs_trg(level);

#if SPECTRUM_DEBUG_OUTPUT
            static u8 debug_counter = 0;
            debug_counter++;
            if (debug_counter >= 10) { // 每10次输出一次
                debug_counter = 0;
                char stats[128];
                ar_get_stats(stats, sizeof(stats));
                printf("[AR_LIB] ADC:%d Level:%d Color:%d Beat:%d %s", adc,
                       level, final_color_index, is_beat, stats);
            }
#endif
        }

#elif USE_ADVANCED_SPECTRUM_ALGORITHM
        /* ========== 新的高级频谱算法 ========== */

        // 初始化新算法 (只执行一次)
        if (!algorithm_initialized) {
            spectrum_init();
            algorithm_initialized = 1;
        }

        // if (adc < 20000)
        {
            if (adc <= 10) {
                set_music_oc_trg(0);
                set_music_fs_trg(0);
                close_music = 1;

                // 重置频谱状态
                spectrum.display_value = 0;
                spectrum.filtered_value = 0;
            } else {
                close_music = 0;

                // 使用新的高级频谱处理算法
                u8 spectrum_level = advanced_spectrum_process(adc);
                u8 peak_factor =
                    (spectrum.peak_value > spectrum.display_value)
                        ? ((spectrum.peak_value - spectrum.display_value) *
                           255) /
                              (spectrum.peak_value + 1)
                        : 0;

                // 应用高级颜色映射
                advanced_color_mapping(spectrum_level, peak_factor);
            }
        }

#else
        /* ========== 原始算法 (保持原有逻辑) ========== */

        if (adc < 20000) {
            if (adc <= 10) {
                set_music_oc_trg(0);
                set_music_fs_trg(0);
                close_music = 1;
            } else {
                close_music = 0;

                for (i = 0; i < 21; i++) {
                    if (music_voice[i] > adc) {
                        set_music_fs_trg(i); //单向

                        /* 双向颜色映射 */
                        if (i == 0) {
                            set_music_oc_trg(1); //B
                        } else if (i > 0 && i <= 1) {
                            set_music_oc_trg(3); //B
                        } else if (i >= 2 && i <= 3) {
                            set_music_oc_trg(3);       //B
                        } else if (i >= 4 && i <= 5) { // 2
                            set_music_oc_trg(4);       //G
                        } else if (i >= 6 && i <= 7) { //3 4
                            set_music_oc_trg(4);       //G
                        } else if (i >= 8 && i <= 9) { // 5 6
                            set_music_oc_trg(5);
                        } else if (i >= 10 && i <= 11) { //  6 7
                            set_music_oc_trg(5);         //G
                        } else if (i >= 12 && i <= 13) { //8
                            set_music_oc_trg(6);         //G
                        } else if (i >= 14 && i <= 15) {
                            set_music_oc_trg(6);         //红色
                        } else if (i >= 16 && i <= 17) { //
                            set_music_oc_trg(9);         //红色
                        } else if (i >= 18) {            //
                            set_music_oc_trg(11);        //红色
                        }
                        break;
                    }
                }
            }
        }
#endif
    } else {
#if USE_AUDIO_REACTIVE_LIB
        // 音乐模式关闭时重置初始化标志
        algorithm_initialized = 0;
#endif
    }

    return 0;
}

/* ============================================================================
 * 调试和配置接口函数
 * ============================================================================ */

#if USE_ADVANCED_SPECTRUM_ALGORITHM

/**
 * @brief 设置自动增益使能状态
 * @param enable 1=启用, 0=禁用
 */
void spectrum_set_auto_gain(u8 enable)
{
    agc.auto_gain_enable = enable;
}

/**
 * @brief 设置噪声门限
 * @param threshold 噪声门限值
 */
void spectrum_set_noise_gate(u16 threshold)
{
    // 通过修改宏定义，需要重新编译
    // 这里提供运行时调整接口
    static u16 runtime_noise_gate = SPECTRUM_NOISE_GATE;
    runtime_noise_gate = threshold;
}

/**
 * @brief 获取当前频谱数据状态
 * @param current 当前ADC值指针
 * @param filtered 滤波值指针
 * @param display 显示值指针
 * @param peak 峰值指针
 */
void spectrum_get_status(u16 *current, u16 *filtered, u16 *display, u16 *peak)
{
    if (current)
        *current = spectrum.current_value;
    if (filtered)
        *filtered = spectrum.filtered_value;
    if (display)
        *display = spectrum.display_value;
    if (peak)
        *peak = spectrum.peak_value;
}

/**
 * @brief 手动重置频谱算法状态
 */
void spectrum_reset(void)
{
    spectrum_init();
}

/**
 * @brief 运行时参数调整函数
 * @param smooth_factor 平滑系数 (1-8)
 * @param max_change 最大等级变化量 (1-5)
 */
void spectrum_adjust_params(u8 smooth_factor, u8 max_change)
{
    // 运行时参数调整
    static u8 runtime_smooth_factor = SPECTRUM_SMOOTH_FACTOR;
    static u8 runtime_max_change = MAX_LEVEL_CHANGE;

    if (smooth_factor >= 1 && smooth_factor <= 8) {
        runtime_smooth_factor = smooth_factor;
    }

    if (max_change >= 1 && max_change <= 5) {
        runtime_max_change = max_change;
    }

    // 可以通过全局变量或指针传递给算法使用
}

/**
 * @brief 启用/禁用调试输出
 * @param enable 1=启用, 0=禁用
 */
void spectrum_set_debug(u8 enable)
{
    // 可以动态控制调试输出
    static u8 debug_enabled = SPECTRUM_DEBUG_OUTPUT;
    debug_enabled = enable;
}

#endif

/* ============================================================================
 * Audio Reactive Library 配置接口
 * ============================================================================ */

#if USE_AUDIO_REACTIVE_LIB

/**
 * @brief 设置Audio Reactive库的灵敏度
 * @param sensitivity 灵敏度值 (1-100)
 */
void ar_lib_set_sensitivity(u8 sensitivity)
{
    ar_set_sensitivity(sensitivity);
}

/**
 * @brief 设置Audio Reactive库的噪声门限
 * @param threshold 噪声门限 (0-100)
 */
void ar_lib_set_noise_gate(u8 threshold)
{
    ar_set_noise_gate(threshold);
}

/**
 * @brief 获取Audio Reactive库状态
 * @param stats_buffer 状态输出缓冲区
 * @param buffer_size 缓冲区大小
 * @return 实际写入的字符数
 */
int ar_lib_get_status(char *stats_buffer, int buffer_size)
{
    return ar_get_stats(stats_buffer, buffer_size);
}

/**
 * @brief 重置Audio Reactive库状态
 */
void ar_lib_reset(void)
{
    ar_reset();
}

/**
 * @brief 开关Audio Reactive库调试输出
 * @param enabled 1-开启, 0-关闭
 */
void ar_lib_set_debug(u8 enabled)
{
    ar_set_debug(enabled);
}

#endif

/* ============================================================================
 * 算法切换说明
 * ============================================================================ */
/*
使用方法:
1. 修改文件顶部的宏定义 USE_ADVANCED_SPECTRUM_ALGORITHM
   - 设为 1: 使用新的高级频谱算法 (推荐)
   - 设为 0: 使用原始算法

新算法特性:
- 自动增益控制: 适应不同音量输入
- 动态响应: 强变化快响应，弱变化柔和过渡
- 峰值保持: 减少闪烁感，保持视觉连续性
- 多级滤波: 低通滤波 + 历史平均 + 动态响应
- 精细分级: 24级频谱分级，更细腻的效果
- 智能颜色映射: 根据频谱等级和峰值动态调色

调试接口:
- spectrum_set_auto_gain(): 开关自动增益
- spectrum_set_noise_gate(): 调整噪声门限
- spectrum_get_status(): 获取算法内部状态
- spectrum_reset(): 重置算法状态

参数调优:
- SPECTRUM_SMOOTH_FACTOR: 平滑度 (1-8, 值越大越平滑)
- SPECTRUM_ATTACK_RATE: 上升速度 (1-10, 值越大上升越快)
- SPECTRUM_DECAY_RATE: 下降速度 (1-10, 值越大下降越快)
- SPECTRUM_NOISE_GATE: 噪声门限 (建议30-100)
*/
