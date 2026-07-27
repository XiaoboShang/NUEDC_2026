import time
from usb import Keyboard


def open_keyboard():
    keyboard = Keyboard(timeout_ms=1000, auto_reconnect=True)

    while True:
        try:
            keyboard.open()
            info = keyboard.info()
            print("USB keyboard ready:", info)
            return keyboard
        except OSError as err:
            print("waiting for USB keyboard:", err)
            time.sleep_ms(500)


keyboard = open_keyboard()

while True:
    frame = keyboard.read(1000)
    if not frame:
        continue

    events = frame["events"]
    if not events:
        continue

    text = frame["text"]
    chars = frame["chars"]

    for keycode, value in events:
        if value == Keyboard.VALUE_PRESSED:
            state = "pressed"
        elif value == Keyboard.VALUE_REPEAT:
            state = "repeat"
        else:
            state = "released"

        print("keycode=%d state=%s" % (keycode, state))

    if chars:
        print(
            "parsed chars=%s text=%r ctrl=%s shift=%s alt=%s meta=%s caps_lock=%s"
            % (
                chars,
                text,
                frame["ctrl"],
                frame["shift"],
                frame["alt"],
                frame["meta"],
                frame["caps_lock"],
            )
        )
