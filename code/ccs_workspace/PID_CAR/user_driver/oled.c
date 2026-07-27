#include "oled.h"
#include "stdlib.h"
#include "oledfont.h"

u8 OLED_GRAM[144][8];
extern void delay_ms(uint32_t ms);

#define OLED_I2C_TIMEOUT_COUNT (200000U)
#define OLED_I2C_START_DELAY_CYCLES (32U)

volatile uint8_t g_oledI2cFault = 0U;
volatile uint8_t g_oledI2cFaultReason = OLED_I2C_FAULT_NONE;
volatile uint32_t g_oledI2cLastStatus = 0U;
volatile uint8_t g_oledI2cLastData = 0U;
volatile uint8_t g_oledI2cLastMode = OLED_CMD;

static uint8_t OLED_I2C_WaitForIdle(void)
{
	uint32_t timeout = OLED_I2C_TIMEOUT_COUNT;
	uint32_t status;

	do
	{
		status = DL_I2C_getControllerStatus(OLED_INST);
		g_oledI2cLastStatus = status;
		if ((status & DL_I2C_CONTROLLER_STATUS_ERROR) != 0U)
		{
			return 0U;
		}

		if ((status & DL_I2C_CONTROLLER_STATUS_IDLE) != 0U)
		{
			return 1U;
		}
	} while (--timeout != 0U);

	return 0U;
}

static void OLED_I2C_SetFault(uint8_t reason, uint32_t status)
{
	g_oledI2cLastStatus = status;
	g_oledI2cFaultReason = reason;
	g_oledI2cFault = 1U;
	DL_I2C_resetControllerTransfer(OLED_INST);
}

// 反显函数
void OLED_ColorTurn(u8 i)
{
	if (i == 0)
		OLED_WR_Byte(0xA6, OLED_CMD); // 正常显示
	if (i == 1)
		OLED_WR_Byte(0xA7, OLED_CMD); // 反色显示
}

// 屏幕旋转180度
void OLED_DisplayTurn(u8 i)
{
	if (i == 0)
	{
		OLED_WR_Byte(0xC8, OLED_CMD); // 正常显示
		OLED_WR_Byte(0xA1, OLED_CMD);
	}
	if (i == 1)
	{
		OLED_WR_Byte(0xC0, OLED_CMD); // 反转显示
		OLED_WR_Byte(0xA0, OLED_CMD);
	}
}

void OLED_WR_Byte(uint8_t dat, uint8_t mode)
{
	uint8_t txData[2];
	uint32_t timeout;
	uint32_t status;

	/* OLED/I2C 故障后直接跳过显示操作，避免阻塞主控制循环。 */
	if (g_oledI2cFault != 0U)
	{
		return;
	}

	g_oledI2cLastData = dat;
	g_oledI2cLastMode = mode;

	// 控制字节: 0x00为命令, 0x40为数据
	txData[0] = mode ? 0x40 : 0x00;
	txData[1] = dat;

	// 1. 等待 I2C 空闲；超时或总线错误时退出，不能永久阻塞。
	if (OLED_I2C_WaitForIdle() == 0U)
	{
		OLED_I2C_SetFault(
			OLED_I2C_FAULT_WAIT_IDLE_BEFORE, g_oledI2cLastStatus);
		return;
	}

	// 2. 将 2 个字节填入发送 FIFO
	DL_I2C_fillControllerTXFIFO(OLED_INST, txData, 2);

	// 3. 启动传输
	DL_I2C_startControllerTransfer(OLED_INST, 0x3C, DL_I2C_CONTROLLER_DIRECTION_TX, 2);

	/*
	 * TI I2C_ERR_13 workaround：启动后先等待至少 3 个 I2C 功能时钟。
	 * 当前配置下 32 个 CPU 周期留有充足余量，且远小于一次 I2C 传输时间。
	 */
	delay_cycles(OLED_I2C_START_DELAY_CYCLES);

	// 4. 等待控制器完成事务；BUSY_BUS 是瞬时总线状态，不能作为必达同步点。
	timeout = OLED_I2C_TIMEOUT_COUNT;
	do
	{
		status = DL_I2C_getControllerStatus(OLED_INST);
		g_oledI2cLastStatus = status;
		if ((status & DL_I2C_CONTROLLER_STATUS_ERROR) != 0U)
		{
			OLED_I2C_SetFault(OLED_I2C_FAULT_CONTROLLER_ERROR, status);
			return;
		}

		if ((status & DL_I2C_CONTROLLER_STATUS_BUSY) == 0U)
		{
			break;
		}
	} while (--timeout != 0U);

	if (timeout == 0U)
	{
		OLED_I2C_SetFault(
			OLED_I2C_FAULT_BUSY_TIMEOUT, g_oledI2cLastStatus);
		return;
	}

	// 5. 确认控制器回到空闲状态。
	if (OLED_I2C_WaitForIdle() == 0U)
	{
		OLED_I2C_SetFault(
			OLED_I2C_FAULT_WAIT_IDLE_AFTER, g_oledI2cLastStatus);
	}
}

