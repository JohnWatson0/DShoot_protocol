#include "main.h"


int main(void) {
	
	uint8_t  flag = 1;
	uint16_t val = 50;
	
	delay_init();
	esc_init();
	
	while (1) { //这里写了一个简单测试，用于自动增减油门
		if (flag) val++;
		else 	  val--;
		
		if (val > 240) flag = 0;
		if (val <  50) flag = 1; 
		
		set_motor(val);
		delay_ms(10);
	}
} 

