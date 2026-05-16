# CAN总线模块 API文档

## 📋 模块概述

CAN(Controller Area Network)控制器局域网总线用于工业和汽车应用:
- 支持CAN 2.0A和2.0B协议
- 标准帧(11位ID)和扩展帧(29位ID)
- 波特率最高1Mbps
- 3个发送邮箱
- 2个接收FIFO
- 28个过滤器组

---

## 📚 相关文件

- **头文件**: `stm32f1xx_hal_can.h`
- **源文件**: `stm32f1xx_hal_can.c`

---

## 🔧 数据类型

### CAN_TxHeaderTypeDef - 发送帧头

```c
typedef struct {
    uint32_t StdId;    // 标准ID (11位)
    uint32_t ExtId;    // 扩展ID (29位)
    uint32_t IDE;      // ID类型
    uint32_t RTR;      // 帧类型
    uint32_t DLC;      // 数据长度 (0-8)
} CAN_TxHeaderTypeDef;
```

### CAN_RxHeaderTypeDef - 接收帧头

```c
typedef struct {
    uint32_t StdId;
    uint32_t ExtId;
    uint32_t IDE;
    uint32_t RTR;
    uint32_t DLC;
    uint32_t Timestamp;
    uint32_t FilterMatchIndex;
} CAN_RxHeaderTypeDef;
```

---

## 🎯 核心函数

### HAL_CAN_Init()

初始化CAN。

```c
HAL_StatusTypeDef HAL_CAN_Init(CAN_HandleTypeDef *hcan);
```

---

### HAL_CAN_Start()

启动CAN。

```c
HAL_StatusTypeDef HAL_CAN_Start(CAN_HandleTypeDef *hcan);
```

---

### HAL_CAN_AddTxMessage()

添加发送消息。

```c
HAL_StatusTypeDef HAL_CAN_AddTxMessage(CAN_HandleTypeDef *hcan,
                                       CAN_TxHeaderTypeDef *pHeader,
                                       uint8_t aData[],
                                       uint32_t *pTxMailbox);
```

---

### HAL_CAN_GetRxMessage()

获取接收消息。

```c
HAL_StatusTypeDef HAL_CAN_GetRxMessage(CAN_HandleTypeDef *hcan,
                                       uint32_t RxFifo,
                                       CAN_RxHeaderTypeDef *pHeader,
                                       uint8_t aData[]);
```

---

## 💡 完整应用示例

### 示例1: CAN初始化和发送

```c
CAN_HandleTypeDef hcan;

void CAN_Init(void) {
    // CAN初始化
    hcan.Instance = CAN1;
    hcan.Init.Prescaler = 9;  // 波特率 = 36MHz / (9 * (1+7+8)) = 250Kbps
    hcan.Init.Mode = CAN_MODE_NORMAL;
    hcan.Init.SyncJumpWidth = CAN_SJW_1TQ;
    hcan.Init.TimeSeg1 = CAN_BS1_7TQ;
    hcan.Init.TimeSeg2 = CAN_BS2_8TQ;
    hcan.Init.TimeTriggeredMode = DISABLE;
    hcan.Init.AutoBusOff = DISABLE;
    hcan.Init.AutoWakeUp = DISABLE;
    hcan.Init.AutoRetransmission = ENABLE;
    hcan.Init.ReceiveFifoLocked = DISABLE;
    hcan.Init.TransmitFifoPriority = DISABLE;
    
    HAL_CAN_Init(&hcan);
    
    // 配置过滤器(接收所有消息)
    CAN_FilterTypeDef sFilterConfig;
    sFilterConfig.FilterBank = 0;
    sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
    sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
    sFilterConfig.FilterIdHigh = 0x0000;
    sFilterConfig.FilterIdLow = 0x0000;
    sFilterConfig.FilterMaskIdHigh = 0x0000;
    sFilterConfig.FilterMaskIdLow = 0x0000;
    sFilterConfig.FilterFIFOAssignment = CAN_RX_FIFO0;
    sFilterConfig.FilterActivation = ENABLE;
    HAL_CAN_ConfigFilter(&hcan, &sFilterConfig);
    
    // 启动CAN
    HAL_CAN_Start(&hcan);
    
    // 激活接收中断
    HAL_CAN_ActivateNotification(&hcan, CAN_IT_RX_FIFO0_MSG_PENDING);
}

void CAN_SendMessage(uint32_t id, uint8_t *data, uint8_t len) {
    CAN_TxHeaderTypeDef TxHeader;
    uint32_t TxMailbox;
    
    TxHeader.StdId = id;
    TxHeader.ExtId = 0;
    TxHeader.IDE = CAN_ID_STD;
    TxHeader.RTR = CAN_RTR_DATA;
    TxHeader.DLC = len;
    TxHeader.TransmitGlobalTime = DISABLE;
    
    HAL_CAN_AddTxMessage(&hcan, &TxHeader, data, &TxMailbox);
}
```

---

### 示例2: CAN接收(中断方式)

```c
// CAN接收中断回调
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan) {
    CAN_RxHeaderTypeDef RxHeader;
    uint8_t RxData[8];
    
    // 获取接收消息
    HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &RxHeader, RxData);
    
    // 处理接收到的数据
    printf("CAN RX ID:0x%03lX Data:", RxHeader.StdId);
    for (int i = 0; i < RxHeader.DLC; i++) {
        printf(" %02X", RxData[i]);
    }
    printf("\n");
}

// 使用示例
int main(void) {
    HAL_Init();
    SystemClock_Config();
    CAN_Init();
    
    uint8_t tx_data[] = {0x11, 0x22, 0x33, 0x44};
    
    while (1) {
        // 发送CAN消息
        CAN_SendMessage(0x123, tx_data, 4);
        HAL_Delay(1000);
    }
}
```

---

## ⚠️ 注意事项

### 1. 波特率计算

```
波特率 = APB1时钟 / (Prescaler × (1 + TimeSeg1 + TimeSeg2))
```

### 2. 终端电阻

CAN总线两端需要120Ω终端电阻。

### 3. 过滤器配置

合理配置过滤器可以减少CPU负担。

### 4. 总线仲裁

ID值越小,优先级越高。

---

## 📖 相关文档

- [GPIO通用IO](HAL_GPIO.md)
- [RCC时钟控制](HAL_RCC.md)