// 开启OLED显示
void OLED_DisPlay_On(void)
{
	OLED_WR_Byte(0x8D, OLED_CMD); // 电荷泵使能
	OLED_WR_Byte(0x14, OLED_CMD); // 开启电荷泵
	OLED_WR_Byte(0xAF, OLED_CMD); // 点亮屏幕
}

// 关闭OLED显示
void OLED_DisPlay_Off(void)
{
	OLED_WR_Byte(0x8D, OLED_CMD); // 电荷泵使能
	OLED_WR_Byte(0x10, OLED_CMD); // 关闭电荷泵
	OLED_WR_Byte(0xAF, OLED_CMD); // 关闭屏幕
}

// 更新显存到OLED
void OLED_Refresh(void)
{
	u8 i, n;
	for (i = 0; i < 8; i++)
	{
		OLED_WR_Byte(0xb0 + i, OLED_CMD); // 设置行起始地址
		OLED_WR_Byte(0x00, OLED_CMD);	  // 设置低列起始地址
		OLED_WR_Byte(0x10, OLED_CMD);	  // 设置高列起始地址
		for (n = 0; n < 128; n++)
			OLED_WR_Byte(OLED_GRAM[n][i], OLED_DATA);
	}
}

// 清屏函数
void OLED_Clear(void)
{
	u8 i, n;
	for (i = 0; i < 8; i++)
	{
		for (n = 0; n < 128; n++)
		{
			OLED_GRAM[n][i] = 0; // 清除所有数据
		}
	}
	OLED_Refresh(); // 更新显示
}

// 画点
void OLED_DrawPoint(u8 x, u8 y)
{
	u8 i, m, n;
	i = y / 8;
	m = y % 8;
	n = 1 << m;
	OLED_GRAM[x][i] |= n;
}

// 清除一个点
void OLED_ClearPoint(u8 x, u8 y)
{
	u8 i, m, n;
	i = y / 8;
	m = y % 8;
	n = 1 << m;
	OLED_GRAM[x][i] = ~OLED_GRAM[x][i];
	OLED_GRAM[x][i] |= n;
	OLED_GRAM[x][i] = ~OLED_GRAM[x][i];
}

// 画线
void OLED_DrawLine(u8 x1, u8 y1, u8 x2, u8 y2)
{
	u8 i, k, k1, k2;
	if ((x1 < 0) || (x2 > 128) || (y1 < 0) || (y2 > 64) || (x1 > x2) || (y1 > y2))
		return;
	if (x1 == x2) // 画竖线
	{
		for (i = 0; i < (y2 - y1); i++)
			OLED_DrawPoint(x1, y1 + i);
	}
	else if (y1 == y2) // 画横线
	{
		for (i = 0; i < (x2 - x1); i++)
			OLED_DrawPoint(x1 + i, y1);
	}
	else // 画斜线
	{
		k1 = y2 - y1;
		k2 = x2 - x1;
		k = k1 * 10 / k2;
		for (i = 0; i < (x2 - x1); i++)
			OLED_DrawPoint(x1 + i, y1 + i * k / 10);
	}
}

