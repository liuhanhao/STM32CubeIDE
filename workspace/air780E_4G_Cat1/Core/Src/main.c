/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : 主程序：4G 模块通信 + OLED 显示
  *
  * 功能说明：
  *   1. 初始化 OLED（I2C，PB8=SCL，PB9=SDA）
  *   2. 通过 USART1（PA9=TX，PA10=RX）驱动 YED-C780 (Air780E) 4G 模块
  *   3. 支持 TCP / UDP / HTTP 三种模式（在 air780e.h 中切换宏）
  *   4. 将收到的数据或 HTTP 响应显示到 OLED 屏
  *
  * 接线速查：
  *   PA9  → 模块 RX（左侧第11脚）
  *   PA10 ← 模块 TX（左侧第10脚）
  *   PA1  → 模块 RST（左侧第9脚）
  *   GND  → 模块 GND（左侧第2/4脚，共地，必须）
  *
  *   供电二选一：
  *     3.3V → 模块 VBAT（左侧第5脚）  快速验证，不稳定时换下面
  *     5V   → 模块 VIN （左侧第1脚）  独立电源，推荐长期使用
  *
  ******************************************************************************
  */
#include "main.h"
#include "OLED.h"
#include "air780e.h"
#include <string.h>
#include <stdio.h>

/* ===================== 服务器 / URL 配置 ===================== */
/* TCP/UDP 模式：填写服务器 IP 和端口
 * 测试服务器：http://test.yinerda.com → 选择对应工具 → 打开 */
#define SERVER_IP   "43.139.170.206"
#define SERVER_PORT 1002

/* HTTP 模式：填写完整 URL * 测试服务器：http://test.yinerda.com → HTTP测试工具 → 打开 → 复制URL
 * 同时只选一个：定义 USE_HTTP_GET 或 USE_HTTP_POST */
//#define USE_HTTP_GET
#define USE_HTTP_POST

#define HTTP_GET_URL  "http://43.139.170.206:1000/61fad1aba7d721c64003e34427b5b922"
#define HTTP_POST_URL "http://43.139.170.206:1000/99ed3c3f3e6f18b65f129462dd73651a"
/* ============================================================= */

void SystemClock_Config(void);
static void MX_GPIO_Init(void);

int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();

  OLED_Init();
  OLED_ShowString(1, 1, "4G Init...");

/* -------- 模式初始化 -------- */
#if defined(USE_TCP) || defined(USE_UDP)
  int ret = Air780E_Init(SERVER_IP, SERVER_PORT);
#elif defined(USE_HTTP)
  int ret = Air780E_Init(NULL, 0);  /* HTTP 不需要预先连接 */
#elif defined(USE_MQTT)
  int ret = Air780E_Init(SERVER_IP, SERVER_PORT);  /* MQTT 需要 Broker 地址和端口 */
#else
  #error "请在 air780e.h 中定义 USE_TCP、USE_UDP、USE_HTTP 或 USE_MQTT 之一"
#endif

  if (ret != 0)
  {
    /* 失败：OLED 显示步骤编号
     * 1=串口通信  2=关回显  3=SIM卡  4=附着网络
     * 5=链路配置  6=快发/APN  7=APN/PDP  8=激活网络  9=查IP  10=建立连接 */
    uint8_t step = Air780E_GetFailStep();
    OLED_Clear();
    OLED_ShowString(1, 1, "FAILED step:");
    OLED_ShowNum(2, 1, step, 2);
    while (1) {}
  }

  OLED_Clear();
#if defined(USE_TCP)
  OLED_ShowString(1, 1, "TCP Connected");
  Air780E_Send("STM32 Online", 12);
#elif defined(USE_UDP)
  OLED_ShowString(1, 1, "UDP Ready");
#elif defined(USE_HTTP)
  OLED_ShowString(1, 1, "HTTP Ready");
#elif defined(USE_MQTT)
  OLED_ShowString(1, 1, "MQTT Connected");
  /* 上线后发一条上线通知到发布 topic */
  Air780E_MqttPublish(MQTT_PUB_TOPIC, "STM32 Online", 12);
