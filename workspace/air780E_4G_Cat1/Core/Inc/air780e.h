#ifndef AIR780E_H
#define AIR780E_H

#include "stm32f1xx_hal.h"
#include <stdint.h>

/**
 * @file    air780e.h
 * @brief   YED-C780 (Air780E) 4G 模块驱动接口
 *          支持 TCP / UDP / HTTP 三种模式
 *
 * 协议切换：只保留其中一个宏，其余注释掉
 */

/* ===== 协议选择（四选一）===== */
//#define USE_TCP    /* TCP 非透传模式  */
//#define USE_UDP    /* UDP 非透传模式  */
//#define USE_HTTP   /* HTTP GET/POST   */
#define USE_MQTT      /* MQTT            */
/* ============================= */

/* 4G 模块状态 */
typedef enum {
    AIR780E_STATE_IDLE      = 0, /* 未初始化 */
    AIR780E_STATE_INIT      = 1, /* 初始化中 */
    AIR780E_STATE_READY     = 2, /* 网络就绪（已激活，未建立连接） */
    AIR780E_STATE_CONNECTED = 3, /* TCP/UDP 连接已建立 */
    AIR780E_STATE_ERROR     = 4  /* 初始化或连接失败 */
} Air780E_State;

/* ==================== 通用接口 ==================== */

/**
 * @brief  初始化模块（阻塞，约 15~60 秒）
 *
 *  TCP/UDP 模式：传入服务器 IP 和端口，函数内完成连接
 *  HTTP  模式：ip 和 port 传 NULL/0，函数只激活网络，不建立连接
 *
 * @retval 0 成功，-1 失败
 */
int Air780E_Init(const char *server_ip, uint16_t server_port);

/**
 * @brief  获取当前模块状态
 */
Air780E_State Air780E_GetState(void);

/**
 * @brief  获取初始化失败步骤编号（调试用）
 *         TCP/UDP：1~10；HTTP：1~9（无步骤10，用11/12代替网络激活步骤）
 */
uint8_t Air780E_GetFailStep(void);

/**
 * @brief  硬件复位模块（RST 高电平 1.2 秒）
 */
void Air780E_HardReset(void);

/**
 * @brief  USART1 中断回调，在 stm32f1xx_it.c 的 USART1_IRQHandler 中调用
 */
void Air780E_UART_IRQHandler(void);

/* ==================== TCP/UDP 专用接口 ==================== */
#if defined(USE_TCP) || defined(USE_UDP)

/**
 * @brief  发送数据（AT+CIPSEND 非透传方式）
 * @param  data 数据指针，len 字节数（最大 1460）
 * @retval 0 成功，-1 失败
 */
int Air780E_Send(const char *data, uint16_t len);

/**
 * @brief  接收数据，解析 +IPD,N:<data> 格式
 * @param  buf     输出缓冲区（至少 max_len+1 字节）
 * @param  max_len 最大读取字节数
 * @retval 实际读取字节数，0 表示暂无数据
 */
uint16_t Air780E_Recv(char *buf, uint16_t max_len);

#endif /* USE_TCP || USE_UDP */

/* ==================== HTTP 专用接口 ==================== */
#ifdef USE_HTTP

/**
 * @brief  发起 HTTP GET 请求
 * @param  url      完整 URL，例如 "http://example.com/api?key=val"
 * @param  resp_buf 响应体输出缓冲区
 * @param  buf_len  缓冲区大小
 * @retval HTTP 状态码（200/404等），-1 表示请求失败
 */
int Air780E_HttpGet(const char *url, char *resp_buf, uint16_t buf_len);

/**
 * @brief  发起 HTTP POST 请求
 * @param  url          完整 URL
 * @param  content_type Content-Type，例如 "application/json"
 * @param  body         POST 请求体数据
 * @param  body_len     请求体字节数
 * @param  resp_buf     响应体输出缓冲区
 * @param  buf_len      缓冲区大小
 * @retval HTTP 状态码（200/404等），-1 表示请求失败
 */
int Air780E_HttpPost(const char *url, const char *content_type,
                     const char *body, uint16_t body_len,
                     char *resp_buf, uint16_t buf_len);

#endif /* USE_HTTP */

/* ==================== MQTT 专用接口 ==================== */
#ifdef USE_MQTT

/* ---- MQTT 参数配置（在此处修改）----
 * 银尔达测试服务器：http://test.yinerda.com → MQTT测试工具 → 打开
 * 页面会给出 Broker IP/端口/ClientID/用户名/密码，复制填入下方
 * SERVER_IP 和 SERVER_PORT 在 main.c 里填写（Broker地址，端口通常 1883）*/
#define MQTT_CLIENT_ID  "798d3ae3c2a7e3828656d072861337d1"   /* 页面提供的 ClientID */
#define MQTT_USERNAME   "798d3ae3c2a7e3828656d072861337d1"    /* 页面提供的用户名    */
#define MQTT_PASSWORD   "88888888"    /* 页面提供的密码      */
#define MQTT_SUB_TOPIC  "test"   /* 订阅的 topic        */
#define MQTT_PUB_TOPIC  "test"   /* 发布用的 topic      */

/**
 * @brief  发布消息到指定 topic（AT+MPUBEX 定长方式，支持任意内容）
 * @param  topic   发布的 topic 字符串
 * @param  payload 消息内容
 * @param  len     消息字节数
 * @retval 0 成功，-1 失败
 */
int Air780E_MqttPublish(const char *topic, const char *payload, uint16_t len);

/**
 * @brief  轮询接收订阅消息，解析 +MSUB:"topic",len,data 格式
 * @param  topic_buf  输出：消息来源 topic（至少 64 字节）
 * @param  payload_buf 输出：消息内容
 * @param  buf_len    payload_buf 缓冲区大小
 * @retval 消息字节数，0 表示暂无新消息
 */
uint16_t Air780E_MqttRecv(char *topic_buf, char *payload_buf, uint16_t buf_len);

#endif /* USE_MQTT */

#endif /* AIR780E_H */