// 画圆
void OLED_DrawCircle(u8 x, u8 y, u8 r)
{
	int a = 0, b = r, num;
	while (2 * b * b >= r * r)
	{
		OLED_DrawPoint(x + a, y - b);
		OLED_DrawPoint(x - a, y - b);
		OLED_DrawPoint(x - a, y + b);
		OLED_DrawPoint(x + a, y + b);
		OLED_DrawPoint(x + b, y + a);
		OLED_DrawPoint(x + b, y - a);
		OLED_DrawPoint(x - b, y - a);
		OLED_DrawPoint(x - b, y + a);

		a++;
		num = (a * a + b * b) - r * r;
		if (num > 0)
		{
			b--;
			a--;
		}
	}
}

// 显示字符
void OLED_ShowChar(u8 x, u8 y, u8 chr, u8 size1)
{
	u8 i, m, temp, size2, chr1;
	u8 y0 = y;
	size2 = (size1 / 8 + ((size1 % 8) ? 1 : 0)) * (size1 / 2);
	chr1 = chr - ' ';
	for (i = 0; i < size2; i++)
	{
		if (size1 == 12)
		{
			temp = asc2_1206[chr1][i];
		}
		else if (size1 == 16)
		{
			temp = asc2_1608[chr1][i];
		}
		else if (size1 == 24)
		{
			temp = asc2_2412[chr1][i];
		}
		else
			return;
		for (m = 0; m < 8; m++)
		{
			if (temp & 0x80)
				OLED_DrawPoint(x, y);
			else
				OLED_ClearPoint(x, y);
			temp <<= 1;
			y++;
			if ((y - y0) == size1)
			{
				y = y0;
				x++;
				break;
			}
		}
	}
}

// 显示字符串
void OLED_ShowString(u8 x, u8 y, u8 *chr, u8 size1)
{
	while ((*chr >= ' ') && (*chr <= '~'))
	{
		OLED_ShowChar(x, y, *chr, size1);
		x += size1 / 2;
		if (x > 128 - size1) // 换行
		{
			x = 0;
			y += size1; // 修复了原代码的y+=2的bug
		}
		chr++;
	}
}

// m^n
u32 OLED_Pow(u8 m, u8 n)
{
	u32 result = 1;
	while (n--)
		result *= m;
	return result;
}

// 显示数字
void OLED_ShowNum(u8 x, u8 y, u32 num, u8 len, u8 size1)
{
	u8 t, temp;
	for (t = 0; t < len; t++)
	{
		temp = (num / OLED_Pow(10, len - t - 1)) % 10;
		if (temp == 0)
			OLED_ShowChar(x + (size1 / 2) * t, y, '0', size1);
		else
			OLED_ShowChar(x + (size1 / 2) * t, y, temp + '0', size1);
	}
}

// 显示汉字
void OLED_ShowChinese(u8 x, u8 y, u8 num, u8 size1)
{
	u8 i, m, n = 0, temp, chr1;
	u8 x0 = x, y0 = y;
	u8 size3 = size1 / 8;
	while (size3--)
	{
		chr1 = num * size1 / 8 + n;
		n++;
		for (i = 0; i < size1; i++)
		{
			if (size1 == 16)
			{
				temp = Hzk1[chr1][i];
			}
			else if (size1 == 24)
			{
				temp = Hzk2[chr1][i];
			}
			else if (size1 == 32)
			{
				temp = Hzk3[chr1][i];
			}
			else if (size1 == 64)
			{
				temp = Hzk4[chr1][i];
			}
			else
				return;

			for (m = 0; m < 8; m++)
			{
				if (temp & 0x01)
					OLED_DrawPoint(x, y);
				else
					OLED_ClearPoint(x, y);
				temp >>= 1;
				y++;
			}
			x++;
			if ((x - x0) == size1)
			{
				x = x0;
				y0 = y0 + 8;
			}
			y = y0;
		}
	}
}

