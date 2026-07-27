import os
import time
import machine

import binascii

while True:
    os.exitpoint()
    
    chipid = binascii.hexlify(machine.chipid())
    print(f"chipid {chipid}")

    time.sleep_ms(500)
