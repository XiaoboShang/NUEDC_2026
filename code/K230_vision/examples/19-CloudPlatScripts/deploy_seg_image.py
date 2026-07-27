# -*- coding: utf-8 -*-
'''
Script: deploy_seg_image.py
脚本名称：deploy_seg_image.py

Description:
    This script performs semantic segmentation on a single image.
    It loads an image, performs inference using a pre-trained Kmodel,
    and displays the segmentation result as a mask.

    The model configuration is loaded from the Canaan online training platform via a JSON config file.  Please prepare the test image yourself.

脚本说明：
    本脚本对单张图像执行语义分割。它加载图像，使用预训练的 Kmodel 进行推理，并将分割结果显示为掩码。

    模型配置文件通过 Canaan 在线训练平台从 JSON 文件加载。 请自行准备测试图片。

Author: Canaan Developer
作者：Canaan 开发者
'''


import os, gc
from libs.PlatTasks import SegmentationApp
from libs.Utils import *

# Load image in CHW format for inference and RGB888 for display
# The image dimensions are used to set up model input and display size
img_chw, img_rgb888 = read_image("/sdcard/test.jpg")
rgb888p_size = [img_chw.shape[2], img_chw.shape[1]]

# Set root directory path for model and config
root_path = "/sdcard/mp_deployment_source/"

# Load deployment configuration
deploy_conf = read_json(root_path + "/deploy_config.json")
kmodel_path = root_path + deploy_conf["kmodel_path"]              # KModel path
labels = deploy_conf["categories"]                                # Label list
model_input_size = deploy_conf["img_size"]                        # Model input size

# Inference configuration
inference_mode = "image"                                          # Inference mode: 'image'
debug_mode = 0                                                    # Debug mode flag

# Initialize semantic segmentation application
seg_app = SegmentationApp(inference_mode,kmodel_path,labels,model_input_size,rgb888p_size,rgb888p_size,debug_mode=debug_mode)

# Configure preprocessing for the model
seg_app.config_preprocess()

# Run inference on the loaded image
res = seg_app.run(img_chw)

# Prepare segmentation mask for IDE display
res["mask"].compress_for_ide()

# Cleanup: De-initialize segmentation app and Run garbage collection to free memory
seg_app.deinit()
gc.collect()

