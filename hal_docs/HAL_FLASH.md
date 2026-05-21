# FLASH闪存模块 API文档

## 📋 模块概述

FLASH模块提供内部Flash存储器的编程和擦除功能,包括:
- Flash编程(半字写入)
- 页擦除和批量擦除
- 选项字节编程
- 写保护配置
- 读保护配置

STM32F1 Flash特性:
- 容量: 16KB~512KB(根据型号)
- 页大小: 1KB(小容量/中容量) 或 2KB(大容量/超大容量)
- 编程单位: 半字(16位)

---

## 🗺️ 使用场景速查

**什么时候用内部 Flash 存储数据**

| 场景 | 说明 |
|------|------|
| 保存设备配置参数（波特率、设备 ID、校准值） | 断电不丢失，开机读取，写入频率极低 |
| Bootloader + App 固件升级 | 通过串口/CAN 接收新固件写入 Flash 后跳转 |
| 出厂信息（序列号、硬件版本） | 生产烧录，程序只读不写 |
| 少量不频繁变化的持久化数据 | 替代外部 EEPROM，节省成本和引脚 |

**内部 Flash vs 外部存储怎么选**

| 对比 | 内部 Flash | AT24C02 EEPROM | W25Q SPI Flash |
|------|-----------|----------------|----------------|
| 擦写寿命 | ~10,000 次 | ~1,000,000 次 | ~100,000 次 |
| 适合频繁写入 | ❌ | ✅ | ✅ |
| 无需额外硬件 | ✅ | ❌ | ❌ |
| 存储容量 | 受限（别占代码区） | 256B ~ 64KB | 1MB ~ 128MB |

> 规则：写入频率低（每天 <几次）→ 内部 Flash；写入频繁 → 外部 EEPROM；存大文件/固件 → 外部 SPI Flash。

**地址规划（STM32F103C8T6，64KB Flash）**

```
0x08000000 ~ 0x0800FBFF  程序代码区（~63KB）
0x0800FC00 ~ 0x0800FFFF  用户数据区（最后 1KB，1 页）
```

> 永远从 Flash 末尾分配数据页，从头部分配代码，避免互相覆盖。

**操作流程必须遵守：擦除 → 写入 → 锁定**

Flash 只能 1→0，不能 0→1，写之前必须先按页擦除（每页 1KB），擦后全为 0xFF。

---

## 📚 相关文件

- **头文件**: `stm32f1xx_hal_flash.h`, `stm32f1xx_hal_flash_ex.h`
- **源文件**: `stm32f1xx_hal_flash.c`, `stm32f1xx_hal_flash_ex.c`

---

## 🎯 核心函数

### HAL_FLASH_Unlock()

解锁Flash控制寄存器。

```c
HAL_StatusTypeDef HAL_FLASH_Unlock(void);
```

**说明**: 编程或擦除Flash前必须先解锁。

---

### HAL_FLASH_Lock()

锁定Flash控制寄存器。

```c
HAL_StatusTypeDef HAL_FLASH_Lock(void);
```

**说明**: 操作完成后应锁定Flash以防止意外写入。

---

### HAL_FLASH_Program()

编程Flash(写入数据)。

```c
HAL_StatusTypeDef HAL_FLASH_Program(uint32_t TypeProgram,
                                    uint32_t Address,
                                    uint64_t Data);
```

**参数:**
- `TypeProgram`: 编程类型
  - `FLASH_TYPEPROGRAM_HALFWORD`: 半字编程
  - `FLASH_TYPEPROGRAM_WORD`: 字编程
  - `FLASH_TYPEPROGRAM_DOUBLEWORD`: 双字编程
- `Address`: Flash地址
- `Data`: 要写入的数据

---

### HAL_FLASHEx_Erase()

擦除Flash页。

```c
HAL_StatusTypeDef HAL_FLASHEx_Erase(FLASH_EraseInitTypeDef *pEraseInit,
                                    uint32_t *PageError);
```

---

## 💡 完整应用示例

### 示例1: 写入和读取Flash

