#ifndef ESC_INIT_H
#define ESC_INIT_H

#include "stdint.h"


#define ESC_BIT_0     		28
#define ESC_BIT_1     		56
#define MOTOR_BITLENGTH 	79
#define ESC_DATA_BUF_LEN	18

void esc_init(void);	//�����ʼ��

void set_motor(const uint16_t value); //�������ݵ����

#endif //!ESC_INIT_H