// 配置写入数据的起始位置
void OLED_WR_BP(u8 x, u8 y)
{
	OLED_WR_Byte(0xb0 + y, OLED_CMD); // 设置行起始地址
	OLED_WR_Byte(((x & 0xf0) >> 4) | 0x10, OLED_CMD);
	OLED_WR_Byte((x & 0x0f) | 0x01, OLED_CMD);
}

// 显示图片
void OLED_ShowPicture(u8 x0, u8 y0, u8 x1, u8 y1, u8 BMP[])
{
	u32 j = 0;
	u8 x = 0, y = 0;
	if (y % 8 == 0)
		y = 0;
	else
		y += 1;
	for (y = y0; y < y1; y++)
	{
		OLED_WR_BP(x0, y);
		for (x = x0; x < x1; x++)
		{
			OLED_WR_Byte(BMP[j], OLED_DATA);
			j++;
		}
	}
}

// OLED的初始化
void OLED_Init(void)
{
	// 4针OLED没有RST引脚，直接延时等待屏幕内部RC电路上电复位完成
	delay_ms(100);

	OLED_WR_Byte(0xAE, OLED_CMD); //--turn off oled panel
	OLED_WR_Byte(0x00, OLED_CMD); //---set low column address
	OLED_WR_Byte(0x10, OLED_CMD); //---set high column address
	OLED_WR_Byte(0x40, OLED_CMD); //--set start line address  Set Mapping RAM Display Start Line (0x00~0x3F)
	OLED_WR_Byte(0x81, OLED_CMD); //--set contrast control register
	OLED_WR_Byte(0xCF, OLED_CMD); // Set SEG Output Current Brightness
	OLED_WR_Byte(0xA1, OLED_CMD); //--Set SEG/Column Mapping     0xa0左右反置 0xa1正常
	OLED_WR_Byte(0xC8, OLED_CMD); // Set COM/Row Scan Direction   0xc0上下反置 0xc8正常
	OLED_WR_Byte(0xA6, OLED_CMD); //--set normal display
	OLED_WR_Byte(0xA8, OLED_CMD); //--set multiplex ratio(1 to 64)
	OLED_WR_Byte(0x3f, OLED_CMD); //--1/64 duty
	OLED_WR_Byte(0xD3, OLED_CMD); //-set display offset	Shift Mapping RAM Counter (0x00~0x3F)
	OLED_WR_Byte(0x00, OLED_CMD); //-not offset
	OLED_WR_Byte(0xd5, OLED_CMD); //--set display clock divide ratio/oscillator frequency
	OLED_WR_Byte(0x80, OLED_CMD); //--set divide ratio, Set Clock as 100 Frames/Sec
	OLED_WR_Byte(0xD9, OLED_CMD); //--set pre-charge period
	OLED_WR_Byte(0xF1, OLED_CMD); // Set Pre-Charge as 15 Clocks & Discharge as 1 Clock
	OLED_WR_Byte(0xDA, OLED_CMD); //--set com pins hardware configuration
	OLED_WR_Byte(0x12, OLED_CMD);
	OLED_WR_Byte(0xDB, OLED_CMD); //--set vcomh
	OLED_WR_Byte(0x40, OLED_CMD); // Set VCOM Deselect Level
	OLED_WR_Byte(0x20, OLED_CMD); //-Set Page Addressing Mode (0x00/0x01/0x02)
	OLED_WR_Byte(0x02, OLED_CMD); //
	OLED_WR_Byte(0x8D, OLED_CMD); //--set Charge Pump enable/disable
	OLED_WR_Byte(0x14, OLED_CMD); //--set(0x10) disable
	OLED_WR_Byte(0xA4, OLED_CMD); // Disable Entire Display On (0xa4/0xa5)
	OLED_WR_Byte(0xA6, OLED_CMD); // Disable Inverse Display On (0xa6/a7)
	OLED_WR_Byte(0xAF, OLED_CMD);
	OLED_Clear();
}
