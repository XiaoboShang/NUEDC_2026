# opencv_edge.py - 摄像头实时边缘检测示例
# 使用 sensor 获取图像 → OpenCV Canny 边缘检测 → 绘制结果

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
    sensor.set_pixformat(Sensor.GRAYSCALE)
    sensor.run()
    time.sleep(0.5)

def detect_edges(img_np):
    """Canny 边缘检测并绘制"""
    blurred = cv2.GaussianBlur(img_np, (3, 3), 0)
    edges = cv2.Canny(blurred, 50, 80)
    return edges

def main():
    init_camera()
    Display.init(Display.VIRT, width=DETECT_WIDTH, height=DETECT_HEIGHT, to_ide=True)

    clock = time.clock()
    try:
        while True:
            clock.tick()
            img = sensor.snapshot()
            img_np = img.to_numpy_ref()
            edges = detect_edges(img_np)
            im_draw=image.Image(DETECT_WIDTH,DETECT_HEIGHT,image.GRAYSCALE,alloc=image.ALLOC_REF,data=edges)
            fps = clock.fps()
            im_draw.draw_string_advanced(10, 10,20, "OpenCV Edges Detection")
            im_draw.draw_string_advanced(10, 30,20, "FPS: %f" % fps)
            Display.show_image(im_draw)
            gc.collect()
    finally:
        if isinstance(sensor, Sensor):
            sensor.stop()
        Display.deinit()

if __name__ == "__main__":
    main()
