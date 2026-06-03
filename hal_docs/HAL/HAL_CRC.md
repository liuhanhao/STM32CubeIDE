# CRC校验模块 API文档

## 📋 模块概述

CRC(Cyclic Redundancy Check)循环冗余校验模块:
- 硬件CRC-32计算
- 多项式: 0x04C11DB7
- 32位数据输入
- 单周期计算

---

## 🗺️ 使用场景速查

**什么时候用硬件 CRC**

| 场景 | 说明 |
|------|------|
| 通信帧完整性校验（UART / CAN 帧末尾附 CRC） | 防止传输错误被当有效数据处理 |
| Bootloader 升级后固件校验 | 验证写入的 App 固件完整后再跳转执行 |
| Flash / EEPROM 存储数据校验 | 读出数据后校验 CRC，检测存储损坏 |
| 配置参数有效性判断 | 比单纯魔数更可靠，能检测出部分字段损坏 |

**硬件 CRC vs 软件 CRC**

| | 硬件 CRC（HAL） | 软件 CRC（查表法） |
|--|----------------|-----------------|
| 速度 | 每个 32 位字 1 个周期，极快 | 循环计算，较慢 |
| 多项式 | 固定 CRC-32（0x04C11DB7） | 可自定义（CRC-8、CRC-16 等） |
| 适用 | 大块数据校验、性能敏感场景 | 需要兼容特定协议（如 MODBUS CRC-16） |

> MODBUS RTU 使用 CRC-16（0x8005），STM32 硬件只支持 CRC-32，MODBUS 场景必须用软件实现。

**注意：输入数组是 `uint32_t`，不是字节**

```c
// 正确：传入 uint32_t 数组，长度为字数
HAL_CRC_Calculate(&hcrc, (uint32_t*)data, byte_len / 4);

// 字节数不是 4 的倍数时，需手动补零对齐后再传入
```

**累加计算（分段数据）**

```c
// 第一段
uint32_t crc = HAL_CRC_Calculate(&hcrc, buf1, len1 / 4);
// 后续段：在前次结果基础上继续计算
crc = HAL_CRC_Accumulate(&hcrc, buf2, len2 / 4);
```

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
