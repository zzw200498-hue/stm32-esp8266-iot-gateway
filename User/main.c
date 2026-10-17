#include "stm32f10x.h"
#include "Delay.h"
#include "Serial.h"     /* USART1：调试日志输出 */
#include "Serial2.h"    /* USART2：连接 ESP8266 */
#include "ESP8266.h"
#include "OLED.h"

/* ====== 运行环境配置：编译前按实际环境修改 ====== */
#define WIFI_NAME      "your-wifi-ssid"
#define WIFI_PASSWORD  "your-password"
#define SERVER_IP      "192.168.1.100"   /* Linux 服务器的局域网地址 */
#define SERVER_PORT    9999

int main(void)
{
	uint8_t res;

	OLED_Init();
	Serial_Init();      /* 初始化调试串口（USART1） */
	Serial2_Init();     /* 初始化 ESP8266 通信串口（USART2） */
	Delay_ms(1000);     /* 等待 ESP8266 上电稳定 */

	OLED_ShowString(1, 1, "connecting...  ");
	res = ESP_Init(WIFI_NAME, WIFI_PASSWORD, SERVER_IP, SERVER_PORT);

	if (res == 0)
	{
		OLED_ShowString(1, 1, "INIT OK        ");
		while (1)
		{
			if (ESP_SendData("Hello Server!") == 0)
			{
				OLED_ShowString(3, 1, "sent OK    ");
			}
			else
			{
				OLED_ShowString(3, 1, "send FAIL  ");
			}
			Delay_ms(5000);
		}
	}
	else
	{
		/* res 为失败步骤编号：1=AT 2=CWMODE 3=CWJAP 4=CIPSTART */
		OLED_ShowString(1, 1, "INIT FAIL      ");
		OLED_ShowString(2, 1, "dead step:");
		OLED_ShowNum(2, 11, res, 1);
		while (1)
		{
		}
	}
}
