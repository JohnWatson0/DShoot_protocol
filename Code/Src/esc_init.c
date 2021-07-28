#include "esc_init.h"
#include "sys.h"
#include "delay.h"

//静态数组声明
static uint16_t esc_data_dma[ESC_DATA_BUF_LEN] = {0};

//静态函数声明
static void bsp_deinit(void);
static void bsp_clk_init(void);
static void bsp_gpio_init(void);
static void bsp_tim_init(void);
static void bsp_pwm_init(void);
static void bsp_dma_init(void);
static void dma_tx_en(void);
static void dshoot_decode(const uint16_t val);
static uint16_t dshoot_crc(const uint16_t val, uint8_t Checkbits);


/**
 * @brief	电调初始化
 * @param	无
 * @retval	无
*/
void esc_init(void) {
	
	uint16_t i = 0;
	
	bsp_deinit();
	bsp_clk_init();
	bsp_gpio_init();
	bsp_tim_init();
	bsp_pwm_init();
	bsp_dma_init();	
	
	TIM_Cmd(TIM4, ENABLE);	//启动定时器
	
	delay_ms(1000);	//等待电调识别到信号
	
	while (i++ < 220) {	//连续发送0上锁命令2秒，电机解锁
		set_motor(0);
		delay_ms(10);
	}
}


/**
* @brief	向电调发送数据
 * @param	无
 * @retval	无
*/
void set_motor(const uint16_t value) {
	
	dshoot_decode(value);	//调用编码函数，对油门数据进行编码
	
	dma_tx_en();	//启动DMA发送
	//注：这里由于只有一个电机，每次发送间隔10ms
	//如果间隔时间小到DMA无法发送完上一帧数据，就要进行下一个传输
	//需要在这里做一下处理，等DMA发送完，大概50us
	//或者在DMA发送完成中断里面处理
}

/**
 * @brief	DShoot编码
* @param	需要发送的数据
 * @retval	无
*/
static void dshoot_decode(const uint16_t val) {
	
	uint16_t value = val;
	uint8_t i;
	
	value = (value > 2047)? 2048 : value;//限幅判断，避免出现超过2048的情况发生
	value = dshoot_crc(value, 0);	//调用计算校验后的数据
	
	for (i = 0; i < 16; ++i) {
		//根据油门数据来确定没一个数据项里面是逻辑1还是逻辑0
		esc_data_dma[i] = ((value << i) & 0x8000)? ESC_BIT_1 : ESC_BIT_0;
	}
	
	esc_data_dma[16] = 0;	//用于电调判断帧间隔，避免出现数据线上全是数据
	esc_data_dma[17] = 0;
}


/**
 * @brief	DShoot校验计算
* @param	val：需要进行校验的数据，Checkbits:是否添加回传
 * @retval	校验后的数据
*/
static uint16_t dshoot_crc(const uint16_t val, uint8_t Checkbits) {
	
	//将油门数据往左移动一个位置，然后根据是否需要回传，将该位置位
	uint16_t pack = (val << 1) | (Checkbits? 1 : 0);	
	
	uint16_t sum = 0;
	uint16_t tmp = pack;
	
	for (int i = 0; i < 3; i++) {	//只有12位数据，所以仅需要3次
		sum ^= tmp;	//使用异或校验
		tmp >>= 4;	//每次往右移动4位
	}
	
	pack = (pack << 4) | (sum & 0xf); //将油门数据往左移动4位，并上校验位
	
	return pack;
}


/**
 * @brief	初始化外设时钟
 * @param	无
 * @retval	无
*/
static void bsp_clk_init(void) {
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
}


/**
 * @brief	初始化GPIO
 * @param	无
 * @retval	无
*/
static void bsp_gpio_init(void) {
	
	GPIO_InitTypeDef GPIO_InitStruct;
	
	GPIO_InitStruct.GPIO_Mode	= GPIO_Mode_AF_PP;
	GPIO_InitStruct.GPIO_Pin	= GPIO_Pin_6;
	GPIO_InitStruct.GPIO_Speed	= GPIO_Speed_10MHz;
	GPIO_Init(GPIOB, &GPIO_InitStruct);
	
	GPIO_ResetBits(GPIOB, GPIO_Pin_6);
}

