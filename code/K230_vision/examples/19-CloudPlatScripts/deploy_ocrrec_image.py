# -*- coding: utf-8 -*-
'''
Script: deploy_ocrrec_image.py
脚本名称：deploy_ocrrec_image.py

Description:
    This script performs optical character recognition (OCR) on a single image.
    It loads an image, performs inference using a pre-trained Kmodel,
    and displays the recognized text results.

    The model configuration is retrieved from the Canaan online training platform via a JSON config file. Please prepare the test image yourself.

    Note: OCR recognition is not suitable for video stream inference. This script is intended for single image inference only.

脚本说明：
    本脚本对单张图像执行光学字符识别（OCR）。它加载图像，使用预训练的 Kmodel 进行推理，并显示识别出的文本结果。

    模型配置文件通过 Canaan 在线训练平台从 JSON 文件获取。 请自行准备测试图片。

    注意：OCR 识别不适用于视频流推理，此脚本仅用于单图推理。

Author: Canaan Developer
作者：Canaan 开发者
'''


import os, gc
from libs.PlatTasks import OCRRecognitionApp
from libs.Utils import *

# Load image in CHW format for inference and RGB888 for display
# The image dimensions are used for setting the input size for the inference model
img_chw, img_rgb888 = read_image("/sdcard/test.jpg")
rgb888p_size = [img_chw.shape[2], img_chw.shape[1]]

# Set root directory path for model, config, and dictionary
root_path = "/sdcard/mp_deployment_source/"

# Load deployment configuration
deploy_conf = read_json(root_path + "/deploy_config.json")
kmodel_path = root_path + deploy_conf["kmodel_path"]              # KModel path

# Load character dictionary for OCR decoding
dict_path = root_path + "dict.txt"
with open(dict_path, 'r') as file:
    line_one = file.read(100000)
    line_list = line_one.split("\n")
DICT = {num: char.replace("\r", "").replace("\n", "") for num, char in enumerate(line_list)}  # Character map

# Extract model input size from configuration
model_input_size = deploy_conf["img_size"]

# Initialize OCR recognition application for image inference
inference_mode = "image"                                           # Inference mode: 'image'
debug_mode = 0                                                     # Debug mode flag
ocrrec_app = OCRRecognitionApp(inference_mode, kmodel_path, model_input_size, DICT, rgb888p_size, rgb888p_size, debug_mode=debug_mode)

# Configure preprocessing for the model
ocrrec_app.config_preprocess()

# Run inference on the loaded image
res = ocrrec_app.run(img_chw)

# Print OCR recognition results on the image
ocrrec_app.print_result(img_rgb888, res)

# Prepare image for IDE display
img_rgb888.compress_for_ide()

# Cleanup: De-initialize OCR recognition app and Run garbage collection to free memory
ocrrec_app.deinit()
gc.collect()

