#include "stm32f1xx_hal.h"
#include "OLED_Font.h"

/* 引脚配置（软件模拟 I2C，使用 PB8=SCL，PB9=SDA）*/
#define OLED_W_SCL(x)		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, (GPIO_PinState)(x))
#define OLED_W_SDA(x)		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, (GPIO_PinState)(x))

/* 引脚初始化 */
void OLED_I2C_Init(void)
{
	__HAL_RCC_GPIOB_CLK_ENABLE();

	GPIO_InitTypeDef GPIO_InitStructure = {0};
	GPIO_InitStructure.Mode  = GPIO_MODE_OUTPUT_OD;		/* 开漏输出，适合 I2C */
	GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH;
	GPIO_InitStructure.Pin   = GPIO_PIN_8 | GPIO_PIN_9;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStructure);

	OLED_W_SCL(1);
	OLED_W_SDA(1);
}

/**
  * @brief  I2C 开始
  * @param  无
  * @retval 无
  */
void OLED_I2C_Start(void)
{
	OLED_W_SDA(1);
	OLED_W_SCL(1);
	OLED_W_SDA(0);
	OLED_W_SCL(0);
}

/**
  * @brief  I2C 停止
  * @param  无
  * @retval 无
  */
void OLED_I2C_Stop(void)
{
	OLED_W_SDA(0);
	OLED_W_SCL(1);
	OLED_W_SDA(1);
}

/**
  * @brief  I2C 发送一个字节
  * @param  Byte 要发送的字节
  * @retval 无
  */
void OLED_I2C_SendByte(uint8_t Byte)
{
	uint8_t i;
	for (i = 0; i < 8; i++)
	{
		OLED_W_SDA(!!(Byte & (0x80 >> i)));
		OLED_W_SCL(1);
		OLED_W_SCL(0);
	}
	OLED_W_SCL(1);		/* 额外一个时钟，不处理应答信号 */
	OLED_W_SCL(0);
}

/**
  * @brief  OLED 写命令
  * @param  Command 要写入的命令字节
  * @retval 无
  */
void OLED_WriteCommand(uint8_t Command)
{
	OLED_I2C_Start();
	OLED_I2C_SendByte(0x78);		/* 从机地址 */
	OLED_I2C_SendByte(0x00);		/* 写命令标志 */
	OLED_I2C_SendByte(Command);
	OLED_I2C_Stop();
}

/**
  * @brief  OLED 写数据
  * @param  Data 要写入的数据字节
  * @retval 无
  */
void OLED_WriteData(uint8_t Data)
{
	OLED_I2C_Start();
	OLED_I2C_SendByte(0x78);		/* 从机地址 */
	OLED_I2C_SendByte(0x40);		/* 写数据标志 */
	OLED_I2C_SendByte(Data);
	OLED_I2C_Stop();
}

/**
  * @brief  OLED 设置光标位置
  * @param  Y 行（页），范围：0~7
  * @param  X 列，范围：0~127
  * @retval 无
  */
void OLED_SetCursor(uint8_t Y, uint8_t X)
{
	OLED_WriteCommand(0xB0 | Y);					/* 设置页地址 */
	OLED_WriteCommand(0x10 | ((X & 0xF0) >> 4));	/* 设置列地址高 4 位 */
	OLED_WriteCommand(0x00 | (X & 0x0F));			/* 设置列地址低 4 位 */
}

/**
  * @brief  OLED 清屏
  * @param  无
  * @retval 无
  */
void OLED_Clear(void)
{
	uint8_t i, j;
	for (j = 0; j < 8; j++)
	{
		OLED_SetCursor(j, 0);
		for (i = 0; i < 128; i++)
		{
			OLED_WriteData(0x00);
		}
	}
}

/**
  * @brief  OLED 显示一个字符
  * @param  Line   行位置，范围：1~4
  * @param  Column 列位置，范围：1~16
  * @param  Char   要显示的 ASCII 可见字符
  * @retval 无
  */
void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char)
{
	uint8_t i;
	OLED_SetCursor((Line - 1) * 2, (Column - 1) * 8);		/* 光标定位到上半部分 */
	for (i = 0; i < 8; i++)
	{
		OLED_WriteData(OLED_F8x16[Char - ' '][i]);			/* 写上半部分字模 */
	}
	OLED_SetCursor((Line - 1) * 2 + 1, (Column - 1) * 8);	/* 光标定位到下半部分 */
	for (i = 0; i < 8; i++)
	{
		OLED_WriteData(OLED_F8x16[Char - ' '][i + 8]);		/* 写下半部分字模 */
	}
}

/**
  * @brief  OLED 显示字符串
  * @param  Line   起始行，范围：1~4
  * @param  Column 起始列，范围：1~16
  * @param  String 要显示的 ASCII 字符串
  * @retval 无
  */
void OLED_ShowString(uint8_t Line, uint8_t Column, char *String)
{
	uint8_t i;
	for (i = 0; String[i] != '\0'; i++)
	{
		OLED_ShowChar(Line, Column + i, String[i]);
	}
}

