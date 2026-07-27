import time
from machine import Pin
import neopixel

# === CONFIGURATION ===
NEOPIXEL_PIN = 42     # Change to your actual GPIO pin
NUM_PIXELS   = 10      # Number of WS2812 LEDs

# === INITIALIZE NEOPIXEL OBJECT ===
np = neopixel.NeoPixel(Pin(NEOPIXEL_PIN), NUM_PIXELS)

# === FUNCTION: Show solid colors ===
def test_colors():
    print("[TEST] Setting colors...")
    colors = [(255, 0, 0), (0, 255, 0), (0, 0, 255), (128, 128, 0)]

    for i, color in enumerate(colors):
        for j in range(NUM_PIXELS):
            np[j] = color
        np.write()
        print(f"  → Color: {color}")
        time.sleep(0.5)

# === FUNCTION: Test individual pixel addressing ===
def test_pixels():
    print("[TEST] Lighting up pixels one by one...")
    for i in range(NUM_PIXELS):
        np.fill((0, 0, 0))
        np[i] = (0, 255, 128)
        np.write()
        print(f"  → Pixel {i} ON")
        time.sleep(0.2)

# === FUNCTION: Clear all pixels ===
def clear():
    np.fill((0, 0, 0))
    np.write()
    print("[TEST] Cleared all pixels.")

# === MAIN TEST SEQUENCE ===
def run_test():
    test_colors()
    test_pixels()
    clear()
    print("[DONE] NeoPixel test completed.")

run_test()
