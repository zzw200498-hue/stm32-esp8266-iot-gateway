# stm32-esp8266-iot-gateway

基于 STM32F103C8T6 + ESP8266 的**温控风扇物联网网关**：本地按温度自动分档调速，温湿度数据周期上报 Linux 服务器，服务器可远程下发命令反向控制风扇——完整的"上行遥测 + 下行控制"双车道。

![实物原型](docs/hardware-prototype.jpg)

## 功能

- **环境采集**：DHT11 单总线读取温湿度（自制时序驱动，含超时保护与校验）
- **本地显示**：0.96 寸 OLED 四行界面——温湿度 / 风扇档位 / 网络状态
- **温控调速**：按温度阈值分四档 PWM 调速（TIM2，20kHz，无啸叫），读数失败时风扇保持上一档
- **上行遥测**：ESP8266 + AT 指令连 WiFi/TCP，每 4.5 秒上报 JSON：`{"temp":26.7,"humi":77.0,"fan":40}`
- **下行控制**：服务器下发 `FAN=ON / FAN=OFF / FAN=AUTO`，远程强制开关风扇并交还自动模式
- **健壮设计**：联网失败不阻塞本地温控；AT 指令带超时与重试；串口日志全流程可观测

## 系统架构

```
DHT11 ──> STM32F103 ──USART3──> ESP8266 ──WiFi──> 路由器 ──> Windows(:9999)
           │  ▲             (AT指令)                     │ netsh portproxy
        OLED  PWM                                    VMware NAT 转发
        PA0/PA4/PA5                                       ▼
                                        Ubuntu VM · TCP Server :9999
```

- **固件侧（本仓库）**：标准外设库开发，USART 中断接收 + `strstr` 匹配 AT 应答；CIPSTART 失败自动重试 3 次
- **服务端侧**：Linux TCP Server（`nc -l 9999` 即可演示），规划接入 SQLite 落库与历史曲线
- **网络细节**：虚拟机走 NAT 时，Windows 宿主机用 `netsh portproxy` 将 9999 端口转发进虚拟机——ESP 无法直连 NAT 网段

## 硬件

| 器件 | 说明 |
|---|---|
| STM32F103C8T6 最小系统板 | 主控，48 引脚全资源原理图见 [docs/schematic-lceda.pdf](docs/schematic-lceda.pdf) |
| DHT11 模块 | 温湿度，单总线时序自制实现 |
| 0.96 寸 OLED（SSD1306） | 软件 I2C（PB8/PB9） |
| ESP-01S + 转接板 | 转接板板载 LDO 与电平转换 |
| TB6612FNG | 电机驱动，STBY 使能，双输入控方向 |
| 5V 直流风扇电机 | PWM 调速（PA0），方向控制（PA4/PA5） |
| AMS1117-3.3 ×2 | 逻辑 3.3V 与 ESP 供电各独立一路 |
| CH340 USB-TTL | 烧录 + PA2 调试串口日志（115200） |

原理图（立创 EDA 绘制，含 ERC 修正）与面包板接线图见 [docs/](docs/) 目录。

## 串口资源分配

| 外设 | 引脚 | 用途 |
|---|---|---|
| USART1 | ~~PA9/PA10~~ | 调试中损坏，弃用 |
| USART2 | PA2（TX，115200） | 调试日志输出 |
| USART3 | PB10/PB11（115200） | ESP8266 AT 指令通道 |
| TIM2 CH1 | PA0（PWM 20kHz） | 风扇调速 |

## 编译与烧录

1. **Keil**：打开 `Project.uvprojx`（标准外设库 V3.5 已含在仓库内）编译
2. **VSCode + CMake + arm-none-eabi-gcc**：仓库含 `CMakeLists.txt` / `cmake/` / `.vscode/`，`cmake --build build` 一键构建
3. 私有配置（WiFi/IP）在 `User/config.h`（被 .gitignore 忽略），从 `config.h.example` 复制填写

## 调试踩过的坑

- **DHT11 读出全 0 但校验"假通过"**：软件延时循环开销远大于标称 1µs，"1"位实测只计到 25~35 次，判位阈值从 40 降到 15 后解决——**软件延时的实际精度永远比标称差**
- **OLED 正常但串口无输出**：引脚丝印被系统性看偏 2 格，改用"已验证引脚做锚点"定位，最终确认 PA9 排针损坏，调试口迁移到 USART2
- **电机只响不转**：满速也带不动 = 供电不足，电机/ESP 这类大电流负载必须独立供电，不能吃稳压轨
- **ESP 发射瞬间 300mA**：供电不足表现为"一联网就复位"，独立 LDO + 电解电容是解药
- **NAT 里的虚拟机外部不可达**：Windows `netsh portproxy` 做端口转发，数据链穿透三层网络
- **AT 指令以 `\r\n` 结尾**；CWMODE 兼容 `OK` / `no change` 两种固件回复；CWJAP 后延时 1 秒规避 `busy p...` 吞指令

## 路线图

- [x] 双串口架构：调试日志（USART2）与 ESP 通道（USART3）分离
- [x] DHT11 采集 + OLED 显示 + 温控分档调速
- [x] 上行遥测：JSON 周期上报 Linux TCP Server
- [x] 下行控制：FAN=ON / OFF / AUTO 远程遥控
- [ ] 服务端 Python 常驻脚本：数据落盘 CSV + 自动重连 + 温度曲线
- [ ] TCP 断线检测与固件侧自动重连
- [ ] 平滑调速：温度到占空比的连续映射
