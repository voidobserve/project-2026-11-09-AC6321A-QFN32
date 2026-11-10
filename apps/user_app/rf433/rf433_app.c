
#include "event.h"
#include "rf433.h"
#include "key_driver.h"
#include "app_main.h"
#include "rf433_app.h"
#include "led_strand_effect.h"
#include "WS2812FX.H"
#include "one_wire.h"
#include "btstack/btstack_typedef.h"
#include "ble_trans_profile.h"
#include "btstack/le/att.h"
#include "save_flash.h"

// 读取键值有效时间
u8 rf433_valid_t;

void rf433_valid_cnt(void)
{
    if (rf433_valid_t < 80)//80ms
    {
        rf433_valid_t += 10;
    }
}

// extern u16 T0;
// extern u16 T1;
extern u8 rf_key_val;
static u8 stepmotpor_speed_cnt = 0;
extern u8 Ble_Addr[6]; //蓝牙地址

u32 RF433_CODE = 0xFFFFFFFE;
u32 RF433_CODE2 = 0xFFFFFFFE;

extern sys_cb_t sys_cb;

extern void fb_rf433_key_value(u8 key_vv);
extern void fb_pair_state(u8 key_vv);

#define long_press 0x1
#define hold_press 0x2

u8 enable_pair = 0;
volatile u8 pair_index = 0;

#if 0
void rf433_handle(struct sys_event *event)
{
    u8 event_type = 0;
    u8 key_value = 0;
    T0++;   //10ms加1
    static u32 hold_cnt = 0;

    //短按
    if (T0 > 15 && T1 <= 30)
    {
        key_value = rf_key_get();
        if (key_value == 255) return;
        rf_key_val = NO_KEY;
        T0 = 0;
        T1 = 0;

        if (enable_pair == 1)
        {
            // printf("key_value = %d,%d\r\n",key_value, pair_index);
            if (key_value == pair_code && pair_index == 1)
            {
                fb_pair_state(0x12);
                RF433_CODE = sys_cb.RFAddrCode1;
                save_user_data_area3();
            }
            if (key_value == pair_code && pair_index == 2)
            {
                fb_pair_state(0x12);
                RF433_CODE2 = sys_cb.RFAddrCode1;
                save_user_data_area3();
            }
        }
        else if (sys_cb.RFAddrCode1 == RF433_CODE || sys_cb.RFAddrCode1 == RF433_CODE2)  //判断客户码
        {
            fb_rf433_key_value(key_value);
        }
    }
    else  if (T0 > 15)  //松手
    {
        T1 = 0;
        hold_cnt = 0;
        key_value = rf_key_get();
        rf_key_val = NO_KEY;  //
        // printf("clead.......................");
        save_user_data_area3();
    }
    //长按
    if (T1 > 30)  // 30*50ms = 1.5s   T150ms+1   250ms 
    {
        key_value = rf_key_get();

        if (key_value == 255) return;
        rf_key_val = NO_KEY;
        T0 = 0;

        if (sys_cb.RFAddrCode1 == RF433_CODE || sys_cb.RFAddrCode1 == RF433_CODE2)  //判断客户码 原本遥控
        {
            hold_cnt++;
            if (hold_cnt == 1)
            {
                key_value = (key_value | long_press);
                fb_rf433_key_value(key_value);
            }
            else if ((hold_cnt % 5) == 0)
            {
                key_value = (key_value | hold_press);
                fb_rf433_key_value(key_value);
            }
        }
    }
}
#endif

void rf433_key_event_deal(struct sys_event *event)
{
    u8 key_event;
    u8 key_value;

    if (!event || event->u.key.type != KEY_DRIVER_TYPE_RF433) {
        return;
    }
    key_event = event->u.key.event;
    key_value = event->u.key.value;

    if (enable_pair)
    {
        if (key_value == RF_PAIR_KEY_VALUE)
        {
            if (/*key_value == pair_code &&*/ pair_index == 1)
            {
                fb_pair_state(0x12);
                RF433_CODE = sys_cb.RFAddrCode1;
                save_user_data_area3();
            }
            if (/*key_value == pair_code &&*/ pair_index == 2)
            {
                fb_pair_state(0x12);
                RF433_CODE2 = sys_cb.RFAddrCode1;
                save_user_data_area3();
            }
        }
        return;  // 对码时不响应其他按键
    }

    if (key_event == KEY_EVENT_CLICK)
    {   // 短按
        if (sys_cb.RFAddrCode1 == RF433_CODE || sys_cb.RFAddrCode1 == RF433_CODE2)  // 判断客户码
        {
            fb_rf433_key_value(key_value);
        }
    }
    else if (key_event == KEY_EVENT_LONG)
    {   // 长按
        if (sys_cb.RFAddrCode1 == RF433_CODE || sys_cb.RFAddrCode1 == RF433_CODE2)  // 判断客户码 原本遥控
        {
            fb_rf433_key_value(key_value | long_press);
        }
    }
    else if (key_event == KEY_EVENT_HOLD)
    {   // 持续按
        if (sys_cb.RFAddrCode1 == RF433_CODE || sys_cb.RFAddrCode1 == RF433_CODE2)  // 判断客户码 原本遥控
        {
            fb_rf433_key_value(key_value | hold_press);
        }
    }
}
