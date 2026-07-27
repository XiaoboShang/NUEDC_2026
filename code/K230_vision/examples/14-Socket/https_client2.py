import time
import network
import requests

# Connect to Wi-Fi
def connect_wifi(ssid="TEST", password="12345678"):
    wlan = network.WLAN(0)
    wlan.connect(ssid, password)

    print("Connecting to Wi-Fi", end="")
    for _ in range(20):
        if wlan.ifconfig()[0] != '0.0.0.0':
            break
        print(".", end="")
        time.sleep(0.5)
    print()

    if wlan.ifconfig()[0] == '0.0.0.0':
        raise RuntimeError("Wi-Fi connection failed")
    print("[✓] Wi-Fi Connected:", wlan.ifconfig())

# Use urequests to make HTTPS request
def test_https_baidu():
    print("\n[TEST] HTTPS GET using urequests")
    try:
        response = requests.get("https://www.baidu.com")
        print(response.text)
        response.close()
        print("[✓] HTTPS request completed")
    except Exception as e:
        print("❌ HTTPS request failed:", e)

# Run the test
connect_wifi()
test_https_baidu()
