# Rectangle detection:
#
# This example is for detecting rectangles in an image.
import time, os, gc, sys

from media.sensor import *
from media.display import *
from media.media import *
import image


DETECT_WIDTH = ALIGN_UP(320, 16)
DETECT_HEIGHT = 240

sensor = None

# construct a Sensor object with default configure
sensor = Sensor(width=1280, height=960, fps=90)
# sensor reset
sensor.reset()
# set chn0 output size
sensor.set_framesize(width=DETECT_WIDTH,height=DETECT_HEIGHT)
# set chn0 output format
sensor.set_pixformat(Sensor.RGB565)

# use IDE as display output
Display.init(Display.VIRT, width=DETECT_WIDTH, height=DETECT_HEIGHT, to_ide=True)

# init media manager
MediaManager.init()
# sensor start run
sensor.run()
clock = time.clock()
while True:
    clock.tick()
    img = sensor.snapshot()
    for r in img.find_rects(roi=(0,0,img.width(),img.height()),threshold = 10000):
        img.draw_rectangle([v for v in r.rect()], color = (0, 255, 0))
    fps = clock.fps()
    img.draw_string_advanced(10, 10,20, "OpenMV find rects",color=(0, 255, 0))
    img.draw_string_advanced(10, 30,20, "FPS: %f" % fps,color=(0, 255, 0))
    Display.show_image(img)
    gc.collect()
# sensor stop run
sensor.stop()
# deinit display
Display.deinit()
# sleep
os.exitpoint(os.EXITPOINT_ENABLE_SLEEP)
time.sleep_ms(100)
# release media buffer
MediaManager.deinit()

