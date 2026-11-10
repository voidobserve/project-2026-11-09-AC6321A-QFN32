
#ifndef save_flash_h
#define save_flash_h
#include "led_strand_effect.h"

typedef enum
{
    FRIST_BYTE,         //第一次上电标志
    LED_LEDGTH_MS,      //灯带长度高8位
    LED_LEDGTH_LS,      //灯带长度低8位
}FLASH_BYTE_FLAG;

#define HEADER_VALUE    (0xAA)  // 每次更新软件时应修改这个，让升级后第一次清空除遥控外的记忆

#pragma pack (1)
typedef struct 
{
    unsigned char header;           //头部
    fc_effect_t fc_save;
     int8_t ble_state;
     u32 rf433_code;
     u32 rf433_code2;
}save_flash_t;
#pragma pack ()

void save_user_data_area3(void);
void save_user_data_area3_deal(void);

#endif