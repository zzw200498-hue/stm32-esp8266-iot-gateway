#include "stm32f10x.h"
#include "Delay.h"
#include <string.h>
#include "Serial.h"     /* 调试日志：USART2，PA2=TX，115200（PA9 损坏后的替代口） */
#include "Serial2.h"    /* ESP8266：USART3，PB10=TX / PB11=RX，115200 */
#include "ESP8266.h"
#include "OLED.h"
#include "DHT11.h"
#include "Motor.h"

#include "config.h"     /* 私有配置：WiFi/IP 等（模板见 config.h.example，不上传） */

/* ====== 温控风扇分档阈值（单位：℃），按手感调 ====== */
#define TEMP_LEVEL_OFF  28  /* 低于此温度：停转 */
#define TEMP_LEVEL_LOW  30  /* 28~29：低速 */
#define TEMP_LEVEL_MID  32  /* 30~31：中速，>=32：高速 */

/*
 * 函    数：根据温度计算风扇档位
 * 参    数：temp_int 温度整数部分
 * 返 回 值：PWM 占空比 0 / 40 / 70 / 100
 * 说    明：只取整数部分判断，简单可靠；以后要更平滑可以加上小数部分
 */
static uint8_t Fan_CalcSpeed(uint8_t temp_int)
{
    if (temp_int < TEMP_LEVEL_OFF)
    {
        return 0;       /* 停转 */
    }
    if (temp_int < TEMP_LEVEL_LOW)
    {
        return 40;      /* 低速 */
    }
    if (temp_int < TEMP_LEVEL_MID)
    {
        return 70;      /* 中速 */
    }
    return 100;         /* 高速 */
}

/*
 * 函    数：把风扇档位变成 OLED 上显示的字符串
 * 参    数：fan_speed 占空比 0~100
 * 说    明：直接写 OLED 第 3 行
 */
static void Fan_ShowOnOLED(uint8_t fan_speed)
{
    if (fan_speed == 0)
    {
        OLED_ShowString(3, 1, "Fan: OFF      ");
    }
    else if (fan_speed <= 40)
    {
        OLED_ShowString(3, 1, "Fan: LOW      ");
    }
    else if (fan_speed <= 70)
    {
        OLED_ShowString(3, 1, "Fan: MID      ");
    }
    else
    {
        OLED_ShowString(3, 1, "Fan: HIGH     ");
    }
}

