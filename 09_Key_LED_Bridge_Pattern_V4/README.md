# mcu-rtos-learning-labs

MCU and RTOS learning labs, including STM32 drivers, communication
interfaces, FreeRTOS demos and embedded software modules.

## 09_Key_LED_Bridge_Pattern_V4

本工程用于练习基于 STM32 + FreeRTOS 的 LED 高阶驱动设计。

当前版本重点实现了 LED handler 层的异步控制框架：

* driver 层负责单个 LED 的基础控制；
* handler 层负责管理多个 LED driver；
* APP 层不直接操作 driver，而是通过 handler + index 控制目标 LED；
* handler 通过 OS queue 和 thread 实现异步控制链路。

## 1. Module Architecture

整体分层如下：

```text
APP
 |
 |  handler_1.pf_led_countroler(...)
 v
LED Handler Layer
 |
 |  register / control / queue / thread
 v
LED Driver Layer
 |
 |  led_on / led_off / delay / time_base
 v
GPIO / HAL / Board
```

### Driver Layer

driver 层用于描述和控制一盏具体 LED。

主要内容：

* `bsp_led_driver_t`
* `led_driver_inst()`
* `pf_led_countroler`
* `led_operations_t`
* `os_delay_t`
* `time_base_ms_t`

driver 层解决的问题是：

```text
一盏 LED 怎么亮、灭、闪烁。
```

### Handler Layer

handler 层用于统一管理多个 LED driver，并对 APP 提供更高层接口。

主要内容：

* `bsp_led_handler_t`
* `led_handler_inst()`
* `pf_led_register`
* `pf_led_countroler`
* `instance_registered_t`
* `led_index_t`
* `led_event_t`
* `handler_thread`

handler 层解决的问题是：

```text
多盏 LED 怎么注册、编号、统一管理，并通过异步方式控制。
```

## 2. New Features in V4

相比前一版本，本版本主要新增了 handler 层后半部分逻辑。

### 2.1 OS API Adapter

为了避免 handler 层直接依赖 FreeRTOS API，本版本为 OS 提供了抽象接口：

* queue create
* queue put
* queue get
* queue delete
* critical enter
* critical exit
* thread create
* thread delete
* delay
* time base

handler 层只调用抽象接口，例如：

```c
self->p_os_queue_interface->pf_os_queue_put(...);
```

而不是直接调用：

```c
xQueueSend(...);
```

这样做的目的是降低 handler 层和 FreeRTOS 的耦合度。后续如果更换 RTOS，只需要重新适配 OS 接口，handler 业务逻辑不需要大改。

### 2.2 LED Register

`led_register()` 用于把底层 LED driver 注册到 handler 中。

核心逻辑：

```text
1. 检查 handler 和 led_driver 是否有效；
2. 检查 led_driver 是否已经初始化；
3. 把 led_driver 指针保存到 led_instance_group[]；
4. 返回当前 LED 对应的 index；
5. led_instance_num 加 1。
```

注册后，APP 不再直接使用 `&led1` 控制 LED，而是使用 handler 返回的 index。

示例：

```c
led_index_t handler_1_led_index_1 = LED_NOT_INITIALIZED;

ret = handler_1.pf_led_register(&handler_1,
                                &led1,
                                &handler_1_led_index_1);
```

### 2.3 LED Control

`handler_led_control()` 是 handler 层提供给 APP 的 LED 控制接口。

APP 调用示例：

```c
ret = handler_1.pf_led_countroler(&handler_1,
                                  100U,
                                  1U,
                                  PROPORTIONN_1_2,
                                  handler_1_led_index_1);
```

该函数的核心职责不是直接控制 LED，而是把 APP 的控制请求打包成事件消息：

```text
Cycle_time
blink_times
proportion_on_off
index
```

然后通过 OS queue 发送给后台线程处理。

这样 APP 调用 control 后可以快速返回，不需要等待 LED 闪烁动作执行完成。

### 2.4 LED Event Queue

本版本定义了 `led_event_t`，用于描述一次 LED 控制请求。

```c
typedef struct {
    uint32_t       Cycle_time;
    uint32_t       blink_times;
    proportion_t   proportion_on_off;
    led_index_t    index;
} led_event_t;
```

它相当于一张 LED 控制工单。

control 函数负责生产事件：

```text
APP call
  -> handler_led_control()
  -> build led_event_t
  -> queue put
```

handler thread 负责消费事件：

```text
handler_thread()
  -> queue get
  -> parse led_event_t
  -> find target LED by index
  -> call driver control
```

### 2.5 Handler Thread

`handler_thread()` 是 LED handler 层的后台任务。

它的职责是：

```text
1. 接收创建线程时传入的 handler 参数；
2. 从 handler 中拿到 queue_handler；
3. 循环从 OS queue 中读取 led_event_t；
4. 收到事件后，根据 index 找到对应 LED driver；
5. 调用底层 driver 执行 LED 控制。
```

该线程让 LED 控制从同步调用变成异步处理。

整体链路：

```text
APP
 |
 | pf_led_countroler()
 v
handler_led_control()
 |
 | queue put led_event_t
 v
OS Queue
 |
 | queue get led_event_t
 v
handler_thread()
 |
 | led_instance_group[index]
 v
bsp_led_driver_t
 |
 | pf_led_countroler()
 v
LED action
```

