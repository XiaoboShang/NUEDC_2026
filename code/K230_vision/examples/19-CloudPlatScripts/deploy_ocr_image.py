# -*- coding: utf-8 -*-
'''
Script: deploy_ocr_image.py
脚本名称：deploy_ocr_image.py

Description:
    This script performs Optical Character Recognition (OCR) on a single image.
    It uses a two-stage approach:
    - OCR detection to locate text regions.
    - OCR recognition to recognize text content from each detected region.

    Both models (detection and recognition) are configured via JSON files exported
    from the Canaan online training platform. Please prepare the test image yourself.

脚本说明：
    本脚本对单张图像执行光学字符识别（OCR）。
    它采用两阶段方法：
    - OCR 检测：定位文本区域；
    - OCR 识别：从每个检测到的区域识别文本内容。

    两个模型（检测和识别）的配置均通过 Canaan 在线训练平台导出的 JSON 文件进行设置。 请自行准备测试图片。

Author: Canaan Developer
作者：Canaan 开发者
'''


import os, gc
from libs.PlatTasks import OCRDetectionApp, OCRRecognitionApp
from libs.PipeLine import PipeLine
from libs.Utils import *

# Load image in CHW format for inference and RGB888 for display
# The image dimensions are used for setting the input size for the inference model
img_chw, img_rgb888 = read_image("/sdcard/test.jpg")
rgb888p_size = [img_chw.shape[2], img_chw.shape[1]]

# ------------------------------ OCR Detection Configuration ------------------------------

# Set root directory path for OCR detection model and config
ocrdet_root_path = "/sdcard/ocrdet_mp_deployment_source/"

# Load OCR detection deployment configuration
ocrdet_deploy_conf = read_json(ocrdet_root_path + "/deploy_config.json")
ocrdet_kmodel_path = ocrdet_root_path + ocrdet_deploy_conf["kmodel_path"]    # KModel path
mask_threshold = ocrdet_deploy_conf["mask_threshold"]                        # Mask threshold
box_threshold = ocrdet_deploy_conf["box_threshold"]                          # Box threshold
ocrdet_model_input_size = ocrdet_deploy_conf["img_size"]                     # Model input size

# ------------------------------ OCR Recognition Configuration ------------------------------

# Set root directory path for OCR recognition model and config
ocrrec_root_path = "/sdcard/ocrrec_mp_deployment_source/"

# Load OCR recognition deployment configuration
ocrrec_deploy_conf = read_json(ocrrec_root_path + "/deploy_config.json")
ocrrec_kmodel_path = ocrrec_root_path + ocrrec_deploy_conf["kmodel_path"]   # KModel path

# Load character dictionary for recognition
dict_path = ocrrec_root_path + "dict.txt"
with open(dict_path, 'r') as file:
    line_one = file.read(100000)
    line_list = line_one.split("\n")
DICT = {num: char.replace("\r", "").replace("\n", "") for num, char in enumerate(line_list)}

ocrrec_model_input_size = ocrrec_deploy_conf["img_size"]                     # Model input size

# ------------------------------ Initialize OCR Apps ------------------------------

inference_mode = "image"                                                     # Inference mode: 'image'
debug_mode = 0                                                               # Debug mode flag

# Initialize OCR detection and recognition applications
ocrdet_app = OCRDetectionApp(inference_mode, ocrdet_kmodel_path, ocrdet_model_input_size,mask_threshold, box_threshold,rgb888p_size, rgb888p_size, debug_mode=debug_mode)
ocrrec_app = OCRRecognitionApp(inference_mode, ocrrec_kmodel_path, ocrrec_model_input_size,DICT, rgb888p_size, rgb888p_size, debug_mode=debug_mode)

# Configure preprocessing for OCR detection model
ocrdet_app.config_preprocess()

# Run OCR detection to get text boxes and cropped regions
det_res = ocrdet_app.run(img_chw)

# Perform OCR recognition on each detected text region
rec_texts = []
for i in range(len(det_res["boxes"])):
    # Convert the cropped region from HWC to CHW
    det_ = det_res["crop_images"][i][0]
    det_chw = hwc2chw(det_)

    # Configure recognition input size for each cropped region
    ocrrec_app.config_preprocess(input_image_size=[det_chw.shape[2], det_chw.shape[1]])

    # Run recognition
    res = ocrrec_app.run(det_chw)
    rec_texts.append(res["text"])

# Draw OCR results (bounding boxes and recognized text) on the image
ocrrec_app.draw_result(img_rgb888, det_res["boxes"], rec_texts)

# Prepare image for IDE display
img_rgb888.compressed_for_ide()

# Cleanup: De-initialize applications and free memory
ocrdet_app.deinit()      # De-initialize OCR detection app
ocrrec_app.deinit()      # De-initialize OCR recognition app
gc.collect()             # Run garbage collection to free memory
