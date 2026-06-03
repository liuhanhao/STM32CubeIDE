/**
  ******************************************************************************
  * @file    ble_hc08.h
  * @brief   HC-08 蓝牙模块驱动头文件（基于 USART1：PA9=TX, PA10=RX）
  *
  * ============================================================
  *  HC-08 BLE 协议信息（由模块固件内置，STM32 无需配置）
  * ============================================================
  *
  *  广播名称（默认）     : HC-08
  *                         可通过 AT 指令修改：AT+NAMExxx
  *
  *  透传服务 UUID        : 0xFFF0
  *  完整格式             : 0000FFF0-0000-1000-8000-00805F9B34FB
  *
  *  写入特征 UUID        : 0xFFF3  (Properties: Write)
  *  完整格式             : 0000FFF3-0000-1000-8000-00805F9B34FB
  *                         └─ Write → 手机发数据给 STM32
  *
  *  通知特征 UUID        : 0xFFF4  (Properties: Notify)
  *  完整格式             : 0000FFF4-0000-1000-8000-00805F9B34FB
  *                         └─ Notify → STM32 发数据给手机（需在 App 中先订阅）
  *
  *  说明：BLE 层由 HC-08 固件自动处理，STM32 只需通过 USART1
  *        收发字节，模块负责透传到手机 App 对应的特征上。
  * ============================================================
  ******************************************************************************
  */

#ifndef __BLE_HC08_H
#define __BLE_HC08_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f1xx_hal.h"

/* 接收缓冲区大小 */
#define BLE_RX_BUFFER_SIZE      128

/* AT 指令响应超时（ms），仅在蓝牙未连接时有效 */
#define BLE_AT_TIMEOUT_MS       500

/* ---------------------------------------------------------------
 * HC-08 BLE 协议常量（仅供参考，实际由模块固件处理，无需写入寄存器）
 *
 * ⚠️ 手机 App 使用以下 UUID 连接（来自官方用户手册 HC-08V3.1）：
 *   服务 UUID   : 0xFFE0   （AT+SUUID，默认 FFE0）
 *   透传 UUID   : 0xFFE1   （AT+TUUID，默认 FFE1，读/写/通知 共用此特征）
 *
 * 推荐 App（Android）: 汇承官方"BLE串口助手" http://www.hc01.com
 * 推荐 App（iOS）    : LightBlue / nRF Connect（手动选 FFE1 特征进行 Write）
 * --------------------------------------------------------------- */
#define BLE_DEVICE_NAME              "HC-08"   /* 默认广播名，AT+NAME 可改 */
#define BLE_SERVICE_UUID             0xFFE0    /* 透传服务 UUID（AT+SUUID 默认值）*/
#define BLE_CHARACTERISTIC_TRANSPARENT 0xFFE1 /* 透传特征 UUID（AT+TUUID 默认值，读/写/通知均使用此特征）*/

/* 无结束符时，超过此时间（ms）自动视为一条完整消息 */
#define BLE_RX_TIMEOUT_MS       50

/* 对外暴露 UART 句柄，供中断文件使用 */
extern UART_HandleTypeDef huart1;

/**
  * @brief  返回累计收到的原始字节数（调试用）
  * @retval 字节计数
  */
uint32_t BLE_GetRxByteCount(void);

/**
  * @brief  初始化 HC-08（USART1，9600 波特率）并通过 AT 指令测试模块
  * @retval HAL_OK=成功，HAL_ERROR=失败
  */
HAL_StatusTypeDef BLE_Init(void);

/**
  * @brief  发送 AT 指令并等待响应（仅在模块未连接状态下有效）
  * @param  cmd      AT 指令字符串（如 "AT"、"AT+NAME?"）
  * @param  respBuf  接收响应的缓冲区，NULL 表示忽略响应
  * @param  bufLen   缓冲区长度
  * @retval 1=收到响应，0=超时
  */
uint8_t BLE_SendAT(const char *cmd, char *respBuf, uint8_t bufLen);

/**
  * @brief  通过蓝牙发送字符串（阻塞）
  * @param  str  以 '\0' 结尾的字符串
  */
void BLE_SendString(const char *str);

/**
  * @brief  查询是否收到一条完整消息（以 '\n' 或 '\r' 结尾）
  * @retval 1=有消息，0=无消息
  */
uint8_t BLE_IsMessageReady(void);

/**
  * @brief  读取已接收的消息内容
  * @param  buf     存放消息的缓冲区
  * @param  maxLen  缓冲区最大长度
  */
void BLE_GetMessage(char *buf, uint8_t maxLen);

/**
  * @brief  清除消息标志，准备接收下一条
  */
void BLE_ClearMessage(void);

/**
  * @brief  超时检测，在主循环中轮询调用
  *         用于处理 App 发送不带 \r\n 结尾的数据
  */
void BLE_Poll(void);

#ifdef __cplusplus
}
#endif

#endif /* __BLE_HC08_H */
