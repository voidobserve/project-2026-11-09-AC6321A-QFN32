#include "app_config.h"
#include "system/includes.h"
#include "syscfg_id.h"
#include "save_flash.h"
#include "rf433.h"


#define CFG_USER_LED_LEDGTH_DATA    3

extern sys_cb_t sys_cb;
extern u32 RF433_CODE;
extern u32 RF433_CODE2;

u8 ble_state = 1; //默认开启BLE模块

/*******************************************************************************************************
**函数名：上电读取FLASH里保存的指令数据
**输  出：
**输  入：读取CFG_USER_COMMAND_BUF_DATA里保存的最后一条接收到的指令，
**描  述：读取CFG_USER_LED_LEDGTH_DATA里保存的第一次上电标志，灯带长度，顺序是：：第1字节：第一次上电标志位，第2、3字节：灯带长度
**说  明：
**版  本：
**修改日期：
*******************************************************************************************************/
void read_flash_device_status_init(void)
{
#if 1
    u8 res;
    save_flash_t save_flash3;

    memset((u8 *)&save_flash3, 0, sizeof(save_flash_t));
    save_flash3.rf433_code = 0xFFFFFFFE;
    save_flash3.rf433_code2 = 0xFFFFFFFE;

    res = syscfg_read(CFG_USER_LED_LEDGTH_DATA,(u8 *)(&save_flash3), sizeof(save_flash_t));

    if(save_flash3.header != HEADER_VALUE)  //第一次上电
    {
        fc_data_init();
        ble_state = 1; /* BLE底座状态不在本次适配中变更 */
        RF433_CODE = 0xFFFFFFFE;
        RF433_CODE2 = 0xFFFFFFFE;
    }
    else
    {
        printf("\n flash_size=%d", sizeof(fc_effect_t));
        memcpy( (u8*)(&fc_effect), (u8*)(&save_flash3.fc_save) , sizeof(fc_effect_t));
        
        ble_state = save_flash3.ble_state;
        RF433_CODE = save_flash3.rf433_code;
        RF433_CODE2 = save_flash3.rf433_code2;
    }
        printf("vm read rgbw on_off: l%d , n%d\n", save_flash3.fc_save.last_rgbw_on_off_flag, save_flash3.fc_save.rgbw_on_off_flag);
        printf("vm read led  on_off: l%d , n%d\n", save_flash3.fc_save.last_led_on_off_flag, save_flash3.fc_save.led_on_off_flag);
        printf("vm read led1 on_off: l%d , n%d\n", save_flash3.fc_save.last_led1_on_off_flag, save_flash3.fc_save.led1_on_off_flag);

#ifdef USER_DEBUG_ENABLE
    RF433_CODE = 0xFFFF;
    RF433_CODE2 = 0xFFFF;
#endif
#endif
}


// 把用户数据写到区域3
#if 0
void save_user_data_area3(void)
{
   // printf("\n save_user_data_area3");
    save_flash_t save_data;
    save_data.header = 0x55;
    save_data.ble_state = ble_state;
    save_data.rf433_code = RF433_CODE;
    save_data.rf433_code2 = RF433_CODE2;
    memcpy((u8*)(&save_data.fc_save) , (u8*)(&fc_effect), sizeof(fc_effect_t));
    syscfg_write(CFG_USER_LED_LEDGTH_DATA, (u8 *)(&save_data), sizeof(save_flash_t));

}
#else

// static uint32_t save_user_data_area3_timeout_p100ms_count = 0;  // 写FLASH超时自动保存计数
static uint32_t save_user_data_area3_timeout_p10ms_count = 0;  // 写FLASH超时自动保存计数

void save_user_data_area3(void)
{
    // save_user_data_area3_timeout_p100ms_count = 30;
    save_user_data_area3_timeout_p10ms_count = 30;
}

void save_user_data_area3_deal(void)
{
    // if (save_user_data_area3_timeout_p100ms_count && --save_user_data_area3_timeout_p100ms_count == 0)
    if (save_user_data_area3_timeout_p10ms_count && --save_user_data_area3_timeout_p10ms_count == 0)
    {
        printf("%s\n", __func__);

        save_flash_t save_data;
        save_data.header = HEADER_VALUE;
        save_data.ble_state = ble_state;
        save_data.rf433_code = RF433_CODE;
        save_data.rf433_code2 = RF433_CODE2;
        memcpy((u8 *)(&save_data.fc_save), (u8 *)(&fc_effect), sizeof(fc_effect_t));
        syscfg_write(CFG_USER_LED_LEDGTH_DATA, (u8 *)(&save_data), sizeof(save_flash_t));
        printf("rgbw on_off: l%d , n%d\n", save_data.fc_save.last_rgbw_on_off_flag, save_data.fc_save.rgbw_on_off_flag);
        printf("led  on_off: l%d , n%d\n", save_data.fc_save.last_led_on_off_flag, save_data.fc_save.led_on_off_flag);
        printf("led1 on_off: l%d , n%d\n", save_data.fc_save.last_led1_on_off_flag, save_data.fc_save.led1_on_off_flag);
    }
}
#endif


void read_user_data_area3(void)
{

}
















