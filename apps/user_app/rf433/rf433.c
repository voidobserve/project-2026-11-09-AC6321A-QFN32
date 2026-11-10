#include "app_config.h"
#include "system/includes.h"
#include "rf433.h"
#include "rf433_app.h"

// key_driver.c的key_driver_init注册定时器
#if TCFG_RF433_ENABLE

#define RF_PIN              IO_PORTA_00
#define RF_DECODE_PIN()     (JL_PORTA->IN & BIT(0))

sys_cb_t sys_cb;
u8 rf_key_val = NO_KEY;
// u16 T0;
// u16 T1;

#define PAIRING_SUCCESS_THRESHOLD   (3)     // 配对时连续 PAIRING_SUCCESS_THRESHOLD 次的地址均相同才发出成功按键值消息

#define IO_LEVEL_FILTER_PARAM   (0b00000011)
#define RF_IO_HIGH              (0x01)
#define RF_IO_LOW               (0x00)
#define RF_TIMEOUT_COUNT        (3000)      // 超时周期数(根据实际计数的定时器而定)

#define RF_ADDR_MASK            (0xFFFF)    // 16位地址 -- 需要根据实际情况修改
#define RF_KEY_MASK             (0xFF)      // 8位键值
// #define RF_ADDR_MASK            (0xFFFFF)   // 20位地址 -- 需要根据实际情况修改
// #define RF_KEY_MASK             (0xF)       // 4位键值

static volatile uint32_t rf_decode_addr = 0xFFFFFFFF;
static volatile uint8_t  rf_decode_key  = 0xFF;

/*
奥威斯遥控采集信号(低电平)：
头: 236x50us = 11800us = 236
长:  23x50us =  1150us =  23
短:   8x50us =   400us =   8
*/
#define RF_BIT_SHORT_RANGE_MIN      (2)
#define RF_BIT_SHORT_RANGE_MAX      (15)
#define RF_BIT_LONG_RANGE_MIN       (15)
#define RF_BIT_LONG_RANGE_MAX       (30)
#define RF_BIT_FINISHED_RANGE_MIN   (45)

AT_VOLATILE_RAM_CODE
static void timer50us_hook(void)
{
    static volatile uint8_t  io_level_filter = 0xFF;                 // 电平滤波器
    static volatile uint8_t  io_level = 1;                           // 滤波后电平
    static volatile uint32_t io_low_count = 0;                       // 低电平计数
    static volatile uint32_t decode_data = 0;                        // 解码数据(一般是24位，此处故意设置32位为了避免真的有24位全是1的有效信号存在，加长到32位杜绝这种情况发生)
    static volatile uint8_t  decode_bit_count = 0;                   // 解码位计数
    static volatile uint32_t decode_timeout_count = 0;               // 解码超时计数，超时后当作抬手，期间当作仍在按下，用于一些干扰导致的错码打断长按，变相提高容错率
    static volatile uint8_t  pairing_success_count = 1;

    /**
     * @note 电平滤波，用(io_level_filter)记录历史电平，连续 N 个周期保持一致认为是有效电平
     * @note N 的具体值由(IO_LEVEL_FILTER_PARAM)宏定义，(IO_LEVEL_FILTER_PARAM)低位连续有多少个1就代表N的值
     * @note 例如(IO_LEVEL_FILTER_PARAM)为0b00000111时，N = 3，
     *       就是连续3个周期为高电平(io_level_filter=0b00000111)才是有效高电平，此时level才会被置为1
     *       就是连续3个周期为低电平(io_level_filter=0b00000000)才是有效低电平，此时level才会被置为0
     *       io_level_filter为其他值则维持level不变，之前是什么就是什么
     */
    io_level_filter <<= 1;                                  // 先向左移1位
    io_level_filter |= (RF_DECODE_PIN() ? 0x01 : 0x00);     // 记录当前电平到最低位
    io_level_filter &= IO_LEVEL_FILTER_PARAM;               // 只保留有效位数量
    if (io_level_filter == IO_LEVEL_FILTER_PARAM)           // 判断记录的电平值为全1才更新为高电平
        io_level = RF_IO_HIGH;
    else if (io_level_filter == 0x00)                       // 判断记录的电平值为全0才更新为低电平
        io_level = RF_IO_LOW;
    else
        io_level = io_level;                                // 维持原有电平状态(添加此语句为了方便理解，无实际意义)

    // io_level 滤波后的电平

    if (io_level == RF_IO_LOW)
    {
        io_low_count++;
        if (io_low_count > RF_BIT_FINISHED_RANGE_MIN)
        {   // 持续低电平超过(RF_BIT_FINISHED_RANGE_MIN)个周期，认为其已经结束
            if (decode_bit_count == 24)
            {   // 处理完整数据
                rf_decode_addr = decode_data & RF_ADDR_MASK;
                rf_decode_key = (decode_data >> 16) & RF_KEY_MASK;
                // printf("rf433 addr=%04X, key=%02X\n", rf_decode_addr, rf_decode_key);
                decode_timeout_count = RF_TIMEOUT_COUNT;  // 重置超时计数

                // 奥威斯原接口
                if (enable_pair && (rf_decode_key == RF433_PAIR_CODE_LEGACY || rf_decode_key == RF433_PAIR_CODE_NEW))
                {
                    if (sys_cb.RFAddrCode1 == rf_decode_addr)
                        pairing_success_count++;
                    else
                        pairing_success_count = 1;
                    sys_cb.RFAddrCode1 = rf_decode_addr;

                    if (pairing_success_count == PAIRING_SUCCESS_THRESHOLD)
                        rf_key_val = RF_PAIR_KEY_VALUE;  // 连续 N 次相同才认为成功
                    else //if (pairing_success_count < PAIRING_SUCCESS_THRESHOLD)
                        rf_key_val = NO_KEY;
                }
                else if (enable_pair == 0)
                {
                    pairing_success_count = 1;
                    if (rf_decode_addr == RF433_CODE || rf_decode_addr == RF433_CODE2)
                    {
                        sys_cb.RFAddrCode1 = rf_decode_addr;
                        rf_key_val = rf_decode_key;
                    }
                }
            }
            decode_bit_count = 0;
        }
    }
    else
    {   // 只检测低电平的长度，通过低电平的长度判断数据位，短的是数据1(说明高电平时间长)，长的是数据0(说明高电平时间短)
        if (io_low_count > RF_BIT_SHORT_RANGE_MIN && io_low_count <= RF_BIT_SHORT_RANGE_MAX)
        {   // 数据1
            decode_data >>= 1;
            decode_data |= 0x800000;
            decode_bit_count++;
        }
        else if (io_low_count > RF_BIT_LONG_RANGE_MIN && io_low_count <= RF_BIT_LONG_RANGE_MAX)
        {   // 数据0
            decode_data >>= 1;
            decode_bit_count++;
        }
        // if (io_low_count) put_buf(&io_low_count, 1);  // 打印信号低电平的时间长度
        io_low_count = 0;
    }

    if (decode_timeout_count > 0 && (--decode_timeout_count) == 0)
    {   // 超时，认为用户已经抬手
        rf_decode_addr = 0xFFFFFFFF;
        rf_decode_key  = 0xFF;
        
        // 奥威斯原接口
        // sys_cb.RFAddrCode1 = 0xFFFFFFFF;
        rf_key_val = NO_KEY;
    }
}

