#include "app_config.h"
#include "includes.h"
#include "audio_reactive_lib.h"

// 全局上下文
ar_context_t ar_ctx;

static const char *version_str = "audio_reactive_lib v1.0";

int ar_init(ar_mode_t mode, ar_output_type_t output_type)
{
    memset(&ar_ctx, 0, sizeof(ar_ctx));
    ar_ctx.mode = mode;
    ar_ctx.output_type = output_type;
    ar_ctx.sensitivity = 50; // 中等灵敏度
    ar_ctx.noise_gate = 3;   // 相对小的噪声门限（百分比）

    // AGC 默认值
    ar_ctx.agc.min_value = 0x7FFF;
    ar_ctx.agc.max_value = 0;
    ar_ctx.agc.sample_index = 0;
    ar_ctx.agc.sample_count = 0;
    ar_ctx.agc.gain_factor = 256;
    ar_ctx.agc.enabled = 1;

    // 平滑参数
    ar_ctx.smooth.history_index = 0;
    ar_ctx.smooth.sum_history = 0;
    for (int i = 0; i < AR_HISTORY_SIZE; i++) ar_ctx.smooth.history[i] = 0;
    ar_ctx.smooth.attack_rate = 6;
    ar_ctx.smooth.decay_rate = 3;

    // 峰值
    ar_ctx.peak.peak_value = 0;
    ar_ctx.peak.peak_hold_counter = 0;
    ar_ctx.peak.peak_decay_rate = 4;

    // 节拍检测
    for (int i = 0; i < AR_BEAT_HISTORY; i++) ar_ctx.beat.history[i] = 0;
    ar_ctx.beat.history_index = 0;
    ar_ctx.beat.threshold = 200; // 初始阈值（相对）

    ar_ctx.initialized = 1;
    ar_ctx.debug_enabled = 0;
    ar_ctx.process_count = 0;

    return 0;
}

static u16 apply_agc(u16 in)
{
    if (!ar_ctx.agc.enabled) return in;

    // 更新观测最小/最大
    if (in < ar_ctx.agc.min_value) ar_ctx.agc.min_value = in;
    if (in > ar_ctx.agc.max_value) ar_ctx.agc.max_value = in;

    // 简单滑动样本缓存
    ar_ctx.agc.samples[ar_ctx.agc.sample_index++] = in;
    if (ar_ctx.agc.sample_index >= AR_AGC_SAMPLES) ar_ctx.agc.sample_index = 0;
    if (ar_ctx.agc.sample_count < AR_AGC_SAMPLES) ar_ctx.agc.sample_count++;

    // 计算局部平均并更新增益
    u32 sum = 0;
    for (int i = 0; i < ar_ctx.agc.sample_count; i++) sum += ar_ctx.agc.samples[i];
    u16 avg = (ar_ctx.agc.sample_count) ? (sum / ar_ctx.agc.sample_count) : in;

    // 目标 RMS 设为 0x2000 (~8192) 以便在常见输入范围内保持动态
    const u16 target = 0x2000;
    if (avg == 0) avg = 1;
    // gain_factor 为 256 表示 1.0
    u32 gf = ((u32)target << 8) / avg;
    if (gf < 64) gf = 64;   // 最小 0.25x
    if (gf > 1024) gf = 1024; // 最大 4x
    ar_ctx.agc.gain_factor = (u16)gf;

    u32 out = ((u32)in * ar_ctx.agc.gain_factor) >> 8;
    if (out > 0x7FFF) out = 0x7FFF;
    return (u16)out;
}

static u16 smooth_process(u16 in)
{
    // 环形历史更新
    ar_ctx.smooth.sum_history -= ar_ctx.smooth.history[ar_ctx.smooth.history_index];
    ar_ctx.smooth.history[ar_ctx.smooth.history_index] = in;
    ar_ctx.smooth.sum_history += in;
    ar_ctx.smooth.history_index = (ar_ctx.smooth.history_index + 1) % AR_HISTORY_SIZE;

    u16 avg = ar_ctx.smooth.sum_history / AR_HISTORY_SIZE;

    // attack/decay 非线性响应
    u16 cur = ar_ctx.smooth.filtered_value;
    if (in > cur) {
        // 上升更快 (attack_rate 值越大越快)
        int step = ((int)(in - cur) * ar_ctx.smooth.attack_rate) / 8;
        if (step < 1) step = 1;
        cur += step;
        if ((u32)cur > in) cur = in;
    } else if (in < cur) {
        // 下降更慢 (decay_rate 值越大下降越快)
        int step = ((int)(cur - in) * ar_ctx.smooth.decay_rate) / 16;
        if (step < 1) step = 1;
        cur -= step;
        if (cur < in) cur = in;
    }

    ar_ctx.smooth.filtered_value = cur;
    return cur;
}