int main(void)
{
    uint8_t res;                        /* 各环节返回值：0 = 成功 */
    uint8_t net_ok = 0;                 /* 联网标志：1 = TCP 已连上服务器 */
    uint8_t read_count = 0;             /* 采样计数器，控制上传频率 */
    uint8_t fan_speed;
    uint8_t manual_mode = 0;            /* 1 = 服务器手动控制风扇，0 = 自动温控 */
    uint8_t manual_speed = 0;           /* 手动模式下的风扇占空比 */
    uint8_t humi_int, humi_dec, temp_int, temp_dec;
    char send_buf[64];                  /* 上传 JSON 的拼接缓冲 */
    char cmd_buf[16];                   /* 服务器下行命令缓冲 */

    OLED_Init();
    Serial_Init();          /* 调试串口 PA2@115200 */
    Serial2_Init();         /* ESP8266 串口 PB10/PB11@115200 */
    DHT11_Init();
    Motor_Init();           /* 风扇电机：方向 PA4/PA5 + 调速 PA0(PWM) */
    Delay_ms(1500);         /* 等 DHT11 和 ESP8266 上电稳定 */

#if ENABLE_ESP
    /* ---- 联网：失败不卡死，主循环照常采集显示 ---- */
    OLED_ShowString(1, 1, "connecting...  ");
    res = ESP_Init(WIFI_NAME, WIFI_PASSWORD, SERVER_IP, SERVER_PORT);
    if (res == 0)
    {
        net_ok = 1;
        Serial_Printf("ESP init OK, TCP connected\r\n");
    }
    else
    {
        /* res 是失败步骤编号：1=AT 2=CWMODE 3=CWJAP 4=CIPSTART */
        Serial_Printf("ESP init FAIL, dead step=%d\r\n", (int)res);
    }
#endif

    while (1)
    {
        res = DHT11_ReadData(&humi_int, &humi_dec, &temp_int, &temp_dec);

        /* ---- 下行通道：先查有没有服务器命令要执行 ---- */
#if ENABLE_ESP
        if (net_ok == 1 && ESP_GetCommand(cmd_buf, sizeof(cmd_buf)) == 1)
        {
            Serial_Printf("cmd recv: %s\r\n", cmd_buf);
            if (strcmp(cmd_buf, "FAN=ON") == 0)
            {
                manual_mode = 1;        /* 强制开：手动模式，70% 风力 */
                manual_speed = 70;
            }
            else if (strcmp(cmd_buf, "FAN=OFF") == 0)
            {
                manual_mode = 1;        /* 强制关：手动模式，停转 */
                manual_speed = 0;
            }
            else if (strcmp(cmd_buf, "FAN=AUTO") == 0)
            {
                manual_mode = 0;        /* 恢复自动温控 */
            }
        }
#endif

        if (res == 0)
        {
            OLED_ShowString(1, 1, "Temp:  . C");
            OLED_ShowNum(1, 6, temp_int, 2);
            OLED_ShowNum(1, 9, temp_dec, 1);
            OLED_ShowString(2, 1, "Humi:  . %");
            OLED_ShowNum(2, 6, humi_int, 2);
            OLED_ShowNum(2, 9, humi_dec, 1);

            /* 温控核心：手动模式听服务器的，自动模式听温度的 */
            if (manual_mode == 1)
            {
                fan_speed = manual_speed;
            }
            else
            {
                fan_speed = Fan_CalcSpeed(temp_int);
            }
            Motor_SetSpeed((int8_t)fan_speed);
            Fan_ShowOnOLED(fan_speed);
            if (manual_mode == 1)
            {
                OLED_ShowString(3, 14, "MAN");    /* 第 3 行末尾提示手动接管 */
            }
            else
            {
                OLED_ShowString(3, 14, "   ");
            }

            /* 网络状态常显在第 4 行 */
#if ENABLE_ESP
            if (net_ok == 1)
            {
                OLED_ShowString(4, 1, "NET OK         ");
            }
            else
            {
                OLED_ShowString(4, 1, "NET FAIL step: ");
                OLED_ShowNum(4, 15, res, 1);
            }
#endif

            Serial_Printf("T=%d.%dC H=%d.%d%% fan=%d\r\n",
                          (int)temp_int, (int)temp_dec,
                          (int)humi_int, (int)humi_dec, (int)fan_speed);
        }
        else
        {
            /* 读数失败时风扇维持上一档，避免传感器抖动导致风扇乱跳 */
            OLED_ShowString(4, 1, "DHT11 ERR:  ");
            OLED_ShowNum(4, 11, res, 1);
            Serial_Printf("DHT11 read FAIL, code=%d\r\n", (int)res);
            Delay_ms(1500);
            continue;       /* 这轮数据无效，不上传 */
        }

        /* ---- 定期上传：每 SEND_PERIOD 次（约 4.5 秒）发一次 JSON ---- */
#if ENABLE_ESP
        read_count = read_count + 1;
        if (read_count >= SEND_PERIOD)
        {
            read_count = 0;
            if (net_ok == 1)
            {
                /* uint8_t 传给 sprintf 要先转 int，避免变参取错 */
                sprintf(send_buf, "{\"temp\":%d.%d,\"humi\":%d.%d,\"fan\":%d}",
                        (int)temp_int, (int)temp_dec,
                        (int)humi_int, (int)humi_dec, (int)fan_speed);

                if (ESP_SendData(send_buf) == 0)
                {
                    Serial_Printf("send OK: %s\r\n", send_buf);
                }
                else
                {
                    Serial_Printf("send FAIL: %s\r\n", send_buf);
                }
            }
        }
#endif

        Delay_ms(1500);     /* DHT11 采样间隔必须大于 1 秒 */
    }
}
