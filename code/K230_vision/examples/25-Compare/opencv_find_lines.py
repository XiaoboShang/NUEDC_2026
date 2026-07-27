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
    Display.init(Display.VIRT, width=DETECT_WIDTH, height=DETECT_HEIGHT, to_ide=True)

    clock = time.clock()
    try:
        while True:
            clock.tick()
            img = sensor.snapshot()
            img_np = img.to_numpy_ref()
            n = find_lines(img_np)
            fps = clock.fps()
            img.draw_string_advanced(10, 10,20, "OpenCV find lines",color=(0, 255, 0))
            img.draw_string_advanced(10, 30,20, "FPS: %f" % fps,color=(0, 255, 0))
            Display.show_image(img)
            gc.collect()
    finally:
        if isinstance(sensor, Sensor):
            sensor.stop()
        Display.deinit()

if __name__ == "__main__":
    main()
