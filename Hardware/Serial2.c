#include "stm32f10x.h"                  // Device header
#include "Serial2.h"
#include "ESP8266.h"

/*
 * 接收缓冲区：由 USART2 接收中断写入，供 AT 驱动匹配应答使用。
 * volatile：该变量在中断中修改、主循环中读取。
 */
char esp_rx_buffer[ESP_RX_BUFFER_SIZE];
volatile uint16_t esp_rx_len = 0;

/*
 * 初始化 USART2：PA2=TX（复用推挽），PA3=RX（浮空输入），
 * 115200 8N1，使能接收中断。
 * 注意：USART2 挂载在 APB1 总线（USART1 才是 APB2）。
 */
void Serial2_Init(void)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	USART_InitTypeDef USART_InitStructure;
	USART_InitStructure.USART_BaudRate = 115200;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_Init(USART2, &USART_InitStructure);

	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);

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
		/* 等待发送寄存器空闲 */
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

/*
 * USART2 接收中断：将收到的字节追加到 esp_rx_buffer，
 * 并保持 '\0' 结尾以便 strstr 匹配；缓冲区写满后丢弃后续字节。
 */
void USART2_IRQHandler(void)
{
	if (USART_GetITStatus(USART2, USART_IT_RXNE) == SET)
	{
		uint8_t byte = USART_ReceiveData(USART2);

		if (esp_rx_len < ESP_RX_BUFFER_SIZE - 1)
		{
			esp_rx_buffer[esp_rx_len] = byte;
			esp_rx_len = esp_rx_len + 1;
			esp_rx_buffer[esp_rx_len] = '\0';
		}

		USART_ClearITPendingBit(USART2, USART_IT_RXNE);
	}
}
