# Event-Driven STM32+FreeRTOS System

##  项目简介
本项目基于 STM32F407ZGT6 和 FreeRTOS，实现了一个高并发、高可靠的嵌入式多任务系统。
项目采用 **APP/BSP 分层架构**，重点解决了 RTOS 环境下高优先级任务（如传感器时序）与低速外设（如串口交互）之间的资源竞争问题。

##  核心架构与亮点

### 1. 串口命令解析 (Advanced UART Architecture)
采用了 **"中断接收 -> 队列缓冲 -> 任务解析"** 的异步处理模型，彻底解决了轮询模式在高负载下的丢包问题。
- **Hardware Layer**: `HAL_UART_RxCpltCallback` 负责极速接收单字节，存入缓冲队列。
- **Middleware**: `queueUartByteHandle` (Depth=128) 作为 FIFO 缓冲区。
- **App Layer**: `CmdTask` 异步从队列读取数据，拼接指令。

### 2. 线程安全打印 (Thread-Safe Logging)
- 实现了基于 **Mutex (互斥锁)** 的 `uart_printf`。
- 防止多任务并发打印时的字符交织（乱码）问题。

### 3. 任务调度策略
| 任务名称 | 优先级 | 职责 | 栈大小 |
| :--- | :--- | :--- | :--- |
| **SensorTask** | **High** | 读取 DHT11 温湿度 (微秒级时序敏感) | 512 Words |
| **LedTask** | Normal | 响应心跳与指令，控制 LED 闪烁 | 512 Words |
| **CmdTask** | Normal | 解析串口指令 ("toggle" 等) | 512 Words |
| **PrintTask** | Normal | 产生心跳数据 | 256 Words |

### 4. 系统整体架构图
```mermaid
graph TD
    %% =======================
    %% 1. 中断层 (ISR Layer) - 数据的源头
    %% =======================
    subgraph ISR_Layer ["⚡ ISR Layer (中断层)"]
        direction TB
        UART_ISR["UART1 ISR<br/>(RxCallback)"]
        KEY_ISR["EXTI PF6 ISR<br/>(Button Press)"]
    end

    %% =======================
    %% 2. 缓冲层 (Queue Layer) - 数据的管道
    %% =======================
    subgraph Queue_Layer ["📥 Queue Layer (缓冲层)"]
        Q_Byte(("queueUartByte<br/>(Raw Char)"))
        Q_Cmd(("queueCmd<br/>(CmdType)"))
        Q_Key(("queueKey<br/>(KeyEvt)"))
        Q_Heart(("queueHeartbeat<br/>(uint32)"))
    end

    %% =======================
    %% 3. 任务层 (Task Layer) - 逻辑处理核心
    %% =======================
    subgraph Task_Layer ["⚙️ Task Layer (任务层)"]
        CmdTask["CmdTask<br/>(Priority: Normal)"]
        LedTask["LedTask<br/>(Priority: Normal)"]
        PrintTask["PrintTask<br/>(Priority: Normal)"]
        SensorTask["SensorTask<br/>(Priority: High)"]
    end

    %% =======================
    %% 4. 硬件层 (Hardware) - 执行者
    %% =======================
    subgraph HW_Layer ["🔌 Hardware (硬件)"]
        HW_LED["BSP_LED"]
        HW_DHT11["BSP_DHT11"]
        HW_UART["BSP_UART"]
    end

    %% =======================
    %% 连线关系 (Data Flow)
    %% =======================

    %% --- 1. 串口数据流 ---
    UART_ISR -->|"Push 't','o'..."| Q_Byte
    Q_Byte -->|"Pop Char"| CmdTask
    CmdTask -- "Parse 'toggle'<br/>Push CMD" --> Q_Cmd

    %% --- 2. 按键数据流 ---
    KEY_ISR -->|"Push Press Evt"| Q_Key
    Q_Key -->|"Pop Evt"| CmdTask
    %% 注：这里假设按键也可能触发命令，或者有专门的 KeyTask，根据你实际情况调整

    %% --- 3. 命令控制流 ---
    Q_Cmd -->|"Pop CMD"| LedTask
    LedTask -->|"Control"| HW_LED

    %% --- 4. 传感器与心跳 ---
    PrintTask -- "Push Count" --> Q_Heart
    Q_Heart -->|"Pop"| LedTask
    
    SensorTask -->|"Read Temp/Humi"| HW_DHT11
    SensorTask -- "Mutex Printf" --> HW_UART
    CmdTask -- "Mutex Printf" --> HW_UART
    LedTask -- "Mutex Printf" --> HW_UART

    %% =======================
    %% 样式美化
    %% =======================
    style ISR_Layer fill:#fff0f0,stroke:#ff0000,stroke-width:2px
    style Queue_Layer fill:#fffde7,stroke:#fbc02d,stroke-width:2px
    style Task_Layer fill:#e3f2fd,stroke:#1565c0,stroke-width:2px
    style HW_Layer fill:#f1f8e9,stroke:#33691e,stroke-width:2px

    classDef isrNode fill:#ffcdd2,stroke:#b71c1c,color:black;
    class UART_ISR,KEY_ISR isrNode;

    classDef qNode fill:#fff9c4,stroke:#f57f17,color:black;
    class Q_Byte,Q_Cmd,Q_Key,Q_Heart qNode;
  ```

##  硬件环境
- **MCU**: STM32F407ZGT6
- **Sensor**: DHT11 Temperature & Humidity Sensor
- **Interface**: USB-TTL ST-Link
- **Baudrate**: 115200

##  目录结构
```text
FreeRTOS_Demo/
├── Core/                   # STM32CubeMX 生成的核心与硬件初始化
│   ├── main.c              # 系统入口，硬件初始化调用
│   ├── freertos.c          # RTOS 任务句柄定义、队列创建、任务创建
│   ├── stm32f4xx_it.c      # 中断服务函数 (ISR)
│   ├── gpio.c              # GPIO 初始化
│   ├── usart.c             # 串口初始化
│   └── ...
│
├── User/                   # 用户代码区 (核心业务逻辑)
│   ├── App/                # 业务逻辑层 (Application Layer)
│   │   ├── app_task_key.c    # 按键扫描与处理任务
│   │   ├── app_task_led.c    # LED 状态机与控制任务
│   │   ├── app_task_sensor.c # DHT11 传感器定时读取任务
│   │   ├── app_task_uart.c   # 串口指令解析任务 (含中断回调逻辑)
│   │   └── app_error.c       # 全局错误处理与异常挂钩
│   │
│   └── Bsp/                # 板级支持包 (Board Support Package)
│       ├── bsp_dht11.c       # DHT11 底层时序驱动
│       ├── bsp_uart.c        # 封装线程安全的 printf 与发送函数
│       ├── bsp_key.c         # 按键 GPIO 读写驱动
│       └── bsp_led.c         # LED GPIO 读写驱动
│
├── Middlewares/            # FreeRTOS 操作系统源码
├── Drivers/                # STM32 HAL 库与 CMSIS 驱动
├── MDK-ARM/                # Keil uVision 工程文件 (.uvprojx)
└── README.md               # 项目说明文档
