import time
import os
import sys
import gc
import network

from libs.WBCRtsp import WBCRtsp

from media.sensor import *
from media.display import *
from media.media import *
from machine import UART, FPIOA


# 摄像头、显示和串口
SENSOR_ID = 2
DISPLAY_WIDTH = 800
DISPLAY_HEIGHT = 480
UART_TX_PIN = 11
UART_RX_PIN = 12
UART_BAUDRATE = 115200

# Wi-Fi AP 和 RTSP 推流
AP_SSID = "K230-BALL"
AP_PASSWORD = "12345678"
RTSP_PORT = 8554
RTSP_SESSION = "test"
WBC_SOURCE_WIDTH = 480
WBC_SOURCE_HEIGHT = 800

# 钢球 LAB 阈值
BALL_THRESHOLD = (31, 63, -128, 21, -128, 8)

# 检测区域，图像左上角为原点
# 左右边界分别作为 -11 cm、+11 cm 参考线
DETECT_X_MIN = 110
DETECT_X_MAX = 750
DETECT_Y_MIN = 200
DETECT_Y_MAX = 250

# 水管 -5 cm、0 cm、+5 cm 刻度线 X 坐标
PIPE_NEG_5CM_X = 280
PIPE_ZERO_CM_X = 413
PIPE_POS_5CM_X = 555

# 检测区域显示参数
DETECT_REGION_COLOR = (0, 255, 0)
DETECT_REGION_THICKNESS = 4
DETECT_REGION_TEXT_SCALE = 2

# find_blobs 初步过滤参数
FIND_PIXELS_THRESHOLD = 30
FIND_AREA_THRESHOLD = 50
FIND_MERGE = False
MERGE_MARGIN = 5

# 钢球像素数量范围
BALL_PIXELS_MIN = 300
BALL_PIXELS_MAX = 1000

# 钢球形状过滤参数
BALL_ASPECT_RATIO_MIN = 0.60
BALL_FILL_RATIO_MIN = 0.35
BALL_CENTER_OFFSET_X_MAX = 0.22
BALL_CENTER_OFFSET_Y_MAX = 0.22
REJECT_ROI_EDGE_BLOBS = True
ROI_EDGE_MARGIN = 2

# 备用钢球识别参数
# 是否将外接矩形面积最大的黄色候选认为是钢球
ENABLE_FALLBACK_BALL = True

# 调试显示参数
SHOW_REJECTED_BLOBS = True
REJECTED_BOX_COLOR = (255, 255, 0)
REJECTED_BOX_THICKNESS = 2
PRINT_REJECTED_REASON = False

# 钢球检测框参数
BOX_PADDING = 5
BOX_COLOR = (255, 0, 0)
BOX_THICKNESS = 3
BLOB_CENTER_COLOR = (255, 0, 0)
BOX_CENTER_COLOR = (0, 0, 255)


def start_wifi_ap():
    """创建 K230 无线热点并返回热点对象和 IP 地址。"""
    print("开始初始化 Wi-Fi AP")

    ap = network.WLAN(network.AP_IF)

    if not ap.active():
        ap.active(True)

    time.sleep(1)

    # 当前庐山派 CanMV 固件已验证可用的参数，不添加 channel。
    ap.config(
        ssid=AP_SSID,
        key=AP_PASSWORD
    )

    # 等待热点和 DHCP 服务就绪。
    time.sleep(5)

    if not ap.active():
        raise RuntimeError("Wi-Fi AP 未成功激活")

    ip_address = ap.ifconfig()[0]

    if ip_address == "0.0.0.0":
        raise RuntimeError("Wi-Fi AP 没有有效 IP 地址")

    print("Wi-Fi AP 启动成功")
    print("SSID:", AP_SSID)
    print("PASSWORD:", AP_PASSWORD)
    print("IP:", ip_address)

    return ap, ip_address


