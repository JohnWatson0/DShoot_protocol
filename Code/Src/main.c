#include "main.h"


int main(void) {
	
	uint8_t  flag = 1;
	uint16_t val = 50;
	
	delay_init();
	esc_init();
	
	while (1) { //����д��һ���򵥲��ԣ������Զ���������
		if (flag) val++;
		else 	  val--;
		
		if (val > 240) flag = 0;
		if (val <  50) flag = 1; 
		
		set_motor(val);
		delay_ms(10);
	}
} 

