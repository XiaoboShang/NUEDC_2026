# opencv_find_rects.py - 摄像头实时找矩形示例
# 使用 sensor 获取图像 → Threshold → findContours → approxPolyDP 识别矩形

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

def find_rectangles(img_np):
    """检测矩形轮廓并绘制"""
    gray = cv2.cvtColor(img_np, cv2.COLOR_BGR2GRAY)
    blurred = cv2.GaussianBlur(gray, (5, 5), 0)

    binary = cv2.adaptiveThreshold(blurred, 255,
                                    cv2.ADAPTIVE_THRESH_GAUSSIAN_C,
                                    cv2.THRESH_BINARY_INV, 11, 2)

    contours, _ = cv2.findContours(binary, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)

    rect_count = 0
    min_area = 300

    for cnt in contours:
        area = cv2.contourArea(cnt)
        if area < min_area:
            continue

        peri = cv2.arcLength(cnt, True)
        approx = cv2.approxPolyDP(cnt, 0.03 * peri, True)

        if len(approx) == 4:
            cv2.drawContours(img_np, [approx], -1, (0, 255, 0), 3)
            x, y, w, h = cv2.boundingRect(cnt)
            cv2.rectangle(img_np, (x, y), (x + w, y + h), (0, 0, 255), 2)
            cv2.putText(img_np, f"{w}x{h}", (x, y - 5),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 255, 0), 1)
            rect_count += 1

    return rect_count

def main():
    init_camera()
    Display.init(Display.VIRT, width=DETECT_WIDTH, height=DETECT_HEIGHT, to_ide=True)

    clock = time.clock()
    try:
        while True:
            clock.tick()
            img = sensor.snapshot()
            img_np = img.to_numpy_ref()
            n = find_rectangles(img_np)
            fps = clock.fps()
            img.draw_string_advanced(10, 10,20, "OpenCV find rects",color=(0, 255, 0))
            img.draw_string_advanced(10, 30,20, "FPS: %f" % fps,color=(0, 255, 0))
            Display.show_image(img)
            gc.collect()
    finally:
        if isinstance(sensor, Sensor):
            sensor.stop()
        Display.deinit()

if __name__ == "__main__":
    main()
