# Desktop Dog - 电子桌宠

基于ESP32的智能电子宠物，集成语音助手、IoT云平台、环境监测等功能。

## 功能概述

### 宠物系统
- **三大属性**: 快乐值(H)、饥饿值(F)、疲倦值(T)，范围0-100
- **七种心情**: Happy、Normal、Sad、Hungry、Tired、Sick、Dead
- **自动衰减**: 属性随时间变化，需要定期照顾
- **死亡与复活**: 属性极端时宠物死亡，可长按复活

### 语音助手
- 唤醒词激活
- 语音识别(百度ASR)
- AI对话(支持工具调用)
- 语音合成(百度TTS)

### IoT云平台(天枢)
- 实时数据上报
- 远程命令控制
- 设备状态监控

### 环境监测
- DHT11温湿度传感器
- OLED显示屏(128x64)

---

## 按钮操作

| 按钮 | 短按 | 长按 |
|------|------|------|
| BTN1 (GPIO38) | 喂食 | 强制喂食 |
| BTN2 (GPIO39) | 玩耍 | - |
| BTN3 (GPIO40) | 休息 | 强制休息 |
| BTN4 (GPIO41) | 切换模式* | 进入/退出曲线视图** |

*模式切换: 普通 → 极速 → 暂停 → 普通 (循环)
**曲线视图中短按BTN4切换显示变量，死亡状态长按BTN4复活

---

## 显示界面

### 主界面
```
25C 60%              Happy    <- 温湿度 + 心情
H:50 F:60 T:30               <- 属性值
─────────────────────────────
        [表情]         FAST   <- 32x32表情 + 模式
─────────────────────────────
Feed  Play  Rest  Mode       <- 按钮提示
```

### 曲线视图 (5页)
```
H:50 (1/5)                   <- 变量名:值 (页码)
│
│      /\    /\              <- 曲线
│_____/  \__/  \_____
└─────────────────────────
-60s              now        <- 时间轴
```

页面: H(快乐) → F(饥饿) → T(疲倦) → Temp(温度) → Hum(湿度)

---

## 速度模式

| 模式 | 值 | 衰减间隔 | 显示 |
|------|-----|---------|------|
| 普通 | 0 | 30秒 | (无) |
| 极速 | 1 | 1秒 | FAST |
| 暂停 | 2 | 不衰减 | PAUSE |

---

## 死亡条件

满足任一条件即死亡:
- 快乐值 = 0
- 饥饿值 >= 100
- 疲倦值 >= 100

---

## 云端命令 (天枢平台)

| 命令 | 参数 | 说明 |
|------|------|------|
| `pet_feed` | 无 | 喂食 |
| `pet_play` | 无 | 玩耍 |
| `pet_sleep` | 无 | 休息 |
| `pet_revive` | 无 | 复活 |
| `pet_set_mode` | "0"/"1"/"2" | 设置模式 |

---

## 云端上报数据

| 数据点 | 类型 | 说明 |
|--------|------|------|
| `temperature` | float | 温度(°C) |
| `humidity` | float | 湿度(%) |
| `pet_happiness` | int | 快乐值 |
| `pet_hunger` | int | 饥饿值 |
| `pet_fatigue` | int | 疲倦值 |
| `pet_mood` | int | 心情(0-6) |

---

## AI语音工具

| 工具 | 参数 | 说明 |
|------|------|------|
| `get_weather` | city | 查天气 |
| `play_music` | song | 播放音乐 |
| `stop_music` | - | 停止音乐 |
| `set_volume` | volume(0-100) | 设置音量 |
| `get_pet_status` | - | 查宠物状态 |
| `pet_feed` | - | 喂宠物 |
| `pet_play` | - | 和宠物玩 |
| `reset_context` | - | 重置对话 |
| `continue_chat` | - | 继续对话 |

---

# 开发说明

## 项目结构

```
main/
├── app_config.h        # 配置常量(GPIO、定时器等)
├── app_signals.h       # 信号ID定义
├── app_payloads.h      # 数据结构定义
├── app_buffers.c/h     # 全局数据缓冲区
├── main.c              # 入口，实体初始化
│
├── entities/           # 实体(状态机)
│   ├── ent_system.c/h      # 系统实体(OLED、传感器)
│   ├── ent_pet.c/h         # 宠物实体
│   ├── ent_tianshu.c/h     # 天枢IoT实体
│   ├── ent_wake_word.c/h   # 唤醒词检测
│   ├── ent_recorder.c/h    # 录音
│   ├── ent_asr.c/h         # 语音识别
│   ├── ent_ai.c/h          # AI对话
│   ├── ent_tts.c/h         # 语音合成
│   ├── ent_audio.c/h       # 音频播放
│   └── ent_sensor.c/h      # 传感器
│
├── pet/                # 宠物模块
│   ├── pet_attributes.c/h  # 属性计算
│   ├── pet_storage.c/h     # NVS存储
│   ├── pet_display.c/h     # OLED显示
│   └── pet_sound.c/h       # 声音反馈
│
├── drivers/            # 驱动
│   └── button.c/h          # 按钮驱动
│
└── 其他文件...
    ├── mimo_ai.c           # AI对话+工具调用
    ├── baidu_asr.c/h       # 百度语音识别
    ├── baidu_tts.c/h       # 百度语音合成
    ├── dht11.c/h           # 温湿度传感器
    └── wifi.c/h            # WiFi连接
```

