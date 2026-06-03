#ifndef __DHT11_H
#define __DHT11_H

#include "stm32f1xx_hal.h"

/* DHT11 数据引脚：PA0 */
#define DHT11_PORT   GPIOA
#define DHT11_PIN    GPIO_PIN_0

/* 函数返回码 */
#define DHT11_OK     0
#define DHT11_ERROR  1

/* 湿度舒适度判断阈值 */
#define DHT11_HUMI_DRY_MAX      40   /* 低于此值：Dry     干燥 */
#define DHT11_HUMI_COMFORT_MAX  60   /* 低于此值：Comfort 舒适，超过则为 Humid 潮湿 */

/* 温度舒适度判断阈值 */
#define DHT11_TEMP_COOL_MAX     22   /* 低于此值：Cool 凉爽 */
#define DHT11_TEMP_HOT_MIN      30   /* 高于等于此值：Hot  闷热，中间段为 Nice 舒适 */

/* DHT11 读取结果 */
typedef struct {
    uint8_t humi_int;  /* 湿度整数部分 */
    uint8_t humi_dec;  /* 湿度小数部分（DHT11 通常为 0）*/
    uint8_t temp_int;  /* 温度整数部分 */
    uint8_t temp_dec;  /* 温度小数部分（DHT11 通常为 0）*/
} DHT11_Data_t;

void    DHT11_Init(void);
uint8_t DHT11_Read(DHT11_Data_t *data);

#endif /* __DHT11_H */
