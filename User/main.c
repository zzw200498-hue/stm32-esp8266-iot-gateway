#include "stm32f10x.h"
#include "Delay.h"
#include "Serial.h"     // 串口1，调试打印（必须初始化，原因见下）
#include "Serial2.h"    // 串口2，连ESP8266
#include "ESP8266.h"
#include "OLED.h"

/* ====== 按自己的环境修改这里 ======
 * 注意：WiFi 密码请填占位符或本地修改，不要把真实密码提交到公开仓库！ */
#define WIFI_NAME      "your-wifi-ssid"
#define WIFI_PASSWORD  "your-password"
#define SERVER_IP      "192.168.0.186"   /* Linux 服务器的局域网 IP */
#define SERVER_PORT    9999

/*
 * 重要：Serial_Init() 不能省！
 * ESP8266.c 里每一步成功/失败都会调 Serial_SendString 打日志，
 * 如果 USART1 没初始化（时钟没开），Serial_SendByte 里的
 * "等 TXE 标志"会永远等不到 → 程序在第一次打日志时就卡死。
 * 这就是之前"收到 AT OK 之后就没动静"的根因。
 * （USART1 的 PA9/PA10 现在虽然没接线，打印发不出去也无所谓，
 *   但时钟必须开着，让 TXE 标志正常工作。）
 */
int main(void)
{
	uint8_t res;

	OLED_Init();
	Serial_Init();      /* USART1 时钟必须开，否则打印卡死（见上） */
	Serial2_Init();
	Delay_ms(1000);     /* 等 ESP8266 上电稳定 */

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
		/* res = 死在第几步（1=AT 2=CWMODE 3=CWJAP 4=CIPSTART） */
		OLED_ShowString(1, 1, "INIT FAIL      ");
		OLED_ShowString(2, 1, "dead step:");
		OLED_ShowNum(2, 11, res, 1);
		while (1)
		{
			/* 停在这里，屏幕上能看到死因 */
		}
	}
}