## 3. Test Cases

### Test_1: Driver Layer Test

`Test_1()` 用于验证 driver 层基础功能。

测试内容：

* 创建 `led1`、`led2`
* 调用 `led_driver_inst()`
* 直接调用 driver 的 `pf_led_countroler()`

测试目标：

```text
验证单个 LED driver 可以独立完成初始化和控制。
```

### Test_2: Handler Layer Test

`Test_2()` 用于验证 handler 层主要链路。

测试内容：

* 创建并初始化 `handler_1`
* 注入 OS delay / queue / critical / thread / time base 接口
* 创建 `led1`、`led2`
* 注册 LED driver 到 handler
* 获取 `handler_1_led_index_1` 和 `handler_1_led_index_2`
* 通过 handler control API 发送 LED 控制请求
* 验证 queue put 和 handler thread queue get 链路

测试目标：

```text
验证 handler 能够完成初始化、注册、事件发送和线程接收。
```

### Test_3: System Integration Test

`Test_3()` 用于验证更接近系统运行状态的完整流程。

测试流程：

```text
1. System Starting
2. led_handler_inst()
3. led_driver_inst()
4. pf_led_register()
5. pf_led_countroler()
6. handler_thread receives event
```

在 `Test_3()` 中加入了 `while(1)`，用于避免测试函数返回后局部对象生命周期结束。

原因：

```text
handler_1 是局部变量，但它的地址被传给 handler_thread；
led1 是局部变量，但它的地址被注册进 handler；
如果 Test_3 返回，这些局部变量会失效；
后台线程继续访问这些地址，会产生悬空指针风险。
```

因此测试阶段使用：

```c
while(1);
```

让 `Test_3()` 不返回，保证 `handler_1` 和 `led1` 在测试过程中仍然有效。

工程化写法建议：

```c
static bsp_led_handler_t handler_1;
static bsp_led_driver_t  led1;
```

长期存在的 handler 和 driver 对象应放在 static/global 区，而不应依赖普通局部变量生命周期。

## 4. Key Concepts

### 4.1 Decoupling

本工程通过函数指针和接口表实现解耦。

handler 层不直接依赖 FreeRTOS API，而是依赖抽象接口：

```c
handler_os_queue_t
handler_os_thread_t
handler_os_critical_t
handler_time_base_ms_t
```

实际 FreeRTOS API 由 `freertos.c` 中的适配函数完成封装。

### 4.2 Index-Based Resource Management

handler 通过 index 管理多个 LED driver。

注册阶段：

```text
led_instance_group[index] = &led_driver;
```

控制阶段：

```text
APP passes index;
handler finds led_driver by index;
thread calls driver control.
```

这样 APP 不需要直接操作底层 driver 指针，降低了 APP 和 driver 层的耦合。

### 4.3 Asynchronous Control

本版本通过 queue + thread 实现异步控制。

同步方式：

```text
APP call control
  -> LED action
  -> APP blocked
```

异步方式：

```text
APP call control
  -> send event to queue
  -> APP returns
  -> handler_thread executes LED action later
```

异步方式可以避免 APP 被 LED 闪烁过程阻塞。

### 4.4 Object Lifetime

本版本测试过程中遇到了局部变量生命周期问题。

需要注意：

```text
如果一个对象的地址会被线程长期访问，
或者会被注册到其他模块长期保存，
这个对象不应该是普通局部变量。
```

适合使用：

* static variable
* global variable
* module private static object
* dynamically allocated object with clear release policy

## 5. Current Status

当前版本已完成：

* driver 层基础控制框架
* handler 层实例化
* OS API 抽象接口注入
* LED driver 注册机制
* index 引入
* control 函数实现
* led_event_t 消息结构
* queue put / queue get 链路
* handler_thread 后台任务框架
* Test_2 handler 层测试
* Test_3 系统集成测试
* 生命周期问题分析与临时处理

## 6. TODO

后续可继续完善：

* [ ] 确认 `led_event_t` 打包时包含 `.index = index`
* [ ] 完善 `handler_thread()` 中根据 `msg.index` 查找 LED driver 的逻辑
* [ ] 在线程中调用底层 driver 的 `pf_led_countroler()`
* [ ] 优化 `led_handler_inst()` 中 queue 和 thread 的创建顺序
* [ ] 将长期存在的 handler/driver 对象改为 static/global
* [ ] 增加非法 index 测试
* [ ] 增加 queue full 测试
* [ ] 增加多 LED 注册和多 LED 控制测试
* [ ] 增加 unregister 机制
* [ ] 增加 LED 状态查询接口

## 7. Learning Notes

本版本重点练习了以下嵌入式软件设计能力：

* C 语言结构体模拟对象
* 函数指针模拟接口
* self 指针传递当前对象
* OS API 适配层设计
* handler 和 driver 分层
* index 资源管理
* FreeRTOS queue
* FreeRTOS thread
* 异步事件驱动模型
* 局部变量生命周期和悬空指针问题

## 8. Commit Summary

This version adds the handler asynchronous control framework.

Main changes:

```text
feat(handler): add async LED control framework

- add handler control implementation
- add led_event_t event message
- send LED control request through OS queue
- add handler_thread for background event processing
- add index-based LED resource selection
- add Test_2 for handler layer testing
- add Test_3 for integration testing
- document object lifetime issue in Test_3
```
