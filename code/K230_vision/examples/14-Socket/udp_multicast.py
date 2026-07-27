import socket
import struct
import time
import network

MULTICAST_GROUP = '239.255.0.1'
MULTICAST_PORT = 5007

# Toggle for WLAN or LAN
IS_WLAN = True
# Toggle for sender or receiver
IS_SENDER = False

def inet_aton(ip_str):
    """Convert dotted string to 4-byte IP (like socket.inet_aton)."""
    return bytes(map(int, ip_str.split('.')))

def network_use_wlan(is_wlan=True):
    """Initialize network interface and return IP address."""
    if is_wlan:
        sta = network.WLAN(0)
        sta.connect("TEST","12345678")
        print("[WLAN] Connecting...")
        while sta.ifconfig()[0] == '0.0.0.0':
            time.sleep(0.5)
        ip = sta.ifconfig()[0]
        print("[WLAN] Connected:", sta.ifconfig())
    else:
        lan = network.LAN()
        if not lan.active():
            raise RuntimeError("LAN interface is not active.")
        lan.ifconfig("dhcp")
        while lan.ifconfig()[0] == '0.0.0.0':
            time.sleep(0.5)
        ip = lan.ifconfig()[0]
        print("[LAN] Connected:", lan.ifconfig())
    return ip

def multicast_sender():
    """Send UDP multicast messages every 2 seconds."""
    ip = network_use_wlan(IS_WLAN)

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    ttl = struct.pack('b', 1)
    sock.setsockopt(0, 33, ttl)  # IPPROTO_IP = 0, IP_MULTICAST_TTL = 33

    count = 0
    print(f"[SENDER] Sending to {MULTICAST_GROUP}:{MULTICAST_PORT}")
    while True:
        msg = f"[{ip}] Multicast message {count}"
        try:
            sock.sendto(msg.encode(), (MULTICAST_GROUP, MULTICAST_PORT))
            print(f"[SENDER] Sent: {msg}")
        except Exception as e:
            print("[SENDER] Error:", e)
        count += 1
        time.sleep(2)

def multicast_receiver():
    """Listen to UDP multicast messages."""
    network_ip = network_use_wlan(IS_WLAN)

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind(('0.0.0.0', MULTICAST_PORT))

    mreq = struct.pack("4s4s",
                       inet_aton(MULTICAST_GROUP),
                       inet_aton('0.0.0.0'))  # Use 'network_ip' for stricter binding

    sock.setsockopt(0, 35, mreq)  # IPPROTO_IP = 0, IP_ADD_MEMBERSHIP = 35

    print(f"[RECEIVER] Listening on {MULTICAST_GROUP}:{MULTICAST_PORT} ...")

    while True:
        try:
            data, addr = sock.recvfrom(1024)
            if data:
                try:
                    print(f"[RECEIVER] From {addr}: {data.decode()}")
                except UnicodeError:
                    print(f"[RECEIVER] From {addr}: <binary data>")
            else:
                print(f"[RECEIVER] From {addr}: <empty packet>")
        except Exception as e:
            print("[RECEIVER] Error:", e)
            break

def main():
    if IS_SENDER:
        multicast_sender()
    else:
        multicast_receiver()

main()
