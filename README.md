# TianShu IoT Platform (天枢物联平台)

<div align="center">

[![License: GPL-3.0](https://img.shields.io/badge/License-GPL--3.0-blue.svg)](LICENSE)
[![Python: 3.10+](https://img.shields.io/badge/Python-3.10+-yellow)](https://www.python.org/)
[![Vue: 3.4+](https://img.shields.io/badge/Vue-3.4+-green)](https://vuejs.org/)
[![Flutter: 3.6+](https://img.shields.io/badge/Flutter-3.6+-blue)](https://flutter.dev/)
[![Platform: ESP32](https://img.shields.io/badge/Platform-ESP32-brightgreen)](https://www.espressif.com/en/products/socs/esp32)

**天枢是一个通用物联网平台，包含云端服务、Web 管理后台、低代码 IDE，配套开源的 ESP32 设备固件示例。**

</div>

---

## 目录

- [特性](#特性)
- [架构概览](#架构概览)
- [快速开始](#快速开始)
- [项目结构](#项目结构)
- [后端服务](#后端服务)
- [前端管理后台](#前端管理后台)
- [设备固件示例](#设备固件示例)
- [天枢 Studio 低代码 IDE](#天枢-studio-低代码-ide)
- [API 参考](#api-参考)
- [常见问题](#常见问题)
- [贡献指南](#贡献指南)
- [开源协议](#开源协议)

---

## 特性

### 通用设备接入

- **多协议支持** — HTTP/WebSocket 双通道，设备通过心跳保活，支持命令下发与数据上报
- **设备模板系统** — 预定义设备类型，统一数据点 (Data Point) 和命令 (Command) 格式
- **灵活配置** — 每台设备独立配置项（音量、阈值、模式等），以 JSON Schema 描述
- **分组管理** — 支持设备分组，批量操作与聚合监控

### AI Agent 能力

- **自定义 AI Agent** — 为设备绑定专属 AI 助手，配置系统提示词、模型参数
- **工具函数调用** — AI 可主动查询设备状态、下发控制命令、读取历史数据
- **多轮对话** — 支持完整对话上下文记忆
- **OpenAI 兼容** — Agent 后端对接 OpenAI / Claude / Mimo 等任意兼容 API

### 灵活 API 网关

- **AI 模型代理** — 配置 base_url + api_key，对接任意 OpenAI 兼容 API
- **普通 HTTP API** — 转发设备请求到外部 API，支持 JSONPath 响应提取
- **Python 处理器** — 用 Python 代码自由处理请求逻辑，无需架设额外服务
- **请求/响应转换** — 模板变量替换、自定义处理代码

### 低代码应用生成

- **拖拽式 UI 编辑器** — 可视化设计应用界面（Flutter）
- **一键代码生成** — 生成完整 Flutter 项目，含设备绑定、数据展示、控制界面
- **AI 辅助开发** — 集成 AI Agent 辅助编程

### 开源固件示例

- **ESP32 固件** — 完整的语音交互 + 云端通信示例代码，基于 ESP-IDF
- **MicroReactor 状态机** — 事件驱动的实体架构，易于扩展新功能
- **驱动组件** — I2S 音频、DHT11 传感器、OLED 显示屏等常用外设驱动

---

## 架构概览

```
┌────────────────────────────────────────────────────────────────────────────┐
│                              客户端层                                       │
├─────────────────────┬──────────────────────┬──────────────────────────────┤
│   Vue.js 管理后台     │    天枢 Studio IDE   │     Flutter 生成的 App        │
│   (Web 浏览器)        │    (Flutter Desktop) │     (移动端 / 桌面端)          │
└─────────┬───────────┴──────────┬───────────┴──────────────┬─────────────────┘
          │                      │ 自动生成                   │
          ▼                      ▼                            ▼
┌────────────────────────────────────────────────────────────────────────────┐
│                           天枢后端服务 (Flask)                              │
├──────────────┬──────────────┬──────────────┬──────────────┬─────────────────┤
│  认证 API    │  设备 API    │  AI Agent    │  API 网关    │  WebSocket      │
│  (JWT)       │  (CRUD)      │  (对话/工具)  │  (代理/处理)  │  (实时通信)      │
│  ─ 数据 API  │  ─ 模板 API  │  ─ 会话 API  │              │                 │
└──────┬───────┴──────┬───────┴──────┬───────┴──────┬───────┴────────┬────────┘
       │              │              │              │               │
       ▼              ▼              ▼              ▼               ▼
┌────────────────────────────────────────────────────────────────────────────┐
│                           设备层 (自选硬件)                                  │
├────────────────────────────────────────────────────────────────────────────┤
│  天枢 SDK / ESP32 固件示例 / 任意能发起 HTTP/WebSocket 的设备               │
│  ┌──────────────┐ ┌──────────────┐ ┌──────────────┐ ┌──────────────┐     │
│  │ 温湿度传感    │ │ 语音交互      │ │ 继电器控制    │ │ 摄像头       │     │
│  │ DHT11        │ │ ASR/TTS/AI   │ │ 开关/调光     │ │ MJPEG 流     │     │
│  └──────────────┘ └──────────────┘ └──────────────┘ └──────────────┘     │
└────────────────────────────────────────────────────────────────────────────┘
```

### 数据流

**设备上报数据：**
```
设备采集数据 (传感器/状态)
  → HTTP POST /api/v1/data/report
  → 后端写入 DataRecord 表
  → WebSocket 推送前端
  → 前端实时展示
```

**云端下发命令：**
```
用户在 Web 后台操作
  → HTTP POST /api/v1/devices/{id}/command
  → WebSocket 推送到设备
  → 设备执行并上报结果
```

**AI Agent 控制设备：**
```
用户在 Web 端与 Agent 对话
  → 后端 AI 服务处理（携带 Agent 工具函数）
  → Agent 调用 device_control / device_query 等工具
  → 工具函数通过设备 API 与设备通信
  → 结果返回 Agent 生成回复
```

---

## 快速开始

### 环境要求

| 组件 | 要求 |
|------|------|
| Python | 3.10+ |
| Node.js | 18+ |
| npm / pnpm | 最新稳定版 |
| 数据库 | SQLite（默认）、MySQL、PostgreSQL 均可 |

### 1. 后端服务

```bash
cd server

# 创建虚拟环境
python -m venv .venv
.venv\Scripts\activate      # Windows
# source .venv/bin/activate  # Linux/macOS

# 安装依赖
pip install -r requirements.txt

# 配置环境变量
cp .env.example .env
# 编辑 .env 填入你的配置（见下方配置说明）

# 初始化数据库
python init_data.py

# 启动服务
flask run --host 0.0.0.0 --port 5000
```

> 首次运行会自动创建管理员账号：`admin` / `changeme`

### 2. 前端管理后台

```bash
cd web

npm install

# 开发模式
npm run dev

# 生产构建
npm run build
```

访问 `http://localhost:5173`，默认代理请求到 `http://localhost:5000/api/v1`。

### 3. 设备接入（可选）

参考 [设备固件示例](#设备固件示例) 编译 ESP32 固件，或使用任意能发起 HTTP 请求的设备接入天枢平台。

### 4. 低代码 IDE（可选）

```bash
cd tianshu_studio
flutter pub get
flutter run -d windows
```

---

## 项目结构

```
D:\Project\IoT\
├── server/                         # Python Flask 后端
│   ├── app/
│   │   ├── __init__.py             # Flask 应用工厂
│   │   ├── config.py               # 配置管理
│   │   ├── extensions.py           # Flask 扩展
│   │   ├── models/                 # 数据库模型
│   │   │   ├── user.py             # 用户
│   │   │   ├── device.py           # 设备
│   │   │   ├── agent.py            # AI Agent
│   │   │   ├── gateway.py          # API 网关配置
│   │   │   ├── data_record.py      # 时序数据
│   │   │   ├── device_template.py  # 设备模板
│   │   │   └── log.py             # 操作日志
│   │   ├── api/v1/                 # REST API 路由
│   │   │   ├── auth.py             # 认证
│   │   │   ├── devices.py         # 设备 CRUD
│   │   │   ├── data.py            # 数据上报/查询
│   │   │   ├── agents.py          # AI Agent
│   │   │   ├── gateway.py         # API 网关
│   │   │   └── templates.py       # 设备模板
│   │   ├── services/               # 业务逻辑
│   │   │   ├── agent_service.py   # AI 对话
│   │   │   ├── agent_tools.py     # Agent 工具函数
│   │   │   ├── device_service.py  # 设备管理
│   │   │   └── api_gateway_service.py
│   │   └── websocket.py            # WebSocket 事件
│   ├── handlers/                    # API 网关 Python 处理器示例
│   ├── docs/                        # API 文档
│   ├── migrations/                 # Alembic 数据库迁移
│   ├── run.py                       # 入口
│   ├── init_data.py                # 数据库初始化
│   ├── requirements.txt
│   └── .env.example
│
├── web/                            # Vue.js 前端
│   ├── src/
│   │   ├── api/                    # API 请求封装
│   │   ├── views/                  # 页面
│   │   │   ├── Login.vue           # 登录
│   │   │   ├── Dashboard.vue       # 数据看板
│   │   │   ├── Devices.vue         # 设备列表
│   │   │   ├── DeviceDetail.vue    # 设备详情
│   │   │   ├── Gateway.vue         # API 网关
│   │   │   ├── Agents.vue          # Agent 列表
│   │   │   ├── AgentChat.vue       # Agent 对话
│   │   │   ├── DataView.vue        # 数据查看
│   │   │   ├── Templates.vue       # 设备模板
│   │   │   └── Settings.vue        # 设置
│   │   ├── router/
│   │   ├── stores/
│   │   └── components/
│   ├── package.json
│   └── vite.config.js
│
├── hardware/
│   └── desktop_dog/                 # ESP32 固件示例
│       ├── main/                    # 主程序
│       │   ├── main.c               # 入口
│       │   ├── app_config.h         # 硬件/网络配置
│       │   ├── mimo_ai.c/h         # AI 对话
│       │   ├── baidu_asr.c/h       # 语音识别
│       │   ├── baidu_tts.c/h       # 语音合成
│       │   ├── dht11.c/h           # 温湿度传感器
│       │   ├── wifi.c/h            # WiFi 连接
│       │   ├── oled_display.c/h    # OLED 驱动
│       │   └── ...
│       ├── entities/                # 实体模块（10个）
│       ├── pet/                     # 电子宠物模块（示例功能）
│       ├── MicroReactor/            # 实体状态机框架
│       └── components/              # ESP-IDF 组件
│
└── tianshu_studio/                  # Flutter 低代码 IDE
    ├── lib/
    │   ├── features/
    │   │   ├── canvas/              # 拖拽 UI 编辑器
    │   │   ├── codegen/             # 代码生成
    │   │   ├── project/             # 项目管理
    │   │   └── widgets/             # 组件库
    │   └── main.dart
    └── pubspec.yaml
```

---

## 后端服务

### 技术栈

| 技术 | 用途 |
|------|------|
| Flask 3.x | Web 框架 |
| SQLAlchemy 2.x | ORM |
| Flask-SocketIO | WebSocket 实时通信 |
| Flask-JWT-Extended | JWT 身份认证 |
| APScheduler | 定时任务 |
| Alembic | 数据库迁移 |

### 数据库模型

**User** — 用户账户，Bcrypt 密码哈希

**Device** — IoT 设备，UUID 标识，Secret Key 认证
- 关联用户、所有配置项
- 关联模板 (DeviceTemplate)
- 关联 Agent (DeviceAgent)

**DeviceTemplate** — 设备类型模板，定义：
- 数据点 (Data Point)：属性名、类型、单位
- 命令 (Command)：名称、参数定义
- 配置 Schema：JSON Schema 格式
- 通信设置：心跳间隔、上报间隔、协议类型

**DeviceConfig** — 设备运行时配置，JSON 格式存储

**Agent** — AI Agent：
- 系统提示词 (system_prompt)
- 模型名称 (gpt-4 / claude-3 等)
- 工具函数列表 (device_list / device_control / data_query 等)
- 模型参数 (temperature, max_tokens)

**ApiConfig** — API 网关配置，支持三种类型：
- `ai_model` — OpenAI 兼容 API（base_url + api_key）
- `normal_api` — 任意 HTTP API（请求转发 + JSONPath 响应提取）
- `python_handler` — Python 代码处理器

**DataRecord** — 时序数据记录

**OperationLog** — 用户操作日志

### 内置 Agent 工具函数

| 工具函数 | 功能 |
|---------|------|
| `device_list` | 列出用户所有设备及在线状态 |
| `device_control` | 向设备下发控制命令 |
| `device_query` | 查询设备最新状态 |
| `data_query` | 查询设备历史数据 |
| `config_get` | 读取设备配置项 |
| `config_set` | 修改设备配置项 |

### 配置说明 (.env)

```bash
FLASK_APP=run.py
FLASK_ENV=development
SECRET_KEY=your-secret-key          # Flask 密钥
DEBUG=True

DATABASE_URL=sqlite:///tianshu.db    # 可切换 MySQL/PostgreSQL

JWT_SECRET_KEY=your-jwt-secret
JWT_ACCESS_TOKEN_EXPIRES=3600

# 外部 AI API（Agent 后端）
OPENAI_API_KEY=
OPENAI_API_URL=https://api.openai.com/v1
CLAUDE_API_KEY=
CLAUDE_API_URL=https://api.anthropic.com

HOST=0.0.0.0
PORT=5000
```

---

## 前端管理后台

### 页面

| 页面 | 功能 |
|------|------|
| 登录 / 注册 | JWT 认证 |
| 数据看板 | 设备统计、在线率图表 |
| 设备列表 | 设备 CRUD、状态监控 |
| 设备详情 | 实时数据、配置管理、命令下发 |
| API 网关 | 外部 API / 处理器配置 |
| AI Agent | Agent 创建、配置 |
| Agent 对话 | 与 AI Agent 多轮对话 |
| 数据查看 | 历史数据曲线图 |
| 设备模板 | 设备类型模板管理 |
| 操作日志 | 审计记录 |

### 环境变量

```bash
# API 地址（留空使用开发服务器代理）
VITE_API_URL=
```

---

## 设备固件示例

位于 `hardware/desktop_dog/`，基于 ESP32-S3 的固件示例，供接入天枢平台时参考。

### 主要功能模块

- **WiFi 连接** — 自动重连，保存凭据到 NVS
- **天枢云通信** — WebSocket 心跳、HTTP 数据上报、命令接收
- **DHT11 传感器** — 定时采集温湿度上报
- **OLED 显示** — 128x64 SSD1306，文字/图形显示
- **语音交互链路** — 唤醒词 → ASR → AI → TTS（示例功能）

### 编译固件

```bash
# 安装 ESP-IDF 5.x
git clone -b v5.5.2 --recursive https://github.com/espressif/esp-idf.git
cd esp-idf && ./install.sh && . ./export.sh

cd hardware/desktop_dog
idf.py set-target esp32s3
idf.py menuconfig    # 配置 WiFi、天枢服务器地址
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

### MicroReactor 实体状态机

固件采用 MicroReactor 框架，10 个实体分工协作：

| 实体 | 功能 |
|------|------|
| System | 协调调度，OLED 显示 |
| WakeWord | 唤醒词检测 |
| Recorder | VAD 录音 |
| ASR | 语音识别 |
| AI | AI 对话 |
| TTS | 语音合成 |
| Sensor | 数据采集 |
| TianShu | 云端通信 |
| Audio | 音频播放 |
| Pet | 电子宠物（示例功能）|

> **注意：** 该固件仅作为参考实现。实际项目中请根据你的硬件选型和业务需求进行适配。

### 硬件接线

```
ESP32-S3
├── I2C OLED (SSD1306 128x64): SCL=GPIO9, SDA=GPIO8
├── DHT11: DATA=GPIO45
├── I2S 麦克风 (INMP441): WS=GPIO10, SCK=GPIO11, SD=GPIO12
├── I2S 功放 (MAX98357A): BCLK=GPIO7, LRCK=GPIO6, DIN=GPIO8
└── 按键 x4: GPIO38/39/40/41
```

---

## 天枢 Studio 低代码 IDE

基于 Flutter Desktop 的可视化开发工具：

**拖拽式 UI 编辑器** — 从组件面板拖拽到画布，设置属性和数据绑定，实时预览

**代码生成** — 一键生成完整 Flutter 项目：
- WebSocket 设备服务
- 数据展示页面
- 控制页面

```bash
cd tianshu_studio
flutter pub get
flutter run -d windows
```

---

## API 参考

### 认证

```
POST /api/v1/auth/register      # 注册
POST /api/v1/auth/login         # 登录 → JWT
POST /api/v1/auth/refresh       # 刷新 Token
```

### 设备

```
GET    /api/v1/devices                  # 列表
POST   /api/v1/devices                  # 注册
GET    /api/v1/devices/<id>             # 详情
PUT    /api/v1/devices/<id>             # 更新
DELETE /api/v1/devices/<id>             # 删除
POST   /api/v1/devices/<id>/command     # 下发命令
GET    /api/v1/devices/<id>/config      # 读取配置
PUT    /api/v1/devices/<id>/config      # 更新配置
```

### 数据

```
POST   /api/v1/data/report              # 设备上报
GET    /api/v1/data/<device_id>         # 历史数据
GET    /api/v1/data/<device_id>/latest  # 最新数据
```

### AI Agent

```
GET    /api/v1/agents                    # 列表
POST   /api/v1/agents                    # 创建
GET    /api/v1/agents/<id>              # 详情
PUT    /api/v1/agents/<id>              # 更新
DELETE /api/v1/agents/<id>              # 删除
POST   /api/v1/agents/<id>/chat         # 对话
GET    /api/v1/agents/<id>/conversations # 对话历史
```

### API 网关

```
GET    /api/v1/gateway                   # 配置列表
POST   /api/v1/gateway                   # 创建配置
GET    /api/v1/gateway/<id>             # 详情
PUT    /api/v1/gateway/<id>             # 更新
DELETE /api/v1/gateway/<id>             # 删除
POST   /api/v1/gateway/<id>/invoke      # 触发调用
```

### WebSocket 事件

**设备认证：**
```json
{"type": "auth", "device_id": "xxx", "secret_key": "xxx"}
{"type": "auth_ok"}
```

**心跳：**
```json
{"type": "heartbeat"}
{"type": "heartbeat_ack"}
```

**命令下发：**
```json
{"type": "command", "command": "set_expression", "params": {"key": "value"}}
{"type": "command_ack", "status": "ok"}
```

**状态推送：**
```json
{"type": "device_status", "device_id": "xxx", "status": "online", "data": {...}}
```

完整 API 文档见 `server/docs/API.md`。

---

## 常见问题

### Q: 如何接入自定义设备？
设备只需实现：
1. HTTP POST 上报数据到 `/api/v1/data/report`
2. WebSocket 连接 `/socket.io` 进行认证和心跳
3. 接收 `command` 事件并执行

无需使用 ESP-IDF，任意能发起 HTTP/WebSocket 请求的平台均可接入。

### Q: 如何切换数据库？
修改 `server/.env` 中的 `DATABASE_URL`，然后运行：
```bash
cd server
flask db upgrade
```

### Q: AI Agent 对接自己的模型？
在 Agent 配置中填写你的 API base_url 和 api_key，天枢后端会使用 OpenAI 兼容格式调用。

### Q: Python 处理器如何使用？
在 API 网关创建配置，选择 `python_handler` 类型，编写处理代码。处理函数接收 `params` 字典，返回值赋给 `result` 变量。

### Q: 前端无法连接后端？
1. 确认后端已启动 (`flask run`)
2. 确认 CORS 配置允许前端域名
3. Vite 开发模式下自动代理 `/api` 到后端，无需额外配置

---

## 贡献指南

1. **Fork** 本仓库
2. 创建分支：`git checkout -b feature/your-feature`
3. 提交：`git commit -m 'Add feature'`
4. 推送：`git push origin feature/your-feature`
5. 创建 Pull Request

**代码风格：** Python (PEP 8 + Black)、JavaScript (ESLint + Prettier)、C (Linux kernel style)

---

## 开源协议

**GNU General Public License v3.0 (GPL-3.0)**

```
TianShu IoT Platform
Copyright (C) 2024-2026

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
```
