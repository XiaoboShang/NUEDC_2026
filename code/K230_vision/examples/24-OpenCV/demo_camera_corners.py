# demo_camera_corners.py - 摄像头实时角点检测示例
# 使用 sensor 获取图像 → OpenCV goodFeaturesToTrack → 绘制角点

import time, os, gc, sys
from media.sensor import *
from media.display import *
from media.media import *
import cv2
from ulab import numpy as np

DETECT_WIDTH  = 320
DETECT_HEIGHT = 240

sensor = None

def init_camera():
    global sensor
    sensor = Sensor(width=1280, height=960, fps=90)
    sensor.reset()
    sensor.set_framesize(width=DETECT_WIDTH, height=DETECT_HEIGHT)
    sensor.set_pixformat(Sensor.RGB888)
    sensor.run()
    time.sleep(0.5)

def detect_corners(img_np):
    """检测 Shi-Tomasi 角点并绘制"""
    gray = cv2.cvtColor(img_np, cv2.COLOR_BGR2GRAY)

    corners = cv2.goodFeaturesToTrack(gray, 40, 0.01, 10, blockSize=3)
    if corners is None:
        return 0

    n = corners.shape[0]
    for i in range(n):
        x, y = corners[i, 0], corners[i, 1]
        cv2.circle(img_np, (int(x), int(y)), 3, (0, 255, 0), -1)
        cv2.circle(img_np, (int(x), int(y)), 5, (0, 0, 255), 1)

    return n

def main():
    init_camera()
    Display.init(Display.ST7701, width=800, height=480, to_ide=True)

    fps_time = time.ticks_ms()
    frame_cnt = 0
    try:
        while True:
            img = sensor.snapshot()
            img_np = img.to_numpy_ref()
            n = detect_corners(img_np)
            gc.collect()
            Display.show_image(img)

            frame_cnt += 1
            if time.ticks_diff(time.ticks_ms(), fps_time) >= 1000:
                fps = frame_cnt
                frame_cnt = 0
                fps_time = time.ticks_ms()
                print(f"FPS: {fps}, 检测到角点: {n}")
    finally:
        if isinstance(sensor, Sensor):
            sensor.stop()
        Display.deinit()

if __name__ == "__main__":
    main()
