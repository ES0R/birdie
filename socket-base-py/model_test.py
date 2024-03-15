from model import Model
import cv2
import numpy as np


# Load the model
model_path = 'models/yolov8n-seg_int8.tflite'
model = Model(model_path)

# Load the image
image_path = 'golf_01.png'
image = cv2.imread(image_path)

# reshape image 
resized_img = cv2.resize(image, (640, 640))
reshaped_img = np.expand_dims(resized_img, axis=0)
# reshaped_img = reshaped_img / 255.0

# Run inference
# bbox_tensor, mask_tensor = model.run_inference(reshaped_img)
out = model.run_inference(reshaped_img)
print(out)

# print(bbox_tensor.shape)
# print(mask_tensor.shape)