/****************************************************************************************
**名称:定时器设置，定时50us中断
**功能:
**说明:
**备注:
**日期:
*****************************************************************************************/
static const u16 timer_div[] = {
    /*0000*/    1,
    /*0001*/    4,
    /*0010*/    16,
    /*0011*/    64,
    /*0100*/    2,
    /*0101*/    8,
    /*0110*/    32,
    /*0111*/    128,
    /*1000*/    256,
    /*1001*/    4 * 256,
    /*1010*/    16 * 256,
    /*1011*/    64 * 256,
    /*1100*/    2 * 256,
    /*1101*/    8 * 256,
    /*1110*/    32 * 256,
    /*1111*/    128 * 256,
};

#define APP_TIMER_CLK           24000000//(CONFIG_BT_NORMAL_HZ/2) //clk_get("timer")
#define MAX_TIME_CNT            0x7fff
#define MIN_TIME_CNT            0x100
#define TIMER_UNIT				1

#define TIMER_CON               JL_TIMER2->CON
#define TIMER_CNT               JL_TIMER2->CNT
#define TIMER_PRD               JL_TIMER2->PRD
#define TIMER_VETOR             IRQ_TIME2_IDX

/****************************************************************************************
**名称:定时中断服务函数
**功能:
**说明:
**备注:
**日期:
*****************************************************************************************/

___interrupt
AT_VOLATILE_RAM_CODE
void user_timer_isr(void)
{
	static u8 timer_cnt;
	TIMER_CON |= BIT(14);

    timer50us_hook();
}

//定时器初始化
void user_timer_init(void)
{
	u32 prd_cnt;
	u8 index;

	printf("********* user_timer_init **********\n");
	for (index = 0; index < (sizeof(timer_div) / sizeof(timer_div[0])); index++)
	{
        prd_cnt = TIMER_UNIT * (APP_TIMER_CLK / 20000) / timer_div[index];
        if (prd_cnt > MIN_TIME_CNT && prd_cnt < MAX_TIME_CNT) {
            break;
        }
    }

	TIMER_CNT = 0;
	TIMER_PRD = prd_cnt;
	request_irq(TIMER_VETOR, 3, user_timer_isr, 0);
	// TIMER_CON = (index << 4) | BIT(0) | BIT(3);
	TIMER_CON = (0b0001 << 10) | (index << 4) | BIT(0);
}

void rf433_gpio_init(void)
{
	gpio_set_die(RF_PIN, 1);
	gpio_set_direction(RF_PIN, 1);
    gpio_set_pull_up(RF_PIN, 0);
}

u8 rf_key_get_value(void)
{
    return rf_key_val;
}

u8 rf_key_get(void)
{
    u8 key_val = NO_KEY;
    key_val = rf_key_val;
	return key_val;
}

struct key_driver_para rf_scan_para = {
    .scan_time 	  	  = 20,				//按键扫描频率, 单位: ms
    .last_key 		  = NO_KEY,  		//上一次get_value按键值, 初始化为NO_KEY;
    .filter_time  	  = 0,				//按键消抖延时;
    .long_time 		  = 75,  			//按键判定长按数量
    .hold_time 		  = (75 + 12),  	//按键判定HOLD数量
    .click_delay_time = 0,			    //按键被抬起后等待连击延时数量
    .key_type		  = KEY_DRIVER_TYPE_RF433,
    .get_value 		  = rf_key_get_value,
};

#endif
