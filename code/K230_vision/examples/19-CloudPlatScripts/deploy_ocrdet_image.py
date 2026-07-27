# -*- coding: utf-8 -*-
'''
Script: deploy_ocrdet_image.py
脚本名称：deploy_ocrdet_image.py

Description:
    This script performs OCR text region detection on a single image.
    It loads an image, performs inference using a pre-trained Kmodel,
    and displays the detected text bounding boxes.

    The model configuration is retrieved from the Canaan online training platform via a JSON config file. Please prepare the test image yourself.

脚本说明：
    本脚本对单张图像执行 OCR 文本区域检测。它加载图像，使用预训练的 Kmodel 进行推理，并显示检测到的文本边界框。

    模型配置文件通过 Canaan 在线训练平台从 JSON 文件获取。请自行准备测试图片。

Author: Canaan Developer
作者：Canaan 开发者
'''


import os, gc
from libs.PlatTasks import OCRDetectionApp
from libs.Utils import *

# Load image in CHW format for inference and RGB888 for display
# The image dimensions are used for setting the input size for the inference model
img_chw, img_rgb888 = read_image("/sdcard/test.jpg")
rgb888p_size = [img_chw.shape[2], img_chw.shape[1]]

# Set root directory path for model and config
root_path = "/sdcard/mp_deployment_source/"

# Load deployment configuration
deploy_conf = read_json(root_path + "/deploy_config.json")
kmodel_path = root_path + deploy_conf["kmodel_path"]              # KModel path
mask_threshold = deploy_conf["mask_threshold"]                    # Mask threshold for binarizing text area
box_threshold = deploy_conf["box_threshold"]                      # Box confidence threshold for detection
model_input_size = deploy_conf["img_size"]                        # Model input size

# Initialize OCR detection application for image inference
inference_mode = "image"                                          # Inference mode: 'image'
debug_mode = 0                                                    # Debug mode flag
ocrdet_app = OCRDetectionApp(inference_mode, kmodel_path, model_input_size, mask_threshold, box_threshold, rgb888p_size, rgb888p_size, debug_mode=debug_mode)

# Configure preprocessing for the model
ocrdet_app.config_preprocess()

# Run inference on the loaded image
res = ocrdet_app.run(img_chw)

# Draw OCR detection results (e.g. bounding boxes) on the image
ocrdet_app.draw_result(img_rgb888, res)

# Prepare image for IDE display
img_rgb888.compress_for_ide()

# Cleanup: De-initialize OCR detection app and Run garbage collection to free memory
ocrdet_app.deinit()
gc.collect()

