/**
 * @file uart_a.c
 * @brief UART通信模块 - 实现串口协议解析、LED控制命令处理
 * @author 奥威斯项目组
 * @date 2025-10-29
 */

/* ============================================================================
 * 系统头文件
 * ============================================================================ */
#include "system/app_core.h"
#include "system/includes.h"
#include "server/server_core.h"
#include "app_config.h"
#include "app_action.h"
#include "os/os_api.h"
#include "btcontroller_config.h"
#include "btctrler/btctrler_task.h"
#include "config/config_transport.h"
#include "btstack/avctp_user.h"
#include "btstack/btstack_task.h"
#include "bt_common.h"
#include "rcsp_bluetooth.h"
#include "rcsp_user_update.h"
#include "app_charge.h"
#include "app_chargestore.h"
#include "app_power_manage.h"
#include "app_comm_bt.h"
#include "app_main.h"
#include "led_strand_effect.h"
#include "led_strip_drive.h"

/* ============================================================================
 * 宏定义
 * ============================================================================ */
#define UART_DEV_USAGE_TEST_SEL 2 //uart_dev.c api接口使用方法选择
//  选择1  串口中断回调函数推送事件，由事件响应函数接收串口数据
//  选择2  由task接收串口数据

#define UART_DEV_TEST_MULTI_BYTE                                               \
    1 //uart_dev.c 读写多个字节api / 读写1个字节api 选择
#define UART_DEV_FLOW_CTRL 0

#define UART_CBUF_SIZE 0x100 // UART缓冲区大小
#define UART_FRAM_SIZE                                                         \
    0x100 // 单次数据包最大值(每次cbuf缓存达到fram或串口收到一次数据, 就会起一次中断)

/* ============================================================================
 * 外部变量声明
 * ============================================================================ */
extern fc_effect_t fc_effect; // 幻彩灯串效果数据
extern u8 pair_index;         // 配对索引
extern u16 dingshi_cnt;       // 定时计数器
extern u8 enable_pair;        // 配对使能标志

/* ============================================================================
 * 外部函数声明 - 系统函数
 * ============================================================================ */
extern void printf_buf(u8 *buf, u32 len);
extern void ble_send_user_data(u8 *buffer, u8 buffer_size);
extern void app_effect_collections(u8 key_v);
extern void save_user_data_area3(void);
extern void sound_handle(void);

/* ============================================================================
 * 外部函数声明 - RF433相关
 * ============================================================================ */
extern void fb_rf433_music_on_off(u8 on_off);
extern void aws_pixels_dynamic_mode_change(u8 dir);
extern void rf433_reset_mode(void);
extern void rf433_color_switch(void);
extern void rf433_rgbw_switch(void);
extern void rf433_speed_adjust(void);
extern void rf433_bright_adjust(void);

/* ============================================================================
 * 外部函数声明 - 遥控灯效控制
 * ============================================================================ */
extern void aws_pixels_static_mode_change(u8 dir);
extern void bright_sub(void);
extern void bright_plus(void);
extern void speed_fast(void);
extern void speed_slow(void);
extern void soft_turn_off_lights(void);
extern void soft_turn_on_the_light(void);

/* ============================================================================
 * 外部函数声明 - LED效果相关
 * ============================================================================ */
extern void rgbw_app_effect_collections(u8 index);
extern void rgbw_set_mutil_color(u8 index, u32 color);
extern void WS2812FX_setBrightness(u8 brightness);
extern void rf433_pixel_color_bright_up(void);
extern void rf433_pixel_color_bright_down(void);
extern void rf433_rgbw_color_bright_up(void);
extern void rf433_rgbw_color_bright_down(void);

/* ============================================================================
 * 静态变量定义
 * ============================================================================ */
static u8 devBuffer_static[UART_CBUF_SIZE]
    __attribute__((aligned(4))); // DMA内存缓冲区
static u8 uart_print_buffer[256] __attribute__((aligned(4)));

struct _cmd_string_item_t
{
    uint32_t index;
    const char *string;
};
static const struct _cmd_string_item_t rx_cmd_string_table[] = {
    {0x011D, "指示灯:待机状态(黄)"},
    {0x011E, "指示灯:蓝牙状态(蓝闪)"},
    {0x011F, "指示灯:蓝牙主机(蓝紫跳)"},
    {0x0120, "指示灯:蓝牙从机(蓝青跳)"},
    {0x0121, "指示灯: U盘状态(红)"},
    {0x0122, "指示灯: U盘主机(红闪)"},
    {0x0123, "指示灯: U盘从机(红青跳)"},
    {0x0124, "指示灯: AUX状态(绿)"},
    {0x0125, "指示灯:蓝牙连接(蓝)"},

    {0x0400, "音乐能量"},
    {0x04FF, "音乐能量"},

    {0x0500, "默认灯光"},
    {0x0501, "强制开幻彩"},
    {0x0502, "强制关幻彩"},
    {0x0503, "开/关幻彩"},
    {0x0504, "开七彩"},
    {0x0505, "关七彩"},
    {0x0506, "开/关七彩"},
    {0x0507, "幻彩效果切换"},
    {0x0508, "幻彩速度切换"},
    {0x0509, "幻彩/七彩切换(预留)"},
    {0x050A, "七彩亮度切换"},
    {0x050B, "关机(全局)"},
    {0x050C, "开机(全局)"},
    {0x050D, "选择七彩(预留)"},
    {0x050E, "选择幻彩(预留)"},
    {0x050F, "切换效果"},
    {0x0510, "幻彩静态色切换"},
    {0x0511, "配对(全局)"},
    {0x0514, "打开白光灯模式"},
    {0x0515, "关闭白光灯模式"},
    {0x0516, "开关切换白光灯模式"},
    {0x0540, "开/关前幻彩"},
    {0x0541, "开/关后幻彩"},
    {0x0542, "七彩模式向上切换"},
    {0x0543, "七彩模式向下切换"},
    {0x0544, "幻彩模式向上切换"},
    {0x0545, "幻彩模式向下切换"},
    {0x0546, "七彩亮度加"},
    {0x0547, "七彩亮度减"},
    {0x0548, "幻彩速度加"},
    {0x0549, "幻彩速度减"},
};
static const struct _cmd_string_item_t tx_cmd_string_table[] = {
    {0x0300, "RF433按键"},
    ///TODO:补全按键值
    {0x03FF, "RF433按键"},

    {0x0512, "配对成功"},

    {0x0600, "关闭音乐能量传输"},
    {0x0601, "开启音乐能量传输"},
};

