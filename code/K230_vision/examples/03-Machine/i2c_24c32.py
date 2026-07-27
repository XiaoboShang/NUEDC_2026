import time
from machine import I2C

# 24C32参数
EEPROM_ADDR = 0x50  # 24C32的I2C地址
PAGE_SIZE = 32      # 24C32的页大小(字节)
MEM_SIZE = 4096     # 24C32总容量(4KB)

# 初始化I2C
i2c = I2C(2, scl=11, sda=12, freq=40000)

def test_24c32():
    print("24C32 EEPROM测试开始...")

    # 测试数据 - 生成0x00-0xFF的循环模式
    test_data = bytearray([x & 0xFF for x in range(PAGE_SIZE)])
    test_addr = 0x0000  # 测试起始地址

    print("写入数据: [{}]".format(", ".join("0x{:02x}".format(x) for x in test_data)))

    # 写入一页数据 (24C32支持页写入)
    i2c.writeto_mem(EEPROM_ADDR, test_addr, test_data, addrsize=16)
    print("写入完成，地址: 0x{:04x}".format(test_addr))

    # 等待EEPROM完成写入(重要!)
    time.sleep_ms(10)  # 24C32页写入典型时间5ms

    # 读取回数据
    read_data = i2c.readfrom_mem(EEPROM_ADDR, test_addr, len(test_data), addrsize=16)
    print("读取数据: [{}]".format(", ".join("0x{:02x}".format(x) for x in read_data)))

    # 验证数据
    if test_data == read_data:
        print("测试成功! 写入和读取数据匹配")
    else:
        print("测试失败! 数据不匹配")

    # 随机地址读写测试
    print("\n随机地址读写测试...")
    import urandom
    test_addr = urandom.getrandbits(12)  # 生成12位随机地址(0-0xFFF)
    test_byte = urandom.getrandbits(8)   # 生成随机测试字节

    print("在地址0x{:04x}写入字节: 0x{:02x}".format(test_addr, test_byte))
    i2c.writeto_mem(EEPROM_ADDR, test_addr, bytearray([test_byte]), addrsize=16)
    time.sleep_ms(5)  # 等待写入完成

    read_byte = i2c.readfrom_mem(EEPROM_ADDR, test_addr, 1, addrsize=16)[0]
    print("读取到的字节: 0x{:02x}".format(read_byte))

    if test_byte == read_byte:
        print("随机地址测试成功!")
    else:
        print("随机地址测试失败!")

# 运行测试
test_24c32()

