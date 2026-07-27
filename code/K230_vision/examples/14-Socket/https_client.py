import time
import network
import usocket
import ussl

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

# HTTPS GET to www.baidu.com
def test_https_baidu():
    try:
        print("\n[TEST] HTTPS GET https://www.baidu.com")
        addr = usocket.getaddrinfo("www.baidu.com", 443)[0][-1]
        sock = usocket.socket()
        sock.connect(addr)

        # Wrap with TLS — SNI is required by Baidu
        ssl_sock = ussl.wrap_socket(sock, server_hostname="www.baidu.com")

        # Send HTTPS GET request
        ssl_sock.write(b"GET / HTTP/1.1\r\nHost: www.baidu.com\r\nConnection: close\r\n\r\n")

        # Read and print the response
        while True:
            data = ssl_sock.read()
            if not data:
                break
            print(data.decode(), end='')

        ssl_sock.close()
        print("\n[✓] HTTPS request to Baidu completed")
    except Exception as e:
        print("❌ HTTPS request failed:", e)

# Run everything
connect_wifi()
test_https_baidu()