/* ============================================================================
 * 全局变量定义
 * ============================================================================ */
const uart_bus_t *uart_bus_a = NULL; // UART总线句柄
u8 uart_rx_flag = 1;                 // UART接收标志
u8 _rx_buf[300];                     // UART接收缓冲区
u8 _rx_len = 0;                      // UART接收数据长度
static u8 uart_stream_buf[UART_CBUF_SIZE * 2];
static u16 uart_stream_len;
u16 music_adc = 0;   // 音乐ADC值
u8 rgbw_ctrl_f = 0;  // RGBW控制标志
u8 color_ctrl_f = 0; // 颜色控制标志
u8 rgbw_anxia_f = 0; // RGBW按下标志

/* ============================================================================
 * 函数声明
 * ============================================================================ */
u16 uart_a_send_api(uint8_t *data, u16 len);
uint32_t uart_decode_packet(uint8_t *buffer, uint32_t len, uint8_t *out_packet,
                            uint32_t *out_len);
void uart_key_handle(uint8_t *buffer, uint32_t len);

/* ============================================================================
 * 工具函数
 * ============================================================================ */

/**
 * @brief 生成校验码 (累加和算法)
 * @param cmd 命令缓冲区指针
 * @param len 数据长度
 * @return 校验码 (失败返回0xFF)
 */
u8 creat_crc(u8 *cmd, u8 len)
{
    if (cmd == NULL || len == 0)
        return 0xFF;

    u32 sum = 0;
    for (u8 i = 0; i < len - 1; i++) {
        sum += cmd[i];
    }
    // 校验码取低8位
    return (sum & 0xff);
}

/**
 * @brief 获取音乐ADC值
 * @return 音乐ADC值
 */
u16 get_music_adc(void)
{
    return music_adc;
}

/* ============================================================================
 * UART底层驱动函数
 * ============================================================================ */

/**
 * @brief UART接收处理函数
 */
void uart_rx_handler_a(void)
{
#if (UART_DEV_USAGE_TEST_SEL == 1)
    u8 len;

    if (uart_bus_a) {
        len = uart_bus_a->read(_rx_buf, UART_FRAM_SIZE, 0); //全部收齐
        _rx_len = len;
        printf("[UART_APP]uart_rx:");
        put_buf(_rx_buf, _rx_len);
        if (len > UART_FRAM_SIZE) {
            len = 0;
            printf("uart overflow\n");
        }

        uart_rx_flag = 1;
    }
#endif
}

#if (UART_DEV_USAGE_TEST_SEL == 1)
/**
 * @brief UART中断回调函数
 * @param ut_bus UART总线句柄
 * @param status 中断状态
 */
static void user_uart_isr_cb(void *ut_bus, u32 status)
{
    struct sys_event e;

    if (status == UT_RX || status == UT_RX_OT) {
#if FLOW_CONTROL
        uart1_flow_ctl_rts_suspend();
#endif
        e.type = SYS_DEVICE_EVENT;
        e.arg = (void *)DEVICE_EVENT_FROM_BOARD_UART;
        e.u.dev.event = 0;
        e.u.dev.value = 0;
        sys_event_notify(&e);
    }
}
#elif (UART_DEV_USAGE_TEST_SEL == 2)
#ifdef USER_DEBUG_ENABLE
static void uart_buffer_print(uint8_t *tag, uint8_t *buffer, u32 size,
                              u32 flags)
{
    u32 offset = 0;
    u32 temp_len = sizeof(uart_print_buffer);

    memset(uart_print_buffer, 0, temp_len);
    snprintf(uart_print_buffer, temp_len,
             "[\e[32m\e[1m%s\e[0m] Buffer[\e[32m\e[1m%d\e[0m]: ", tag, size);
    offset = strlen(uart_print_buffer);
    for (uint32_t i = 0; i < size; i++) {
        if (temp_len - offset <= 3)
            break;
        snprintf(uart_print_buffer + offset, temp_len - offset, "%02x ",
                 buffer[i]);
        offset += 3;
    }

    if (flags & 0x80000000) {
        // 处理特殊标志
        uint16_t cmd_index = (flags & 0xFFFF);
        for (uint32_t i = 0; i < ARRAY_SIZE(rx_cmd_string_table); i++) {
            if (rx_cmd_string_table[i].index == cmd_index) {
                printf("%s <<< \e[31m\e[1m%s\e[0m\n", uart_print_buffer,
                       rx_cmd_string_table[i].string);
                goto __finished;
            }
        }
    } else if (flags & 0x40000000) {
        // 处理特殊标志
        uint16_t cmd_index = (flags & 0xFFFF);
        for (uint32_t i = 0; i < ARRAY_SIZE(tx_cmd_string_table); i++) {
            if (tx_cmd_string_table[i].index == cmd_index) {
                printf("%s >>> \e[33m\e[1m%s\e[0m\n", uart_print_buffer,
                       tx_cmd_string_table[i].string);
                goto __finished;
            }
        }
    }

    if (flags == 0)
        printf("%s\n", uart_print_buffer);
    else
        printf("%s !!! UNKNOW CMD\n", uart_print_buffer);

__finished:
    return;
}
#endif