# BALL,valid,x,error_neg11,error_neg5,error_zero,error_pos5,error_pos11
def send_ball_position(uart_device, ball_blobs):
    if not ball_blobs:
        frame = 'BALL,0,-1,0,0,0,0,0\n'
    else:
        controlled_ball = ball_blobs[0]
        for ball_blob in ball_blobs[1:]:
            if ball_blob.pixels() > controlled_ball.pixels():
                controlled_ball = ball_blob

        ball_x = controlled_ball.cx()
        frame = 'BALL,1,{},{},{},{},{},{}\n'.format(
            ball_x,
            ball_x - DETECT_X_MIN,
            ball_x - PIPE_NEG_5CM_X,
            ball_x - PIPE_ZERO_CM_X,
            ball_x - PIPE_POS_5CM_X,
            ball_x - DETECT_X_MAX
        )

    try:
        uart_device.write(frame)
    except Exception as uart_error:
        print('UART 发送失败：', uart_error)


uart = None
sensor = None
sensor_started = False
display_inited = False
media_inited = False
wifi_ap = None
rtsp_started = False


try:
    print("steel_ball_detection")
    print("LAB 阈值：", BALL_THRESHOLD)

    print(
        "钢球像素范围：",
        BALL_PIXELS_MIN,
        "~",
        BALL_PIXELS_MAX
    )

    print(
        "钢球识别 X 轴范围：",
        DETECT_X_MIN,
        "~",
        DETECT_X_MAX
    )

    print(
        "钢球识别 Y 轴范围：",
        DETECT_Y_MIN,
        "~",
        DETECT_Y_MAX
    )

    print("最小宽高比：", BALL_ASPECT_RATIO_MIN)
    print("最小填充率：", BALL_FILL_RATIO_MIN)

    print(
        "最大质心偏移比例：",
        BALL_CENTER_OFFSET_X_MAX,
        BALL_CENTER_OFFSET_Y_MAX
    )

    print(
        "最大黄框备用识别：",
        ENABLE_FALLBACK_BALL
    )

    # --------------------------------------------------------
    # 检查 X 轴检测范围是否合法
    # --------------------------------------------------------

    if DETECT_X_MIN < 0:
        raise ValueError("DETECT_X_MIN 不能小于 0")

    if DETECT_X_MAX >= DISPLAY_WIDTH:
        raise ValueError(
            "DETECT_X_MAX 不能大于或等于 DISPLAY_WIDTH"
        )

    if DETECT_X_MIN >= DETECT_X_MAX:
        raise ValueError(
            "DETECT_X_MIN 必须小于 DETECT_X_MAX"
        )

    # --------------------------------------------------------
    # 检查 Y 轴检测范围是否合法
    # --------------------------------------------------------

    if DETECT_Y_MIN < 0:
        raise ValueError("DETECT_Y_MIN 不能小于 0")

    if DETECT_Y_MAX >= DISPLAY_HEIGHT:
        raise ValueError(
            "DETECT_Y_MAX 不能大于或等于 DISPLAY_HEIGHT"
        )

    if DETECT_Y_MIN >= DETECT_Y_MAX:
        raise ValueError(
            "DETECT_Y_MIN 必须小于 DETECT_Y_MAX"
        )

    # --------------------------------------------------------
    # 检查水管刻度竖线是否在检测区域内
    pipe_mark_x_positions = (
        ('PIPE_NEG_5CM_X', PIPE_NEG_5CM_X),
        ('PIPE_ZERO_CM_X', PIPE_ZERO_CM_X),
        ('PIPE_POS_5CM_X', PIPE_POS_5CM_X)
    )

    for pipe_mark_name, pipe_mark_x in pipe_mark_x_positions:
        if (
            pipe_mark_x < DETECT_X_MIN
            or pipe_mark_x > DETECT_X_MAX
        ):
            raise ValueError(
                pipe_mark_name
                + ' 必须位于 DETECT_X_MIN 和 DETECT_X_MAX 之间'
            )

    print(
        '水管刻度 X 坐标：',
        '-5cm =', PIPE_NEG_5CM_X,
        '0cm =', PIPE_ZERO_CM_X,
        '5cm =', PIPE_POS_5CM_X
    )

    # --------------------------------------------------------
    # 计算 find_blobs 使用的 ROI
    #
    # ROI 格式：
    # (x, y, width, height)
    # --------------------------------------------------------

    detect_roi = (
        DETECT_X_MIN,
        DETECT_Y_MIN,
        DETECT_X_MAX - DETECT_X_MIN + 1,
        DETECT_Y_MAX - DETECT_Y_MIN + 1
    )

    print("钢球识别 ROI：", detect_roi)

    fpioa = FPIOA()
    fpioa.set_function(UART_TX_PIN, FPIOA.UART2_TXD)
    fpioa.set_function(UART_RX_PIN, FPIOA.UART2_RXD)
    uart = UART(
        UART.UART2,
        baudrate=UART_BAUDRATE,
        bits=UART.EIGHTBITS,
        parity=UART.PARITY_NONE,
        stop=UART.STOPBITS_ONE
    )

    # 创建 K230 自身的 Wi-Fi 热点。
    wifi_ap, wifi_ip_address = start_wifi_ap()

    # 创建摄像头对象
    sensor = Sensor(id=SENSOR_ID)
    sensor.reset()

    # 设置摄像头输出尺寸
    sensor.set_framesize(
        width=DISPLAY_WIDTH,
        height=DISPLAY_HEIGHT,
        chn=CAM_CHN_ID_0
    )

    # 使用 RGB565，才能使用 LAB 阈值
    sensor.set_pixformat(
        Sensor.RGB565,
        chn=CAM_CHN_ID_0
    )

    # 初始化 LCD
    Display.init(
        Display.ST7701,
        width=DISPLAY_WIDTH,
        height=DISPLAY_HEIGHT,
        to_ide=True
    )
    display_inited = True

    # 初始化媒体管理器
    MediaManager.init()
    media_inited = True

    # 启动摄像头
    sensor.run()
    sensor_started = True

    # 捕获最终显示输出并以 H.264 RTSP 推流。
    WBCRtsp.configure(
        wbc_width=WBC_SOURCE_WIDTH,
        wbc_height=WBC_SOURCE_HEIGHT
    )
    WBCRtsp.start()
    rtsp_started = True

    rtsp_url = "rtsp://{}:{}/{}".format(
        wifi_ip_address,
        RTSP_PORT,
        RTSP_SESSION
    )

    print("摄像头和 LCD 启动成功")
    print("RTSP 推流启动成功")
    print("VLC 地址:", rtsp_url)

    while True:
        os.exitpoint()

        # 获取原始彩色图像
        img = sensor.snapshot(chn=CAM_CHN_ID_0)

        # 查找并筛选钢球候选

        blobs = img.find_blobs(
            [BALL_THRESHOLD],
            roi=detect_roi,
            pixels_threshold=FIND_PIXELS_THRESHOLD,
            area_threshold=FIND_AREA_THRESHOLD,
            merge=FIND_MERGE,
            margin=MERGE_MARGIN
        )

        ball_blobs = []
        rejected_blobs = []
        fallback_ball_used = False
        fallback_original_reason = None
        fallback_box_area = 0

        for blob in blobs:
            x = blob.x()
            y = blob.y()
            w = blob.w()
            h = blob.h()

            pixel_count = blob.pixels()
            box_area = w * h

            cx = blob.cx()
            cy = blob.cy()

            reject_reason = None

            if w <= 0 or h <= 0 or box_area <= 0:
                reject_reason = "invalid_size"

            elif pixel_count < BALL_PIXELS_MIN:
                reject_reason = "pixels_too_small"

            elif pixel_count > BALL_PIXELS_MAX:
                reject_reason = "pixels_too_large"

            else:
                short_side = min(w, h)
                long_side = max(w, h)

                aspect_ratio = short_side / long_side

                fill_ratio = pixel_count / box_area

                box_center_x = x + (w // 2)
                box_center_y = y + (h // 2)

                center_offset_x_ratio = (
                    abs(cx - box_center_x) / w
                )

                center_offset_y_ratio = (
                    abs(cy - box_center_y) / h
                )

                blob_right = x + w - 1
                blob_bottom = y + h - 1

                touches_roi_edge = (
                    x <= DETECT_X_MIN + ROI_EDGE_MARGIN
                    or y <= DETECT_Y_MIN + ROI_EDGE_MARGIN
                    or blob_right
                    >= DETECT_X_MAX - ROI_EDGE_MARGIN
                    or blob_bottom
                    >= DETECT_Y_MAX - ROI_EDGE_MARGIN
                )

                if aspect_ratio < BALL_ASPECT_RATIO_MIN:
                    reject_reason = "bad_aspect_ratio"

                elif fill_ratio < BALL_FILL_RATIO_MIN:
                    reject_reason = "fill_ratio_too_low"

                elif (
                    center_offset_x_ratio
                    > BALL_CENTER_OFFSET_X_MAX
                ):
                    reject_reason = "center_x_offset"

                elif (
                    center_offset_y_ratio
                    > BALL_CENTER_OFFSET_Y_MAX
                ):
                    reject_reason = "center_y_offset"

                elif (
                    REJECT_ROI_EDGE_BLOBS
                    and touches_roi_edge
                ):
                    reject_reason = "touch_roi_edge"

            if reject_reason is None:
                ball_blobs.append(blob)

            else:
                rejected_blobs.append(
                    (blob, reject_reason)
                )

                if PRINT_REJECTED_REASON:
                    print(
                        "淘汰候选：",
                        reject_reason,
                        "位置 =", x, y,
                        "宽高 =", w, h,
                        "像素数 =", pixel_count,
                        "外接框面积 =", box_area
                    )

        # 无正常结果时，使用面积最大的淘汰候选作为备用钢球

        if (
            ENABLE_FALLBACK_BALL
            and len(ball_blobs) == 0
            and len(rejected_blobs) > 0
        ):
            largest_rejected_index = 0
            largest_rejected_area = -1

            for rejected_index in range(len(rejected_blobs)):
                rejected_item = rejected_blobs[rejected_index]
                rejected_blob = rejected_item[0]

                rejected_area = (
                    rejected_blob.w()
                    * rejected_blob.h()
                )

                if rejected_area > largest_rejected_area:
                    largest_rejected_area = rejected_area
                    largest_rejected_index = rejected_index

            fallback_item = rejected_blobs[
                largest_rejected_index
            ]

            fallback_ball_blob = fallback_item[0]
            fallback_original_reason = fallback_item[1]
            fallback_box_area = largest_rejected_area

            rejected_blobs.pop(largest_rejected_index)
            ball_blobs.append(fallback_ball_blob)

            fallback_ball_used = True

            print(
                "正常筛选未识别到钢球，",
                "启用最大黄框备用识别：",
                "中心 =",
                fallback_ball_blob.cx(),
                fallback_ball_blob.cy(),
                "像素数 =",
                fallback_ball_blob.pixels(),
                "外接框面积 =",
                fallback_box_area,
                "原淘汰原因 =",
                fallback_original_reason
            )

        send_ball_position(uart, ball_blobs)

        # 4. 显示黑白二值图像

#        img.binary(
#            [BALL_THRESHOLD],
#            invert=False
#        )

        # 标出检测区域和水管刻度

        region_width = DETECT_X_MAX - DETECT_X_MIN
        region_height = DETECT_Y_MAX - DETECT_Y_MIN

        img.draw_rectangle(
            DETECT_X_MIN,
            DETECT_Y_MIN,
            region_width,
            region_height,
            color=DETECT_REGION_COLOR,
            thickness=DETECT_REGION_THICKNESS
        )

        for pipe_mark_x in (
            PIPE_NEG_5CM_X,
            PIPE_ZERO_CM_X,
            PIPE_POS_5CM_X
        ):
            img.draw_line(
                pipe_mark_x,
                DETECT_Y_MIN,
                pipe_mark_x,
                DETECT_Y_MAX,
                color=DETECT_REGION_COLOR,
                thickness=2
            )

        if DETECT_Y_MIN >= 30:
            region_text_y = DETECT_Y_MIN - 28
        else:
            region_text_y = DETECT_Y_MIN + 8

        region_text = (
            "X:"
            + str(DETECT_X_MIN)
            + "-"
            + str(DETECT_X_MAX)
            + " Y:"
            + str(DETECT_Y_MIN)
            + "-"
            + str(DETECT_Y_MAX)
        )

        img.draw_string(
            DETECT_X_MIN,
            region_text_y,
            region_text,
            color=DETECT_REGION_COLOR,
            scale=DETECT_REGION_TEXT_SCALE
        )

        if SHOW_REJECTED_BLOBS:
            for rejected_item in rejected_blobs:
                rejected_blob = rejected_item[0]

                img.draw_rectangle(
                    rejected_blob.x(),
                    rejected_blob.y(),
                    rejected_blob.w(),
                    rejected_blob.h(),
                    color=REJECTED_BOX_COLOR,
                    thickness=REJECTED_BOX_THICKNESS
                )

        ball_count = len(ball_blobs)

        for ball_index, ball_blob in enumerate(ball_blobs):
            x = ball_blob.x()
            y = ball_blob.y()
            w = ball_blob.w()
            h = ball_blob.h()

            cx = ball_blob.cx()
            cy = ball_blob.cy()

            pixel_count = ball_blob.pixels()
            box_area = w * h

            aspect_ratio = min(w, h) / max(w, h)
            fill_ratio = pixel_count / box_area

            box_center_x = x + (w // 2)
            box_center_y = y + (h // 2)

            center_offset_x_ratio = (
                abs(cx - box_center_x) / w
            )

            center_offset_y_ratio = (
                abs(cy - box_center_y) / h
            )

            # 检测框向外扩大
            x1 = max(0, x - BOX_PADDING)
            y1 = max(0, y - BOX_PADDING)

            x2 = min(
                DISPLAY_WIDTH - 1,
                x + w + BOX_PADDING
            )

            y2 = min(
                DISPLAY_HEIGHT - 1,
                y + h + BOX_PADDING
            )

            box_w = x2 - x1
            box_h = y2 - y1

            # 绘制红色钢球检测框
            img.draw_rectangle(
                x1,
                y1,
                box_w,
                box_h,
                color=BOX_COLOR,
                thickness=BOX_THICKNESS
            )

            # 绘制 Blob 实际质心：红色大十字
            img.draw_cross(
                cx,
                cy,
                color=BLOB_CENTER_COLOR,
                size=8,
                thickness=2
            )

            # 绘制外接框几何中心：蓝色小十字
            img.draw_cross(
                box_center_x,
                box_center_y,
                color=BOX_CENTER_COLOR,
                size=4,
                thickness=1
            )

            # 显示钢球编号

            if fallback_ball_used:
                ball_label = "F1"
            else:
                ball_label = "B" + str(ball_index + 1)

            label_y = y1 - 18

            if label_y < 0:
                label_y = y1 + 5

            img.draw_string(
                x1,
                label_y,
                ball_label,
                color=BOX_COLOR,
                scale=1
            )

            if fallback_ball_used:
                print(
                    "备用钢球",
                    "中心 =", cx, cy,
                    "像素数 =", pixel_count,
                    "宽高 =", w, h,
                    "外接框面积 =", box_area,
                    "宽高比 =", aspect_ratio,
                    "填充率 =", fill_ratio,
                    "中心偏移 =",
                    center_offset_x_ratio,
                    center_offset_y_ratio,
                    "原淘汰原因 =",
                    fallback_original_reason
                )

            else:
                print(
                    "钢球",
                    ball_index + 1,
                    "中心 =", cx, cy,
                    "像素数 =", pixel_count,
                    "宽高 =", w, h,
                    "外接框面积 =", box_area,
                    "宽高比 =", aspect_ratio,
                    "填充率 =", fill_ratio,
                    "中心偏移 =",
                    center_offset_x_ratio,
                    center_offset_y_ratio
                )

        if fallback_ball_used:
            count_text = (
                "BALLS:"
                + str(ball_count)
                + " FALLBACK"
            )
        else:
            count_text = "BALLS:" + str(ball_count)

        img.draw_string(
            10,
            10,
            count_text,
            color=BOX_COLOR,
            scale=2
        )

        if ball_count == 0:
            print("指定区域内未检测到任何候选钢球")

        elif fallback_ball_used:
            print(
                "当前使用最大黄框作为备用钢球，",
                "钢球数量：",
                ball_count
            )

        else:
            print(
                "当前正常识别钢球数量：",
                ball_count
            )

        # 显示处理结果
        Display.show_image(img)

        gc.collect()


except KeyboardInterrupt:
    print("用户停止程序")

except BaseException as e:
    print("异常：", e)

finally:
    # 必须先停止 WBC/RTSP，再释放摄像头、显示和媒体资源。
    if rtsp_started:
        try:
            WBCRtsp.stop()
        except BaseException as rtsp_cleanup_error:
            print("停止 RTSP 推流失败：", rtsp_cleanup_error)

    if uart is not None:
        uart.deinit()

    if sensor_started and sensor is not None:
        sensor.stop()

    if display_inited:
        Display.deinit()

    os.exitpoint(os.EXITPOINT_ENABLE_SLEEP)
    time.sleep_ms(100)

    if media_inited:
        MediaManager.deinit()

    print("程序已退出")


