# demo_camera_color_shape.py - 摄像头颜色分割 + 形状分类示例
# 使用 sensor 获取图像 → inRange 颜色分割 → findContours → 形状分类(圆/矩形/三角形)

import time, os, gc, sys
from media.sensor import *
from media.display import *
from media.media import *
import cv2
from ulab import numpy as np

DETECT_WIDTH  = 320
DETECT_HEIGHT = 240

sensor = None

# HSV 颜色阈值: 绿色范围
COLOR_LO = np.array([35, 60, 60], dtype=np.uint8)
COLOR_HI = np.array([85, 255, 255], dtype=np.uint8)

def init_camera():
    global sensor
    sensor = Sensor(width=1280, height=960, fps=90)
    sensor.reset()
    sensor.set_framesize(width=DETECT_WIDTH, height=DETECT_HEIGHT)
    sensor.set_pixformat(Sensor.RGB888)
    sensor.run()
    time.sleep(0.5)

def classify_shape(approx):
    """根据多边形顶点数分类形状"""
    n = len(approx)
    if n == 3:
        return "TRI"
    elif n == 4:
        x, y, w, h = cv2.boundingRect(approx)
        ratio = float(w) / float(h) if h > 0 else 0
        return "SQR" if 0.85 < ratio < 1.15 else "RECT"
    elif n >= 8:
        return "CIRCLE"
    elif n >= 5:
        return "POLY"
    return "?"

def detect_shapes(img_np):
    """颜色分割 + 形状检测 + 标注"""
    hsv = cv2.cvtColor(img_np, cv2.COLOR_BGR2HSV)
    mask = cv2.inRange(hsv, COLOR_LO, COLOR_HI)

    kernel = cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (3, 3))
    mask = cv2.morphologyEx(mask, cv2.MORPH_OPEN, kernel, iterations=1)

    contours, _ = cv2.findContours(mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)

    stats = {"TRI": 0, "RECT": 0, "SQR": 0, "CIRCLE": 0, "POLY": 0}
    min_area = 200

    for cnt in contours:
        area = cv2.contourArea(cnt)
        if area < min_area:
            continue

        peri = cv2.arcLength(cnt, True)
        approx = cv2.approxPolyDP(cnt, 0.03 * peri, True)
        shape_type = classify_shape(approx)

        x, y, w, h = cv2.boundingRect(cnt)
        cx, cy = x + w // 2, y + h // 2

        cv2.drawContours(img_np, [approx], -1, (0, 255, 0), 3)
        cv2.putText(img_np, shape_type, (cx - 15, cy),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 0, 255), 2)

        stats[shape_type] += 1

    y0 = 20
    for k, v in stats.items():
        if v > 0:
            cv2.putText(img_np, f"{k}:{v}", (8, y0),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.45, (255, 255, 255), 1)
            y0 += 18

    return stats

def main():
    init_camera()
    Display.init(Display.ST7701, width=800, height=480, to_ide=True)

    fps_time = time.ticks_ms()
    frame_cnt = 0
    try:
        while True:
            img = sensor.snapshot()
            img_np = img.to_numpy_ref()
            stats = detect_shapes(img_np)
            gc.collect()
            Display.show_image(img)

            frame_cnt += 1
            if time.ticks_diff(time.ticks_ms(), fps_time) >= 1000:
                fps = frame_cnt
                frame_cnt = 0
                fps_time = time.ticks_ms()
                total = sum(stats.values())
                print(f"FPS: {fps}, 检测到 {total} 个目标: {stats}")
    finally:
        if isinstance(sensor, Sensor):
            sensor.stop()
        Display.deinit()

if __name__ == "__main__":
    main()