static void uart_u_task(void *arg)
{
    const uart_bus_t *uart_bus = arg;
    static uint8_t packet[UART_CBUF_SIZE + 3];
    u32 uart_rxcnt;

    printf("uart_u_task start\n");
    while (1) {
        uart_rxcnt = uart_bus->read(_rx_buf, UART_CBUF_SIZE, 0);
        if (!uart_rxcnt) {
            continue;
        }

        /* 以流方式处理，保留被拆分的旧4字节裸帧和新CRC帧。 */
        if (uart_rxcnt > sizeof(uart_stream_buf) - uart_stream_len) {
            uart_stream_len = 0;
        }
        memcpy(uart_stream_buf + uart_stream_len, _rx_buf, uart_rxcnt);
        uart_stream_len += uart_rxcnt;

        while (uart_stream_len) {
            uint32_t remain;
            uint32_t packet_len = 0;
            uint32_t consumed;

            while (uart_stream_len && uart_stream_buf[0] != 0xEA) {
                memmove(uart_stream_buf, uart_stream_buf + 1,
                        --uart_stream_len);
            }
            if (!uart_stream_len) {
                break;
            }

            remain = uart_decode_packet(uart_stream_buf, uart_stream_len,
                                        packet, &packet_len);
            if (remain == (uint32_t)-1) {
                break; /* 不完整帧，等待下一次read */
            }
            consumed = uart_stream_len - remain;
            if (!consumed) {
                break;
            }
            memmove(uart_stream_buf, uart_stream_buf + consumed, remain);
            uart_stream_len = remain;
            if (packet_len) {
                uart_key_handle(packet, packet_len);
            }
        }
    }
}
#endif

/**
 * @brief UART初始化函数
 * @param baud 波特率
 * @return 初始化结果 (1-成功, 0-失败)
 */
int ct_uart_init_a(u32 baud)
{
    struct uart_platform_data_t u_arg = {0};
    //  u_arg.tx_pin = IO_PORT_DP;
    //  u_arg.rx_pin = IO_PORT_DM;
    u_arg.tx_pin = IO_PORTB_06;
    u_arg.rx_pin = IO_PORTB_07;
    u_arg.rx_cbuf = devBuffer_static;
    u_arg.rx_cbuf_size = UART_CBUF_SIZE;
    u_arg.frame_length = 0xFFFFFFFF;
    u_arg.rx_timeout = 5; //ms
#if (UART_DEV_USAGE_TEST_SEL == 2)
    u_arg.isr_cbfun = NULL; // 方式2采用线程查询方式，不需要中断回调推送消息
#else
    u_arg.isr_cbfun = user_uart_isr_cb;
#endif
    u_arg.baud = baud;
    u_arg.is_9bit = 0;

    uart_bus_a = uart_dev_open(&u_arg);

    if (uart_bus_a != NULL) {
        printf("uart_dev_open() success\n");
#if (UART_DEV_USAGE_TEST_SEL == 2)
        os_task_create(uart_u_task, (void *)uart_bus_a, 31, 512, 0,
                       "uart_u_task");
#endif
        return 1;
    }
    return 0;
}

/**
 * @brief UART发送API函数
 * @param data 发送数据指针
 * @param len 数据长度
 * @return 发送的数据长度
 */
u16 uart_a_send_api(uint8_t *data, u16 len)
{
    if (uart_bus_a) {
#ifdef USER_DEBUG_ENABLE
        uart_buffer_print("UART_TX_PACKET", data, len,
                          0x40000000 | ((data[2] << 8) & 0xFF00) |
                              ((data[3]) & 0xFF));
#endif
        uart_bus_a->write(data, len);
    }
    return len;
}

/* ============================================================================
 * 协议处理函数
 * ============================================================================ */

/**
 * @brief 七彩灯状态映射表
 * @note 定义各种设备状态对应的灯光颜色
 */
//@note 七彩灯状态映射
//  [0x011d]待机;     黄色
//  [0x011e]蓝牙;     闪烁蓝色
//  [0x011f]蓝牙主机;  蓝紫跳变
//  [0x0120]蓝牙从机;  蓝青跳变
//  [0x0121]U盘;     红色
//  [0x0122]U盘主机;  红跳变
//  [0x0123]U盘从机;  红青跳变
//  [0x0124]AUX;     绿色
//  [0x0125]蓝牙已连接; 蓝色

