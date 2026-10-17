#ifndef __ESP8266_H
#define __ESP8266_H

#include "stm32f10x.h"

/* AT 应答接收缓冲区大小 */
#define ESP_RX_BUFFER_SIZE  256

/* 接收缓冲区：Serial2.c 中定义，由 USART2 接收中断写入，本模块读取匹配应答 */
extern char esp_rx_buffer[ESP_RX_BUFFER_SIZE];
extern volatile uint16_t esp_rx_len;

/*
 * 发送一条 AT 指令并等待应答关键字。
 * 参数：cmd        指令内容（不含 CRLF，内部自动追加）
 *       expect     应答中必须出现的关键字
 *       timeout_ms 超时时间（毫秒）
 * 返回：0 匹配成功，1 超时
 */
uint8_t ESP_SendCmd(const char *cmd, const char *expect, uint16_t timeout_ms);

/*
 * 初始化 ESP8266：AT 握手 → STA 模式 → 连接 WiFi → 连接 TCP 服务器。
 * 返回：0 全部成功；1~4 为失败步骤编号（1=AT 2=CWMODE 3=CWJAP 4=CIPSTART）
 */
uint8_t ESP_Init(const char *wifi_name, const char *wifi_password,
                 const char *server_ip, uint16_t server_port);

/*
 * 通过已建立的 TCP 连接发送数据（AT+CIPSEND 两段式）。
 * 返回：0 成功，1 失败
 */
uint8_t ESP_SendData(const char *data);

#endif
