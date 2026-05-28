[English](#stm32-iot-project) | 中文
---
# STM32 物联网项目

> STM32F103C8T6 + ESP8266 + OneNet MQTT + 微信小程序
> 温湿度采集 · OLED 实时显示 · 远程 LED 控制

---

## 功能简介

- **采集**：DHT11 传感器采集温度 / 湿度数据
- **显示**：0.96 寸 OLED 实时显示温湿度及 LED 状态
- **上云**：ESP8266 通过 MQTT 协议将数据上报至 OneNet 平台
- **远控**：微信小程序实时查看温湿度，并远程控制板载 LED 亮灭

---

## 硬件清单

| 硬件 | 说明 / 接线 |
|------|------------|
| STM32F103C8T6 | 主控芯片（Blue Pill），72MHz |
| DHT11 | 温湿度传感器，OUT → PA0 |
| ESP8266-01S | WiFi 模块，RX → PA2，TX → PA3 |
| 0.96 寸 OLED | I²C 显示屏，SCL → PB1，SDA → PB0 |
| 板载 LED | PC13，低电平亮 |
| USB-TTL / STLink | 烧录调试 |

---

## 系统架构

```
【上行】DHT11 → STM32 → OLED显示 → ESP8266 → OneNet → 微信小程序
【下行】微信小程序 → OneNet REST API → MQTT推送 → ESP8266 → STM32 → LED
```

---

## 软件工具链

| 工具 | 用途 |
|------|------|
| STM32CubeMX | 外设图形化配置，生成 HAL 初始化代码 |
| VSCode + EIDE 插件 | 代码编写、ARM GCC 编译、烧录 |
| 微信开发者工具 | 小程序开发、预览、上传 |
| OneNet 控制台 | 设备管理、物模型配置、API 调试 |

---

## 目录结构

```
.
├── Core/                   # 业务逻辑代码（main.c、DHT11、OLED、ESP8266 驱动）
├── Drivers/                # STM32 HAL 库
├── u8g2/                   # OLED 图形库
├── cmake/                  # CMake 工具链配置
├── CMakeLists.txt          # 构建入口
├── CMakePresets.json       # 编译预设
├── STM32F103XX_FLASH.ld    # 链接脚本
├── startup_stm32f103xb.s  # 启动文件
├── 1_led.ioc               # STM32CubeMX 工程文件
├── .clang-format           # 代码格式规范
└── README.md
```

---

## 快速开始

### 1. 克隆仓库

```bash
git clone https://github.com/你的用户名/项目名.git
cd 项目名
```

### 2. 配置信息

在esp8266.h中填写个人信息

```c
#define WIFI_SSID        "你的WiFi名称"
#define WIFI_PASSWORD    "你的WiFi密码"
#define ONENET_HOST      "mqtts.heclouds.com"
#define ONENET_PORT      1883
#define ONENET_PRODUCT_ID "你的产品ID"
#define ONENET_DEVICE_ID  "你的设备名"
#define ONENET_TOKEN      "你的MQTT Token"
```


### 3. 编译烧录

用 VSCode + EIDE 插件打开工程，选择对应编译预设，点击编译并烧录到开发板。

---

## OneNet 平台配置

1. 登录 [OneNet 控制台](https://open.iot.10086.cn)，新建产品（协议选 MQTT）
2. 在物模型中添加以下属性：

| 标识符 | 类型 | 权限 |
|--------|------|------|
| `temperature` | int32 | 只读 |
| `humidity` | int32 | 只读 |
| `led` | bool | 读写 |

3. 创建设备，使用 Token 工具生成 MQTT 接入 Token，填入 `config.h`

---

## 微信小程序

小程序通过 OneNet REST API 与设备交互：

---

## 关键设计说明

| 设计点 | 方案 | 原因 |
|--------|------|------|
| MQTT 接入 | 手动 TCP + MQTT 包 | 兼容所有 ESP8266 固件版本 |
| UART 接收 | 中断 + 静态缓冲区 | 不阻塞主程序 |
| DHT11 时序保护 | 关全局中断 | 防止中断打乱微秒级时序 |
| 主循环架构 | 超级循环 + 非阻塞计时 | 简单可靠，LED 命令响应延迟极低 |
| 上报 QoS | QoS 0 | 温湿度偶尔丢一条可接受，开销最小 |
| 订阅 QoS | QoS 1 | LED 控制命令不能丢失 |
| OLED 驱动 | 软件 I²C（PB0/PB1）| 避开 F103 硬件 I²C 已知 Bug |
| 小程序体验 | 乐观更新 | 点击即响应，无需等待 3 秒超时 |

---

## 常见问题

| 现象 | 排查方向 |
|------|----------|
| OLED 不亮 | 检查 I²C 地址（0x78），确认 PB0/PB1 已初始化为 GPIO_Output |
| DHT11 返回 1 | 检查 PA0 接线、3.3V 供电、内部上拉配置 |
| ESP8266 无响应 | 确认 TX/RX 交叉连接（MCU PA2 → ESP RX，PA3 → ESP TX） |
| WiFi 连接失败 | 确认 SSID 和密码正确，避免使用中文 SSID |
| MQTT 连接失败 | 检查 Token 是否过期（et 时间戳），确认 Host/Port |
| 小程序鉴权失败 | REST API 的 Token version 必须为 `2022-05-01` |
| 手机端不显示数据 | 检查合法域名配置，配置后须重新上传小程序 |

---

## 后续扩展方向

- **硬件**：添加光照、CO₂、气压传感器；继电器控制；蜂鸣器报警
- **软件**：断线自动重连；MQTT 心跳保活；历史数据折线图；OTA 固件升级
- **架构**：引入 FreeRTOS 多任务；DMA + 空闲中断替代 RXNE 中断；WebSocket 实时推送

---

## License

MIT
---
# STM32 IoT Project

> STM32F103C8T6 + ESP8266 + OneNet MQTT + WeChat Mini Program  
> Temperature & Humidity Sensing · OLED Real-time Display · Remote LED Control

---

## Features

- **Sensing**: DHT11 sensor collects temperature and humidity data
- **Display**: 0.96" OLED shows real-time temperature, humidity, and LED status
- **Cloud**: ESP8266 reports data to OneNet platform via MQTT protocol
- **Remote Control**: WeChat Mini Program monitors data in real time and remotely controls the onboard LED

---

## Hardware

| Component | Description / Wiring |
|-----------|----------------------|
| STM32F103C8T6 | Main MCU (Blue Pill), 72MHz |
| DHT11 | Temperature & humidity sensor, OUT → PA0 |
| ESP8266-01S | WiFi module, RX → PA2, TX → PA3 |
| 0.96" OLED | I²C display, SCL → PB1, SDA → PB0 |
| Onboard LED | PC13, active low |
| USB-TTL / STLink | Flashing & debugging |

---

## System Architecture

```
[Uplink]   DHT11 → STM32 → OLED → ESP8266 → OneNet → WeChat Mini Program
[Downlink] WeChat Mini Program → OneNet REST API → MQTT → ESP8266 → STM32 → LED
```

---

## Software Toolchain

| Tool | Purpose |
|------|---------|
| STM32CubeMX | GUI peripheral configuration, HAL code generation |
| VSCode + EIDE | Code editing, ARM GCC compilation, flashing |
| WeChat DevTools | Mini Program development, preview, upload |
| OneNet Console | Device management, data model config, API testing |

---

## Project Structure

```
.
├── Core/                   # Application code (main.c, DHT11, OLED, ESP8266 drivers)
├── Drivers/                # STM32 HAL library
├── u8g2/                   # OLED graphics library
├── cmake/                  # CMake toolchain configuration
├── CMakeLists.txt          # Build entry point
├── CMakePresets.json       # Build presets
├── STM32F103XX_FLASH.ld    # Linker script
├── startup_stm32f103xb.s  # Startup file
├── 1_led.ioc               # STM32CubeMX project file
├── .clang-format           # Code formatting rules
└── README.md
```

---

## Getting Started

### 1. Clone the repository

```bash
git clone https://github.com/your-username/your-repo.git
cd your-repo
```

### 2. Configure credentials

Fill in your personal information in `esp8266.h`:

```c
#define WIFI_SSID         "Your_WiFi_SSID"
#define WIFI_PASSWORD     "Your_WiFi_Password"
#define ONENET_HOST       "mqtts.heclouds.com"
#define ONENET_PORT       1883
#define ONENET_PRODUCT_ID "Your_Product_ID"
#define ONENET_DEVICE_ID  "Your_Device_Name"
#define ONENET_TOKEN      "Your_MQTT_Token"
```

### 3. Build & Flash

Open the project in VSCode with the EIDE plugin, select the appropriate build preset, then compile and flash to the board.

---

## OneNet Platform Setup

1. Log in to [OneNet Console](https://open.iot.10086.cn) and create a new product (protocol: MQTT)
2. Add the following properties to the data model:

| Identifier | Type | Permission |
|------------|------|------------|
| `temperature` | int32 | Read-only |
| `humidity` | int32 | Read-only |
| `led` | bool | Read/Write |

3. Create a device, generate an MQTT access Token using the Token tool, and fill it into `esp8266.h`

---

## WeChat Mini Program

The Mini Program communicates with the device via OneNet REST API:

- **Fetch data**: `GET https://iot-api.heclouds.com/thingmodel/query-device-property`
- **Control LED**: `POST https://iot-api.heclouds.com/thingmodel/set-device-property`

Before releasing, add the following to the server domain whitelist at [WeChat MP Console](https://mp.weixin.qq.com) → Development → Server Domain:

```
https://iot-api.heclouds.com
```

---

## Key Design Decisions

| Design Point | Approach | Reason |
|--------------|----------|--------|
| MQTT connection | Manual TCP + raw MQTT packets | Compatible with all ESP8266 firmware versions |
| UART receive | Interrupt + static buffer | Non-blocking, doesn't stall the main loop |
| DHT11 timing protection | Disable global interrupts | Prevents ISR from disrupting µs-level timing |
| Main loop architecture | Super loop + non-blocking timer | Simple and reliable, near-zero LED response latency |
| Publish QoS | QoS 0 | Occasional data loss is acceptable for sensor readings |
| Subscribe QoS | QoS 1 | LED control commands must not be lost |
| OLED driver | Software I²C (PB0/PB1) | Avoids known hardware I²C bug on STM32F103 |
| Mini Program UX | Optimistic update | Instant UI response without waiting for 3s timeout |

---

## Troubleshooting

| Symptom | What to Check |
|---------|---------------|
| OLED blank | Verify I²C address (0x78), confirm PB0/PB1 initialized as GPIO_Output |
| DHT11 returns 1 | Check PA0 wiring, 3.3V power, internal pull-up config |
| ESP8266 no response | Confirm TX/RX are crossed (MCU PA2 → ESP RX, PA3 → ESP TX) |
| WiFi connection fails | Check SSID and password, avoid Chinese SSID |
| MQTT connection fails | Check Token expiry (et timestamp), verify Host/Port |
| Mini Program auth error | REST API Token version must be `2022-05-01` |
| No data on mobile | Check server domain config; re-upload Mini Program after any changes |

---

## Future Extensions

- **Hardware**: Add light, CO₂, barometric sensors; relay control; buzzer alerts
- **Software**: Auto reconnect on disconnect; MQTT keepalive heartbeat; historical data charts; OTA firmware update
- **Architecture**: FreeRTOS multi-tasking; DMA + idle interrupt to replace RXNE interrupt; WebSocket real-time push

---

## License

MIT
