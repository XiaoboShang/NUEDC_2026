import time
from usb import Mouse


def open_mouse():
    mouse = Mouse(timeout_ms=1000, auto_reconnect=True)

    while True:
        try:
            mouse.open()
            info = mouse.info()
            print("USB mouse ready:", info)
            print("button masks:", Mouse.BTN_LEFT_MASK, Mouse.BTN_RIGHT_MASK, Mouse.BTN_MIDDLE_MASK)
            return mouse
        except OSError as err:
            print("waiting for USB mouse:", err)
            time.sleep_ms(500)


mouse = open_mouse()

while True:
    frame = mouse.read(1000)
    if not frame:
        continue

    moved = frame["has_rel"] or frame["has_abs"]
    button_edge = frame["pressed_mask"] or frame["released_mask"]
    wheel_move = frame["wheel"] or frame["hwheel"]

    if not moved and not button_edge and not wheel_move:
        continue

    print(
        "buttons=%d pressed=%d released=%d rel=(%d,%d) abs=(%d,%d) wheel=(%d,%d)" % (
            frame["buttons"],
            frame["pressed_mask"],
            frame["released_mask"],
            frame["rel_x"],
            frame["rel_y"],
            frame["abs_x"],
            frame["abs_y"],
            frame["wheel"],
            frame["hwheel"],
        )
    )