uint32_t uart_decode_packet(uint8_t *buffer, uint32_t len, uint8_t *out_packet,
                            uint32_t *out_len)
{
    uint32_t packet_len;
    uint8_t crc;

    if (!buffer || !out_packet || !out_len || len < 4) {
        return (uint32_t)-1;
    }
    *out_len = 0;
    if (buffer[0] != 0xEA) {
        return len - 1;
    }

    /* 旧主机报文不带CRC：控制为EA 00 05 <subcmd>，
     * 音乐能量为EA 00 04 <high> <low>。 */
    if (buffer[1] == 0x00 && buffer[2] == 0x05) {
        memcpy(out_packet, buffer, 4);
        *out_len = 4;
        return len - 4;
    }
    if (buffer[1] == 0x00 && buffer[2] == 0x04) {
        if (len < 5) {
            return (uint32_t)-1;
        }
        memcpy(out_packet, buffer, 5);
        *out_len = 5;
        return len - 5;
    }

    if (len < 5) {
        return (uint32_t)-1;
    }
    packet_len = (uint32_t)buffer[1] + 3;
    if (packet_len > UART_CBUF_SIZE + 3) {
        return len - 1;
    }
    if (len < packet_len) {
        return (uint32_t)-1;
    }

    crc = creat_crc(buffer, packet_len);
    if (crc != buffer[packet_len - 1]) {
        return len - 1;
    }
    memcpy(out_packet, buffer, packet_len);
    *out_len = packet_len;
    return len - packet_len;
}

/**
 * @brief UART协议解析和命令处理主函数
 * @note 协议格式: [0xEA] [Length] [Command] [Data] [CRC]
 */