#endif

  char recv_buf[128];

  while (1)
  {
/* ======================================================
 *  TCP 模式主循环：轮询接收，收到数据显示到 OLED 并回复
 * ====================================================== */
#ifdef USE_TCP
    /* TCP 断连检测 */
    if (Air780E_GetState() != AIR780E_STATE_CONNECTED)
    {
      OLED_Clear();
      OLED_ShowString(1, 1, "Disconnected");
      while (Air780E_GetState() != AIR780E_STATE_CONNECTED)
        HAL_Delay(500);
    }

    uint16_t len = Air780E_Recv(recv_buf, sizeof(recv_buf) - 1);
    if (len > 0)
    {
      recv_buf[len] = '\0';
      /* 过滤不可打印字符（\r\n 等），OLED 只支持 0x20~0x7E */
      for (uint16_t i = 0; i < len; i++)
        if (recv_buf[i] < 0x20 || recv_buf[i] > 0x7E) recv_buf[i] = ' ';
      while (len > 0 && recv_buf[len - 1] == ' ') recv_buf[--len] = '\0';

      /* 刷新 OLED（最多显示两行） */
      OLED_Clear();
      OLED_ShowString(1, 1, "Recv:");
      char display[17] = {0};
      uint16_t show = (len < 16) ? len : 16;
      memcpy(display, recv_buf, show);
      OLED_ShowString(2, 1, display);
      if (len > 16)
      {
        uint16_t remain = len - 16;
        show = (remain < 16) ? remain : 16;
        memcpy(display, recv_buf + 16, show);
        display[show] = '\0';
        OLED_ShowString(3, 1, display);
      }

      /* 回复 OK */
      Air780E_Send("OK", 2);
    }
    HAL_Delay(100);

/* ======================================================
 *  UDP 模式主循环：轮询接收（UDP 无断连概念）
 * ====================================================== */
#elif defined(USE_UDP)
    uint16_t len = Air780E_Recv(recv_buf, sizeof(recv_buf) - 1);
    if (len > 0)
    {
      recv_buf[len] = '\0';
      for (uint16_t i = 0; i < len; i++)
        if (recv_buf[i] < 0x20 || recv_buf[i] > 0x7E) recv_buf[i] = ' ';
      while (len > 0 && recv_buf[len - 1] == ' ') recv_buf[--len] = '\0';

      OLED_Clear();
      OLED_ShowString(1, 1, "Recv:");
      char display[17] = {0};
      uint16_t show = (len < 16) ? len : 16;
      memcpy(display, recv_buf, show);
      OLED_ShowString(2, 1, display);
      if (len > 16)
      {
        uint16_t remain = len - 16;
        show = (remain < 16) ? remain : 16;
        memcpy(display, recv_buf + 16, show);
        display[show] = '\0';
        OLED_ShowString(3, 1, display);
      }
      Air780E_Send("OK", 2);
    }
    HAL_Delay(100);

/* ======================================================
 *  HTTP 模式主循环
 *  USE_HTTP_GET：每 10 秒发一次 GET
 *  USE_HTTP_POST：每 10 秒发一次 POST
 * ====================================================== */
#elif defined(USE_HTTP)
    static uint32_t last_req = 0;
    uint32_t now = HAL_GetTick();

    if (now - last_req >= 10000)
    {
      last_req = now;

#ifdef USE_HTTP_GET
      OLED_Clear();
      OLED_ShowString(1, 1, "HTTP GET...");
      int code = Air780E_HttpGet(HTTP_GET_URL, recv_buf, sizeof(recv_buf));
      OLED_Clear();
      if (code == 200)
      {
        OLED_ShowString(1, 1, "GET 200 OK");
        char display[17] = {0};
        uint16_t show = (strlen(recv_buf) < 16) ? (uint16_t)strlen(recv_buf) : 16;
        memcpy(display, recv_buf, show);
        OLED_ShowString(2, 1, display);
      }
      else
      {
        OLED_ShowString(1, 1, "GET FAILED");
        OLED_ShowNum(2, 1, (uint32_t)(code < 0 ? 0 : code), 3);
      }

#elif defined(USE_HTTP_POST)
      OLED_Clear();
      OLED_ShowString(1, 1, "HTTP POST...");
      const char *post_body = "{\"device\":\"STM32\",\"msg\":\"hello\"}";
      int code = Air780E_HttpPost(HTTP_POST_URL,
                                  "application/json",
                                  post_body, (uint16_t)strlen(post_body),
                                  recv_buf, sizeof(recv_buf));
      OLED_Clear();
      if (code == 200)
      {
        OLED_ShowString(1, 1, "POST 200 OK");
        char display[17] = {0};
        uint16_t show = (strlen(recv_buf) < 16) ? (uint16_t)strlen(recv_buf) : 16;
        memcpy(display, recv_buf, show);
        OLED_ShowString(2, 1, display);
      }
      else
      {
        OLED_ShowString(1, 1, "POST FAILED");
        OLED_ShowNum(2, 1, (uint32_t)(code < 0 ? 0 : code), 3);
      }
#endif /* USE_HTTP_GET / USE_HTTP_POST */
    }

    HAL_Delay(100);

/* ======================================================
 *  MQTT 模式主循环：轮询订阅消息，收到后显示到 OLED 并回发确认
 * ====================================================== */
#elif defined(USE_MQTT)
    char mqtt_topic[64];

    uint16_t len = Air780E_MqttRecv(mqtt_topic, recv_buf, sizeof(recv_buf) - 1);
    if (len > 0)
    {
      recv_buf[len] = '\0';
      /* 过滤不可打印字符 */
      for (uint16_t i = 0; i < len; i++)
        if (recv_buf[i] < 0x20 || recv_buf[i] > 0x7E) recv_buf[i] = ' ';
      while (len > 0 && recv_buf[len - 1] == ' ') recv_buf[--len] = '\0';

      /* OLED 第1行显示 topic，第2行显示消息内容 */
      OLED_Clear();
      char display[17] = {0};
      uint16_t show = (strlen(mqtt_topic) < 16) ? (uint16_t)strlen(mqtt_topic) : 16;
      memcpy(display, mqtt_topic, show);
      OLED_ShowString(1, 1, display);

      show = (len < 16) ? len : 16;
      memcpy(display, recv_buf, show);
      display[show] = '\0';
      OLED_ShowString(2, 1, display);

      /* 回发确认：发布到 pub topic */
      Air780E_MqttPublish(MQTT_PUB_TOPIC, "OK", 2);
    }
    HAL_Delay(100);
#endif /* USE_TCP / USE_UDP / USE_HTTP / USE_MQTT */
  }
}

/* ==================== 系统函数 ==================== */
/**
  * @brief  系统时钟配置
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                              | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief  GPIO 初始化
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
}

/**
  * @brief  错误处理函数
  * @retval None
  */
void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}

#ifdef USE_FULL_ASSERT
/**
  * @brief  断言失败时报告文件名和行号
  * @param  file: 源文件名指针
  * @param  line: 错误所在行号
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif /* USE_FULL_ASSERT */
