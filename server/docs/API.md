# 天枢IoT平台 API文档

**版本**: v2.0
**Base URL**: `/api/v1`
**WebSocket**: `/socket.io/`

---

## 认证方式

| 方式 | 适用场景 | 传递方式 |
|------|----------|----------|
| JWT Token | Web前端/App | Header: `Authorization: Bearer <token>` |
| Secret Key | 设备端 | 请求体 `secret_key` 或 Header `X-Secret-Key` |

---

## 1. 认证 API

### 1.1 用户注册

```
POST /auth/register
```

**请求体**:
```json
{
  "username": "user1",
  "password": "123456",
  "email": "user@example.com"
}
```

**响应**:
```json
{
  "code": 0,
  "message": "User registered successfully",
  "data": {
    "id": 1,
    "username": "user1",
    "email": "user@example.com"
  }
}
```

### 1.2 用户登录

```
POST /auth/login
```

**请求体**:
```json
{
  "username": "user1",
  "password": "123456"
}
```

**响应**:
```json
{
  "code": 0,
  "message": "Login successful",
  "data": {
    "token": "eyJhbGciOiJIUzI1NiIs...",
    "user": {
      "id": 1,
      "username": "user1"
    }
  }
}
```

### 1.3 获取当前用户

```
GET /auth/me
```

**认证**: JWT

**响应**:
```json
{
  "code": 0,
  "data": {
    "id": 1,
    "username": "user1",
    "email": "user@example.com"
  }
}
```

---

## 2. 设备管理 API

### 2.1 获取设备列表

```
GET /devices
```

**认证**: JWT

**响应**:
```json
{
  "code": 0,
  "data": [
    {
      "device_id": "7d88e242-5e82-48fe-8cf5-34f0c54136c6",
      "name": "温湿度传感器",
      "device_type": "sensor",
      "status": "online",
      "last_seen": "2024-01-26T10:00:00Z"
    }
  ]
}
```

### 2.2 创建设备

```
POST /devices
```

**认证**: JWT

**请求体**:
```json
{
  "name": "温湿度传感器",
  "device_type": "sensor",
  "description": "客厅温湿度监测",
  "tags": ["室内", "传感器"]
}
```

**响应** (仅创建时返回secret_key):
```json
{
  "code": 0,
  "message": "Device created",
  "data": {
    "device_id": "7d88e242-5e82-48fe-8cf5-34f0c54136c6",
    "secret_key": "034c784a3cde46d18e15fa632f7b9285...",
    "name": "温湿度传感器"
  }
}
```

### 2.3 获取设备详情

```
GET /devices/<device_id>
```

**认证**: JWT

**响应**:
```json
{
  "code": 0,
  "data": {
    "device_id": "7d88e242-...",
    "name": "温湿度传感器",
    "device_type": "sensor",
    "status": "online",
    "description": "客厅温湿度监测",
    "tags": ["室内", "传感器"],
    "last_seen": "2024-01-26T10:00:00Z"
  }
}
```

### 2.4 更新设备

```
PUT /devices/<device_id>
```

**认证**: JWT

**请求体**:
```json
{
  "name": "新名称",
  "description": "新描述",
  "tags": ["标签1"]
}
```

### 2.5 删除设备

```
DELETE /devices/<device_id>
```

**认证**: JWT

### 2.6 获取设备密钥

```
GET /devices/<device_id>/secret
```

**认证**: JWT

**响应**:
```json
{
  "code": 0,
  "data": {
    "device_id": "7d88e242-...",
    "secret_key": "034c784a3cde..."
  }
}
```

### 2.7 重置设备密钥

```
POST /devices/<device_id>/secret
```

**认证**: JWT

### 2.8 设备获取自身信息

```
GET /devices/<device_id>/info?secret_key=xxx
```

**认证**: Secret Key (查询参数或Header `X-Secret-Key`)

