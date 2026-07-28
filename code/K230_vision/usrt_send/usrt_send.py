# 立创·庐山派-K230-CanMV开发板资料与相关扩展板软硬件资料官网全部开源
# 开发板官网：www.lckfb.com
# 技术支持常驻论坛，任何技术问题欢迎随时交流学习
# 立创论坛：www.jlc-bbs.com/lckfb
# 关注bilibili账号：【立创开发板】，掌握我们的最新动态！
# 不靠卖板赚钱，以培养中国工程师为己任
# 编写者：LCKFB-YZH
import time, os
from machine import UART, FPIOA

# 配置引脚
fpioa = FPIOA()
fpioa.set_function(11, FPIOA.UART2_TXD)
fpioa.set_function(12, FPIOA.UART2_RXD)

# 初始化 UART2，115200@8N1
uart = UART(UART.UART2, baudrate=115200, bits=UART.EIGHTBITS, parity=UART.PARITY_NONE, stop=UART.STOPBITS_ONE)

count = 0

try:
    while True:
        os.exitpoint()
        message = "Hello LuShan-Pi! Count: {}\n".format(count)
        uart.write(message)
        print(f"[发送] {message.strip()}")
        count += 1
        time.sleep_ms(500)
except KeyboardInterrupt:
    print("[INFO] 用户停止")
finally:
    uart.deinit()
    print("[INFO] UART 资源已释放")
