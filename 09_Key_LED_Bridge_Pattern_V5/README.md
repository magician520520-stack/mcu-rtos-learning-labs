# LED Bridge Pattern Driver - Handler Layer Business Logic

## 1. 项目简介

本项目是基于 **桥接模式思想** 设计的 LED 驱动框架，主要用于学习嵌入式软件中的分层设计、面向对象 C 写法、函数指针封装、FreeRTOS 队列通信以及异步控制思想。

当前版本已经完成了：

* LED Driver 层封装
* LED Handler 层封装
* 多 LED 注册管理
* FreeRTOS Queue 消息传递
* Handler Thread 消息接收
* Event Process 事件处理
* Handler 层 LED 闪烁业务逻辑闭环
* Test1 / Test2 / Test3 单元测试与集成测试

本版本的重点是：
**Handler 线程收到 LED 控制消息后，可以通过 `event_process` 找到对应 LED Driver，并调用 `led_blink_handler` 执行实际闪烁逻辑。**

---

## 2. 当前实现目标

前几个版本中，LED 控制链路已经完成到：

```text
APP 层调用 handler 控制接口
        |
        v
handler_led_control()
        |
        v
封装 led_event_t
        |
        v
queue_put()
        |
        v
handler_thread()
        |
        v
queue_get() 收到消息
```

但是旧版本只完成了“消息能被线程收到”，还没有真正根据消息内容控制 LED。

本版本继续补全后半段链路：

```text
handler_thread()
        |
        v
__event_process()
        |
        v
根据 msg.index 找到对应 LED Driver
        |
        v
写入 cycle_time / blink_times / proportion
        |
        v
led_blink_handler()
        |
        v
调用 pf_led_on / pf_led_off
        |
        v
LED 执行闪烁动作
```

最终形成完整的异步 LED 控制闭环。

---

## 3. 代码分层说明

### 3.1 Driver 层

核心文件：

```text
bsp_led_driver.h
bsp_led_driver.c
```

Driver 层用于描述“一颗 LED 对象”。

每一个 `bsp_led_driver_t` 对象内部保存：

* 初始化状态
* 闪烁周期 `cycle_time_ms`
* 闪烁次数 `blink_times`
* 亮灭比例 `proportion_on_off`
* LED on/off 操作接口
* OS delay 接口
* time base 接口
* LED 控制函数指针 `pf_led_countroler`

Driver 层的主要职责是：

```text
管理单颗 LED 的底层控制能力
```

---

### 3.2 Handler 层

核心文件：

```text
bsp_led_handler.h
bsp_led_handler.c
```

Handler 层用于管理多个 LED Driver。

每一个 `bsp_led_handler_t` 对象内部保存：

* 当前 Handler 初始化状态
* LED Driver 注册数组
* 已注册 LED 数量
* Queue 句柄
* Thread 句柄
* OS Queue 接口
* OS Thread 接口
* Critical Section 接口
* 对 APP 暴露的 LED 控制接口
* 对 Driver 暴露的 LED 注册接口

Handler 层的主要职责是：

```text
接收 APP 层请求
通过 Queue 异步传递消息
在线程中取出消息
根据 index 找到对应 Driver
执行 LED 业务逻辑
```

---

### 3.3 APP / Test 层

核心文件：

```text
freertos.c
```

当前主要在 `freertos.c` 中完成测试代码，包括：

* Driver 层单元测试
* Handler 层集成测试
* Queue / Thread / Critical 接口适配
* Test1 / Test2 / Test3 测试流程

---

## 4. 本版本新增内容

### 4.1 event_process 事件处理函数

本版本新增事件处理函数，用于处理 Handler Thread 从 Queue 中收到的 LED 控制消息。

核心职责：

```text
1. 检查 msg.index 是否合法
2. 检查对应位置是否已经注册 LED Driver
3. 根据 index 找到目标 LED Driver
4. 将 msg 中的控制参数写入目标 Driver
5. 调用 led_blink_handler() 执行 LED 闪烁
```

事件结构体：

```c
typedef struct {
    uint32_t      Cycle_time;
    uint32_t      blink_times;
    proportion_t  proportion_on_off;
    led_index_t   index;
} led_event_t;
```

其中：

* `Cycle_time`：LED 闪烁周期
* `blink_times`：LED 闪烁次数
* `proportion_on_off`：LED 亮灭比例
* `index`：目标 LED 在 Handler 注册数组中的编号

---

### 4.2 handler_thread 线程逻辑完善

Handler 线程主要负责从 Queue 中取出消息。

当前逻辑：

