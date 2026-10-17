#include "stm32f10x.h"
#include "ESP8266.h"
#include "Serial.h"     /* USART1 调试打印：Serial_SendString（照你现有 Serial.h 的实际函数名改） */
#include "Delay.h"      /* Delay_ms：等待用 */
#include <string.h>
#include <stdio.h>

/* 接收缓冲区：Serial2.c 里定义，中断往里存，这里只读 */
extern char esp_rx_buffer[ESP_RX_BUFFER_SIZE];
extern volatile uint16_t esp_rx_len;

/*
 * ESP_ClearRx —— 清空接收缓冲区
 * 每发一条新指令前必须调用：防止上一条指令的回复残留在缓冲区里，
 * 导致 strstr 找到的是"旧回复"里的关键字，判断出错。
 */
static void ESP_ClearRx(void)
{
    esp_rx_len = 0;
    esp_rx_buffer[0] = '\0';
}

/*
 * ESP_SendString —— 把一个字符串经 USART2 原样发出
 * （就是你 Serial.c 里 SendString 的 USART2 版）
 */
static void ESP_SendString(const char *str)
{
    uint16_t i = 0;

    while (str[i] != '\0')
    {
        USART_SendData(USART2, (uint8_t)str[i]);
        while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET)
        {
            /* 等发送寄存器腾空，再发下一个字节 */
        }
        i = i + 1;
    }
}

/*
 * ESP_FindInRx —— 在接收缓冲区里找关键字
 * 找到返回 1，没找到返回 0
 */
static uint8_t ESP_FindInRx(const char *target)
{
    if (strstr(esp_rx_buffer, target) != NULL)
    {
        return 1;
    }
    return 0;
}

uint8_t ESP_SendCmd(const char *cmd, const char *expect, uint16_t timeout_ms)
{
    uint16_t waited_ms = 0;

    ESP_ClearRx();

    /* 发指令本体，再补 AT 指令规定的回车换行（缺了 ESP 直接不理你） */
    ESP_SendString(cmd);
    ESP_SendString("\r\n");

    /* 每毫秒查一次缓冲区，查到关键字立刻返回成功，超时返回失败 */
    while (waited_ms < timeout_ms)
    {
        if (ESP_FindInRx(expect) == 1)
        {
            return 0;
        }
        Delay_ms(1);
        waited_ms = waited_ms + 1;
    }

    return 1;
}

uint8_t ESP_Init(const char *wifi_name, const char *wifi_password,
                 const char *server_ip, uint16_t server_port)
{
    //第 1 步：AT 握手
    if(ESP_SendCmd("AT", "OK", 2000) != 0)
    {
        Serial_SendString("ESP: AT failed\r\n");
        return 1;
    }
    Serial_SendString("ESP: AT OK\r\n");
    //第 2 步：设为 STA 模式（当客户端）
    /* 有的固件在模式不变时会回 "no change" 而不带 OK，所以两个关键字都要等 */
    ESP_ClearRx();
    ESP_SendString("AT+CWMODE=1");
    ESP_SendString("\r\n");

    uint16_t waited_ms = 0;
    uint8_t got_reply = 0;
    while (waited_ms < 2000)
    {
        if (ESP_FindInRx("OK") == 1 || ESP_FindInRx("no change") == 1)
        {
            got_reply = 1;
            break;
        }
        Delay_ms(1);
        waited_ms = waited_ms + 1;
    }
    if (got_reply == 0)
    {
        Serial_SendString("ESP: CWMODE failed\r\n");
        return 2;
    }
    Serial_SendString("ESP: CWMODE OK\r\n");
    //第 3 步：连 WiFi
    char cmd[96];
    sprintf(cmd, "AT+CWJAP=\"%s\",\"%s\"", wifi_name, wifi_password);
    if(ESP_SendCmd(cmd, "WIFI GOT IP", 20000) != 0)
    {
        Serial_SendString("ESP: CWJAP failed\r\n");
        return 3;
    }
    Serial_SendString("ESP: CWJAP OK\r\n");
    //第 4 步：连 TCP 服务器
    /* 坑：CWJAP 刚结束时 ESP 内部还在忙，立刻发 CIPSTART 会收到
       "busy p..." 并把指令吞掉。所以先歇 1 秒，连不上再自动重试两次 */
    Delay_ms(1000);

    char cmd2[64];
    sprintf(cmd2, "AT+CIPSTART=\"TCP\",\"%s\",%u", server_ip, server_port);

    uint8_t connect_ok = 0;
    uint8_t try_count = 0;
    while (try_count < 3)
    {
        if (ESP_SendCmd(cmd2, "CONNECT", 10000) == 0)
        {
            connect_ok = 1;
            break;
        }
        Delay_ms(2000);
        try_count = try_count + 1;
    }
    if (connect_ok == 0)
    {
        Serial_SendString("ESP: CIPSTART failed\r\n");
        return 4;
    }
    Serial_SendString("ESP: CIPSTART OK\r\n");

    return 0;
}

uint8_t ESP_SendData(const char* data)
{
    //第 1 段：先告诉 ESP 要发多少个字节
    char cmd[32];
    uint16_t waited_ms = 0;
    sprintf(cmd, "AT+CIPSEND=%u", (unsigned int)strlen(data));
    if (ESP_SendCmd(cmd, ">", 2000) != 0)
    {
        Serial_SendString("ESP: CIPSEND failed\r\n");
        return 1;
    }

    ESP_ClearRx();
    //第 2 段：ESP_SendString(data) 把数据本体发出去
    ESP_SendString(data);

    while (waited_ms < 5000)
    {
        if (ESP_FindInRx("SEND OK") == 1)
        {
            Serial_SendString("ESP: SEND OK\r\n");
            return 0;
        }
        Delay_ms(1);
        waited_ms = waited_ms + 1;
    }
    Serial_SendString("ESP Err: Send data timeout\r\n");

    return 1;
}

