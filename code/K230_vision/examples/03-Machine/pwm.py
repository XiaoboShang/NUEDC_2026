import time
from machine import PWM, FPIOA

CONSTRUCT_WITH_FPIOA = False

PWM_CHANNEL = 0

PWM_PIN = 42
TEST_FREQ = 1000  # Hz


# Initialize PWM with 50% duty
try:
    if CONSTRUCT_WITH_FPIOA:
        fpioa = FPIOA()
        fpioa.set_function(PWM_PIN, fpioa.PWM0 + PWM_CHANNEL)
        pwm = PWM(PWM_CHANNEL, freq=TEST_FREQ, duty=50)
    else:
        pwm = PWM(PWM_PIN, freq=TEST_FREQ, duty=50)
except Exception:
    print("FPIOA setup skipped or failed")


print("[INIT] freq: {}Hz, duty: {}%".format(pwm.freq(), pwm.duty()))
time.sleep(0.5)

# duty() getter and setter
print("[TEST] duty()")
pwm.duty(25)
print("Set duty to 25%, got:", pwm.duty(), "→ duty_u16:", pwm.duty_u16(), "→ duty_ns:", pwm.duty_ns())
time.sleep(0.2)

# duty_u16()
print("[TEST] duty_u16()")
pwm.duty_u16(32768)  # 50%
print("Set duty_u16 to 32768, got:", pwm.duty_u16(), "→ duty():", pwm.duty(), "→ duty_ns():", pwm.duty_ns())
time.sleep(0.2)

# duty_ns()
print("[TEST] duty_ns()")
period_ns = 1000000000 // pwm.freq()
duty_ns_val = (period_ns * 75) // 100  # 75%
pwm.duty_ns(duty_ns_val)
print("Set duty_ns to", duty_ns_val, "ns (≈75%), got:", pwm.duty_ns(), "→ duty():", pwm.duty(), "→ duty_u16():", pwm.duty_u16())
time.sleep(0.2)

# Change frequency and re-check duty values
print("[TEST] Change frequency to 500Hz")
pwm.freq(500)
print("New freq:", pwm.freq())
print("Duty after freq change → duty():", pwm.duty(), "→ duty_u16():", pwm.duty_u16(), "→ duty_ns():", pwm.duty_ns())
time.sleep(0.2)

# Clean up
pwm.deinit()
print("[DONE] PWM test completed")