**响应**:
```json
{
  "code": 0,
  "data": {
    "device_id": "7d88e242-...",
    "name": "温湿度传感器",
    "status": "online"
  }
}
```

### 2.9 设备心跳

```
POST /devices/heartbeat
```

**认证**: Secret Key

**请求体**:
```json
{
  "device_id": "7d88e242-5e82-48fe-8cf5-34f0c54136c6",
  "secret_key": "034c784a3cde46d18e15fa632f7b9285..."
}
```

**响应**:
```json
{
  "code": 0,
  "message": "Heartbeat received",
  "data": {
    "status": "online"
  }
}
```

### 2.10 检查所有设备状态

```
POST /devices/check-status
```

**认证**: JWT

### 2.11 发送命令给设备 (HTTP)

```
POST /devices/<device_id>/command
```

**认证**: JWT

**请求体**:
```json
{
  "command": "set_interval",
  "params": {
    "interval": 30
  }
}
```

**响应**:
```json
{
  "code": 0,
  "message": "Command sent"
}
```

### 2.12 获取在线设备列表

```
GET /devices/online
```

**认证**: JWT

**响应**:
```json
{
  "code": 0,
  "data": ["7d88e242-...", "abc123-..."]
}
```

### 2.13 设备获取数据记录

```
GET /devices/<device_id>/data?secret_key=xxx&limit=100&data_type=sensor
```

**认证**: Secret Key

**响应**:
```json
{
  "code": 0,
  "data": [
    {
      "id": 1,
      "data_type": "sensor",
      "data": {"temperature": 25.5, "humidity": 60.0},
      "timestamp": "2024-01-26T10:00:00Z"
    }
  ]
}
```

---

## 3. 数据 API

### 3.1 设备上报数据

```
POST /data/report
```

**认证**: Secret Key

**请求体**:
```json
{
  "device_id": "7d88e242-5e82-48fe-8cf5-34f0c54136c6",
  "secret_key": "034c784a3cde46d18e15fa632f7b9285...",
  "data_type": "sensor",
  "data": {
    "temperature": 25.5,
    "humidity": 60.0
  }
}
```

**响应**:
```json
{
  "code": 0,
  "message": "Data received",
  "data": {
    "record_id": 1
  }
}
```

**副作用**: 服务器会通过WebSocket广播 `device_data` 事件。

### 3.2 获取设备数据 (Web端)

```
GET /data/<device_id>?limit=100
```

**认证**: JWT

---

## 4. 配置 API

### 4.1 设备拉取配置

```
POST /config/pull
```

**认证**: Secret Key

**请求体**:
```json
{
  "device_id": "7d88e242-...",
  "secret_key": "034c784a..."
}
```

**响应**:
```json
{
  "code": 0,
  "data": {
    "report_interval": 60
  }
}
```

### 4.2 获取设备配置

```
GET /devices/<device_id>/config
```

**认证**: JWT

### 4.3 更新设备配置

```
PUT /devices/<device_id>/config
```

**认证**: JWT

**请求体**:
```json
{
  "report_interval": 30
}
```

---

## 5. 模板 API

### 5.1 获取模板列表

```
GET /templates
```

**认证**: JWT

### 5.2 获取模板详情

```
GET /templates/<template_id>
```

**认证**: JWT

### 5.3 创建模板

```
POST /templates
```

**认证**: JWT

### 5.4 更新模板

```
PUT /templates/<template_id>
```

**认证**: JWT

### 5.5 删除模板

```
DELETE /templates/<template_id>
```

**认证**: JWT

---

## 6. Agent API

### 6.1 获取Agent列表

```
GET /agents
```

**认证**: JWT

### 6.2 创建Agent

```
POST /agents
```

**认证**: JWT

### 6.3 获取Agent详情

```
GET /agents/<agent_id>
```

**认证**: JWT

### 6.4 更新Agent

```
PUT /agents/<agent_id>
```

**认证**: JWT

