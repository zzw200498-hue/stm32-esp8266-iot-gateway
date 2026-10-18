#include "stm32f10x.h"
#include "ESP8266.h"
#include "Serial.h"     /* USART1 调试日志输出 */
#include "Delay.h"
#include <string.h>
#include <stdio.h>

/* 接收缓冲区：Serial2.c 中定义，由 USART3 接收中断写入 */
extern char esp_rx_buffer[ESP_RX_BUFFER_SIZE];
extern volatile uint16_t esp_rx_len;

/*
 * 清空接收缓冲区。
 * 每条 AT 指令发送前调用，避免上一条指令的残留应答干扰关键字匹配。
 * 清空期间短暂关闭接收中断，防止清零操作被中断打断。
 */
static void ESP_ClearRx(void)
{
    USART_ITConfig(USART3, USART_IT_RXNE, DISABLE);
    esp_rx_len = 0;
    esp_rx_buffer[0] = '\0';
    USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);
}

/* 经 USART3 发送一个以 '\0' 结尾的字符串 */
static void ESP_SendString(const char *str)
{
    uint16_t i = 0;

    while (str[i] != '\0')
    {
        USART_SendData(USART3, (uint8_t)str[i]);
        while (USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET)
        {
            /* 等待发送寄存器空闲 */
        }
        i = i + 1;
    }
}

/* 在接收缓冲区中查找子串，找到返回 1，未找到返回 0 */
static uint8_t ESP_FindInRx(const char *target)
{
    if (strstr(esp_rx_buffer, target) != NULL)
    {
        return 1;
    }
    return 0;
}

/*
 * 发送一条 AT 指令并等待应答。
 * ESP 的应答异步到达，因此发送后在超时时间内轮询接收缓冲区匹配关键字。
 *
 * 参数：cmd        指令内容（不含 CRLF，函数内部自动追加）
 *       expect     应答中必须出现的关键字
 *       timeout_ms 超时时间（毫秒）
 * 返回：0 匹配成功，1 超时
 */
uint8_t ESP_SendCmd(const char *cmd, const char *expect, uint16_t timeout_ms)
{
    uint16_t waited_ms = 0;

    ESP_ClearRx();

    ESP_SendString(cmd);
    ESP_SendString("\r\n");     /* AT 指令必须以 CRLF 结尾 */

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

/*
 * 初始化 ESP8266：AT 握手 → STA 模式 → 连接 WiFi → 连接 TCP 服务器。
 *
 * 返回：0 全部成功；1~4 为失败步骤编号（1=AT 2=CWMODE 3=CWJAP 4=CIPSTART）
 *
 * 固件兼容性说明：
 * - 部分固件在模式未变化时对 CWMODE 回复 "no change" 而非 "OK"，两者均视为成功
 * - CWJAP 刚完成时模块短暂忙碌，立即发送下一条指令会被
 *   "busy p..." 吞掉，故连接 TCP 前延时 1 秒并重试 3 次
 */
uint8_t ESP_Init(const char *wifi_name, const char *wifi_password,
                 const char *server_ip, uint16_t server_port)
{
    /* 第 1 步：AT 握手，确认模块与串口链路正常 */
    if (ESP_SendCmd("AT", "OK", 2000) != 0)
    {
        Serial_SendString("ESP: AT failed\r\n");
        return 1;
    }
    Serial_SendString("ESP: AT OK\r\n");

    /* 第 2 步：设置为 STA 模式（客户端） */
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

    /* 第 3 步：连接 WiFi，等待获取 IP（关联与 DHCP 需要数秒） */
    char cmd[96];
    sprintf(cmd, "AT+CWJAP=\"%s\",\"%s\"", wifi_name, wifi_password);
    if (ESP_SendCmd(cmd, "WIFI GOT IP", 20000) != 0)
    {
        Serial_SendString("ESP: CWJAP failed\r\n");
        return 3;
    }
    Serial_SendString("ESP: CWJAP OK\r\n");

    /* 第 4 步：建立 TCP 连接，失败自动重试 */
    Delay_ms(1000);     /* 避开 CWJAP 之后的模块忙碌期 */

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

/*
 * 通过已建立的 TCP 连接发送数据（两段式）：
 * 1. 发送 AT+CIPSEND=<长度>，等待 ">" 提示符
 * 2. 发送数据本体——按声明的字节数原样传输，末尾不追加 CRLF
 *
 * 返回：0 成功，1 失败
 */
uint8_t ESP_SendData(const char *data)
{
    char cmd[32];
    uint16_t waited_ms = 0;

    sprintf(cmd, "AT+CIPSEND=%u", (unsigned int)strlen(data));
    if (ESP_SendCmd(cmd, ">", 2000) != 0)
    {
        Serial_SendString("ESP: CIPSEND failed\r\n");
        return 1;
    }

    ESP_ClearRx();
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

/*
 * 下行命令表：服务器可通过 TCP 连接直接发送这些字符串。
 * ESP 收到 TCP 数据后会包成 "+IPD,长度:内容" 写进接收缓冲，
 * 所以用 strstr 搜命令名即可，不用解析 +IPD 格式。
 */
static const char *const esp_cmd_table[] = {"FAN=ON", "FAN=OFF", "FAN=AUTO"};
#define ESP_CMD_COUNT 3

/*
 * 函    数：查询服务器是否下发了命令
 * 参    数：cmd_buf  命令名输出缓冲
 *           buf_size 缓冲大小
 * 返 回 值：1 收到命令（cmd_buf 里是命令名），0 没有命令
 * 说    明：找到命令后立即清空接收缓冲，防止同一条命令被重复触发。
 *           局限：若命令恰好在上传流程的清缓冲窗口到达会丢失，
 *           命令频率低时可忽略；以后要求严格可靠再改用环形缓冲。
 */
uint8_t ESP_GetCommand(char *cmd_buf, uint16_t buf_size)
{
    uint8_t i;
    uint16_t copy_len;

    for (i = 0; i < ESP_CMD_COUNT; i++)
    {
        if (ESP_FindInRx(esp_cmd_table[i]) == 1)
        {
            /* 把命中的命令名复制给调用者 */
            copy_len = 0;
            while (esp_cmd_table[i][copy_len] != '\0' && copy_len < buf_size - 1)
            {
                cmd_buf[copy_len] = esp_cmd_table[i][copy_len];
                copy_len = copy_len + 1;
            }
            cmd_buf[copy_len] = '\0';

            ESP_ClearRx();      /* 命令已取走，防止重复匹配 */
            return 1;
        }
    }
    return 0;
}