/**
 * @brief	初始化定时器
 * @param	无
 * @retval	无
*/
static void bsp_tim_init(void) {
	
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStruct;
	
	TIM_TimeBaseInitStruct.TIM_ClockDivision	= TIM_CKD_DIV1;
	TIM_TimeBaseInitStruct.TIM_CounterMode		= TIM_CounterMode_Up;
	TIM_TimeBaseInitStruct.TIM_Period			= MOTOR_BITLENGTH;
	TIM_TimeBaseInitStruct.TIM_Prescaler		= 2;
	TIM_TimeBaseInit(TIM4, &TIM_TimeBaseInitStruct);
	
	TIM_DMACmd(TIM4, TIM_DMA_CC1, ENABLE); //一定记得打开TIM的DMA发送
	TIM_Cmd(TIM4, DISABLE);	//默认初始化不启动定时器
}

/**
 * @brief	初始化定时器pwm输出
 * @param	无
 * @retval	无
*/
static void bsp_pwm_init(void) {
	
	TIM_OCInitTypeDef TIM_OCInitStruct;
	
	TIM_OCInitStruct.TIM_OCMode			= TIM_OCMode_PWM1;
	TIM_OCInitStruct.TIM_OutputState	= TIM_OutputState_Enable;
	TIM_OCInitStruct.TIM_OCPolarity		= TIM_OCPolarity_High;
	TIM_OCInitStruct.TIM_Pulse			= 0;
	TIM_OC1Init(TIM4, &TIM_OCInitStruct);
	
	TIM_OC1PreloadConfig(TIM4, TIM_OCPreload_Enable); //使能预装载
	
}

/**
 * @brief	初始化DMA
 * @param	无
 * @retval	无
*/
static void bsp_dma_init(void) {
	
	DMA_InitTypeDef DMA_InitStruct;
	
	DMA_InitStruct.DMA_DIR		  = DMA_DIR_PeripheralDST;
	DMA_InitStruct.DMA_BufferSize = ESC_DATA_BUF_LEN;
	
	DMA_InitStruct.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStruct.DMA_MemoryInc	 = DMA_MemoryInc_Enable;
	
	DMA_InitStruct.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
	DMA_InitStruct.DMA_MemoryDataSize	  = DMA_MemoryDataSize_HalfWord;
	
	DMA_InitStruct.DMA_Mode		= DMA_Mode_Normal;
	DMA_InitStruct.DMA_Priority	= DMA_Priority_High;
	DMA_InitStruct.DMA_M2M		= DMA_M2M_Disable;
	
	DMA_InitStruct.DMA_PeripheralBaseAddr = (uint32_t)&TIM4->CCR1;
	DMA_InitStruct.DMA_MemoryBaseAddr	  = (uint32_t)&esc_data_dma;
	
	DMA_Init(DMA1_Channel1, &DMA_InitStruct);
	
	DMA_Cmd(DMA1_Channel1, DISABLE);	//默认关闭DMA传输
}


/**
* @brief	使能DMA发送
 * @param	无
 * @retval	无
*/
static void dma_tx_en(void) {
	
	DMA_Cmd(DMA1_Channel1, DISABLE);	//关闭DMA
	DMA_SetCurrDataCounter(DMA1_Channel1, ESC_DATA_BUF_LEN);	//重新填写需要发送的数据数量
	DMA_Cmd(DMA1_Channel1, ENABLE);		//打开DMA，就可以启动传输了
}


static void bsp_deinit(void) {
	
	GPIO_DeInit(GPIOB);
	TIM_DeInit(TIM4);
	DMA_DeInit(DMA1_Channel1);
}

