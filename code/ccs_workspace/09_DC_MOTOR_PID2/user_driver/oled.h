#ifndef __OLED_H
#define __OLED_H 

#include "ti_msp_dl_config.h"
#include <stdint.h>
#include <stdlib.h>

// ---------------------------------------------------------
#define OLED_CMD  0	//写命令
#define OLED_DATA 1	//写数据

typedef unsigned char u8;
typedef unsigned int  u32;

/**
 * @brief 清除 OLED 显存中的一个像素点
 *
 * @param x 像素点的横坐标，范围通常为 0~127
 *          - 0 表示最左侧列
 *          - 127 表示最右侧列
 *
 * @param y 像素点的纵坐标，范围通常为 0~63
 *          - 0 表示最上方行
 *          - 63 表示最下方行
 *
 * @note 本函数操作的是显存 OLED_GRAM 中对应点的状态，
 *       是否立即在屏幕上消失取决于后续是否调用 OLED_Refresh()。
 */
void OLED_ClearPoint(u8 x,u8 y);

/**
 * @brief 设置 OLED 是否反色显示
 *
 * @param i 显示模式选择
 *          - 0：正常显示（黑底白字/亮点）
 *          - 1：反色显示（白底黑字/灭点）
 *
 * @note 该函数直接向 OLED 发送控制命令，修改的是整屏显示效果，
 *       不影响显存数据本身。
 */
void OLED_ColorTurn(u8 i);

/**
 * @brief 设置 OLED 是否旋转 180 度显示
 *
 * @param i 显示方向选择
 *          - 0：正常方向显示
 *          - 1：旋转 180° 显示
 *
 * @note 常用于屏幕安装方向与程序默认方向不一致时进行翻转适配。
 *       该函数修改的是屏幕扫描方向，不改变显存内容。
 */
void OLED_DisplayTurn(u8 i);

/**
 * @brief I2C 起始信号
 *
 * @note 该函数通常用于软件模拟 I2C（GPIO 模拟时序）中，
 *       表示主机发起一次新的 I2C 通信。
 *       在你当前这份代码里 OLED_WR_Byte() 使用的是 MSPM0 硬件 I2C，
 *       所以这个函数可能是旧版接口保留，当前未实际使用。
 */
void I2C_Start(void);

/**
 * @brief I2C 停止信号
 *
 * @note 该函数通常用于软件模拟 I2C 中，
 *       表示主机结束当前 I2C 通信。
 *       若项目改为硬件 I2C 驱动，则通常不需要手动写这个函数。
 */
void I2C_Stop(void);

/**
 * @brief 等待从机应答信号
 *
 * @note 常用于软件模拟 I2C，主机发送完一个字节后，
 *       等待从机返回 ACK 应答。
 *       若 OLED 未正确接线、地址错误或设备未上电，
 *       通常会在这里无应答。
 */
void I2C_WaitAck(void);

/**
 * @brief 通过 I2C 发送一个字节
 *
 * @param dat 要发送的 8 位数据
 *            - 可以是 OLED 命令
 *            - 也可以是 OLED 显示数据
 *
 * @note 该函数一般配合软件模拟 I2C 使用。
 *       在当前硬件 I2C 实现中，其功能通常被 OLED_WR_Byte() 取代。
 */
void Send_Byte(u8 dat);

/**
 * @brief 向 OLED 写入一个字节的数据或命令
 *
 * @param dat 要写入 OLED 的 8 位内容
 *            - 当 mode 表示命令时，dat 为控制命令
 *            - 当 mode 表示数据时，dat 为要显示到 GDDRAM 的数据
 *
 * @param mode 写入模式选择
 *             - 0：写命令（OLED_CMD）
 *             - 1：写数据（OLED_DATA）
 *
 * @note 在当前代码中，本函数通过硬件 I2C 发送两个字节：
 *       第一个字节是控制字节（0x00 或 0x40），
 *       第二个字节才是真正写给 OLED 的内容 dat。
 */
void OLED_WR_Byte(u8 dat,u8 mode);

