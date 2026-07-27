from machine import UART
import time

if not hasattr(UART, "UART_GS"):
    raise ValueError("UART_GS requires CONFIG_ENABLE_DUAL_CDC_PORT")

# Virtual CDC UART exposed as /dev/ttyGS1 on the board side.
# On the PC side this is the second COM port, for example COM17.
# Use it like the hardware UART constants: UART.UART_GS.

uart = UART(
    UART.UART_GS,
    baudrate=115200,
    bits=UART.EIGHTBITS,
    parity=UART.PARITY_NONE,
    stop=UART.STOPBITS_ONE,
    timeout=0,
)

print("UART_GS echo ready")
uart.write(b"UART_GS echo ready\r\n")

last_was_cr = False

while True:
    data = uart.read(1)
    if not data:
        time.sleep_ms(1)
        continue

    ch = data[0]

    # Most PC serial tools send CR or CRLF when Enter is pressed.
    # Normalize that into a single newline on both GS1 and the IDE side.
    if ch == 0x0d:
        uart.write(b"\r\n")
        print()
        last_was_cr = True
        continue

    if ch == 0x0a:
        if last_was_cr:
            last_was_cr = False
            continue
        uart.write(b"\r\n")
        print()
        continue

    last_was_cr = False
    uart.write(data)
    print(chr(ch), end="")

    time.sleep_ms(1)
