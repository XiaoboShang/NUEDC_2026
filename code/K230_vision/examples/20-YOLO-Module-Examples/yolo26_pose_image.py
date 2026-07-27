from libs.YOLO import YOLO26
from libs.Utils import *
import os,sys,gc
import ulab.numpy as np
import image

if __name__=="__main__":
    # 这里仅为示例，自定义场景请修改为您自己的测试图片、模型路径、标签名称、模型输入大小,关键点数量和关键点维度
    img_path="/sdcard/examples/utils/test_pose.jpg"
    kmodel_path="/sdcard/examples/kmodel/yolo26n-pose.kmodel"
    labels = ['person']
    model_input_size=[320,320]
    kp_num=17
    kp_dim=3

    confidence_threshold = 0.5
    nms_threshold=0.45
    img,img_ori=read_image(img_path)
    rgb888p_size=[img.shape[2],img.shape[1]]
    # 初始化 YOLO26 模型
    yolo=YOLO26(task_type="pose",mode="image",kmodel_path=kmodel_path,labels=labels,rgb888p_size=rgb888p_size,model_input_size=model_input_size,kp_num=kp_num,kp_dim=kp_dim,conf_thresh=confidence_threshold,max_boxes_num=100,debug_mode=0)
    yolo.config_preprocess()
    res=yolo.run(img)
    print(res)
    yolo.draw_result(res,img_ori)
    yolo.deinit()
    gc.collect()
