/**
 * @file audio_reactive_lib.h
 * @brief 单值音频能量reactive算法库 - 专为每几十毫秒获取一次音频能量值设计
 * @author 奥威斯项目组
 * @date 2025-10-30
 * @version 1.0
 * 
 * 功能特点：
 * - 专门处理单一音频能量值（0~0x7FFF）输入
 * - 集成自动增益控制(AGC)、节拍检测、平滑处理
 * - 轻量级设计，适合MCU环境
 * - 提供多种映射模式：VU表、频谱柱、颜色映射
 */

#ifndef AUDIO_REACTIVE_LIB_H
#define AUDIO_REACTIVE_LIB_H

#include "system/includes.h"

/* ============================================================================
 * 库版本信息
 * ============================================================================ */
#define AUDIO_REACTIVE_LIB_VERSION_MAJOR    1
#define AUDIO_REACTIVE_LIB_VERSION_MINOR    0
#define AUDIO_REACTIVE_LIB_VERSION_PATCH    0

/* ============================================================================
 * 配置宏定义
 * ============================================================================ */
#define AR_HISTORY_SIZE         16      // 历史数据缓存大小
#define AR_AGC_SAMPLES          64      // AGC采样窗口
#define AR_COLOR_LEVELS         24      // 颜色/级别数量（对应LED数量）
#define AR_BEAT_HISTORY         8       // 节拍检测历史长度

/* ============================================================================
 * 处理模式定义
 * ============================================================================ */
typedef enum {
    AR_MODE_VU_METER    = 0,    // VU表模式（单向上升）
    AR_MODE_SPECTRUM    = 1,    // 频谱柱模式（带峰值保持）
    AR_MODE_WAVE        = 2,    // 波形模式（上下波动）
    AR_MODE_PULSE       = 3,    // 脉冲模式（节拍增强）
    AR_MODE_SMOOTH      = 4,    // 平滑模式（最少闪烁）
} ar_mode_t;

/* ============================================================================
 * 输出类型定义
 * ============================================================================ */
typedef enum {
    AR_OUTPUT_LEVEL     = 0,    // 输出级别（0-23）
    AR_OUTPUT_COLOR     = 1,    // 输出颜色索引（0-23）
    AR_OUTPUT_PERCENT   = 2,    // 输出百分比（0-100）
    AR_OUTPUT_RAW       = 3,    // 输出原始处理值
} ar_output_type_t;

/* ============================================================================
 * 数据结构定义
 * ============================================================================ */

/**
 * @brief 自动增益控制结构体
 */
typedef struct {
    u16 min_value;              // 最小值
    u16 max_value;              // 最大值
    u16 samples[AR_AGC_SAMPLES];// 采样缓存
    u8  sample_index;           // 当前采样索引
    u8  sample_count;           // 有效采样计数
    u16 gain_factor;            // 增益因子 (256 = 1.0)
    u8  enabled;                // AGC开关
} ar_agc_t;

/**
 * @brief 节拍检测结构体
 */
typedef struct {
    u16 history[AR_BEAT_HISTORY];   // 能量历史
    u8  history_index;              // 历史索引
    u16 threshold;                  // 节拍阈值
    u8  detected;                   // 节拍检测标志
    u8  hold_counter;              // 节拍保持计数
    u16 energy_avg;                // 平均能量
} ar_beat_t;

/**
 * @brief 平滑处理结构体
 */
typedef struct {
    u16 history[AR_HISTORY_SIZE];   // 历史数据
    u8  history_index;              // 历史索引
    u16 sum_history;                // 历史数据总和
    u16 filtered_value;             // 滤波后的值
    u16 output_value;               // 输出值
    u8  attack_rate;                // 上升速度 (1-10)
    u8  decay_rate;                 // 下降速度 (1-10)
} ar_smooth_t;

/**
 * @brief 峰值检测结构体
 */
typedef struct {
    u16 peak_value;                 // 峰值
    u8  peak_hold_counter;          // 峰值保持计数
    u8  peak_decay_rate;            // 峰值衰减速度
} ar_peak_t;

/**
 * @brief 音频reactive主控制结构体
 */
