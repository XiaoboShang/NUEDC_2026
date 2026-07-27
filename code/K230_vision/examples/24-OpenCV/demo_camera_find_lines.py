# demo_camera_find_lines.py - 摄像头实时找直线示例
# 使用 sensor 获取图像 → OpenCV Canny + 霍夫直线检测 → 绘制结果

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

def find_lines(img_np):
    """检测线段并绘制"""
    gray = cv2.cvtColor(img_np, cv2.COLOR_BGR2GRAY)
    blurred = cv2.GaussianBlur(gray, (5, 5), 0)
    edges = cv2.Canny(blurred, 60, 150)

    lines = cv2.HoughLinesP(edges, 1, np.pi / 180, 60, minLineLength=40, maxLineGap=15)
    if lines is not None:
        n = lines.shape[0]
        for i in range(n):
            x1, y1, x2, y2 = lines[i, 0], lines[i, 1], lines[i, 2], lines[i, 3]
            cv2.line(img_np, (int(x1), int(y1)), (int(x2), int(y2)), (0, 255, 0), 2)
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
            n = find_lines(img_np)
            gc.collect()
            Display.show_image(img)

            frame_cnt += 1
            if time.ticks_diff(time.ticks_ms(), fps_time) >= 1000:
                fps = frame_cnt
                frame_cnt = 0
                fps_time = time.ticks_ms()
                print(f"FPS: {fps}, 检测到线段: {n}")
    finally:
        if isinstance(sensor, Sensor):
            sensor.stop()
        Display.deinit()

if __name__ == "__main__":
    main()