/**
  * @brief  幂运算辅助函数
  * @retval X 的 Y 次方
  */
uint32_t OLED_Pow(uint32_t X, uint32_t Y)
{
	uint32_t Result = 1;
	while (Y--)
	{
		Result *= X;
	}
	return Result;
}

/**
  * @brief  OLED 显示十进制无符号数
  * @param  Line   起始行，范围：1~4
  * @param  Column 起始列，范围：1~16
  * @param  Number 要显示的数字，范围：0~4294967295
  * @param  Length 显示位数，范围：1~10
  * @retval 无
  */
void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
	uint8_t i;
	for (i = 0; i < Length; i++)
	{
		OLED_ShowChar(Line, Column + i, Number / OLED_Pow(10, Length - i - 1) % 10 + '0');
	}
}

/**
  * @brief  OLED 显示十进制有符号数
  * @param  Line   起始行，范围：1~4
  * @param  Column 起始列，范围：1~16
  * @param  Number 要显示的数字，范围：-2147483648~2147483647
  * @param  Length 显示位数，范围：1~10
  * @retval 无
  */
void OLED_ShowSignedNum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length)
{
	uint8_t i;
	uint32_t Number1;
	if (Number >= 0)
	{
		OLED_ShowChar(Line, Column, '+');
		Number1 = Number;
	}
	else
	{
		OLED_ShowChar(Line, Column, '-');
		Number1 = -Number;
	}
	for (i = 0; i < Length; i++)
	{
		OLED_ShowChar(Line, Column + i + 1, Number1 / OLED_Pow(10, Length - i - 1) % 10 + '0');
	}
}

/**
  * @brief  OLED 显示十六进制数
  * @param  Line   起始行，范围：1~4
  * @param  Column 起始列，范围：1~16
  * @param  Number 要显示的数字，范围：0~0xFFFFFFFF
  * @param  Length 显示位数，范围：1~8
  * @retval 无
  */
void OLED_ShowHexNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
	uint8_t i, SingleNumber;
	for (i = 0; i < Length; i++)
	{
		SingleNumber = Number / OLED_Pow(16, Length - i - 1) % 16;
		if (SingleNumber < 10)
		{
			OLED_ShowChar(Line, Column + i, SingleNumber + '0');
		}
		else
		{
			OLED_ShowChar(Line, Column + i, SingleNumber - 10 + 'A');
		}
	}
}

/**
  * @brief  OLED 显示二进制数
  * @param  Line   起始行，范围：1~4
  * @param  Column 起始列，范围：1~16
  * @param  Number 要显示的数字，范围：0~1111 1111 1111 1111
  * @param  Length 显示位数，范围：1~16
  * @retval 无
  */
void OLED_ShowBinNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
	uint8_t i;
	for (i = 0; i < Length; i++)
	{
		OLED_ShowChar(Line, Column + i, Number / OLED_Pow(2, Length - i - 1) % 2 + '0');
	}
}

/**
  * @brief  OLED 初始化
  * @param  无
  * @retval 无
  */
void OLED_Init(void)
{
	HAL_Delay(100);				/* 上电延时，等待 OLED 稳定 */

	OLED_I2C_Init();			/* 初始化 I2C 引脚 */

	OLED_WriteCommand(0xAE);	/* 关闭显示 */

	OLED_WriteCommand(0xD5);	/* 设置显示时钟分频比/振荡器频率 */
	OLED_WriteCommand(0x80);

	OLED_WriteCommand(0xA8);	/* 设置多路复用率 */
	OLED_WriteCommand(0x3F);

	OLED_WriteCommand(0xD3);	/* 设置显示偏移 */
	OLED_WriteCommand(0x00);

	OLED_WriteCommand(0x40);	/* 设置显示开始行 */

	OLED_WriteCommand(0xA1);	/* 设置左右方向：0xA1 正常，0xA0 左右反置 */

	OLED_WriteCommand(0xC8);	/* 设置上下方向：0xC8 正常，0xC0 上下反置 */

	OLED_WriteCommand(0xDA);	/* 设置 COM 引脚硬件配置 */
	OLED_WriteCommand(0x12);

	OLED_WriteCommand(0x81);	/* 设置对比度 */
	OLED_WriteCommand(0xCF);

	OLED_WriteCommand(0xD9);	/* 设置预充电周期 */
	OLED_WriteCommand(0xF1);

	OLED_WriteCommand(0xDB);	/* 设置 VCOMH 取消选择级别 */
	OLED_WriteCommand(0x30);

	OLED_WriteCommand(0xA4);	/* 设置全局显示开关（跟随 RAM 内容）*/

	OLED_WriteCommand(0xA6);	/* 设置正常显示（非反转）*/

	OLED_WriteCommand(0x8D);	/* 启用电荷泵 */
	OLED_WriteCommand(0x14);

	OLED_WriteCommand(0xAF);	/* 开启显示 */

	OLED_Clear();				/* 清屏 */
}
