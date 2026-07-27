# demo_camera_binary_morph.py - 摄像头二值化 + 形态学开闭运算示例
# 使用 sensor 获取图像 → cvtColor → threshold → morphologyEx → 显示

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

def process_morphology(img_np):
    """二值化 + 形态学处理, 将结果画到原图"""
    gray = cv2.cvtColor(img_np, cv2.COLOR_BGR2GRAY)
    blurred = cv2.GaussianBlur(gray, (7, 7), 0)

    _, binary = cv2.threshold(blurred, 0, 255, cv2.THRESH_BINARY + cv2.THRESH_OTSU)

    kernel = cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (5, 5))
    opened = cv2.morphologyEx(binary, cv2.MORPH_OPEN, kernel, iterations=2)
    closed = cv2.morphologyEx(opened, cv2.MORPH_CLOSE, kernel, iterations=2)

    contours, _ = cv2.findContours(closed, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)

    obj_count = 0
    min_area = 150
    for cnt in contours:
        area = cv2.contourArea(cnt)
        if area < min_area:
            continue
        x, y, w, h = cv2.boundingRect(cnt)
        cv2.rectangle(img_np, (x, y), (x + w, y + h), (0, 255, 0), 2)
        cv2.putText(img_np, f"A:{int(area)}", (x, y - 5),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.4, (255, 255, 0), 1)
        obj_count += 1

    return obj_count

def main():
    init_camera()
    Display.init(Display.ST7701, width=800, height=480, to_ide=True)

    fps_time = time.ticks_ms()
    frame_cnt = 0
    try:
        while True:
            img = sensor.snapshot()
            img_np = img.to_numpy_ref()
            n = process_morphology(img_np)
            gc.collect()
            Display.show_image(img)

            frame_cnt += 1
            if time.ticks_diff(time.ticks_ms(), fps_time) >= 1000:
                fps = frame_cnt
                frame_cnt = 0
                fps_time = time.ticks_ms()
                print(f"FPS: {fps}, 检测到目标: {n}")
    finally:
        if isinstance(sensor, Sensor):
            sensor.stop()
        Display.deinit()

if __name__ == "__main__":
    main()
