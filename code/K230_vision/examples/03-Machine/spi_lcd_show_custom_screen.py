import time, image
from machine import FPIOA, Pin, SPI, SPI_LCD

def send_initial_sequence(lcd):
    # Send initialization commands
    lcd.command(0x11, None)  # Sleep out
    lcd.command(0x36, [0xA0])  # MADCTL: Memory Access Control
    lcd.command(0x3A, [0x55])  # COLMOD: Interface Pixel Format
    lcd.command(0xB2, [0x0C, 0x0C, 0x00, 0x33, 0x33])  # PORCTRL: Porch Setting
    lcd.command(0xB7, [0x75])  # GCTRL: Gate Control, VGH=14.97V, VGL=-7.67V
    lcd.command(0xBB, [0x1A])  # VCOMS: VCOM Setting
    lcd.command(0xC0, [0x2C])  # LCMCTRL: LCM Control
    lcd.command(0xC2, [0x01])  # VDVVRHEN: VDV and VRH Command Enable
    lcd.command(0xC3, [0x13])  # VRHS: VRH Set
    lcd.command(0xC4, [0x20])  # VDVS: VDV Set
    lcd.command(0xC6, [0x0F])  # FRCTRL2: Frame Rate Control in Normal Mode
    lcd.command(0xD0, [0xA4, 0xA1])  # PWCTRL1: Power Control 1
    lcd.command(0xD6, [0xA1])  # PWCTRL3: Power Control 3
    # Gamma settings
    lcd.command(0xE0, [0xD0, 0x0D, 0x14, 0x0D, 0x0D, 0x09, 0x38, 0x44, 0x4E, 0x3A, 0x17, 0x18, 0x2F, 0x30])  # Positive Gamma Correction
    lcd.command(0xE1, [0xD0, 0x09, 0x0F, 0x08, 0x07, 0x14, 0x37, 0x44, 0x4D, 0x38, 0x15, 0x16, 0x2C, 0x2E])  # Negative Gamma Correction
    # Display on
    lcd.command(0x20, None)  # Display Inversion Off
    lcd.command(0x29, None)  # Display On
    lcd.command(0x2C, None)  # Memory Write

fpioa = FPIOA()

fpioa.set_function(19, FPIOA.GPIO19)
pin_cs = Pin(19, Pin.OUT, pull=Pin.PULL_NONE, drive=15)
pin_cs.value(1)

fpioa.set_function(20, FPIOA.GPIO20)
pin_dc = Pin(20, Pin.OUT, pull=Pin.PULL_NONE, drive=15)
pin_dc.value(1)

fpioa.set_function(44, FPIOA.GPIO44, pu = 1)
pin_rst = Pin(44, Pin.OUT, pull=Pin.PULL_UP, drive=15)

# spi
fpioa.set_function(15, fpioa.QSPI0_CLK)
fpioa.set_function(16, fpioa.QSPI0_D0)

spi1 = SPI(1,baudrate=1000*1000*50, polarity=1, phase=1, bits=8)

lcd = SPI_LCD(spi1, pin_dc, pin_cs, pin_rst)
lcd.configure(320, 240, hmirror = False, vflip = False, bgr = False)

print(lcd)

send_initial_sequence(lcd)
img = lcd.init(True)
print(img)

img.clear()
img.draw_string_advanced(0,0,32, "RED, 你好世界~", color = (255, 0, 0))
img.draw_string_advanced(0,40,32, "GREEN, 你好世界~", color = (0, 255, 0))
img.draw_string_advanced(0,80,32, "BLUE, 你好世界~", color = (0, 0, 255))

lcd.show()
