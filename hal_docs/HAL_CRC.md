# CRC校验模块 API文档

## 📋 模块概述

CRC(Cyclic Redundancy Check)循环冗余校验模块:
- 硬件CRC-32计算
- 多项式: 0x04C11DB7
- 32位数据输入
- 单周期计算

---

## 📚 相关文件

- **头文件**: `stm32f1xx_hal_crc.h`
- **源文件**: `stm32f1xx_hal_crc.c`

---

## 🎯 核心函数

### HAL_CRC_Init()

初始化CRC。

```c
HAL_StatusTypeDef HAL_CRC_Init(CRC_HandleTypeDef *hcrc);
```

---

### HAL_CRC_Calculate()

计算CRC值。

```c
uint32_t HAL_CRC_Calculate(CRC_HandleTypeDef *hcrc,
                           uint32_t pBuffer[],
                           uint32_t BufferLength);
```

---

### HAL_CRC_Accumulate()

累加计算CRC。

```c
uint32_t HAL_CRC_Accumulate(CRC_HandleTypeDef *hcrc,
                            uint32_t pBuffer[],
                            uint32_t BufferLength);
```

---

## 💡 应用示例

### 示例1: 数据校验

```c
CRC_HandleTypeDef hcrc;

void CRC_Init(void) {
    __HAL_RCC_CRC_CLK_ENABLE();
    
    hcrc.Instance = CRC;
    HAL_CRC_Init(&hcrc);
}

uint32_t Calculate_CRC(uint8_t *data, uint32_t len) {
    // 将字节数组转换为字数组
    uint32_t word_len = (len + 3) / 4;
    uint32_t *word_data = (uint32_t*)data;
    
    return HAL_CRC_Calculate(&hcrc, word_data, word_len);
}

// 使用示例
void CRC_Test(void) {
    uint8_t data[] = "Hello CRC!";
    uint32_t crc = Calculate_CRC(data, sizeof(data)-1);
    printf("CRC: 0x%08lX\n", crc);
}
```

---

### 示例2: 数据完整性检查

```c
typedef struct {
    uint8_t data[256];
    uint32_t crc;
} DataPacket_t;

// 发送数据包
void Send_Packet(DataPacket_t *packet, uint32_t len) {
    // 计算CRC
    packet->crc = Calculate_CRC(packet->data, len);
    
    // 发送数据
    // ...
}

// 接收数据包
bool Receive_Packet(DataPacket_t *packet, uint32_t len) {
    // 接收数据
    // ...
    
    // 验证CRC
    uint32_t calc_crc = Calculate_CRC(packet->data, len);
    if (calc_crc == packet->crc) {
        printf("CRC校验通过\n");
        return true;
    } else {
        printf("CRC校验失败\n");
        return false;
    }
}
```

---

## ⚠️ 注意事项

### 1. 数据对齐

CRC模块按32位字处理数据,注意数据对齐。

### 2. 时钟使能

使用前必须使能CRC时钟:
```c
__HAL_RCC_CRC_CLK_ENABLE();
```

---

## 📖 相关文档

- [HAL核心](HAL_Core.md)