void uart_key_handle(uint8_t *buffer, uint32_t len)
{
    if (!buffer || len < 4 || buffer[0] != 0xEA) {
        return;
    }

#if USER_DEBUF_ENABLE
    put_buf(buffer, len);
#endif

    /* len==4 / EA 00 05 xx 为旧产品裸协议；其它帧已在收包层完成CRC校验。 */
    if (buffer[0] == 0xEA) {
#ifdef USER_DEBUG_ENABLE
        if (buffer[2] != 0x04) // 屏蔽能量数据
            uart_buffer_print("UART_RX_PACKET", buffer, len,
                              0x80000000 | ((buffer[2] << 8) & 0xFF00) |
                                  ((buffer[3]) & 0xFF));
#endif

        if (buffer[2] == 0x05) // 按键扩展命令
        {
            // 设备开启状态下的命令处理
            if (fc_effect.on_off_flag == DEVICE_ON) {
                switch (buffer[3]) {
                case 0x00: // 灯光默认
                    rf433_reset_mode();
                    break;

                case 0x01: // 强制开幻彩 (旧单路灯带)
                    fc_effect.led_on_off_flag = DEVICE_ON;
                    fc_effect.led1_on_off_flag = DEVICE_OFF;
                    ws2812_power_unlock();
                    color_ctrl_f = 1;
                    if (fc_effect.Now_state == IS_light_music) {
                        fb_rf433_music_on_off(1);
                    }
                    break;

                case 0x02: // 强制关幻彩 (旧单路灯带)
                    fc_effect.led_on_off_flag = DEVICE_OFF;
                    fc_effect.led1_on_off_flag = DEVICE_OFF;
                    ws2812_power_off();
                    fc_effect.color_i = 6;
                    fb_rf433_music_on_off(0);
                    break;

                case 0x03: // 幻彩开关切换 (遥控)
                    rf433_color_switch();
                    break;

                case 0x04: // 七彩开
                    fc_effect.rgbw_on_off_flag = DEVICE_ON;
                    rgbw_ctrl_f = 1;
                    color_ctrl_f = 0;
                    break;

                case 0x05: // 七彩关
                    fc_effect.rgbw_on_off_flag = DEVICE_OFF;
                    break;

                case 0x06: // RGBW开关切换 (遥控)
                    rf433_rgbw_switch();
                    break;

                case 0x07: // 幻彩效果切换：保持旧面板选择状态门控
                    if (fc_effect.on_off_flag == DEVICE_ON &&
                        color_ctrl_f == 1) {
                        color_ctrl_f = 0;
                        ws2812_power_unlock();
                        legacy_ws_mode_next();
                    }
                    break;

                case 0x08: // 速度调节
                    if (fc_effect.on_off_flag == DEVICE_ON)
                        rf433_speed_adjust();
                    break;

                case 0x09: // 旧协议：RGB效果切换
                    if (rgbw_ctrl_f == 1) {
                        rgbw_ctrl_f = 0;
                        legacy_rgb_mode_next();
                    }
                    break;

                case 0x0A: // 旧协议：按当前选择对象调整亮度
                    dingshi_cnt = 0;
                    if (fc_effect.on_off_flag == DEVICE_ON) {
                        if (color_ctrl_f == 2 || rgbw_ctrl_f == 1) {
                            /* 新架构共用亮度字段，仍保持旧单键由暗到亮循环的手感。 */
                            rf433_bright_adjust();
                        }
                    }
                    break;

                case 0x0D: // 旧协议：选择RGB
                    if (fc_effect.rgbw_on_off_flag == DEVICE_OFF) {
                        fc_effect.rgbw_on_off_flag = DEVICE_ON;
                    } else {
                        rgbw_ctrl_f = 1;
                        color_ctrl_f = 0;
                    }
                    break;

                case 0x0E: // 旧协议：选择WS2812
                    if (fc_effect.led_on_off_flag == DEVICE_OFF) {
                        fc_effect.led_on_off_flag = DEVICE_ON;
                        ws2812_power_unlock();
                    } else {
                        color_ctrl_f = 2;
                        rgbw_ctrl_f = 0;
                    }
                    break;

                case 0x0F: // 旧协议：按当前选择对象切换效果
                    dingshi_cnt = 0;
                    if (color_ctrl_f == 2) {
                        ws2812_power_unlock();
                        legacy_ws_mode_next();
                    } else if (rgbw_ctrl_f == 1) {
                        legacy_rgb_mode_next();
                    }
                    break;

                case 0x10: // 旧协议：WS2812静态颜色切换
                    if (color_ctrl_f == 2) {
                        dingshi_cnt = 0;
                        ws2812_power_unlock();
                        aws_pixels_static_mode_change(CHANGE_DIR_FORWARD);
                    }
                    break;

                case 0x11: // 旧协议：WS2812供电，同时全局进入433配对
                    ws2812_power_unlock();
                    break;

                case 0x12: // 旧协议：强制切断WS2812供电
                    ws2812_power_lock_off();
                    break;

                /* 0x14~0x19 是新框架的独立白灯扩展协议；旧产品无此通道，
                 * 不对旧UART主机开放，白光仅由旧RGB效果的R=G=B产生。 */
                case 0x14:
                case 0x15:
                case 0x16:
                case 0x17:
                case 0x18:
                case 0x19:
                    break;

                case 0x40: // 前面幻彩灯开关
                    fc_effect.led_on_off_flag =
                        (fc_effect.led_on_off_flag == DEVICE_ON) ? DEVICE_OFF
                                                                 : DEVICE_ON;
                    if (fc_effect.led_on_off_flag == DEVICE_ON &&
                        fc_effect.Now_state == IS_light_music) {
                        fb_rf433_music_on_off(1);
                    }
                    break;

                case 0x41: // 后面幻彩灯开关 (靠近USB)
                    fc_effect.led1_on_off_flag =
                        (fc_effect.led1_on_off_flag == DEVICE_ON) ? DEVICE_OFF
                                                                  : DEVICE_ON;
                    if (fc_effect.led1_on_off_flag == DEVICE_ON &&
                        fc_effect.Now_state == IS_light_music) {
                        fb_rf433_music_on_off(1);
                    }
                    break;

                case 0x42: // 灯光模式向上
                    aws_pixels_dynamic_mode_change(CHANGE_DIR_BACKWARD);
                    break;

                case 0x43: // 灯光模式向下
                    aws_pixels_dynamic_mode_change(CHANGE_DIR_FORWARD);
                    break;

                case 0x44: // 灯光颜色向上
                    aws_pixels_static_mode_change(CHANGE_DIR_BACKWARD);
                    break;

                case 0x45: // 灯光颜色向下
                    aws_pixels_static_mode_change(CHANGE_DIR_FORWARD);
                    break;

                case 0x46: // 亮度加
                    if (fc_effect.on_off_flag == DEVICE_ON)
                        bright_plus();
                    break;

                case 0x47: // 亮度减
                    if (fc_effect.on_off_flag == DEVICE_ON)
                        bright_sub();
                    break;

                case 0x48: // 速度加快
                    if (fc_effect.on_off_flag == DEVICE_ON)
                        speed_fast();
                    break;

                case 0x49: // 速度减慢
                    if (fc_effect.on_off_flag == DEVICE_ON)
                        speed_slow();
                    break;
                }

                save_user_data_area3(); // 保存设置到Flash
            }

            // 全局命令处理 (不受设备开关状态限制)
            switch (buffer[3]) {
            case 0x0B: // 旧协议：关闭全部灯
                soft_turn_off_lights();
                break;

            case 0x0C: // 旧协议：打开全部灯
                soft_turn_on_the_light();
                break;

            case 0x11: // 配对
                printf("遥控配对\n");
                enable_pair = 1;
                extern u16 pair_cnt;
                pair_cnt = 0;
                pair_index++;
                if (pair_index > 2)
                    pair_index = 1;
                break;
            }
        } else if (buffer[2] == 0x04 && len >= 5) // 音乐效果命令
        {
            music_adc = buffer[3] << 8 | buffer[4];
        } else if (buffer[2] == 0x01) // 设备状态指示命令
        {
            if (fc_effect.on_off_flag == DEVICE_ON) {
                switch (buffer[3]) {
                case 0x1D: // 待机状态 - 黄色
                    fc_effect.rgbw.r = 255;
                    fc_effect.rgbw.g = 255;
                    fc_effect.rgbw.b = 0;
                    fc_effect.custom_index = 10;
                    break;

                case 0x1E: // 蓝牙状态 - 闪烁蓝色
                    rgbw_set_mutil_color(0, BLUE);
                    rgbw_set_mutil_color(1, BLACK);
                    fc_effect.rgbw_scene.c_n = 2;
                    fc_effect.custom_index = 11;
                    break;

                case 0x1F: // 蓝牙主机 - 蓝紫跳变
                    rgbw_set_mutil_color(0, BLUE);
                    rgbw_set_mutil_color(1, MAGENTA);
                    fc_effect.rgbw_scene.c_n = 2;
                    fc_effect.custom_index = 11;
                    break;

                case 0x20: // 蓝牙从机 - 蓝青跳变
                    rgbw_set_mutil_color(0, BLUE);
                    rgbw_set_mutil_color(1, CYAN);
                    fc_effect.rgbw_scene.c_n = 2;
                    fc_effect.custom_index = 11;
                    break;

                case 0x21: // U盘状态 - 红色
                    fc_effect.rgbw_on_off_flag = DEVICE_ON;
                    fc_effect.rgbw.r = 255;
                    fc_effect.rgbw.g = 0;
                    fc_effect.rgbw.b = 0;
                    fc_effect.custom_index = 10; //静态
                    break;

                case 0x22: // U盘主机 - 红跳变
                    rgbw_set_mutil_color(0, RED);
                    rgbw_set_mutil_color(1, BLACK);
                    fc_effect.rgbw_scene.c_n = 2;
                    fc_effect.custom_index = 11;
                    break;

                case 0x23: // U盘从机 - 红青跳变
                    rgbw_set_mutil_color(0, RED);
                    rgbw_set_mutil_color(1, CYAN);
                    fc_effect.rgbw_scene.c_n = 2;
                    fc_effect.custom_index = 11;
                    break;

                case 0x24: // AUX状态 - 绿色
                    fc_effect.rgbw.r = 0;
                    fc_effect.rgbw.g = 255;
                    fc_effect.rgbw.b = 0;
                    fc_effect.custom_index = 10;
                    break;

                case 0x25: // 蓝牙已连接 - 蓝色
                    fc_effect.rgbw.r = 0;
                    fc_effect.rgbw.g = 0;
                    fc_effect.rgbw.b = 255;
                    fc_effect.custom_index = 10;
                    break;

                default:
                    goto __finished;
                    break;
                }

                fc_effect.b = USER_BRIGHTNESS_MAX;
                WS2812FX_setBrightness(fc_effect.b);
                aws_rgb_mode_update(0);
            }
        }
    }
__finished:
    sound_handle(); // 声控ADC处理
}

