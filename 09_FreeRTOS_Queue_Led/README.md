# Key 按键轮询与队列发送实验

## 1. 分支说明

- 当前分支：`Key`
- 分支来源：`master`
- 本分支目标：
  - 完成按键 GPIO 轮询检测；
  - 检测到按键按下后，通过 FreeRTOS 原生队列 API 发送消息；
  - 验证按键任务能够正常运行，并成功向队列发送数据；
  - 暂不处理 LED 翻转，LED 接收任务将在后续 `Key_Led` 分支完成。

---

## 2. 开发环境

- 开发板：WeAct STM32F411 开发板
- MCU：STM32F411CEU6
- IDE：Keil MDK
- 配置工具：STM32CubeMX
- 软件库：STM32 HAL
- 操作系统：FreeRTOS
- 队列接口：FreeRTOS 原生 API

使用的队列头文件：

```c
#include "queue.h"
```

---

## 3. 实验需求

创建一个按键检测任务，周期性轮询按键状态。

当检测到按键被按下时：

1. 调用按键扫描接口读取按键状态；
2. 判断本次按键扫描是否成功；
3. 判断按键是否处于按下状态；
4. 通过消息队列发送一条按键消息；
5. 通过串口打印任务运行和发送结果，便于验证。

本分支只负责：

```text
按键检测 -> 产生消息 -> 发送到队列
```

完整业务流程将在后续分支中扩展为：

```text
按键任务
    |
    | xQueueSend()
    v
消息队列
    |
    | xQueueReceive()
    v
LED 任务
    |
    v
翻转 LED
```

---

## 4. 软件结构

```text
Application
├── main.c
│   ├── 创建 FreeRTOS 队列
│   ├── 创建按键任务
│   └── 启动 FreeRTOS 调度器
│
└── BSP
    ├── bsp_key.h
    │   ├── 按键状态枚举
    │   ├── 接口返回值枚举
    │   └── key_scan() 函数声明
    │
    └── bsp_key.c
        └── key_scan() 按键扫描实现
```

---

## 5. 按键接口设计

### 5.1 返回状态

```c
typedef enum
{
    KEY_OK             = 0,
    KEY_ERROR          = 1,
    KEY_ERRORTIMEOUT   = 2,
    KEY_ERRORRESOURCE  = 3,
    KEY_ERRORPARAMETER = 4,
    KEY_ERRORNOMEMORY  = 5,
    KEY_ERRORISR       = 6,
    KEY_RESERVED       = 0x7FFFFFFF
} key_status_t;
```

### 5.2 按键状态

```c
typedef enum
{
    KEY_PRESSED     = 0,
    KEY_NOT_PRESSED = 1
} key_press_status_t;
```

### 5.3 扫描接口

```c
key_status_t key_scan(key_press_status_t *key_value);
```

接口职责：

- 读取按键 GPIO 电平；
- 将按键状态通过 `key_value` 返回；
- 将函数执行状态通过返回值返回。

调用示例：

```c
key_status_t key_ret;
key_press_status_t key_status;

key_ret = key_scan(&key_status);
```

---

## 6. 队列设计

### 6.1 队列句柄

```c
QueueHandle_t key_queue;
```

`QueueHandle_t` 本质上是一个指向 FreeRTOS 队列内部对象的指针类型。

```c
typedef struct QueueDefinition *QueueHandle_t;
```

应用层只保存队列句柄，不直接访问队列内部结构。

### 6.2 创建队列

```c
key_queue = xQueueCreate(10, sizeof(uint32_t));
```

参数说明：

- `10`：队列最多保存 10 个消息；
- `sizeof(uint32_t)`：每个消息占 4 字节；
- 返回值：创建成功时返回有效队列句柄，失败时返回 `NULL`。

建议检查创建结果：

```c
if (key_queue == NULL)
{
    printf("key_queue create failed\r\n");
}
else
{
    printf("key_queue create successfully\r\n");
}
```

---

## 7. 按键任务实现