static void peak_update(u16 val)
{
    if (val > ar_ctx.peak.peak_value) {
        ar_ctx.peak.peak_value = val;
        ar_ctx.peak.peak_hold_counter = 6; // 保持若干周期
    } else if (ar_ctx.peak.peak_hold_counter > 0) {
        ar_ctx.peak.peak_hold_counter--;
    } else {
        // 衰减
        if (ar_ctx.peak.peak_value > 0) {
            ar_ctx.peak.peak_value = (ar_ctx.peak.peak_value * 15) / 16;
        }
    }
}

static void beat_detect(u16 val)
{
    // 记录历史并计算短期平均
    ar_ctx.beat.history[ar_ctx.beat.history_index] = val;
    ar_ctx.beat.history_index = (ar_ctx.beat.history_index + 1) % AR_BEAT_HISTORY;

    u32 s = 0;
    for (int i = 0; i < AR_BEAT_HISTORY; i++) s += ar_ctx.beat.history[i];
    u16 short_avg = s / AR_BEAT_HISTORY;

    // 节拍条件：当前值远高于短期平均，且短期平均高于噪声门限
    u16 gate = (ar_ctx.noise_gate * 0x7FFF) / 100; // 将百分比转换为原始范围
    if (short_avg < gate) {
        ar_ctx.beat.detected = 0;
        return;
    }

    int diff = (int)val - (int)short_avg;
    if (diff > (int)ar_ctx.beat.threshold) {
        ar_ctx.beat.detected = 1;
        ar_ctx.beat.hold_counter = 3; // 保持几个周期
    } else if (ar_ctx.beat.hold_counter > 0) {
        ar_ctx.beat.hold_counter--;
        ar_ctx.beat.detected = (ar_ctx.beat.hold_counter > 0) ? 1 : 0;
    } else {
        ar_ctx.beat.detected = 0;
    }
}

static u8 map_to_level(u16 val)
{
    // 基于当前观察到的最小/最大做线性映射到 0..(AR_COLOR_LEVELS-1)
    u16 minv = ar_ctx.agc.min_value;
    u16 maxv = ar_ctx.agc.max_value;
    if (minv >= maxv) {
        // fallback: 使用固定范围
        minv = 0;
        maxv = 0x4000; // 16384
    }

    // 扰动处理: 拉伸动态范围
    if (val < minv) val = minv;
    if (val > maxv) val = maxv;

    u32 span = (u32)maxv - (u32)minv;
    if (span == 0) span = 1;
    u32 rel = (u32)(val - minv);

    u32 lvl = (rel * (AR_COLOR_LEVELS - 1)) / span;
    if (lvl >= AR_COLOR_LEVELS) lvl = AR_COLOR_LEVELS - 1;
    return (u8)lvl;
}

u8 ar_process(u16 input_value)
{
    if (!ar_ctx.initialized) {
        ar_init(AR_MODE_VU_METER, AR_OUTPUT_LEVEL);
    }

    ar_ctx.process_count++;
    ar_ctx.current_input = input_value;

    // 噪声门限
    u16 gate = (ar_ctx.noise_gate * 0x7FFF) / 100;
    if (input_value <= gate) {
        input_value = 0;
    }

    // AGC
    u16 agc_val = apply_agc(input_value);

    // 平滑
    u16 smooth_val = smooth_process(agc_val);

    // 峰值 & 节拍
    peak_update(smooth_val);
    beat_detect(smooth_val);

    // 基于模式和灵敏度做最终映射
    u8 level = map_to_level(smooth_val);

    // 节拍增强: 若检测到节拍，在等级上加一个临时提升
    if (ar_ctx.beat.detected) {
        int boosted = level + (ar_ctx.sensitivity / 25); // sensitivity 越高，boost 越明显
        if (boosted >= AR_COLOR_LEVELS) boosted = AR_COLOR_LEVELS - 1;
        level = (u8)boosted;
    }

    // 输出平滑：限制每次变化幅度，避免闪烁
    int last = ar_ctx.output_level;
    int diff = (int)level - last;
    int max_change = 4; // 默认每周期最大变化
    if (ar_ctx.beat.detected) max_change = 8; // 节拍时允许较大变化
    if (diff > max_change) level = last + max_change;
    else if (diff < -max_change) level = last - max_change;

    ar_ctx.output_level = level;
    ar_ctx.color_index = level; // 简单映射
    ar_ctx.processed_value = smooth_val;

    // 根据输出类型返回不同值
    switch (ar_ctx.output_type) {
        case AR_OUTPUT_LEVEL: return ar_ctx.output_level;
        case AR_OUTPUT_COLOR: return ar_ctx.color_index;
        case AR_OUTPUT_PERCENT: return (ar_ctx.output_level * 100) / (AR_COLOR_LEVELS - 1);
        case AR_OUTPUT_RAW: return (u8)((ar_ctx.processed_value >> 8) & 0xFF);
        default: return ar_ctx.output_level;
    }
}