/**
 * @brief 打开 OLED 显示
 *
 * @note 通常会使能电荷泵并点亮屏幕。
 *       该函数不会清空显存，也不会自动刷新显示内容。
 */
void OLED_DisPlay_On(void);

/**
 * @brief 关闭 OLED 显示
 *
 * @note 关闭后屏幕不再正常显示内容，通常用于省电。
 *       显存中的数据一般不会因此丢失，重新打开后可继续显示。
 */
void OLED_DisPlay_Off(void);

/**
 * @brief 将显存 OLED_GRAM 的内容刷新到 OLED 屏幕
 *
 * @note 前面调用 OLED_DrawPoint()、OLED_ClearPoint()、
 *       OLED_ShowChar()、OLED_ShowString() 等函数时，
 *       多数只是修改缓冲区内容，必须调用本函数后，
 *       屏幕上的显示才会真正更新。
 */
void OLED_Refresh(void);

/**
 * @brief 清空 OLED 显存并刷新到屏幕
 *
 * @note 调用后会清除整个屏幕显示内容。
 *       内部通常先将 OLED_GRAM 全部置 0，再执行 OLED_Refresh()。
 */
void OLED_Clear(void);

/**
 * @brief 在显存中点亮一个像素点
 *
 * @param x 像素点横坐标，范围通常为 0~127
 *          - 0 为最左侧
 *          - 127 为最右侧
 *
 * @param y 像素点纵坐标，范围通常为 0~63
 *          - 0 为最上方
 *          - 63 为最下方
 *
 * @note 本函数默认只修改显存，不一定立即显示到屏幕，
 *       一般需要配合 OLED_Refresh() 使用。
 */
void OLED_DrawPoint(u8 x,u8 y);

/**
 * @brief 在显存中绘制一条直线
 *
 * @param x1 直线起点的横坐标，范围通常为 0~127
 * @param y1 直线起点的纵坐标，范围通常为 0~63
 * @param x2 直线终点的横坐标，范围通常为 0~127
 * @param y2 直线终点的纵坐标，范围通常为 0~63
 *
 * @note
 * - 当 x1 == x2 时绘制竖线
 * - 当 y1 == y2 时绘制横线
 * - 否则绘制斜线
 *
 * @warning 传入坐标应保证在屏幕范围内，否则可能显示异常。
 */
void OLED_DrawLine(u8 x1,u8 y1,u8 x2,u8 y2);

/**
 * @brief 在显存中绘制一个圆
 *
 * @param x 圆心的横坐标，范围通常为 0~127
 * @param y 圆心的纵坐标，范围通常为 0~63
 * @param r 圆的半径，单位为像素
 *
 * @note 若圆超出屏幕边界，则超出部分无法正常显示。
 */
void OLED_DrawCircle(u8 x,u8 y,u8 r);

/**
 * @brief 在指定位置显示一个 ASCII 字符
 *
 * @param x 字符左上角起始横坐标
 *          - 范围通常为 0~127
 *
 * @param y 字符左上角起始纵坐标
 *          - 范围通常为 0~63
 *
 * @param chr 要显示的字符
 *            - 一般为 ASCII 可见字符，如 'A'、'0'、'+'、'a'
 *
 * @param size1 字符字号大小
 *              - 常用取值：12、16、24
 *              - 对应字模表 asc2_1206、asc2_1608、asc2_2412
 *
 * @note
 * - size1 表示字符高度（像素）
 * - 字符宽度通常约为 size1/2
 * - 仅支持字模表中已有的 ASCII 字符
 */
void OLED_ShowChar(u8 x,u8 y,u8 chr,u8 size1);

/**
 * @brief 在指定位置显示一个 ASCII 字符串
 *
 * @param x 字符串起始显示的横坐标
 * @param y 字符串起始显示的纵坐标
 * @param chr 字符串指针，指向以 '\0' 结尾的字符串
 *            - 例如 (u8 *)"Hello"
 *
 * @param size1 字符大小
 *              - 常用取值：12、16、24
 *              - 必须与字模库支持的大小一致
 *
 * @note
 * - 函数会从 chr 指向的地址开始逐个字符显示
 * - 当显示到一行末尾时会自动换行
 * - 仅支持 ASCII 可显示字符范围 ' ' ~ '~'
 */