```c
#define FLASH_USER_START_ADDR   0x0801F000  // 用户数据起始地址(最后4KB)
#define FLASH_USER_END_ADDR     0x0801FFFF

// 写入数据到Flash
HAL_StatusTypeDef Flash_Write(uint32_t address, uint16_t *data, uint16_t len) {
    HAL_StatusTypeDef status;
    
    // 解锁Flash
    HAL_FLASH_Unlock();
    
    // 写入数据
    for (uint16_t i = 0; i < len; i++) {
        status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,
                                   address + i * 2,
                                   data[i]);
        if (status != HAL_OK) {
            HAL_FLASH_Lock();
            return status;
        }
    }
    
    // 锁定Flash
    HAL_FLASH_Lock();
    
    return HAL_OK;
}

// 从Flash读取数据
void Flash_Read(uint32_t address, uint16_t *data, uint16_t len) {
    for (uint16_t i = 0; i < len; i++) {
        data[i] = *(__IO uint16_t*)(address + i * 2);
    }
}

// 使用示例
void Flash_Test(void) {
    uint16_t write_data[] = {0x1234, 0x5678, 0xABCD, 0xEF00};
    uint16_t read_data[4];
    
    // 写入数据
    Flash_Write(FLASH_USER_START_ADDR, write_data, 4);
    
    // 读取数据
    Flash_Read(FLASH_USER_START_ADDR, read_data, 4);
    
    // 验证数据
    for (int i = 0; i < 4; i++) {
        printf("Data[%d]: 0x%04X\n", i, read_data[i]);
    }
}
```

---

### 示例2: 擦除Flash页

```c
// 擦除指定页
HAL_StatusTypeDef Flash_ErasePage(uint32_t page_address) {
    HAL_StatusTypeDef status;
    FLASH_EraseInitTypeDef EraseInitStruct;
    uint32_t PageError = 0;
    
    // 解锁Flash
    HAL_FLASH_Unlock();
    
    // 配置擦除参数
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.PageAddress = page_address;
    EraseInitStruct.NbPages = 1;
    
    // 执行擦除
    status = HAL_FLASHEx_Erase(&EraseInitStruct, &PageError);
    
    // 锁定Flash
    HAL_FLASH_Lock();
    
    return status;
}

// 擦除多个页
HAL_StatusTypeDef Flash_ErasePages(uint32_t start_page, uint32_t num_pages) {
    HAL_StatusTypeDef status;
    FLASH_EraseInitTypeDef EraseInitStruct;
    uint32_t PageError = 0;
    
    HAL_FLASH_Unlock();
    
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.PageAddress = start_page;
    EraseInitStruct.NbPages = num_pages;
    
    status = HAL_FLASHEx_Erase(&EraseInitStruct, &PageError);
    
    HAL_FLASH_Lock();
    
    return status;
}
```

---

### 示例3: 保存配置参数

```c
typedef struct {
    uint16_t magic;        // 魔数,用于验证数据有效性
    uint16_t version;      // 版本号
    uint32_t baudrate;     // 波特率
    uint8_t  device_id;    // 设备ID
    uint8_t  reserved[3];  // 保留
} Config_t;

#define CONFIG_MAGIC  0xA5A5
#define CONFIG_ADDR   0x0801F000

// 保存配置
HAL_StatusTypeDef Config_Save(Config_t *config) {
    HAL_StatusTypeDef status;
    
    // 设置魔数
    config->magic = CONFIG_MAGIC;
    
    // 擦除页
    status = Flash_ErasePage(CONFIG_ADDR);
    if (status != HAL_OK) {
        return status;
    }
    
    // 写入配置
    status = Flash_Write(CONFIG_ADDR, (uint16_t*)config, sizeof(Config_t) / 2);
    
    return status;
}

// 加载配置
HAL_StatusTypeDef Config_Load(Config_t *config) {
    // 读取配置
    Flash_Read(CONFIG_ADDR, (uint16_t*)config, sizeof(Config_t) / 2);
    
    // 验证魔数
    if (config->magic != CONFIG_MAGIC) {
        // 配置无效,使用默认值
        config->magic = CONFIG_MAGIC;
        config->version = 1;
        config->baudrate = 115200;
        config->device_id = 1;
        return HAL_ERROR;
    }
    
    return HAL_OK;
}

// 使用示例
void Config_Test(void) {
    Config_t config;
    
    // 尝试加载配置
    if (Config_Load(&config) != HAL_OK) {
        printf("配置无效,使用默认值\n");
        // 保存默认配置
        Config_Save(&config);
    } else {
        printf("配置加载成功\n");
        printf("波特率: %lu\n", config.baudrate);
        printf("设备ID: %d\n", config.device_id);
    }
    
    // 修改配置
    config.baudrate = 9600;
    Config_Save(&config);
    printf("配置已保存\n");
}
```

