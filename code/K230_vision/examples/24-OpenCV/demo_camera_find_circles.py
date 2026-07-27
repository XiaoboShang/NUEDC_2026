# demo_camera_find_circles.py - 摄像头实时找圆示例
# 使用 sensor 获取图像 → OpenCV 霍夫圆检测 → 绘制结果 → 显示

import time, os, gc, sys
from media.sensor import *
from media.display import *
from media.media import *
import cv2
from ulab import numpy as np

DETECT_WIDTH  = 800
DETECT_HEIGHT = 480

sensor = None

def init_camera():
    global sensor
    sensor = Sensor(width=1280, height=960, fps=90)
    sensor.reset()
    sensor.set_framesize(width=DETECT_WIDTH, height=DETECT_HEIGHT)
    sensor.set_pixformat(Sensor.RGB888)  # OpenCV 处理需 RGB888
    sensor.run()
    time.sleep(0.5)

def find_circles(img_np):
    """在 numpy 图像中检测圆并绘制"""
    gray = cv2.cvtColor(img_np, cv2.COLOR_BGR2GRAY)
    blurred = cv2.GaussianBlur(gray, (9, 9), 2)

    circles = cv2.HoughCircles(blurred, 3, dp=1.2, minDist=50,
                                param1=100, param2=35, minRadius=15, maxRadius=150)
    if circles is not None:
        n = circles.shape[0]
        for i in range(n):
            cx, cy, r = circles[i, 0], circles[i, 1], circles[i, 2]
            cv2.circle(img_np, (int(cx), int(cy)), int(r), (0, 255, 0), 3)
            cv2.circle(img_np, (int(cx), int(cy)), 2, (0, 0, 255), -1)
        return n
    return 0

def main():
    init_camera()
    Display.init(Display.ST7701, width=800, height=480, to_ide=True)

    fps_time = time.ticks_ms()
    frame_cnt = 0
    try:
        while True:
            img = sensor.snapshot()
            img_np = img.to_numpy_ref()
            n = find_circles(img_np)
            gc.collect()
            Display.show_image(img)

            frame_cnt += 1
            if time.ticks_diff(time.ticks_ms(), fps_time) >= 1000:
                fps = frame_cnt
                frame_cnt = 0
                fps_time = time.ticks_ms()
                print(f"FPS: {fps}, 检测到圆: {n}")
    finally:
        if isinstance(sensor, Sensor):
            sensor.stop()
        Display.deinit()

if __name__ == "__main__":
    main()
