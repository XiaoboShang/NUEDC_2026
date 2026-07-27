import time
from machine import PWM, FPIOA

CONSTRUCT_WITH_FPIOA = True

# ---- 用户可配置参数 ----
ANGLE_RANGE = (10.0, 170)  # 舵机摆动角度范围(最小角度, 最大角度)
SWING_SPEED = 10         # 摆动速度(单位：度/秒)
PWM_PIN = 42   #控制舵机的PWM引脚
TEST_FREQ = 50  # Hz

# Initialize PWM with 50% duty
try:
    if CONSTRUCT_WITH_FPIOA:
        fpioa = FPIOA()
        fpioa.set_function(PWM_PIN, fpioa.PWM0)
        pwm = PWM(0)
    else:
        pwm = PWM(PWM_PIN, freq=TEST_FREQ, duty=50)
except Exception:
    print("FPIOA setup skipped or failed")

pwm.freq(TEST_FREQ)
# 计算角度对应的脉冲宽度(单位：毫秒)
def angle_to_pulse(angle):
    return 0.5 + (angle / 180) * 2.0

# 计算脉冲宽度对应的duty值
def pulse_to_duty(pulse_ms):
    return int((pulse_ms / 20) * 65535)

def pulse_to_ns(pulse_ms):
    return int(pulse_ms*1000*1000)

# 移动舵机到指定角度
def move_servo(angle):
    pulse = angle_to_pulse(angle)
    duty = pulse_to_ns(pulse)
    pwm.duty_ns(duty)
    #pulse_to_duty(pulse)
    #pwm.duty_u16(duty)
    print(duty)

#角度变动时的等待时间
DELAY_PER_DEGREE = 0.1

print("舵机摆动启动...")

try:
    while True:
        # 从最小角度移动到最大角度
        for angle in range(ANGLE_RANGE[0], ANGLE_RANGE[1] + 1,1):
            move_servo(angle)
            time.sleep(DELAY_PER_DEGREE)

        # 从最大角度移动到最小角度
        for angle in range(ANGLE_RANGE[1], ANGLE_RANGE[0] - 1, -1):
            move_servo(angle)
            time.sleep(DELAY_PER_DEGREE)

except KeyboardInterrupt:
    print("程序终止")
    # 归中后关闭PWM
    move_servo(90)
    time.sleep(1)
    pwm.deinit()
