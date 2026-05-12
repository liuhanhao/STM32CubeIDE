/**
  ******************************************************************************
  * @file    stm32f1xx_hal_conf.h
  * @brief   HAL 库的 MSP（MCU Support Package）配置文件，用于配置外设的底层初始化
  ******************************************************************************
  */

#ifndef __STM32F1xx_HAL_CONF_H
#define __STM32F1xx_HAL_CONF_H

#ifdef __cplusplus
 extern "C" {
#endif

/* ########################## 模块选择 #################################### */
/**
  * @brief 这里是 HAL 模块的启用/禁用开关
  *        注释掉不需要的模块以减小代码体积
  */
#define HAL_MODULE_ENABLED
#define HAL_CORTEX_MODULE_ENABLED
#define HAL_DMA_MODULE_ENABLED
#define HAL_FLASH_MODULE_ENABLED
#define HAL_GPIO_MODULE_ENABLED
#define HAL_PWR_MODULE_ENABLED
#define HAL_RCC_MODULE_ENABLED
/* #define HAL_ADC_MODULE_ENABLED */
/* #define HAL_CAN_MODULE_ENABLED */
/* #define HAL_I2C_MODULE_ENABLED */
/* #define HAL_SPI_MODULE_ENABLED */
/* #define HAL_TIM_MODULE_ENABLED */
/* #define HAL_UART_MODULE_ENABLED */
/* #define HAL_USART_MODULE_ENABLED */

/* ########################## 振荡器配置 ################################## */
/**
  * @brief 外部高速振荡器 (HSE) 频率 (Hz)
  *        STM32F103C8T6 通常使用 8MHz 外部晶振
  */
#if !defined  (HSE_VALUE) 
  #define HSE_VALUE    8000000U  /*!< 外部晶振频率 8MHz */
#endif

#if !defined  (HSE_STARTUP_TIMEOUT)
  #define HSE_STARTUP_TIMEOUT    100U   /*!< HSE 启动超时时间 (ms) */
#endif

/**
  * @brief 内部高速振荡器 (HSI) 频率 (Hz)
  */
#if !defined  (HSI_VALUE)
  #define HSI_VALUE    8000000U  /*!< 内部 RC 振荡器频率 8MHz */
#endif

/**
  * @brief 内部低速振荡器 (LSI) 频率 (Hz)
  */
#if !defined  (LSI_VALUE) 
 #define LSI_VALUE  40000U      /*!< LSI 典型值 40kHz */
#endif

/**
  * @brief 外部低速振荡器 (LSE) 频率 (Hz)
  */
#if !defined  (LSE_VALUE)
 #define LSE_VALUE  32768U      /*!< LSE 频率 32.768kHz */
#endif

#if !defined  (LSE_STARTUP_TIMEOUT)
  #define LSE_STARTUP_TIMEOUT    5000U   /*!< LSE 启动超时时间 (ms) */
#endif

/* ########################### 系统配置 ################################### */
/**
  * @brief VDD 电压值 (单位: mV)
  */
#define  VDD_VALUE                    3300U /*!< VDD 电压 3.3V */

/**
  * @brief Tick 中断优先级
  */
#define  TICK_INT_PRIORITY            0U    /*!< tick 中断优先级 */

/**
  * @brief 使用 RTOS
  */
#define  USE_RTOS                     0U

/**
  * @brief 预取缓冲区使能
  */
#define PREFETCH_ENABLE               1U

/* ########################## 断言选择 #################################### */
/**
  * @brief 取消注释下面的行以启用完整的断言检查
  *        这会增加代码体积,建议在调试时启用
  */
/* #define USE_FULL_ASSERT    1U */

/* 包含模块头文件 */
#ifdef HAL_RCC_MODULE_ENABLED
  #include "stm32f1xx_hal_rcc.h"
#endif

#ifdef HAL_GPIO_MODULE_ENABLED
  #include "stm32f1xx_hal_gpio.h"
#endif

#ifdef HAL_DMA_MODULE_ENABLED
  #include "stm32f1xx_hal_dma.h"
#endif

#ifdef HAL_CORTEX_MODULE_ENABLED
  #include "stm32f1xx_hal_cortex.h"
#endif

#ifdef HAL_ADC_MODULE_ENABLED
  #include "stm32f1xx_hal_adc.h"
#endif

#ifdef HAL_CAN_MODULE_ENABLED
  #include "stm32f1xx_hal_can.h"
#endif

#ifdef HAL_FLASH_MODULE_ENABLED
  #include "stm32f1xx_hal_flash.h"
#endif

#ifdef HAL_I2C_MODULE_ENABLED
  #include "stm32f1xx_hal_i2c.h"
#endif

#ifdef HAL_PWR_MODULE_ENABLED
  #include "stm32f1xx_hal_pwr.h"
#endif

#ifdef HAL_SPI_MODULE_ENABLED
  #include "stm32f1xx_hal_spi.h"
#endif

#ifdef HAL_TIM_MODULE_ENABLED
  #include "stm32f1xx_hal_tim.h"
#endif

#ifdef HAL_UART_MODULE_ENABLED
  #include "stm32f1xx_hal_uart.h"
#endif

#ifdef HAL_USART_MODULE_ENABLED
  #include "stm32f1xx_hal_usart.h"
#endif

/* ########################## 断言定义 #################################### */
#ifdef  USE_FULL_ASSERT
/**
  * @brief  assert_param 宏用于函数参数检查
  * @param  expr: 如果 expr 为 false,将调用 assert_failed 函数
  *               报告源文件名和错误行号
  */
  #define assert_param(expr) ((expr) ? (void)0U : assert_failed((uint8_t *)__FILE__, __LINE__))
  void assert_failed(uint8_t* file, uint32_t line);
#else
  #define assert_param(expr) ((void)0U)
#endif

#ifdef __cplusplus
}
#endif

#endif /* __STM32F1xx_HAL_CONF_H */
