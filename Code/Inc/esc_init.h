#ifndef ESC_INIT_H
#define ESC_INIT_H

#include "stdint.h"


#define ESC_BIT_0     		28
#define ESC_BIT_1     		56
#define MOTOR_BITLENGTH 	79
#define ESC_DATA_BUF_LEN	18

void esc_init(void);	//电调初始化

void set_motor(const uint16_t value); //发送数据到电调

#endif //!ESC_INIT_H
