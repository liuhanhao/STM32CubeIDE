# STM32F103C8TX HAL 项目

基于 STM32 HAL 库的 STM32F103C8TX 开发项目。

## 项目配置

- **芯片**: STM32F103C8TX (64KB Flash, 20KB RAM)
- **核心**: ARM Cortex-M3
- **系统时钟**: 72MHz (外部 8MHz 晶振 + PLL)
- **HAL 库**: STM32F1xx HAL Driver (已集成)

## 时钟配置

- 外部晶振 (HSE): 8MHz
- 系统时钟 (SYSCLK): 72MHz
- AHB 总线: 72MHz
- APB1 总线: 36MHz (最大 36MHz)
- APB2 总线: 72MHz (最大 72MHz)

## 目录结构

```
├── Inc/                          # 头文件
│   ├── main.h                    # 主程序头文件
│   ├── stm32f1xx_hal_conf.h      # HAL 配置
│   └── stm32f1xx_it.h            # 中断处理头文件
├── Src/                          # 源文件
│   ├── main.c                    # 主程序
│   ├── stm32f1xx_hal_msp.c       # MSP 初始化
│   ├── stm32f1xx_it.c            # 中断处理
│   ├── system_stm32f1xx.c        # 系统初始化
│   ├── syscalls.c                # 系统调用
│   └── sysmem.c                  # 内存管理
├── Drivers/                      # HAL 库
│   ├── STM32F1xx_HAL_Driver/     # HAL 驱动
│   └── CMSIS/                    # CMSIS 核心
└── Startup/                      # 启动文件
    └── startup_stm32f103c8tx.s   # 汇编启动代码
```

## 编译和烧录

### 在 STM32CubeIDE 中

1. **编译**: Project → Build Project (Ctrl+B)
2. **调试**: Run → Debug (F11)
3. **烧录**: Run → Run (Ctrl+F11)

### 命令行编译

```bash
# 清理
make clean

# 编译
make

# 烧录 (使用 ST-Link)
st-flash write build/项目名.bin 0x8000000
```

## 启用 HAL 模块

在 `Inc/stm32f1xx_hal_conf.h` 中启用需要的外设模块:

```c
#define HAL_GPIO_MODULE_ENABLED      // GPIO
#define HAL_UART_MODULE_ENABLED      // 串口
#define HAL_TIM_MODULE_ENABLED       // 定时器
#define HAL_ADC_MODULE_ENABLED       // ADC
#define HAL_I2C_MODULE_ENABLED       // I2C
#define HAL_SPI_MODULE_ENABLED       // SPI
// ... 更多模块
```

## 开发板引脚

STM32F103C8T6 常用引脚:

- **LED**: PC13 (板载 LED,低电平点亮)
- **USART1**: PA9 (TX), PA10 (RX)
- **USART2**: PA2 (TX), PA3 (RX)
- **I2C1**: PB6 (SCL), PB7 (SDA)
- **SPI1**: PA5 (SCK), PA6 (MISO), PA7 (MOSI)

## 参考资源

- [STM32F103 数据手册](https://www.st.com/resource/en/datasheet/stm32f103c8.pdf)
- [STM32F1 参考手册](https://www.st.com/resource/en/reference_manual/cd00171190.pdf)
- [HAL 用户手册](https://www.st.com/resource/en/user_manual/um1850.pdf)
