from machine import OTP
import binascii


def lock_name(lock):
    if lock == OTP.NA:
        return "NA"
    if lock == OTP.RO:
        return "RO"
    if lock == OTP.RW:
        return "RW"
    return "UNKNOWN"


otp = OTP()

# OTP is a singleton. Calling OTP() again returns the same object.
print("same OTP object:", otp is OTP())

# Read bytes from a 4-byte-aligned OTP address.
# A single read can be up to 1024 bytes and must not cross the 0x400 boundary.
data = otp.read(0x000, 32)
print("OTP[0x000:0x020]:", binascii.hexlify(data))

# Query lock states for 4-byte-aligned addresses.
for addr in (0x000, 0x004, 0x400, 0xC00):
    lock = otp.get_lock(addr)
    print("OTP[0x%03x] lock: %s (%d)" % (addr, lock_name(lock), lock))
