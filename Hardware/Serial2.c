#include "stm32f10x.h"                  // Device header
#include "Serial2.h"
#include "ESP8266.h"

/*
 * 接收缓冲区的"定义"放在这里（ESP8266.h 里只有 extern 声明）。
 * USART2 的接收中断往里存字节，ESP8266.c 的 strstr 来这里找回复。
 * volatile：这个变量会被中断改、主循环读，告诉编译器别自作聪明缓存它。
 */
char esp_rx_buffer[ESP_RX_BUFFER_SIZE];
volatile uint16_t esp_rx_len = 0;

/*
 * Serial2_Init —— 初始化 USART2（PA2 发送 / PA3 接收，115200，开接收中断）
 */
void Serial2_Init(void)
{
	/* 注意：USART2 挂在 APB1 总线上，不是 APB2（USART1 才是 APB2） */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

	/* PA2 = USART2 的 TX，复用推挽输出（和原来 PA9 的配法一样） */
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* PA3 = USART2 的 RX，浮空输入 */
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* 波特率必须 115200：这是 ESP8266 AT 固件的默认值 */
	USART_InitTypeDef USART_InitStructure;
	USART_InitStructure.USART_BaudRate = 115200;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;   /* 注意：要收要发，两个都要 */
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_Init(USART2, &USART_InitStructure);

	/* 开"收到一个字节就进中断"的开关 */
	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);

	/* NVIC：让中断真正能到 CPU（你写定时中断时配过，样式一样） */
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_Init(&NVIC_InitStructure);

	USART_Cmd(USART2, ENABLE);
}

void Serial2_SendByte(uint8_t Byte)
{
	USART_SendData(USART2, Byte);
	while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET)
	{
		/* 等发送寄存器腾空 */
	}
}

void Serial2_SendString(char *String)
{
	uint8_t i;

	for (i = 0; String[i] != 0; i = i + 1)
	{
		Serial2_SendByte(String[i]);
	}
}

void USART2_IRQHandler(void)
{
	if (USART_GetITStatus(USART2, USART_IT_RXNE) == SET)
	{
		uint8_t byte = USART_ReceiveData(USART2);

		if (esp_rx_len < ESP_RX_BUFFER_SIZE - 1)
		{
			esp_rx_buffer[esp_rx_len] = byte;
			esp_rx_len = esp_rx_len + 1;
			esp_rx_buffer[esp_rx_len] = '\0';   /* 随时保持字符串结尾，strstr 才能用 */
		}

		USART_ClearITPendingBit(USART2, USART_IT_RXNE);
	}
}