typedef struct {
    // 配置参数
    ar_mode_t mode;                 // 处理模式
    ar_output_type_t output_type;   // 输出类型
    u8 sensitivity;                 // 灵敏度 (1-100)
    u8 noise_gate;                  // 噪声门限 (0-100)
    
    // 处理模块
    ar_agc_t agc;                   // 自动增益控制
    ar_beat_t beat;                 // 节拍检测
    ar_smooth_t smooth;             // 平滑处理
    ar_peak_t peak;                 // 峰值检测
    
    // 运行状态
    u16 current_input;              // 当前输入值
    u16 processed_value;            // 处理后的值
    u8  output_level;               // 输出级别
    u8  color_index;                // 颜色索引
    u8  initialized;                // 初始化标志
    
    // 调试信息
    u8  debug_enabled;              // 调试开关
    u32 process_count;              // 处理计数
} ar_context_t;

/* ============================================================================
 * 全局变量声明
 * ============================================================================ */
extern ar_context_t ar_ctx;

/* ============================================================================
 * 核心API函数
 * ============================================================================ */

/**
 * @brief 初始化音频reactive库
 * @param mode 处理模式
 * @param output_type 输出类型
 * @return 0-成功, 其他-失败
 */
int ar_init(ar_mode_t mode, ar_output_type_t output_type);

/**
 * @brief 处理单个音频能量值
 * @param input_value 输入的音频能量值 (0~0x7FFF)
 * @return 处理后的输出值（根据output_type决定含义）
 */
u8 ar_process(u16 input_value);

/**
 * @brief 获取当前输出级别
 * @return 输出级别 (0-23)
 */
u8 ar_get_level(void);

/**
 * @brief 获取当前颜色索引
 * @return 颜色索引 (0-23)
 */
u8 ar_get_color_index(void);

/**
 * @brief 获取节拍检测状态
 * @return 1-检测到节拍, 0-无节拍
 */
u8 ar_is_beat_detected(void);

/**
 * @brief 获取峰值信息
 * @return 峰值 (0-23)
 */
u8 ar_get_peak_level(void);

/* ============================================================================
 * 配置API函数
 * ============================================================================ */

/**
 * @brief 设置处理模式
 * @param mode 新的处理模式
 */
void ar_set_mode(ar_mode_t mode);

/**
 * @brief 设置灵敏度
 * @param sensitivity 灵敏度值 (1-100)
 */
void ar_set_sensitivity(u8 sensitivity);

/**
 * @brief 设置噪声门限
 * @param threshold 噪声门限 (0-100)
 */
void ar_set_noise_gate(u8 threshold);

/**
 * @brief 设置平滑参数
 * @param attack_rate 上升速度 (1-10)
 * @param decay_rate 下降速度 (1-10)
 */
void ar_set_smooth_params(u8 attack_rate, u8 decay_rate);

/**
 * @brief 开关自动增益控制
 * @param enabled 1-开启, 0-关闭
 */
void ar_set_agc_enabled(u8 enabled);

/**
 * @brief 设置节拍检测参数
 * @param threshold 节拍阈值 (相对于平均能量的倍数)
 */
void ar_set_beat_threshold(u16 threshold);

/* ============================================================================
 * 调试和状态API
 * ============================================================================ */

/**
 * @brief 开关调试输出
 * @param enabled 1-开启, 0-关闭
 */
void ar_set_debug(u8 enabled);

/**
 * @brief 获取处理统计信息
 * @param stats 输出统计信息的缓冲区
 * @param buffer_size 缓冲区大小
 * @return 实际写入的字符数
 */
int ar_get_stats(char *stats, int buffer_size);

/**
 * @brief 重置所有状态
 */
void ar_reset(void);

/**
 * @brief 获取库版本信息
 * @return 版本字符串
 */
const char* ar_get_version(void);

/* ============================================================================
 * 高级功能API
 * ============================================================================ */

/**
 * @brief 动态调整参数（运行时优化）
 * @param input_range_low 输入范围下限
 * @param input_range_high 输入范围上限
 * @param responsiveness 响应性 (1-10, 10最敏感)
 */
void ar_auto_tune(u16 input_range_low, u16 input_range_high, u8 responsiveness);

/**
 * @brief 获取建议的参数值
 * @param mode 目标模式
 * @param environment 环境类型 (0-安静, 1-一般, 2-嘈杂)
 * @return 0-成功设置建议参数, 其他-失败
 */
int ar_get_recommended_params(ar_mode_t mode, u8 environment);

/**
 * @brief 批量处理音频数据
 * @param input_buffer 输入缓冲区
 * @param output_buffer 输出缓冲区
 * @param count 数据数量
 * @return 实际处理的数据数量
 */
int ar_process_batch(u16 *input_buffer, u8 *output_buffer, int count);

#endif // AUDIO_REACTIVE_LIB_H