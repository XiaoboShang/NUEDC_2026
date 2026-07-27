import time
from usb import Touch


def open_touch():
    touch = Touch(timeout_ms=1000, auto_reconnect=True)

    while True:
        try:
            touch.open()
            info = touch.info()
            print("USB touch ready:", info)
            return touch
        except OSError as err:
            print("waiting for USB touch device:", err)
            time.sleep_ms(500)


touch = open_touch()

while True:
    frame = touch.read(1000)
    if not frame:
        continue

    active = frame["touch_seen"] or frame["buttons"] or frame["pressure"]
    moved = frame["has_abs"] or frame["has_rel"]

    if not active and not moved:
        continue

    print(
        "down=%s pressure=%d buttons=%d abs=(%d,%d) rel=(%d,%d)" % (
            frame["touch_down"],
            frame["pressure"],
            frame["buttons"],
            frame["abs_x"],
            frame["abs_y"],
            frame["rel_x"],
            frame["rel_y"],
        )
    )
