#include "stm32f10x.h"                  // Device header
#include "ESP8266.h"

/*
 * Serial2 —— ESP8266 专用串口（USART3，PB10=TX / PB11=RX，115200 8N1）
 *
 * 原来在 USART2(PA2/PA3)，PA2 让给调试日志后 ESP 搬到 USART3。
 * 使能接收中断，应答写入 esp_rx_buffer 供 AT 驱动匹配。
 * 注意：USART3 挂载在 APB1 总线。
 */
char esp_rx_buffer[ESP_RX_BUFFER_SIZE];
volatile uint16_t esp_rx_len = 0;

void Serial2_Init(void)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	USART_InitTypeDef USART_InitStructure;
	USART_InitStructure.USART_BaudRate = 115200;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_Init(USART3, &USART_InitStructure);

	USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_Init(&NVIC_InitStructure);

	USART_Cmd(USART3, ENABLE);
}

void Serial2_SendByte(uint8_t Byte)
{
	USART_SendData(USART3, Byte);
	while (USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET)
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
 * USART3 接收中断：将收到的字节追加到 esp_rx_buffer，
 * 并保持 '\0' 结尾以便 strstr 匹配；缓冲区写满后丢弃后续字节。
 */
void USART3_IRQHandler(void)
{
	if (USART_GetITStatus(USART3, USART_IT_RXNE) == SET)
	{
		uint8_t byte = USART_ReceiveData(USART3);

		if (esp_rx_len < ESP_RX_BUFFER_SIZE - 1)
		{
			esp_rx_buffer[esp_rx_len] = byte;
			esp_rx_len = esp_rx_len + 1;
			esp_rx_buffer[esp_rx_len] = '\0';
		}

		USART_ClearITPendingBit(USART3, USART_IT_RXNE);
	}
}