/* ============================================================================
 * RF433反馈通信函数
 * ============================================================================ */

/**
 * @brief 发送配对状态反馈
 * @param key_vv 配对按键值
 * @note 通过UART反馈RF433配对操作状态
 */
void fb_pair_state(u8 key_vv)
{
    u8 fb_433_buf[5];
    fb_433_buf[0] = 0xEA;                     // 协议头
    fb_433_buf[1] = 0x02;                     // 数据长度
    fb_433_buf[2] = 0x05;                     // 配对命令
    fb_433_buf[3] = 0x12;                     // 配对状态标识
    fb_433_buf[4] = creat_crc(fb_433_buf, 5); // CRC校验

    uart_a_send_api(fb_433_buf, 5); // 串口返回配对状态
}

/**
 * @brief 发送RF433按键值反馈
 * @param key_vv RF433按键值
 * @note 将接收到的RF433遥控按键通过UART反馈给主机
 */
void fb_rf433_key_value(u8 key_vv)
{
    u8 fb_433_buf[5];
    fb_433_buf[0] = 0xEA;                     // 协议头
    fb_433_buf[1] = 0x02;                     // 数据长度
    fb_433_buf[2] = 0x03;                     // RF433按键命令
    fb_433_buf[3] = key_vv;                   // 按键值
    fb_433_buf[4] = creat_crc(fb_433_buf, 5); // CRC校验

    uart_a_send_api(fb_433_buf, 5); // 串口返回遥控键值
}

/**
 * @brief 发送RF433音乐模式开关反馈
 * @param on_off 音乐模式开关状态 (0=关闭, 1=开启)
 * @note 反馈音乐模式的开关状态给主机
 */
void fb_rf433_music_on_off(u8 on_off)
{
    u8 fb_433_buf[5];
    fb_433_buf[0] = 0xEA;                     // 协议头
    fb_433_buf[1] = 0x02;                     // 数据长度
    fb_433_buf[2] = 0x06;                     // 音乐模式命令
    fb_433_buf[3] = on_off;                   // 开关状态
    fb_433_buf[4] = creat_crc(fb_433_buf, 5); // CRC校验

    uart_a_send_api(fb_433_buf, 5); // 串口返回音乐模式状态
}

void uart_update_current_white_light_level(u8 level)
{
    u8 fb_433_buf[6];
    fb_433_buf[0] = 0xEA;                     // 协议头
    fb_433_buf[1] = 0x03;                     // 数据长度
    fb_433_buf[2] = 0x05;                     // 音乐模式命令
    fb_433_buf[3] = 0x17;                     // 白光灯模式设置级数
    fb_433_buf[4] = level;                    // 白光灯模式设置级数
    fb_433_buf[5] = creat_crc(fb_433_buf, 6); // CRC校验

    uart_a_send_api(fb_433_buf, 6); // 串口返回音乐模式状态
}

void uart_update_current_white_light_power(u8 power)
{
    u8 fb_433_buf[6];
    fb_433_buf[0] = 0xEA;                     // 协议头
    fb_433_buf[1] = 0x03;                     // 数据长度
    fb_433_buf[2] = 0x05;                     // 音乐模式命令
    fb_433_buf[3] = 0x16;                     // 开关切换白光灯模式
    fb_433_buf[4] = power;                    // 白光灯模式开关状态
    fb_433_buf[5] = creat_crc(fb_433_buf, 6); // CRC校验

    uart_a_send_api(fb_433_buf, 6); // 串口返回音乐模式状态
}

/* ============================================================================
 * 应用层命令映射与处理 (开发调试用)
 * ============================================================================ */

#if 0
/*
功能说明                串口指令
灯光恢复默认            EA,00,05,00
幻彩开灯                EA,00,05,01
幻彩关灯                EA,00,05,02
幻彩开关灯切换	        EA,00,05,03
RGB开灯	                EA,00,05,04
RGB关灯	                EA,00,05,05
RGB开关灯切换	        EA,00,05,06
幻彩样式切换	        EA,00,05,01	EA,00,05,07
速度（单键版）	        EA,00,05,08
RGB样式切换	            EA,00,05,04	EA,00,05,09
幻彩静态亮度（单键版）	EA,00,05,0E	EA,00,05,0A
RGB静态亮度（单键版）	EA,00,05,0D	EA,00,05,0A
幻彩静态色切换	        EA,00,05,0E	EA,00,05,10
关闭全部灯	            EA,00,05,0B
打开全部灯	            EA,00,05,0C
进入遥控器配对模式	    EA,00,05,11
*/