```c
void key_task_func(void *argument)
{
    key_status_t key_ret = KEY_OK;
    key_press_status_t key_status = KEY_NOT_PRESSED;
    uint32_t counter_tick = 0;

    for (;;)
    {
        counter_tick++;

        key_ret = key_scan(&key_status);

        if ((key_ret == KEY_OK) &&
            (key_status == KEY_PRESSED))
        {
            printf("Key_Pressed\r\n");

            if (xQueueSendToBack(
                    key_queue,
                    &counter_tick,
                    0) == pdTRUE)
            {
                printf("send successfully\r\n");
            }
            else
            {
                printf("send failed\r\n");
            }
        }

        osDelay(100);
    }
}
```

任务处理流程：

```text
任务开始运行
    |
    v
调用 key_scan()
    |
    v
判断扫描是否成功
    |
    v
判断按键是否按下
    |
    v
准备待发送数据
    |
    v
调用 xQueueSendToBack()
    |
    v
打印发送结果
    |
    v
延时后继续下一次轮询
```

---

## 8. FreeRTOS 原生队列 API

本实验使用 FreeRTOS 原生接口：

```c
xQueueCreate()
xQueueSendToBack()
xQueueReceive()
```

而不是 CMSIS-RTOS 队列接口。

需要包含：

```c
#include "queue.h"
```

`xQueueSendToBack()` 本身是一个宏，最终会展开为：

```c
xQueueGenericSend(
    key_queue,
    &counter_tick,
    0,
    queueSEND_TO_BACK
);
```

宏自动补充第四个参数 `queueSEND_TO_BACK`，用于指定消息插入队列尾部。

---

## 9. 实验验证

### 9.1 队列创建验证

上电后串口应打印：

```text
key_queue create successfully
```

如果打印：

```text
key_queue create failed
```

说明队列创建失败，常见原因是 FreeRTOS Heap 空间不足。

### 9.2 按键检测验证

按下按键后，串口应打印：

```text
Key_Pressed
```

### 9.3 队列发送验证

发送成功时，串口应打印：

```text
send successfully
```

如果发送失败，常见原因包括：

- 队列未创建成功；
- 队列已满；
- 队列句柄无效；
- 发送参数错误。

---

## 10. 当前已完成内容

- [x] 配置按键 GPIO 为上拉输入；
- [x] 完成 `bsp_key.h`；
- [x] 完成 `bsp_key.c`；
- [x] 完成 `key_scan()` 接口；
- [x] 创建按键检测任务；
- [x] 使用 FreeRTOS 原生队列 API；
- [x] 创建按键消息队列；
- [x] 检测到按键后发送消息；
- [x] 通过串口验证按键检测和消息发送。

---

## 11. 当前阶段需要注意的问题

### 11.1 机械按键抖动

机械按键按下瞬间可能产生多次电平变化。

当前阶段主要用于验证队列发送流程，后续需要增加：

- 软件消抖；
- 按下沿检测；
- 按下锁存；
- 松开后重新允许下一次触发。

### 11.2 长按重复发送

当前轮询方式下，如果按键持续按住，任务可能连续发送多条消息。

后续应实现：

```text
松开 -> 按下：发送一次消息
持续按住：不重复发送
按键松开：解除锁存
```

### 11.3 队列创建顺序

公共队列应在使用它的任务运行前创建。

推荐顺序：

```text
初始化 FreeRTOS
    |
    v
创建队列
    |
    v
检查队列是否成功
    |
    v
创建任务
    |
    v
启动调度器
```

### 11.4 队列发送方向

普通按键事件应使用：

```c
xQueueSendToBack()
```

保持先进先出的顺序。

`xQueueSendToFront()` 更适合紧急消息插队。

---

## 12. Git 提交步骤

确认当前位于 `Key` 分支：

```bash
git branch
```

添加修改：

```bash
git add .
```

提交代码：

```bash
git commit -m "feat: implement key polling and queue message sending"
```

推送到远程 `Key` 分支：

```bash
git push origin Key
```

---

## 13. 下一步计划

基于当前 `Key` 分支创建 `Key_Led` 分支：

```bash
git switch -c Key_Led
```

下一阶段将完成：

- 创建 LED 控制任务；
- 在 LED 任务中阻塞等待队列消息；
- 接收到按键消息后翻转 LED；
- 完成按键任务与 LED 任务之间的异步通信；
- 验证完整的生产者—队列—消费者模型。

最终流程：

```text
按键任务检测按键
        |
        v
发送队列消息
        |
        v
LED 任务接收消息
        |
        v
翻转 LED
```