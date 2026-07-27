import time, os, sys, gc
import lvgl as lv
import uctypes
from media.sensor import *
from media.display import *
from media.media import *
import image

# =======================
# Configuration
# =======================
DISPLAY_TYPE   = Display.ST7701
DISPLAY_WIDTH  = ALIGN_UP(800, 16)
DISPLAY_HEIGHT = 480

CAM_WIDTH  = 640
CAM_HEIGHT = 480

# Global variables for FPS
last_time = 0
frame_count = 0
stat_label = None

# =======================
# LVGL Initialization
# =======================
def lvgl_init():
    global disp_img1, disp_img2
    lv.init()
    disp_drv = lv.disp_create(DISPLAY_WIDTH, DISPLAY_HEIGHT)

    disp_img1 = image.Image(DISPLAY_WIDTH, DISPLAY_HEIGHT, image.BGRA8888)
    disp_img1.clear()

    disp_img2 = image.Image(DISPLAY_WIDTH, DISPLAY_HEIGHT, image.BGRA8888)
    disp_img2.clear()

    disp_drv.set_color_format(lv.COLOR_FORMAT.ARGB8888)

    disp_drv.set_draw_buffers(
        disp_img1.bytearray(),
        disp_img2.bytearray(),
        disp_img1.size(),
        lv.DISP_RENDER_MODE.FULL
    )

    def flush_cb(drv, area, color):
        if drv.flush_is_last():
            ptr = uctypes.addressof(color.__dereference__())
            if disp_img1.virtaddr() == ptr:
                Display.show_image(disp_img1, layer=Display.LAYER_OSD1)
            else:
                Display.show_image(disp_img2, layer=Display.LAYER_OSD1)
        drv.flush_ready()
    disp_drv.set_flush_cb(flush_cb)

def setup_simple_ui(scr):
    global stat_label
    # 1. Make the screen background transparent to see the sensor behind it
    scr.set_style_bg_opa(lv.OPA.TRANSP, 0)

    # 2. Header Bar (Semi-transparent dark)
    header = lv.obj(scr)
    header.set_size(DISPLAY_WIDTH, 60)
    header.align(lv.ALIGN.TOP_MID, 0, 0)
    header.set_style_bg_color(lv.color_hex(0x222222), 0)
    header.set_style_bg_opa(lv.OPA._60, 0)
    header.set_style_border_width(0, 0)
    header.set_style_radius(0, 0)

    # 3. Clean Title
    title = lv.label(header)
    title.set_text(lv.SYMBOL.VIDEO + " LIVE STREAMING")
    title.set_style_text_color(lv.color_hex(0xFFFFFF), 0)
    title.center()

    # 4. Status Indicator
    status_box = lv.obj(scr)
    status_box.set_size(DISPLAY_WIDTH//2 + 100, 60) # Widened for FPS text
    status_box.align(lv.ALIGN.BOTTOM_MID, 0, -20)
    status_box.set_style_bg_color(lv.color_hex(0x000000), 0)
    status_box.set_style_bg_opa(lv.OPA._40, 0)
    status_box.set_style_border_color(lv.palette_main(lv.PALETTE.BLUE), 0)
    status_box.set_style_border_width(1, 0)

    stat_label = lv.label(status_box)
    # Initialize with formatted string
    stat_label.set_text(f"{CAM_WIDTH}x{CAM_HEIGHT} | RGB888 | FPS: 0.0")
    stat_label.set_style_text_color(lv.color_hex(0x00FF00), 0)
    stat_label.center()

def main():
    global frame_count, last_time, stat_label
    # Initialize Display and LVGL
    lvgl_init()
    Display.init(DISPLAY_TYPE, width=DISPLAY_WIDTH, height=DISPLAY_HEIGHT, to_ide=True, osd_num=1)

    scr = lv.scr_act()
    
    # Using your sensor image container
    img1 = lv.img(scr)
    img1.align(lv.ALIGN.CENTER, 0, 0)

    # Apply the simplified UI overlays
    setup_simple_ui(scr)
    last_time = time.ticks_ms()

    # Sensor Setup from your script
    sensor = Sensor()
    sensor.reset()
    sensor.set_framesize(width=CAM_WIDTH, height=CAM_HEIGHT, crop=True)
    sensor.set_pixformat(Sensor.RGB888)
    sensor.run()

    # Main Loop
    while True:
        os.exitpoint()

        frame_count += 1
        current_time = time.ticks_ms()
        elapsed = time.ticks_diff(current_time, last_time)

        # Update the sensor image in the LVGL object
        img = sensor.snapshot()
        img.as_lvgl_img_src(img1)
        img1.invalidate()

        # Update FPS every 1000ms (1 second)
        if elapsed >= 1000:
            fps = (frame_count * 1000.0) / elapsed
            # Use f-string for sprintf-like formatting
            stat_label.set_text(f"{CAM_WIDTH}x{CAM_HEIGHT} | RGB888 | FPS: {fps:.1f}")
            
            # Reset counters
            frame_count = 0
            last_time = current_time

        lv.task_handler()
        time.sleep_ms(10)

if __name__ == "__main__":
    main()
