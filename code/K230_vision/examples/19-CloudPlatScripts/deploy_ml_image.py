# -*- coding: utf-8 -*-
'''
Script: deploy_ml_image.py
脚本名称：deploy_ml_image.py

Description:
    This script performs metric learning inference on a single image.
    It loads an image, performs inference using a pre-trained Kmodel,
    and displays the results with the corresponding label.

    The model configuration is retrieved from the Canaan online training platform via a JSON config file. Please prepare the test image yourself.

脚本说明：
    本脚本对单张图像执行度量学习推理。它加载图像，使用预训练的 Kmodel 进行推理，并显示对应标签的结果。

    模型配置文件通过 Canaan 在线训练平台从 JSON 文件获取。 请自行准备测试图片。

Author: Canaan Developer
作者：Canaan 开发者
'''


import os, gc
from libs.PlatTasks import MetricLearningApp
from libs.PipeLine import PipeLine
from libs.Utils import *

# Load image in CHW format for inference and RGB888 for display
# The image dimensions are used for setting the input size for the inference model
img_chw, img_rgb888 = read_image("/sdcard/test.jpg")
rgb888p_size = [img_chw.shape[2], img_chw.shape[1]]

# Set root directory path for model and config
root_path = "/sdcard/mp_deployment_source/"

# Load deployment configuration
deploy_conf = read_json(root_path + "/deploy_config.json")
kmodel_path = root_path + deploy_conf["kmodel_path"]               # KModel path
confidence_threshold = deploy_conf["confidence_threshold"]         # Confidence threshold for classification
model_input_size = deploy_conf["img_size"]                         # Model input size

# Initialize metric learning application for image inference
inference_mode = "image"                                           # Inference mode: 'image'
debug_mode = 0                                                     # Debug mode flag
ml_app = MetricLearningApp(inference_mode, kmodel_path, model_input_size,
                           confidence_threshold, rgb888p_size, rgb888p_size, debug_mode=debug_mode)

# Configure preprocessing for the model
ml_app.config_preprocess()

# Load images and their corresponding category labels
ml_app.load_image("/sdcard/examples/ai_test_utils/0.jpg", "菠菜")                      # Load "Spinach" image
ml_app.load_image("/sdcard/examples/ai_test_utils/1.jpg", "菠菜")                      # Load another "Spinach" image
ml_app.load_image("/sdcard/examples/ai_test_utils/4.jpg", "长茄子")                    # Load "Eggplant" image
ml_app.load_image("/sdcard/examples/ai_test_utils/5.jpg", "长茄子")                    # Load another "Eggplant" image
ml_app.load_image("/sdcard/examples/ai_test_utils/6.jpg", "胡萝卜")                    # Load "Carrot" image
ml_app.load_image("/sdcard/examples/ai_test_utils/7.jpg", "胡萝卜")                    # Load another "Carrot" image

# Run inference on the loaded image
res = ml_app.run(img_chw)                                           # Run inference on the image

# Draw the classification result on the image
ml_app.draw_result(img_rgb888, res)                                 # Draw results on image

# Prepare image for IDE display
img_rgb888.compressed_for_ide()                                     # Prepare image for IDE view

# Cleanup: De-initialize metric learning app and Run garbage collection to free memory
ml_app.deinit()
gc.collect()
