# opencv_find_circles.py - 摄像头实时找圆示例
# 使用 sensor 获取图像 → OpenCV 霍夫圆检测 → 绘制结果 → 显示

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
    Display.init(Display.VIRT, width=DETECT_WIDTH, height=DETECT_HEIGHT, to_ide=True)

    clock = time.clock()
    try:
        while True:
            clock.tick()
            img = sensor.snapshot()
            img_np = img.to_numpy_ref()
            n = find_circles(img_np)
            fps = clock.fps()
            img.draw_string_advanced(10, 10,20, "OpenCV find circles",color=(0, 255, 0))
            img.draw_string_advanced(10, 30,20, "FPS: %f" % fps,color=(0, 255, 0))
            Display.show_image(img)
            gc.collect()
    finally:
        if isinstance(sensor, Sensor):
            sensor.stop()
        Display.deinit()

if __name__ == "__main__":
    main()