```text
1. 获取 Handler 对象指针
2. 进入无限循环
3. 调用 queue_get() 从队列中取消息
4. 如果成功收到消息，则调用 __event_process()
5. 周期性 delay，等待下一次消息
```

线程函数不直接处理复杂业务，而是把具体业务交给 `__event_process()`。

这样做的好处是：

```text
handler_thread 只负责调度
__event_process 负责处理业务
代码结构更清晰，后续更容易扩展
```

---

### 4.3 led_blink_handler 闪烁逻辑

本版本在 Handler 层实现了 `led_blink_handler()`。

该函数的核心逻辑来自 Driver 层的 `led_blink()`，主要用于根据 Driver 对象中的参数执行 LED 闪烁。

主要步骤：

```text
1. 检查 Driver 对象是否有效
2. 读取 cycle_time_ms / blink_times / proportion_on_off
3. 根据亮灭比例计算 led_toggle_time
4. 通过循环执行 LED on/off
5. 通过 OS delay 控制时间
```

亮灭比例计算逻辑：

```text
PROPORTIONN_1_1 -> cycle_time / 2
PROPORTIONN_1_2 -> cycle_time / 3
PROPORTIONN_1_3 -> cycle_time / 4
```

例如：

```text
cycle_time = 100ms
proportion = 1:2

则：
亮灯时间 = 100 / 3
灭灯时间 = 100 - 100 / 3
```

---

## 5. 完整运行流程

### 5.1 初始化阶段

系统启动后，首先完成 Handler 和 Driver 的实例化。

```text
led_handler_inst()
        |
        |-- 挂载 OS delay 接口
        |-- 挂载 OS queue 接口
        |-- 挂载 OS critical 接口
        |-- 挂载 OS thread 接口
        |-- 创建 Handler Thread
        |-- 创建 Queue
        |-- 初始化 LED Driver 注册数组
        |
        v
Handler 初始化完成
```

Driver 初始化流程：

```text
led_driver_inst()
        |
        |-- 挂载 LED on/off 接口
        |-- 挂载 OS delay 接口
        |-- 挂载 time base 接口
        |-- 挂载 pf_led_countroler 控制接口
        |
        v
Driver 初始化完成
```

---

### 5.2 注册阶段

APP 或系统初始化代码调用：

```c
handler_1.pf_led_register(&handler_1, &led1, &handler_1_led_index_1);
```

注册后：

```text
handler_1.instances.led_instance_group[0] = &led1
handler_1_led_index_1 = 0
handler_1.instances.led_instance_num++
```

`index` 是 Handler 管理多个 LED 的关键。

后续 APP 层不需要直接操作 `&led1`，只需要通过 `index` 指定目标 LED。

---

### 5.3 控制阶段

APP 层调用：

```c
handler_1.pf_led_countroler(&handler_1,
                            100U,
                            1U,
                            PROPORTIONN_1_2,
                            handler_1_led_index_1);
```

该调用不会直接闪灯，而是先封装消息：

```text
led_event_t event = {
    Cycle_time        = 100,
    blink_times       = 1,
    proportion_on_off = PROPORTIONN_1_2,
    index             = handler_1_led_index_1
}
```

然后发送到 Queue：

```text
queue_put(event)
```

Handler Thread 再从 Queue 中取出该消息：

```text
queue_get(msg)
```

最后交给事件处理函数：

```text
__event_process(handler, msg)
```

---

### 5.4 事件处理阶段

`__event_process()` 根据 `msg.index` 找到对应 LED Driver：

```text
p_driver = handler.instances.led_instance_group[msg.index]
```

然后将消息中的参数写入目标 Driver：

```text
p_driver->cycle_time_ms     = msg.Cycle_time
p_driver->blink_times       = msg.blink_times
p_driver->proportion_on_off = msg.proportion_on_off
```

最后调用：

```text
led_blink_handler(p_driver)
```

执行 LED 闪烁动作。

---

## 6. 测试说明

### 6.1 Test_1：Driver 层测试

测试目标：

```text
验证单颗 LED Driver 能否独立完成初始化和控制
```

主要测试内容：

* 创建 `bsp_led_driver_t led1`
* 创建 `bsp_led_driver_t led2`
* 调用 `led_driver_inst()`
* 调用 `pf_led_countroler()`
* 验证 Driver 层单独控制 LED 的能力

---

### 6.2 Test_2：Handler + Driver 集成测试

测试目标：

```text
验证 Handler 能否管理多个 LED Driver
```

主要测试内容：

* 创建 Handler 对象
* 创建多个 Driver 对象
* 初始化 Handler
* 初始化 Driver
* 将 Driver 注册到 Handler
* 获取每个 LED 对应的 index
* 通过 Handler 控制指定 LED

---