---

### 示例4: Flash数据记录(循环写入)

```c
#define LOG_START_ADDR  0x0801E000
#define LOG_END_ADDR    0x0801FFFF
#define LOG_PAGE_SIZE   1024
#define LOG_ENTRY_SIZE  16

typedef struct {
    uint32_t timestamp;
    uint16_t value1;
    uint16_t value2;
    uint32_t reserved;
} LogEntry_t;

uint32_t log_write_addr = LOG_START_ADDR;

// 写入日志
HAL_StatusTypeDef Log_Write(LogEntry_t *entry) {
    HAL_StatusTypeDef status;
    
    // 检查是否需要擦除
    if (log_write_addr + LOG_ENTRY_SIZE > LOG_END_ADDR) {
        // 擦除整个日志区域
        for (uint32_t addr = LOG_START_ADDR; addr < LOG_END_ADDR; addr += LOG_PAGE_SIZE) {
            Flash_ErasePage(addr);
        }
        log_write_addr = LOG_START_ADDR;
    }
    
    // 写入日志
    entry->timestamp = HAL_GetTick();
    status = Flash_Write(log_write_addr, (uint16_t*)entry, LOG_ENTRY_SIZE / 2);
    
    if (status == HAL_OK) {
        log_write_addr += LOG_ENTRY_SIZE;
    }
    
    return status;
}

// 读取所有日志
void Log_ReadAll(void) {
    LogEntry_t entry;
    uint32_t addr = LOG_START_ADDR;
    
    printf("=== 日志记录 ===\n");
    
    while (addr < log_write_addr) {
        Flash_Read(addr, (uint16_t*)&entry, LOG_ENTRY_SIZE / 2);
        
        // 检查是否为有效数据
        if (entry.timestamp != 0xFFFFFFFF) {
            printf("[%lu] Value1=%d, Value2=%d\n",
                   entry.timestamp, entry.value1, entry.value2);
        }
        
        addr += LOG_ENTRY_SIZE;
    }
}
```

---

## ⚠️ 注意事项

### 1. Flash地址范围

不同型号的Flash地址范围:
- 小容量(16/32KB): 0x08000000 ~ 0x08007FFF
- 中容量(64/128KB): 0x08000000 ~ 0x0801FFFF
- 大容量(256/512KB): 0x08000000 ~ 0x0807FFFF

**重要**: 不要擦除或写入程序代码区域!

---

### 2. 页大小

- 小容量/中容量: 1KB/页
- 大容量/超大容量: 2KB/页

擦除时必须按页擦除,不能擦除单个字节。

---

### 3. 编程前必须擦除

Flash只能从1写成0,不能从0写成1。
写入前必须先擦除(擦除后全为0xFF)。

---

### 4. 写入对齐

- 必须按半字(16位)对齐写入
- 地址必须是2的倍数
- 数据必须是16位的倍数

---

### 5. 写保护

避免意外修改Flash:
```c
// 操作前解锁
HAL_FLASH_Unlock();

// 操作...

// 操作后锁定
HAL_FLASH_Lock();
```

---

### 6. 擦除时间

- 页擦除: 约20ms
- 批量擦除: 约40ms

擦除期间CPU会等待,注意对实时性的影响。

---

### 7. 写入次数限制

Flash有擦写次数限制(通常10000次):
- 避免频繁擦写同一区域
- 使用磨损均衡算法
- 考虑使用外部EEPROM存储频繁变化的数据

---

## 📖 相关文档

- [HAL核心](HAL_Core.md)
- [RCC时钟控制](HAL_RCC.md)
