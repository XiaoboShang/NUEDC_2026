import machine
import time

def test_uart2_uart3_loopback():
    print("Starting UART2 <-> UART3 loopback test")

    fpioa = machine.FPIOA()
    # Map pins - replace with your actual wiring
    fpioa.set_function(11, fpioa.UART2_TXD)
    fpioa.set_function(12, fpioa.UART2_RXD)
    
    fpioa.set_function(50, fpioa.UART3_TXD)
    fpioa.set_function(51, fpioa.UART3_RXD)

    # Helper to create UART
    def create_uart(num, baudrate=115200, bits=machine.UART.EIGHTBITS,
                    parity=machine.UART.PARITY_NONE, stop=machine.UART.STOPBITS_ONE):
        try:
            uart = machine.UART(num, baudrate=baudrate, bits=bits, parity=parity, stop=stop)
            print(f"UART{num} initialized: baud={baudrate}, bits={bits}, parity={parity}, stop={stop}")
            return uart
        except Exception as e:
            print(f"UART{num} init failed: baud={baudrate}, bits={bits}, parity={parity}, stop={stop} -> {e}")
            return None

    test_configs = [
        {'baudrate': 9600,  'bits': machine.UART.EIGHTBITS, 'parity': machine.UART.PARITY_NONE, 'stop': machine.UART.STOPBITS_ONE},
        {'baudrate': 19200, 'bits': machine.UART.SEVENBITS, 'parity': machine.UART.PARITY_EVEN, 'stop': machine.UART.STOPBITS_ONE},
        {'baudrate': 38400, 'bits': machine.UART.EIGHTBITS, 'parity': machine.UART.PARITY_ODD, 'stop': machine.UART.STOPBITS_TWO},
        {'baudrate': 115200,'bits': machine.UART.EIGHTBITS, 'parity': machine.UART.PARITY_NONE, 'stop': machine.UART.STOPBITS_ONE},
    ]

    for cfg in test_configs:
        print("\nTesting config:", cfg)
        uart2 = create_uart(2, **cfg)
        uart3 = create_uart(3, **cfg)
        if uart2 is None or uart3 is None:
            print("Skipping due to initialization failure")
            continue

        uart2.read()  # flush RX buffer
        uart3.read()

        # Test UART2 -> UART3 communication
        msg = b"Hello UART3 from UART2"
        try:
            uart2.write(msg)
            print("UART2 sent:", msg)
            time.sleep_ms(100)
            if uart3.any():
                received = uart3.read()
                print("UART3 received:", received)
                assert received == msg, "UART3 data mismatch!"
            else:
                print("UART3 did not receive data from UART2")
        except Exception as e:
            print("Error UART2->UART3 communication:", e)

        # Test UART3 -> UART2 communication
        msg = b"Hello UART2 from UART3"
        try:
            uart3.write(msg)
            print("UART3 sent:", msg)
            time.sleep_ms(100)
            if uart2.any():
                received = uart2.read()
                print("UART2 received:", received)
                assert received == msg, "UART2 data mismatch!"
            else:
                print("UART2 did not receive data from UART3")
        except Exception as e:
            print("Error UART3->UART2 communication:", e)

        # Test sendbreak on both UARTs
        try:
            uart2.sendbreak()
            print("UART2 sendbreak OK")
        except Exception as e:
            print("UART2 sendbreak error:", e)

        try:
            uart3.sendbreak()
            print("UART3 sendbreak OK")
        except Exception as e:
            print("UART3 sendbreak error:", e)

        uart2.deinit()
        uart3.deinit()
        print("UART2 and UART3 deinitialized for config test")

    print("UART2 <-> UART3 loopback test complete")

test_uart2_uart3_loopback()
