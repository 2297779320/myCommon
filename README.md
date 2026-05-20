# myCommon + SampleApp 合并项目

myCommon 框架与 SampleApp 示例程序的合并版本，展示多模块交互架构。

## 目录结构

```
├── SampleApp/              # IoT 示例应用程序
│   ├── inc/                # 公共头文件
│   ├── src/                # 模块源码
│   │   ├── MainApp/        # 程序入口（myCommon 框架）
│   │   ├── Global/         # STBP 全局客户端
│   │   ├── SysConfig/      # 配置加载
│   │   ├── SysManager/     # 系统心跳
│   │   ├── SensorHub/      # 传感器数据上报
│   │   ├── DeviceCtrl/     # 设备控制
│   │   └── UserAgent/      # 独立 STBP 客户端
│   └── configs/            # JSON 配置文件
├── AppPublic/              # 公共库
│   ├── inc/                # 头文件 (StbpClient, cJSON, etc.)
│   └── src/                # 源码
├── common/                 # myCommon 核心框架
│   ├── osal.c/h            # OS 抽象层
│   ├── tsk.c/h             # 线程管理
│   ├── framework_def.h     # 模块框架定义
│   ├── module_manager.c/h  # 模块管理器
│   ├── comm_que.c/h        # 通信队列
│   ├── msgqueue.c/h        # 消息队列
│   ├── cjson/              # JSON 解析库
│   └── ...                 # 其他公共模块
└── Makefile                # 顶层构建配置
```

## 架构特性

### 模块框架
- 使用 myCommon 的 `framework_create()` / `framework_register_module()` API
- 模块生命周期：`Init` → `Run` → `Destroy`
- 消息驱动：通过 `module_register_handler()` 注册消息处理器

### STBP 客户端
- **Global 客户端**：共享 STBP 连接，用于内部上报（心跳、传感器数据）
- **UserAgent 客户端**：独立 STBP 连接，身份隔离，用于外部消息路由

### 模块交互
```
GlobalInit()
  └─ 创建 "Global" STBP 客户端

framework_register_module()
  ├─ UserAgent    # 独立 STBP 客户端 + Topic 订阅
  ├─ SysManager   # 心跳线程 + 定期发布
  ├─ SensorHub    # 模拟温湿度传感器 + 数据上报
  └─ DeviceCtrl   # 消息处理 + 设备控制

framework_start_main_loop()
  └─ 主循环处理消息队列 + 调用各模块 Run 函数
```

## 构建

```bash
make
```

## 运行

```bash
./SampleApp
```

## 与 standalone 版本的区别

| 特性 | standalone 版本 | 合并版本 |
|------|----------------|----------|
| 模块框架 | T_ModuleV2 宏 | myCommon framework API |
| 消息处理 | T_MsgProcV2 表 | module_register_handler() |
| 线程管理 | TSK_create (standalone) | TSK_create (myCommon) |
| OSAL | third_party/osal/ | common/osal.c |
| 依赖 | 独立 | 使用 myCommon common/ 目录 |

## Git 历史

本项目由两个 git 仓库合并而成：
- **myCommon**: 原始框架代码
- **rk3588-standalone**: SampleApp 独立版本

合并后保留了完整的提交历史。
