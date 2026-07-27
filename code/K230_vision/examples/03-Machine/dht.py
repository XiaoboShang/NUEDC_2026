import time
import machine
import dht

d = dht.DHT11(machine.Pin(12))

while True:
    try:
        d.measure()
        t = d.temperature()  # eg. 23 (°C)
        h = d.humidity()     # eg. 41 (% RH)
        print(f"temperature: {t}°C, humidity: {h}% RH")
    
    except OSError as e:
        if e.errno == 110:  # ETIMEDOUT
            pass  # Silently ignore timeout errors
        else:
            raise  # Re-raise other OSErrors
    
    except Exception as e:
        if "checksum error" in str(e):
            pass  # Silently ignore checksum errors
        else:
            raise  # Re-raise other exceptions
    
    time.sleep(1)
