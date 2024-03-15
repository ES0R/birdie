from model import Model
import cv2
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.patches as patches


# Load the model
model_path = 'models/yolov8n-seg_int8.tflite'
model = Model(model_path)

# Load the image
image_path = 'golf_01.png'
image = cv2.imread(image_path)

# reshape image 
resized_img = cv2.resize(image, (640, 640))
reshaped_img = np.expand_dims(resized_img, axis=0)
reshaped_img = reshaped_img / 255.0

# Run inference
# bbox_tensor, mask_tensor = model.run_inference(reshaped_img)
bboxes, scores,classes = model.run_inference(reshaped_img)
# print(bboxes.shape)

# Plotting
fig, ax = plt.subplots()

img_rgb = cv2.cvtColor(resized_img, cv2.COLOR_BGR2RGB)

# Display the image
# flipped_img = cv2.flip(img_rgb, 1)
# ax.imshow(flipped_img)
ax.imshow(img_rgb)

for box,score,name in zip(bboxes,scores,classes):
    # print(box.shape)
    xmin, ymin, xmax, ymax = box*img_rgb.shape[0]
    rect = patches.Rectangle((xmin, ymin), xmax-xmin, ymax-ymin, linewidth=1, edgecolor='r', facecolor='none')
    ax.add_patch(rect)
    ax.text(xmin, ymin, f"{name}:{score:.2f}", color='white', backgroundcolor='red')
    
# save figure
plt.savefig('output_test.png')
plt.show()