u8 ar_get_level(void) { return ar_ctx.output_level; }
u8 ar_get_color_index(void) { return ar_ctx.color_index; }
u8 ar_is_beat_detected(void) { return ar_ctx.beat.detected; }
u8 ar_get_peak_level(void) { return map_to_level(ar_ctx.peak.peak_value); }

void ar_set_mode(ar_mode_t mode) { ar_ctx.mode = mode; }
void ar_set_sensitivity(u8 sensitivity) { if (sensitivity < 1) sensitivity = 1; if (sensitivity > 100) sensitivity = 100; ar_ctx.sensitivity = sensitivity; }
void ar_set_noise_gate(u8 threshold) { ar_ctx.noise_gate = threshold; }
void ar_set_smooth_params(u8 attack_rate, u8 decay_rate) { if (attack_rate>=1 && attack_rate<=10) ar_ctx.smooth.attack_rate = attack_rate; if (decay_rate>=1 && decay_rate<=10) ar_ctx.smooth.decay_rate = decay_rate; }
void ar_set_agc_enabled(u8 enabled) { ar_ctx.agc.enabled = enabled ? 1 : 0; }
void ar_set_beat_threshold(u16 threshold) { ar_ctx.beat.threshold = threshold; }

void ar_set_debug(u8 enabled) { ar_ctx.debug_enabled = enabled ? 1 : 0; }

int ar_get_stats(char *stats, int buffer_size)
{
    if (!stats || buffer_size <= 0) return 0;
    int n = snprintf(stats, buffer_size,
        "proc:%u in:%u proc_val:%u lvl:%u peak:%u agc_gf:%u min:%u max:%u\n",
        (unsigned)ar_ctx.process_count, (unsigned)ar_ctx.current_input, (unsigned)ar_ctx.processed_value,
        (unsigned)ar_ctx.output_level, (unsigned)ar_ctx.peak.peak_value, (unsigned)ar_ctx.agc.gain_factor,
        (unsigned)ar_ctx.agc.min_value, (unsigned)ar_ctx.agc.max_value);
    return (n > buffer_size) ? buffer_size : n;
}

void ar_reset(void)
{
    ar_ctx.initialized = 0;
    ar_init(ar_ctx.mode, ar_ctx.output_type);
}

const char* ar_get_version(void) { return version_str; }

void ar_auto_tune(u16 input_range_low, u16 input_range_high, u8 responsiveness)
{
    // 简单自动调优示例：设置AGC观测范围
    if (input_range_low < input_range_high) {
        ar_ctx.agc.min_value = input_range_low;
        ar_ctx.agc.max_value = input_range_high;
    }
    if (responsiveness >= 1 && responsiveness <= 10) {
        // map responsiveness to attack/decay
        ar_ctx.smooth.attack_rate = 2 + responsiveness; // 3..12
        ar_ctx.smooth.decay_rate = 1 + (responsiveness / 2);
    }
}

int ar_get_recommended_params(ar_mode_t mode, u8 environment)
{
    // 返回并设置推荐参数（非常保守的实现）
    (void)mode;
    if (environment == 0) { // 安静
        ar_set_noise_gate(2);
        ar_set_sensitivity(40);
        ar_set_smooth_params(4, 3);
    } else if (environment == 1) { // 一般
        ar_set_noise_gate(3);
        ar_set_sensitivity(60);
        ar_set_smooth_params(6, 4);
    } else { // 嘈杂
        ar_set_noise_gate(6);
        ar_set_sensitivity(80);
        ar_set_smooth_params(8, 6);
    }
    return 0;
}

int ar_process_batch(u16 *input_buffer, u8 *output_buffer, int count)
{
    if (!input_buffer || !output_buffer || count <= 0) return 0;
    for (int i = 0; i < count; i++) {
        output_buffer[i] = ar_process(input_buffer[i]);
    }
    return count;
}