/**
 * @brief 应用层命令枚举定义
 * @note 用于调试和开发阶段的命令映射
 */
enum {
    AWS_APP_CMD_START = 0,
    AWS_APP_CMD_RESUME_STYLE,           // 恢复默认样式
    AWS_APP_CMD_WS2812_POWER_ON,        // WS2812开灯
    AWS_APP_CMD_WS2812_POWER_OFF,       // WS2812关灯
    AWS_APP_CMD_WS2812_POWER_SW,        // WS2812开关切换
    AWS_APP_CMD_RGB_POWER_ON,           // RGB开灯
    AWS_APP_CMD_RGB_POWER_OFF,          // RGB关灯
    AWS_APP_CMD_RGB_POWER_SW,           // RGB开关切换
    AWS_APP_CMD_WS2812_STYLE_NEXT,      // WS2812样式切换
    AWS_APP_CMD_WS2812_SPEED_SLOWER,    // WS2812速度减慢
    AWS_APP_CMD_WS2812_SPEED_FASTER,    // WS2812速度加快
    AWS_APP_CMD_RGB_STYLE_NEXT,         // RGB样式切换
    AWS_APP_CMD_WS2812_BRIGHTNESS_UP,   // WS2812亮度增加
    AWS_APP_CMD_WS2812_BRIGHTNESS_DOWN, // WS2812亮度降低
    AWS_APP_CMD_RGB_BRIGHTNESS_UP,      // RGB亮度增加
    AWS_APP_CMD_RGB_BRIGHTNESS_DOWN,    // RGB亮度降低
    AWS_APP_CMD_WS2812_STATIC_NEXT,     // WS2812静态色切换
    AWS_APP_CMD_POWER_OFF,              // 全部关灯
    AWS_APP_CMD_POWER_ON,               // 全部开灯
    AWS_APP_CMD_RF433_ENTER_PAIR,       // 进入RF433配对模式
};

/**
 * @brief 应用层命令重映射处理函数
 * @param index 命令索引
 * @note 将应用层命令映射为标准UART协议命令
 */
void aoweisi_app_remap_uart_cmd(u8 index)
{
    printf("%s\n", __func__);
    
    switch (index)
    {
    case AWS_APP_CMD_RESUME_STYLE:
        uart_key_handle((u8[]) { 0xEA, 0x00, 0x05, 0x00 }, 4);
        break;
    case AWS_APP_CMD_WS2812_POWER_ON:
        uart_key_handle((u8[]) { 0xEA, 0x00, 0x05, 0x01 }, 4);
        break;
    case AWS_APP_CMD_WS2812_POWER_OFF:
        uart_key_handle((u8[]) { 0xEA, 0x00, 0x05, 0x02 }, 4);
        break;
    case AWS_APP_CMD_WS2812_POWER_SW:
        uart_key_handle((u8[]) { 0xEA, 0x00, 0x05, 0x03 }, 4);
        break;
    case AWS_APP_CMD_RGB_POWER_ON:
        uart_key_handle((u8[]) { 0xEA, 0x00, 0x05, 0x04 }, 4);
        break;
    case AWS_APP_CMD_RGB_POWER_OFF:
        uart_key_handle((u8[]) { 0xEA, 0x00, 0x05, 0x05 }, 4);
        break;
    case AWS_APP_CMD_RGB_POWER_SW:
        uart_key_handle((u8[]) { 0xEA, 0x00, 0x05, 0x06 }, 4);
        break;
    case AWS_APP_CMD_WS2812_STYLE_NEXT:
        uart_key_handle((u8[]) { 0xEA, 0x00, 0x05, 0x01 }, 4); uart_key_handle((u8[]) { 0xEA, 0x00, 0x05, 0x07 }, 4);
        break;
        // case AWS_APP_CMD_WS2812_SPEED:
        //     uart_key_handle((u8[]) { 0xEA, 0x00, 0x05, 0x08 }, 4);
        //     break;
    case AWS_APP_CMD_WS2812_SPEED_SLOWER:
        extern void speed_slow(void);
        speed_slow();
        break;
    case AWS_APP_CMD_WS2812_SPEED_FASTER:
        extern void speed_fast(void);
        speed_fast();
        break;
    case AWS_APP_CMD_RGB_STYLE_NEXT:
        uart_key_handle((u8[]) { 0xEA, 0x00, 0x05, 0x04 }, 4); uart_key_handle((u8[]) { 0xEA, 0x00, 0x05, 0x09 }, 4);
        break;
        // case AWS_APP_CMD_WS2812_BRIGHTNESS:
        //     uart_key_handle((u8[]) { 0xEA, 0x00, 0x05, 0x0E }, 4); uart_key_handle((u8[]) { 0xEA, 0x00, 0x05, 0x0A }, 4);
        //     break;
    case AWS_APP_CMD_WS2812_BRIGHTNESS_UP:
        extern void rf433_pixel_color_bright_up(void);
        rf433_pixel_color_bright_up();
        break;
    case AWS_APP_CMD_WS2812_BRIGHTNESS_DOWN:
        extern void rf433_pixel_color_bright_down(void);
        rf433_pixel_color_bright_down();
        break;
        // case AWS_APP_CMD_RGB_BRIGHTNESS:
        //     uart_key_handle((u8[]) { 0xEA, 0x00, 0x05, 0x0D }, 4); uart_key_handle((u8[]) { 0xEA, 0x00, 0x05, 0x0A }, 4);
        //     break;
    case AWS_APP_CMD_RGB_BRIGHTNESS_UP:
        extern void rf433_rgbw_color_bright_up(void);
        rf433_rgbw_color_bright_up();
        break;
    case AWS_APP_CMD_RGB_BRIGHTNESS_DOWN:
        extern void rf433_rgbw_color_bright_down(void);
        rf433_rgbw_color_bright_down();
        break;
    case AWS_APP_CMD_WS2812_STATIC_NEXT:
        uart_key_handle((u8[]) { 0xEA, 0x00, 0x05, 0x0E }, 4); uart_key_handle((u8[]) { 0xEA, 0x00, 0x05, 0x10 }, 4);
        break;
    case AWS_APP_CMD_POWER_OFF:
        uart_key_handle((u8[]) { 0xEA, 0x00, 0x05, 0x0B }, 4);
        uart_key_handle((u8[]) { 0xEA, 0x00, 0x05, 0x02 }, 4);  // 因为AWS_APP_CMD_POWER_OFF这个对应的指令存在有时不能彻底关闭幻彩，所以加多个指令辅助关闭幻彩
        break;
    case AWS_APP_CMD_POWER_ON:
        uart_key_handle((u8[]) { 0xEA, 0x00, 0x05, 0x0C }, 4);
        break;
    case AWS_APP_CMD_RF433_ENTER_PAIR:
        uart_key_handle((u8[]) { 0xEA, 0x00, 0x05, 0x11 }, 4);
        break;
    }
}

