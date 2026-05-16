# STM32CubeMX 创建 STM32F103C8T6 工程指南

## 前置说明

STM32CubeMX 主界面有 4 个入口选项：

| 选项 | 说明 |
|------|------|
| **Start My project from MCU** | 从芯片型号开始，手上没有官方开发板时用这个 ✅ |
| **Start My project from ST Board** | 从 ST 官方开发板开始，自动预设引脚配置 |
| **Start My project from Example** | 从官方示例工程开始，适合学习参考 |
| **Access to Compare Projects** | 对比两个工程的配置差异，辅助工具 |

创建 STM32F103C8T6 工程选第一个：**ACCESS TO MCU SELECTOR**

---

## 第一步：选择芯片

1. 点击 **ACCESS TO MCU SELECTOR**
2. 搜索框输入 `STM32F103C8T6`
3. 在列表中找到 **STM32F103C8Tx**，双击进入配置界面

---

## 第二步：Pinout & Configuration 配置

### 2.1 配置时钟源（RCC）

左侧 `System Core` → `RCC`：
- **High Speed Clock (HSE)** → 选 `Crystal/Ceramic Resonator`

### 2.2 配置调试接口（SYS）

左侧 `System Core` → `SYS`：
- **Debug** → 选 `Serial Wire`（SWD）

> **为什么选 Serial Wire：** STM32F103C8T6（蓝色小板 Blue Pill）配套的 ST-Link 下载器默认使用 SWD 接口，只需接 4 根线（3.3V、GND、SWDIO、SWCLK），引脚占用最少。

**Debug 选项说明：**

| 选项 | 说明 |
|------|------|
| No Debug | 不启用调试，烧录后无法再连接调试器 |
| **Serial Wire** ✅ | SWD 协议，2 根信号线，推荐 |
| JTAG (4 pins) | JTAG 协议，需要 4 根信号线 |
| JTAG (5 pins) | JTAG 协议，需要 5 根信号线 |
| Trace Asynchronous Sw | SWD + SWO 追踪，高级调试用 |

### 2.3 左侧勾选状态说明

配置完 RCC 和 SYS 后，左侧会自动出现勾选，这是正常的：

| 项目 | 状态 | 说明 |
|------|------|------|
| DMA | 无勾 | 正常，未使用 DMA |
| GPIO ✅ | 有勾 | 自动，配置了引脚就会勾上 |
| IWDG | 无勾 | 正常，看门狗默认不开 |
| NVIC ✅ | 有勾 | 自动，有中断就勾上 |
| RCC ✅ | 有勾 | 配置了 HSE 时钟源 |
| SYS ✅ | 有勾 | 配置了 Serial Wire 调试接口 |
| WWDG | 无勾 | 正常，窗口看门狗默认不开 |

---

## 第三步：Clock Configuration 时钟配置

点击顶部 **Clock Configuration** 标签，按以下配置：

| 配置项 | 设置值 |
|--------|--------|
| Input frequency (HSE) | `8` MHz |
| PLL Source Mux | `HSE` |
| PLLMul | `× 9` |
| System Clock Mux | `PLLCLK` |
| AHB Prescaler | `/ 1` |
| APB1 Prescaler | `/ 2` |
| APB2 Prescaler | `/ 1` |

**最终时钟结果：**

```
HSE 8MHz → PLL ×9 = 72MHz (SYSCLK)
                          ↓ AHB /1
                       HCLK = 72MHz  ✅
                          ↓
              APB1 /2 → PCLK1 = 36MHz  ✅（最大 36MHz）
              APB2 /1 → PCLK2 = 72MHz  ✅（最大 72MHz）
```

> **注意：AHB 和 APB1 必须配套设置。** AHB /1 时 HCLK = 72MHz，此时 APB1 必须设 /2，否则 PCLK1 超过 36MHz 限制会报红。

---

## 第四步：Project Manager 工程设置

点击顶部 **Project Manager** 标签：

| 设置项 | 说明 |
|--------|------|
| Project Name | 填写工程名，如 `base_project` |
| Project Location | 保存路径，**必须选没有空格的路径** |
| Application Structure | `Advanced`（默认即可） |
| Toolchain / IDE | 选 `STM32CubeIDE` |
| Generate Under Root | 保持勾选 ✅ |

---

## 第五步：生成代码

点击右上角 **GENERATE CODE** 按钮，生成完成后点 **Open Project** 在 STM32CubeIDE 中打开工程。

---

## 常见弹窗处理

### ❌ 弹窗：Error downloading crdb_full.zip

```
Error downloading the following files:
crdb_full.zip (Problem during download)
```

**原因：** CubeMX 启动时需要从 ST 服务器下载芯片数据库，国内访问经常超时。

**解决方法：**

1. **清除损坏缓存后重试：**
   ```bash
   rm -rf ~/.stm32cubemx/plugins/updater/temp/
   ```
   然后重启 CubeMX。

2. **设置代理：** 菜单 `Help → Settings → Connectivity`，填入代理地址和端口。

3. **挂 VPN 后重试：** 最直接有效的方法。

4. **换用 STM32CubeIDE：** 内置 CubeMX 功能，安装包自带芯片数据库，无需单独下载。

---

### ❌ 弹窗：Project location name can't be Empty or contain...

```
ERROR: Project location name can't be Empty or contain
any of the following characters: \/:*"<>| &
```

**原因：** 工程保存路径中包含空格或特殊字符。

**解决方法：** 点 Browse 重新选择路径，确保路径中没有空格，例如：
```
/Users/你的用户名/Documents/STM32Projects
```

---

### ❓ 弹窗：Do you want to run automatic clock issues solver?

```
Do you want to run automatic clock issues solver?
Otherwise you can do it later by clicking on button "Resolve Clock Issues"
```

**处理：** 点 **Yes**，让 CubeMX 自动解决时钟配置冲突。

---

### ⚠️ 时钟树 APB1 报红

**原因：** AHB Prescaler 设为 /1 后 HCLK = 72MHz，但 APB1 Prescaler 仍为 /1，导致 PCLK1 = 72MHz 超过 36MHz 限制。

**解决方法：** 将 **APB1 Prescaler** 改为 `/2`，PCLK1 降至 36MHz，红色消失。

---

### ❓ 弹窗：Firmware Package not available, do you want to download?

```
The Firmware Package (STM32Cube FW_F1 V1.8.7) or one of its
dependencies required by the Project is not available in your
STM32CubeMX Repository. Do you want to download this now?
```

**处理：** 点 **Yes**。这是 STM32F1 系列的 HAL 库和启动文件，生成代码必须的，大约几十 MB。如果下载失败，挂 VPN 后重试。

---

## 总结：完整配置清单

- [x] 芯片选择：STM32F103C8T6
- [x] RCC → HSE：Crystal/Ceramic Resonator
- [x] SYS → Debug：Serial Wire
- [x] 时钟树：HSE 8MHz → PLL ×9 → 72MHz
- [x] AHB Prescaler：/1（HCLK = 72MHz）
- [x] APB1 Prescaler：/2（PCLK1 = 36MHz）
- [x] APB2 Prescaler：/1（PCLK2 = 72MHz）
- [x] 工程路径：无空格
- [x] Toolchain：STM32CubeIDE
- [x] Generate Under Root：勾选
- [x] 固件包：下载 STM32Cube FW_F1
