from machine import Pin
from machine import FPIOA

import time

flag = 0

def callback(pin):
    global flag
    flag = 1 - flag

fpioa = FPIOA()

fpioa.set_function(52, FPIOA.GPIO52)
pin_led = Pin(52, Pin.OUT, pull=Pin.PULL_UP, drive=7)

fpioa.set_function(21, FPIOA.GPIO21)
pin_intr = Pin(21, Pin.IN, pull=Pin.PULL_UP, drive=7)

pin_intr.irq(trigger=Pin.IRQ_LOW_LEVEL, handler=callback)

while True:
    if flag:
        pin_led.toggle()
        time.sleep_ms(100)
    else:
        pin_led.off()