### 6.3 Test_3：模拟真实业务调用流程

测试目标：

```text
模拟系统启动后，APP 层通过 Handler 控制 LED
```

主要流程：

```text
1. 系统启动
2. 初始化 Handler
3. 初始化 Driver
4. 注册 LED
5. APP 调用 Handler 控制接口
6. Handler 将控制请求发送到 Queue
7. Handler Thread 收到消息
8. event_process 处理消息
9. led_blink_handler 执行闪烁
```

该测试更接近真实项目中的业务调用方式。

---

## 7. 当前代码架构图

```text
APP / Test Layer
        |
        | pf_led_countroler()
        v
LED Handler Layer
        |
        | handler_led_control()
        v
led_event_t
        |
        | queue_put()
        v
FreeRTOS Queue
        |
        | queue_get()
        v
handler_thread()
        |
        | __event_process()
        v
LED Driver Object
        |
        | led_blink_handler()
        v
LED Operation Interface
        |
        | pf_led_on() / pf_led_off()
        v
GPIO / Board Support Layer
```

---

## 8. 当前完成状态

当前版本已经完成：

* [x] Driver 层对象化封装
* [x] Driver 层控制接口挂载
* [x] Handler 层对象化封装
* [x] Handler 管理多个 LED Driver
* [x] LED Driver 注册机制
* [x] LED index 分配机制
* [x] Queue 创建与消息发送
* [x] Handler Thread 创建
* [x] Handler Thread 接收 Queue 消息
* [x] event_process 事件处理
* [x] led_blink_handler 闪烁业务执行
* [x] Test1 / Test2 / Test3 测试流程

---

## 9. 当前版本的不足与后续优化

### 9.1 Handler 直接修改 Driver 成员

当前版本中，Handler 层会直接访问 Driver 结构体中的成员：

```c
driver->cycle_time_ms
driver->blink_times
driver->proportion_on_off
```

这种方式能跑通流程，但从工程封装角度看，Handler 不应该直接修改 Driver 内部状态。

后续可以优化为：

```c
driver->pf_led_countroler(driver,
                          msg.Cycle_time,
                          msg.blink_times,
                          msg.proportion_on_off);
```

或者为 Driver 增加专门的参数配置接口：

```c
driver->pf_led_config_set(driver,
                          msg.Cycle_time,
                          msg.blink_times,
                          msg.proportion_on_off);
```

这样可以让 Driver 自己管理内部状态，Handler 只负责分发请求。

---

### 9.2 Handler 线程内部仍然是串行执行

当前版本中，APP 层调用 Handler 控制接口后，可以快速返回，因此 APP 层是异步的。

但是 Handler Thread 内部执行 `led_blink_handler()` 时，会被 LED 闪烁过程占用。

也就是说：

```text
APP 层异步
Handler Thread 内部串行
```

后续可以考虑：

* 使用软件定时器优化 LED 闪烁
* 使用状态机替代阻塞式 delay
* 支持多个 LED 任务并发控制
* 支持 LED 常亮、常灭、闪烁、错误码闪烁等事件类型

---

### 9.3 参数检查仍可继续完善

后续可以继续增强：

* index 合法性检查
* Driver 是否注册检查
* Queue 创建返回值检查
* Thread 创建失败回滚处理
* 临界区保护完整性
* 非法 proportion 处理
* NULL 指针保护

---

## 10. 本版本学习重点

本版本重点学习了：

```text
1. 如何把业务请求封装成 event
2. 如何通过 Queue 实现异步消息传递
3. Handler Thread 如何从 Queue 中取消息
4. event_process 如何解析消息
5. 如何通过 index 找到对应 LED Driver
6. 如何让 Handler 层业务逻辑最终驱动 LED 动作
7. 线程函数和业务处理函数如何拆分
```

本版本的核心认知是：

```text
thread 函数只负责接收和调度；
event_process 负责解析和分发；
led_blink_handler 负责真正执行动作。
```

---

## 11. 总结

本次代码实现了 LED Handler 层的业务逻辑闭环。

相比之前版本，本版本不再只是完成 Queue 消息发送和接收，而是进一步完成了：

```text
收到消息
  -> 解析消息
  -> 找到目标 Driver
  -> 写入控制参数
  -> 执行 LED 闪烁
```

因此，当前 LED 控制框架已经初步具备了完整的异步控制能力。

整体链路可以总结为：

```text
APP 发请求
Handler 打包 event
Queue 传递 event
Thread 接收 event
event_process 解析 event
根据 index 找到 Driver
led_blink_handler 执行闪烁
```

该版本是从 Driver 层、Handler 层、RTOS Queue、Thread 到业务逻辑层的一次完整串联。