void OLED_ShowString(u8 x,u8 y,u8 *chr,u8 size1);

/**
 * @brief 在指定位置显示一个无符号整数
 *
 * @param x 数字起始显示的横坐标
 * @param y 数字起始显示的纵坐标
 * @param num 要显示的无符号整数值
 *            - 类型为 u32，范围 0 ~ 4294967295
 *
 * @param len 显示数字的位数
 *            - 例如 len=3，num=5，则可能显示为 005
 *            - 例如 len=5，num=123，则显示为 00123
 *
 * @param size1 数字字符的大小
 *              - 常用取值：12、16、24
 *
 * @note 本函数本质上仍然是逐字符调用 OLED_ShowChar() 来显示数字。
 */
void OLED_ShowNum(u8 x,u8 y,u32 num,u8 len,u8 size1);

/**
 * @brief 在指定位置显示一个中文字符（基于字模库索引）
 *
 * @param x 汉字左上角起始横坐标
 * @param y 汉字左上角起始纵坐标
 * @param num 汉字在字模库中的编号/索引
 *            - 不是汉字编码本身
 *            - 而是字模数组中的第几个汉字
 *
 * @param size1 汉字字号大小
 *              - 常用取值：16、24、32、64
 *              - 必须与字模库 Hzk1 / Hzk2 / Hzk3 / Hzk4 对应
 *
 * @note
 * - 使用前必须确保字模库中已经存有对应编号的汉字数据
 * - num 的实际含义取决于你在字库中存放汉字的顺序
 */
void OLED_ShowChinese(u8 x,u8 y,u8 num,u8 size1);

/**
 * @brief 让屏幕内容进行滚动显示
 *
 * @param num 滚动的页数、步数或起始页参数
 *            - 具体含义取决于函数实现
 *            - 常见做法中可表示从第几页开始滚动或滚动范围
 *
 * @param space 滚动速度间隔或滚动步进间距
 *              - 具体含义取决于函数实现
 *              - 常用于控制滚动快慢
 *
 * @note 你提供的代码里没有该函数实现，
 *       这里是根据常见 OLED 滚动接口推断的参数语义。
 *       如果你后面贴出实现，我可以按真实逻辑再精确修订一版。
 */
void OLED_ScrollDisplay(u8 num,u8 space);

/**
 * @brief 设置 OLED 写入数据的起始页和列地址
 *
 * @param x 列起始地址（横向位置），范围通常为 0~127
 *          - 指定从哪一列开始写数据
 *
 * @param y 页地址（page），通常范围为 0~7
 *          - 注意这里不是像素纵坐标，而是“页”
 *          - 每一页对应竖直方向 8 个像素
 *
 * @note 该函数常用于连续写图像数据前先设置写入位置。
 */
void OLED_WR_BP(u8 x,u8 y);

/**
 * @brief 在指定区域显示一张位图图片
 *
 * @param x0 图片显示起始横坐标（左边界）
 * @param y0 图片显示起始页地址（上边界页）
 * @param x1 图片显示结束横坐标（右边界，不含或含取决于实现）
 * @param y1 图片显示结束页地址（下边界页，不含或含取决于实现）
 * @param BMP 位图数组指针，存放图片点阵数据
 *
 * @note
 * - 这里的 y0、y1 通常是页单位，不是像素单位
 * - 一页等于 8 个垂直像素
 * - 图片数据需按 OLED 所要求的页寻址格式提前取模
 */
void OLED_ShowPicture(u8 x0,u8 y0,u8 x1,u8 y1,u8 BMP[]);

/**
 * @brief 初始化 OLED 屏幕
 *
 * @note
 * - 完成 OLED 上电后的初始化配置
 * - 包括显示开关、对比度、扫描方向、寻址模式、电荷泵等设置
 * - 一般在 main() 中只需调用一次
 * - 初始化完成后通常会清屏
 */
void OLED_Init(void);


#endif
