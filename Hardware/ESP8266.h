#ifndef __ESP8266_H
#define __ESP8266_H

#include "stm32f10x.h"

/* 接收缓冲区大小：ESP 的回复有的很长（比如 CWJAP 的连排状态），给大一点 */
#define ESP_RX_BUFFER_SIZE  256

/*
 * 这两个变量由 Serial2 模块"定义"（在 Serial2.c 里写定义），
 * Serial2 的接收中断往里存字节；ESP8266 模块只负责"读"它们来找回复。
 */
extern char esp_rx_buffer[ESP_RX_BUFFER_SIZE];
extern volatile uint16_t esp_rx_len;

/*
 * ESP_SendCmd —— 发一条 AT 指令并等回复（教学示范，已写完，可直接用）
 * 参数：cmd        指令文本（不要带 \r\n，函数内部自动补）
 *       expect     期待在回复里出现的关键字（如 "OK"、"CONNECT"）
 *       timeout_ms 最长等待时间（毫秒），超时就放弃
 * 返回：0 = 等到了关键字；1 = 超时没等到
 */
uint8_t ESP_SendCmd(const char *cmd, const char *expect, uint16_t timeout_ms);

/*
 * ESP_Init —— 总装函数（你来写）
 * 依次完成：AT 握手 → 设 STA 模式 → 连 WiFi → 连 TCP 服务器
 * 每一步失败都通过 USART1 打印是哪步挂了，方便你调试
 * 返回：0 = 四步全部成功；1 = 有步骤失败
 */
uint8_t ESP_Init(const char *wifi_name, const char *wifi_password,
                 const char *server_ip, uint16_t server_port);

/*
 * ESP_SendData —— 往服务器发一段数据（你来写）
 * 流程：发 AT+CIPSEND=<长度> → 等 ">" → 发数据本体
 * 返回：0 = 成功；1 = 失败
 */
uint8_t ESP_SendData(const char *data);

#endif
