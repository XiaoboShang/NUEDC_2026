import machine

# This script resets the machine immediately when run.
# It will disconnect the ide connection and restart the device.

print("Resetting the machine...")
machine.reset()

# or reset to bootloader
# machine.bootloader()
