# 07_Helloword_Git

## 项目简介

本项目是 STM32 串口重定向实验。

本节课主要完成了以下内容：

1. 使用 STM32CubeMX 生成基础工程代码；
2. 配置 USART1 串口；
3. 在 `main.c` 中实现 `printf` 串口重定向；
4. 在 FreeRTOS 默认任务中周期性打印字符串；
5. 使用 USB 转 TTL 模块连接电脑；
6. 通过串口助手显示 STM32 输出的串口信息。

本实验的核心目标是：

让 STM32 可以像电脑程序一样使用 `printf()` 输出调试信息，
并通过串口助手显示出来。

---

## 硬件平台

- MCU：STM32F411CEU6
- 开发环境：Keil MDK-ARM
- 配置工具：STM32CubeMX
- 通信外设：USART1
- 调试工具：USB 转 TTL 模块
- 上位机工具：串口助手

---

## 串口参数

USART1 配置如下：

| 参数 | 配置 |
| --- | --- |
| 波特率 | 115200 |
| 数据位 | 8 bit |
| 停止位 | 1 bit |
| 校验位 | None |
| 硬件流控 | None |
| 模式 | TX/RX |

也就是常说的：

```text
115200, 8N1
```

---

## 硬件连接方式

USB 转 TTL 与 STM32 的连接方式如下：

| USB 转 TTL | STM32 |
| --- | --- |
| TXD | USART1_RX |
| RXD | USART1_TX |
| GND | GND |

注意事项：

1. TXD 和 RXD 需要交叉连接；
2. GND 必须共地；
3. USB 转 TTL 模块的电平需要和 STM32 兼容；
4. 本实验主要使用 USART1_TX 向电脑发送数据。

---

## 软件实现思路

在普通电脑程序中，`printf()` 可以直接把内容打印到终端。

但是在 STM32 单片机中，没有默认的终端窗口。

所以需要把 `printf()` 的底层输出函数改写掉，让它不要输出到屏幕，
而是通过 USART1 串口发送出去。

整体流程如下：

```text
printf()
   ↓
fputc() / _io_putchar()
   ↓
HAL_UART_Transmit()
   ↓
USART1_TX 引脚输出 TTL 串口信号
   ↓
USB 转 TTL 模块
   ↓
电脑串口助手显示
```

---

## 核心代码

### 1. 引入头文件

为了使用 `printf()`，需要在 `main.c` 中包含：

```c
#include <stdio.h>
```

---

### 2. 实现 printf 串口重定向

```c
#ifdef __GNUC__
    #define PUTCHAR_PROTOTYPE int _io_putchar(int ch)
#else
    #define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif

PUTCHAR_PROTOTYPE
{
    HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, 0xFFFF);
    return ch;
}
```

这段代码的作用是：

当程序调用 `printf()` 时，底层会把要打印的内容拆成一个个字符。

每一个字符最终都会进入 `fputc()` 或 `_io_putchar()`。

在这个函数里面，再调用：

```c
HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, 0xFFFF);
```

把这个字符通过 USART1 发送出去。

所以 `printf()` 能够显示在串口助手上，本质上是因为：

```text
printf 的底层字符输出函数被我们改成了串口发送函数
```

---

### 3. FreeRTOS 默认任务中打印信息

在 FreeRTOS 默认任务中周期性调用：

```c
printf("Hellow Eternal Chip\r\n");
```

程序运行后，串口助手会不断显示：

```text
Hellow Eternal Chip
Hellow Eternal Chip
Hellow Eternal Chip
...
```

---

## 运行步骤

1. 使用 STM32CubeMX 配置 USART1；
2. 生成 Keil MDK-ARM 工程；
3. 在 `main.c` 中添加 `printf` 重定向代码；
4. 在 FreeRTOS 默认任务中添加打印代码；
5. 编译工程；
6. 下载程序到开发板；
7. 使用 USB 转 TTL 模块连接 STM32 和电脑；
8. 打开串口助手；
9. 选择对应 COM 口；
10. 设置波特率为 115200；
11. 复位开发板；
12. 查看串口助手输出信息。

---

## 实验现象

程序下载后，打开串口助手，可以看到 STM32 周期性输出：

```text
Hellow Eternal Chip
```

说明：

1. USART1 初始化成功；
2. USB 转 TTL 连接正常；
3. `printf` 重定向成功；
4. STM32 已经可以通过串口输出调试信息。

---

## 本节课学习重点

### 1. 串口的作用

串口是 MCU 和外部设备通信的一种常用方式。

在嵌入式开发中，串口常用于：

- 打印调试信息；
- 和上位机通信；
- 接收控制命令；
- 传输传感器数据；
- 作为日志输出通道。

---

### 2. USB 转 TTL 的作用

STM32 输出的是 TTL 串口信号。

电脑 USB 口不能直接识别 TTL 串口信号。

所以中间需要 USB 转 TTL 模块进行转换。

转换流程如下：

```text
STM32 USART TTL 信号
        ↓
USB 转 TTL 模块
        ↓
电脑 USB 口
        ↓
虚拟 COM 口
        ↓
串口助手显示
```

常见 USB 转 TTL 芯片有：

- CH340
- CP2102
- FT232

---

### 3. printf 重定向的本质

`printf()` 本身只负责格式化字符串。

真正把字符发出去的，是底层输出函数。

在 STM32 中，我们重写底层输出函数：

```c
fputc()
```

或者：

```c
_io_putchar()
```

然后在里面调用串口发送函数：

```c
HAL_UART_Transmit()
```

这样 `printf()` 打印的内容就会通过 USART 发出去。

---

## 当前工程状态

当前工程已经完成：

- STM32CubeMX 基础工程生成；
- USART1 初始化；
- FreeRTOS 默认任务创建；
- `printf` 串口重定向；
- 串口助手显示打印信息。

---

## 后续扩展方向

后续可以在当前工程基础上继续扩展：

1. 串口轮询接收；
2. 串口中断接收；
3. 串口 DMA 接收；
4. 空闲中断接收不定长数据；
5. 环形缓冲区；
6. 串口日志模块；
7. 串口命令行交互；
8. 驱动层封装。

---

## 总结

本实验完成了 STM32 串口调试中非常重要的一步：`printf` 重定向。

以前调试单片机程序时，只能通过 LED 闪烁大概判断程序是否运行。

现在可以直接使用：

```c
printf("debug info\r\n");
```

通过串口助手观察程序运行状态。

这对后续调试驱动、RTOS 任务、通信协议和业务逻辑非常重要。

---

## 费曼总结

我这节课做的是 STM32 串口打印实验。

单片机本身没有电脑那种控制台窗口，所以 `printf()` 默认不知道应该把内容显示到哪里。

为了解决这个问题，我重写了 `printf()` 底层用到的输出函数，
也就是 `fputc()` 或 `_io_putchar()`。

然后我在这个函数里面调用 `HAL_UART_Transmit()`，
让每一个要打印的字符都从 USART1 发出去。

STM32 发出来的是 TTL 串口信号，电脑 USB 口不能直接识别，
所以中间需要一个 USB 转 TTL 模块。

最后电脑识别出一个 COM 口，串口助手打开这个 COM 口后，
就能看到 STM32 打印出来的字符串。

所以，串口重定向的本质就是：

```text
把 printf 的输出方向，从默认输出，改成 USART 串口输出。
```