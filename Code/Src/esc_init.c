#include "esc_init.h"
#include "sys.h"
#include "delay.h"

//��̬��������
static uint16_t esc_data_dma[ESC_DATA_BUF_LEN] = {0};

//��̬��������
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
 * @brief	�����ʼ��
 * @param	��
 * @retval	��
*/
void esc_init(void) {
	
	uint16_t i = 0;
	
	bsp_deinit();
	bsp_clk_init();
	bsp_gpio_init();
	bsp_tim_init();
	bsp_pwm_init();
	bsp_dma_init();	
	
	TIM_Cmd(TIM4, ENABLE);	//������ʱ��
	
	delay_ms(1000);	//�ȴ����ʶ���ź�
	
	while (i++ < 220) {	//��������0��������2�룬�������
		set_motor(0);
		delay_ms(10);
	}
}


/**
* @brief	������������
 * @param	��
 * @retval	��
*/
void set_motor(const uint16_t value) {
	
	dshoot_decode(value);	//���ñ��뺯�������������ݽ��б���
	
	dma_tx_en();	//����DMA����
	//ע����������ֻ��һ�������ÿ�η��ͼ��10ms
	//������ʱ��С��DMA�޷���������һ֡���ݣ���Ҫ������һ������
	//��Ҫ��������һ�´�����DMA�����꣬���50us
	//������DMA��������ж����洦��
}

/**
 * @brief	DShoot����
* @param	��Ҫ���͵�����
 * @retval	��
*/
static void dshoot_decode(const uint16_t val) {
	
	uint16_t value = val;
	uint8_t i;
	
	value = (value > 2047)? 2048 : value;//�޷��жϣ�������ֳ���2048���������
	value = dshoot_crc(value, 0);	//���ü���У��������
	
	for (i = 0; i < 16; ++i) {
		//��������������ȷ��ûһ���������������߼�1�����߼�0
		esc_data_dma[i] = ((value << i) & 0x8000)? ESC_BIT_1 : ESC_BIT_0;
	}
	
	esc_data_dma[16] = 0;	//���ڵ���ж�֡��������������������ȫ������
	esc_data_dma[17] = 0;
}


/**
 * @brief	DShootУ�����
* @param	val����Ҫ����У������ݣ�Checkbits:�Ƿ���ӻش�
 * @retval	У��������
*/
static uint16_t dshoot_crc(const uint16_t val, uint8_t Checkbits) {
	
	//���������������ƶ�һ��λ�ã�Ȼ������Ƿ���Ҫ�ش�������λ��λ
	uint16_t pack = (val << 1) | (Checkbits? 1 : 0);	
	
	uint16_t sum = 0;
	uint16_t tmp = pack;
	
	for (int i = 0; i < 3; i++) {	//ֻ��12λ���ݣ����Խ���Ҫ3��
		sum ^= tmp;	//ʹ�����У��
		tmp >>= 4;	//ÿ�������ƶ�4λ
	}
	
	pack = (pack << 4) | (sum & 0xf); //���������������ƶ�4λ������У��λ
	
	return pack;
}


/**
 * @brief	��ʼ������ʱ��
 * @param	��
 * @retval	��
*/
static void bsp_clk_init(void) {
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
}


/**
 * @brief	��ʼ��GPIO
 * @param	��
 * @retval	��
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
 * @brief	��ʼ����ʱ��
 * @param	��
 * @retval	��
*/
static void bsp_tim_init(void) {
	
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStruct;
	
	TIM_TimeBaseInitStruct.TIM_ClockDivision	= TIM_CKD_DIV1;
	TIM_TimeBaseInitStruct.TIM_CounterMode		= TIM_CounterMode_Up;
	TIM_TimeBaseInitStruct.TIM_Period			= MOTOR_BITLENGTH;
	TIM_TimeBaseInitStruct.TIM_Prescaler		= 2;
	TIM_TimeBaseInit(TIM4, &TIM_TimeBaseInitStruct);
	
	TIM_DMACmd(TIM4, TIM_DMA_CC1, ENABLE); //һ���ǵô�TIM��DMA����
	TIM_Cmd(TIM4, DISABLE);	//Ĭ�ϳ�ʼ����������ʱ��
}

/**
 * @brief	��ʼ����ʱ��pwm���
 * @param	��
 * @retval	��
*/
static void bsp_pwm_init(void) {
	
	TIM_OCInitTypeDef TIM_OCInitStruct;
	
	TIM_OCInitStruct.TIM_OCMode			= TIM_OCMode_PWM1;
	TIM_OCInitStruct.TIM_OutputState	= TIM_OutputState_Enable;
	TIM_OCInitStruct.TIM_OCPolarity		= TIM_OCPolarity_High;
	TIM_OCInitStruct.TIM_Pulse			= 0;
	TIM_OC1Init(TIM4, &TIM_OCInitStruct);
	
	TIM_OC1PreloadConfig(TIM4, TIM_OCPreload_Enable); //ʹ��Ԥװ��
	
}

/**
 * @brief	��ʼ��DMA
 * @param	��
 * @retval	��
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
	
	DMA_Cmd(DMA1_Channel1, DISABLE);	//Ĭ�Ϲر�DMA����
}


/**
* @brief	ʹ��DMA����
 * @param	��
 * @retval	��
*/
static void dma_tx_en(void) {
	
	DMA_Cmd(DMA1_Channel1, DISABLE);	//�ر�DMA
	DMA_SetCurrDataCounter(DMA1_Channel1, ESC_DATA_BUF_LEN);	//������д��Ҫ���͵���������
	DMA_Cmd(DMA1_Channel1, ENABLE);		//��DMA���Ϳ�������������
}


static void bsp_deinit(void) {
	
	GPIO_DeInit(GPIOB);
	TIM_DeInit(TIM4);
	DMA_DeInit(DMA1_Channel1);
}