void aoweisi_app_cmd_event(u8 *buffer, int len)
{
    switch (buffer[0])
    {
    case 0x01:  // 灯开关
        if (buffer[1] == 0x01)
        {   // 幻彩开关
            if (buffer[2] == 0)
                aoweisi_app_remap_uart_cmd(AWS_APP_CMD_WS2812_POWER_OFF);
            else if (buffer[2] == 1)
                aoweisi_app_remap_uart_cmd(AWS_APP_CMD_WS2812_POWER_ON);
            else if (buffer[2] == 2)
                aoweisi_app_remap_uart_cmd(AWS_APP_CMD_WS2812_POWER_SW);
        }
        else if (buffer[1] == 0x02)
        {   // RGB开关
            if (buffer[2] == 0)
                aoweisi_app_remap_uart_cmd(AWS_APP_CMD_RGB_POWER_OFF);
            else if (buffer[2] == 1)
                aoweisi_app_remap_uart_cmd(AWS_APP_CMD_RGB_POWER_ON);
            else if (buffer[2] == 2)
                aoweisi_app_remap_uart_cmd(AWS_APP_CMD_RGB_POWER_SW);
        }
        else if (buffer[1] == 0x03)
        {   // 总开关
            if (buffer[2] == 0)
                aoweisi_app_remap_uart_cmd(AWS_APP_CMD_POWER_OFF);
            else if (buffer[2] == 1)
                aoweisi_app_remap_uart_cmd(AWS_APP_CMD_POWER_ON);
            else if (buffer[2] == 3)
                aoweisi_app_remap_uart_cmd(AWS_APP_CMD_RESUME_STYLE);  // 灯光恢复默认样式
        }
        break;

    case 0x02:  // 幻彩相关控制
        if (buffer[1] == 0x01)
        {   // 样式切换
            if (buffer[2] == 0x01)
            {   // 动态样式
                aoweisi_app_remap_uart_cmd(AWS_APP_CMD_WS2812_STYLE_NEXT);
            }
            else if (buffer[2] == 0x03)
            {   // 静态样式
                aoweisi_app_remap_uart_cmd(AWS_APP_CMD_WS2812_STATIC_NEXT);
            }
        }
        else if (buffer[1] == 0x02)
        {   // 速度
            if (buffer[2] == 0x01)
                aoweisi_app_remap_uart_cmd(AWS_APP_CMD_WS2812_SPEED_FASTER);
            else if (buffer[2] == 0x02)
                aoweisi_app_remap_uart_cmd(AWS_APP_CMD_WS2812_SPEED_SLOWER);
        }
        else if (buffer[1] == 0x03)
        {   // 静态亮度
            if (buffer[2] == 0x01)
                aoweisi_app_remap_uart_cmd(AWS_APP_CMD_WS2812_BRIGHTNESS_UP);
            else if (buffer[2] == 0x02)
                aoweisi_app_remap_uart_cmd(AWS_APP_CMD_WS2812_BRIGHTNESS_DOWN);
        }
        break;

    case 0x03:  // RGB相关控制
        if (buffer[1] == 0x01)
        {   // 样式切换
            if (buffer[2] == 0x01)
                aoweisi_app_remap_uart_cmd(AWS_APP_CMD_RGB_STYLE_NEXT);
        }
        else if (buffer[1] == 0x03)
        {   // 静态亮度
            if (buffer[2] == 0x01)
                aoweisi_app_remap_uart_cmd(AWS_APP_CMD_RGB_BRIGHTNESS_UP);
            else if (buffer[2] == 0x02)
                aoweisi_app_remap_uart_cmd(AWS_APP_CMD_RGB_BRIGHTNESS_DOWN);
        }
        break;

    case 0x04:
        if (buffer[1] == 0x01)
        {
            if (buffer[2] == 0x01)
            {   // 进入RF433配对状态
                aoweisi_app_remap_uart_cmd(AWS_APP_CMD_RF433_ENTER_PAIR);
            }
        }
        break;
    }
}
#endif