### 6.5 绑定设备到Agent

```
POST /agents/<agent_id>/bind
```

**认证**: JWT

### 6.6 与Agent对话

```
POST /agents/<agent_id>/chat
```

**认证**: JWT

### 6.7 删除Agent

```
DELETE /agents/<agent_id>
```

**认证**: JWT

### 6.8 获取对话列表

```
GET /agents/<agent_id>/conversations
```

**认证**: JWT

### 6.9 获取对话详情

```
GET /agents/<agent_id>/conversations/<conv_id>
```

**认证**: JWT

### 6.10 删除对话

```
DELETE /agents/<agent_id>/conversations/<conv_id>
```

**认证**: JWT

### 6.11 获取可用工具

```
GET /agents/tools
```

**认证**: JWT

---

## 7. API网关

### 7.1 获取API配置列表

```
GET /gateway/configs
```

**认证**: JWT

### 7.2 创建API配置

```
POST /gateway/configs
```

**认证**: JWT

### 7.3 获取API配置详情

```
GET /gateway/configs/<config_id>
```

**认证**: JWT

### 7.4 代理API请求

```
POST /gateway/proxy/<config_id>
```

**认证**: JWT

---

## 8. 日志 API

### 8.1 获取操作日志

```
GET /logs?limit=100
```

**认证**: JWT

---

## 9. WebSocket API

**连接地址**: `ws://host:port/socket.io/?EIO=4&transport=websocket`

### 9.1 设备端事件

#### 发送: auth (设备认证)

```json
["auth", {
  "device_id": "7d88e242-...",
  "secret_key": "034c784a..."
}]
```

#### 接收: auth_result

```json
["auth_result", {
  "code": 0,
  "message": "Authenticated"
}]
```

#### 接收: command

```json
["command", {
  "command": "set_interval",
  "params": {"interval": 30}
}]
```

#### 发送: cmd_result

```json
["cmd_result", {
  "device_id": "7d88e242-...",
  "cmd_id": "xxx",
  "success": true,
  "result": "OK"
}]
```

### 9.2 App端事件

#### 发送: send_command

```json
["send_command", {
  "device_id": "7d88e242-...",
  "secret_key": "034c784a...",
  "command": "set_interval",
  "params": {"interval": 30}
}]
```

#### 接收: command_result

```json
["command_result", {
  "code": 0,
  "message": "Command sent",
  "device_id": "7d88e242-...",
  "command": "set_interval"
}]
```

#### 接收: device_data (广播)

```json
["device_data", {
  "device_id": "7d88e242-...",
  "data": {"temperature": 25.5},
  "data_type": "sensor",
  "timestamp": "2024-01-26T10:00:00Z"
}]
```

#### 接收: device_status (广播)

```json
["device_status", {
  "device_id": "7d88e242-...",
  "status": "online"
}]
```

---

## 10. 错误码

| 错误码 | 说明 |
|--------|------|
| 0 | 成功 |
| 1001 | 参数缺失 |
| 1002 | 参数无效 |
| 2001 | 资源不存在/认证失败 |
| 2002 | 设备离线 |
| 3001 | 服务器内部错误 |

---

## 11. 通信流程

```
设备(ESP32)              服务器                    App(Flutter)
    │                      │                          │
    ├──WS connect─────────>│                          │
    │<─────────────────────┤ (Socket.IO握手)          │
    ├──auth───────────────>│                          │
    │<──auth_result────────┤                          │
    │                      ├──device_status(online)──>│
    │                      │                          │
    ├──HTTP /data/report──>│                          │
    │                      ├──device_data────────────>│
    │                      │                          │
    │                      │<──send_command───────────┤
    │<──command────────────┤                          │
    │──cmd_result─────────>│                          │
    │                      ├──command_result─────────>│
    │                      │                          │
    │  (断开连接)          │                          │
    │                      ├──device_status(offline)─>│
```