---

## MicroReactor框架

项目基于MicroReactor状态机框架，核心概念：

### 实体(Entity)
每个功能模块是一个实体，包含状态机和规则表。

```c
ur_entity_t s_entity;

// 状态定义
enum {
    STATE_INIT = 0,
    STATE_ACTIVE,
    STATE_ERROR,
};

// 规则表: 信号 -> 处理函数
static const ur_rule_t s_rules[] = {
    UR_RULE(SIG_SOME_EVENT, 0, handler_func),
    UR_RULE_END
};
```

### 信号(Signal)
实体间通过信号通信：

```c
// 广播信号(所有实体接收)
ur_broadcast((ur_signal_t){
    .id = SIG_EVENT,
    .src_id = ENT_ID_SELF,
    .payload = { .u8 = { value } }
});

// 定向发送(指定实体)
ur_emit_to_id(ENT_ID_TARGET, signal);
```

### payload使用注意
`ur_signal_t.payload`是联合体，包含数组字段：

```c
// 正确写法
.payload = { .u8 = { value } }      // 初始化
sig->payload.u8[0]                   // 读取

// 错误写法
.payload = { .u8 = value }          // 错！u8是数组
sig->payload.u8                      // 错！返回指针
```

---

## uFlow协程

用于需要等待的操作(定时、异步)，基于Duff's Device实现。

### 基本用法
```c
static uint16_t my_flow(ur_entity_t *ent, const ur_signal_t *sig)
{
    UR_FLOW_BEGIN(ent);

    while (1) {
        UR_AWAIT_TIME(ent, 1000);  // 等待1秒
        // 执行操作...
    }

    UR_FLOW_END(ent);
}
```

### 重要限制

**1. 局部变量不持久**
```c
// 错误！data在yield后丢失
UR_FLOW_BEGIN(ent);
pet_data_t *data = get_data();  // 声明在BEGIN后
UR_AWAIT_TIME(ent, 1000);
use(data);  // data已无效！

// 正确：每次重新获取
while (1) {
    UR_AWAIT_TIME(ent, 1000);
    {
        pet_data_t *data = get_data();  // 每次重新获取
        use(data);
    }
}
```

**2. 不能在循环内直接return状态转换**
```c
// 错误！break UR_FLOW_END的do-while
while (1) {
    if (dead) return STATE_DEAD;  // 错！
}
UR_FLOW_END(ent);

// 正确：手动清理flow状态
while (1) {
    if (dead) {
        ent->flow_line = 0;
        ent->flags &= ~UR_FLAG_FLOW_RUNNING;
        return STATE_DEAD;
    }
}
```

---

## 添加新功能指南

### 添加新信号

1. 在`app_signals.h`中定义信号ID：
```c
SIG_MY_EVENT = SIG_USER_BASE + 200,
```

2. 在`main.c`中注册信号名(用于调试)：
```c
ur_trace_register_signal(SIG_MY_EVENT, "MY_EVENT");
```

### 添加新实体

1. 创建`entities/ent_xxx.c/h`
2. 定义状态、规则表、处理函数
3. 在`main.c`中：
   - 增加`NUM_ENTITIES`
   - 添加`#include`
   - 在`s_entities[]`中初始化
   - 注册状态名

### 添加新的宠物交互

1. 在`app_payloads.h`的`pet_interaction_t`中添加类型
2. 在`pet_attributes.c`中实现属性变化逻辑
3. 在`ent_pet.c`的`on_pet_interaction`中处理
4. (可选)在`ent_tianshu.c`添加云端命令
5. (可选)在`mimo_ai.c`添加AI工具

### 添加新的云端数据点

1. 在`ent_tianshu.c`的上报函数中添加：
```c
ts_datapoint_t points[] = {
    { .name = "new_point", .type = TS_TYPE_INT, .value.i = value },
    // ...
};
tianshu_report_batch(points, count);
```

---

## 注意事项

### WiFi初始化顺序
需要网络的操作(TTS生成、HTTP请求)必须在WiFi连接后执行：
- `ent_xxx_init()` - WiFi未就绪
- `SIG_SYSTEM_READY` - WiFi已就绪，可执行网络操作

### OLED资源共享
`ent_system`和`pet_display`共享OLED，使用互斥锁：
```c
// 获取共享资源
u8g2_t *u8g2 = ent_system_get_u8g2();
SemaphoreHandle_t mutex = ent_system_get_oled_mutex();

// 对话时禁用宠物显示
pet_display_set_active(false);  // 进入对话
pet_display_set_active(true);   // 退出对话
```

### NVS存储
宠物数据自动保存到NVS，命名空间`"pet_data"`，键`"stats"`。

---

## 编译与烧录

```bash
# 配置
idf.py menuconfig

# 编译
idf.py build

# 烧录
idf.py -p COMx flash

# 监控
idf.py -p COMx monitor
```

---

## 硬件连接

| 功能 | GPIO |
|------|------|
| BTN1 (Feed) | 38 |
| BTN2 (Play) | 39 |
| BTN3 (Sleep) | 40 |
| BTN4 (Mode) | 41 |
| DHT11 | (见app_config.h) |
| I2S MIC | (见app_config.h) |
| I2S SPK | (见app_config.h) |
| OLED SDA/SCL | (见app_config.h) |

