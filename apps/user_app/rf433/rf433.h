#ifndef rf_433_h
#define rf_433_h

#include "board_ac632n_demo_cfg.h"


typedef struct {
	bool RFStudyFlag;
	u32 RFData;
	u32 RFAddrCode1;
	u32 RFAddrCode2;
	u32 RFAddrCodeLast;
	u8 RFCodeInit;
	u8 RFStudyCount;
	u8 RFKeyTableNum;
	u8 RFKeyTableNum2;
	u8 RFKeyValue;
} sys_cb_t;


extern void rf433_gpio_init(void);



#endif







