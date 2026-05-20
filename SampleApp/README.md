# SampleApp - IoT 多模块交互示例程序

## 概述
本程序仿照 media 架构设计，展示多模块系统中的核心架构模式。适用于学习 Texas Instruments DSP 框架下的模块化开发。

## 架构模式展示

### 1. 模块生命周期管理
每个模块遵循 `Init -> Fxn -> Delete` 三段式生命周期：
```c
DECLARE_MODULE_V2(APP_MODULE_SENSOR, SensorHubInit, SensorHubDelete, SensorHubFxn, ...)
```
- **Init**: 分配私有数据、创建线程、注册消息表
- **Fxn**: 模块主循环，调用 `CommonMsgProcessV2()` 处理消息
- **Delete**: 清理资源、停止线程、释放内存

### 2. 模块间消息传递
- 使用 `T_MsgV2` 结构定义消息（strMsgId, pcBody, strReply）
- 每个模块注册 `T_MsgProcV2` 消息处理表
- `CommonMsgProcessV2()` 自动将消息分发到对应模块的处理函数

### 3. STBP 通信框架
- **Global 客户端**: 所有内部模块通过 `GetUserClientHandle()` 共享，用于数据上报
- **UserAgent 客户端**: 独立创建，身份隔离，负责订阅外部 Topic 和路由消息

### 4. 线程管理
- 使用 `TSK_create()` 创建工作线程
- 使用 `CommQue` 实现线程间安全通信
- 使用 `TSK_delete()` 安全停止线程

### 5. 配置文件加载
- 配置文件存储在 `configs/topic_data/<module>/` 目录
- 启动时加载 JSON 配置，模块可按需读取

## 目录结构
```
SampleApp/
├── inc/                     # 公共头文件
│   ├── global.h             # 全局定义、Topic 常量
│   ├── SampleApp.h          # 应用公共常量
│   ├── SysManager.h         # 系统管理模块
│   ├── SensorHub.h          # 传感器模块
│   ├── DeviceCtrl.h         # 设备控制模块
│   ├── UserAgent.h          # 用户代理模块
│   └── SysConfig.h          # 系统配置
├── src/
│   ├── MainApp/             # 程序入口
│   ├── Global/              # 全局初始化（STBP 客户端）
│   ├── SysConfig/           # 配置加载
│   ├── SysManager/          # 心跳管理
│   ├── SensorHub/           # 传感器数据采集上报
│   ├── DeviceCtrl/          # 设备开关控制
│   └── UserAgent/           # 独立 STBP 客户端
├── configs/                 # 配置文件
│   └── topic_data/sample.v1/
│       ├── sys.json
│       └── sensor.json
└── Makefile
```

## 模块说明

| 模块 | 功能 | STBP 客户端 | 关键展示 |
|------|------|------------|---------|
| SysManager | 系统心跳 | Global | 线程创建、周期 Publish |
| SensorHub | 模拟温湿度数据 | Global | 数据采集线程、定时上报 |
| DeviceCtrl | 设备开关 | Global | 消息表处理、状态变更 Publish |
| UserAgent | 外部消息路由 | 独立 | 身份隔离、Topic 订阅、消息路由 |

## Topic 定义
| Topic | 方向 | 说明 |
|-------|------|------|
| `$report.heartbeat.*.*.*.sample.v1.state` | 上行 | 系统心跳 |
| `$report.$data.*.*.*.sample.v1.sensor.temp` | 上行 | 温度上报 |
| `$report.$data.*.*.*.sample.v1.sensor.humi` | 上行 | 湿度上报 |
| `$request.$set.*.*.*.sample.v1.devCtrl.on` | 下行 | 开设备 |
| `$request.$set.*.*.*.sample.v1.devCtrl.off` | 下行 | 关设备 |

## 构建
```bash
cd SampleApp
make
```

## 运行
```bash
./SampleApp
```

## 与 media 架构的对应关系
| media | SampleApp | 说明 |
|-------|-----------|------|
| GlobalInit | GlobalInit | 创建全局 STBP 客户端 |
| UserAgent | UserAgent | 独立客户端，身份隔离 |
| MO/MI | DeviceCtrl/SensorHub | 输出/输入模块 |
| SyncTick | SysManager | 系统级周期任务 |
| SysConfig | SysConfig | 配置加载/保存 |
