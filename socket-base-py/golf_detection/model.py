import tflite_runtime.interpreter as tflite
import cv2
import numpy as np


class Model:
    def __init__(self, model_path):
        self.interpreter = tflite.Interpreter(model_path=model_path)
        self.interpreter.allocate_tensors()

        # Get input and output tensors.
        self.input_details = self.interpreter.get_input_details()
        self.output_details = self.interpreter.get_output_details()


        # interpreter = tflite.Interpreter(model_path='/content/yolov8n-seg_int8.tflite')

    def run_inference(self, image, process_output=True):
        # Preprocess the image
        # preprocessed_image = self.preprocess_image(image)
        # self.interpreter.set_tensor(self.input_details[0]['index'], preprocessed_image)

        processed_image = self.preprocess_image(image)

        # set input tensor
        # print(processed_image.shape)
        self.interpreter.set_tensor(self.input_details[0]['index'], processed_image.astype(np.float32)) #TODO: investigate np.float32
        
        self.interpreter.invoke()

        # Get the output tensors
        bbox_tensor = self.interpreter.get_tensor(self.output_details[0]['index'])
        # mask_tensor = self.interpreter.get_tensor(self.output_details[1]['index'])
                    
        # Postprocess the output
        bboxes, scores = self.postprocess_output(bbox_tensor)

        # scale bboxes to image size
        scaled_bboxes = bboxes * processed_image.shape[1]
        
        return scaled_bboxes, scores
    
    def preprocess_image(self, image):
        # Normalizes the image and adds a batch dimension
        # print(image.shape)
        reshaped_img = cv2.resize(image, (640, 640))
        # print(reshaped_img.shape)
        reshaped_img = np.expand_dims(reshaped_img, axis=0)
        reshaped_img = reshaped_img / 255.0
        # print(reshaped_img.shape)
        return reshaped_img
    
    def postprocess_output(self, bbox_tensor):
        # Convert center coordinates to corner coordinates
        boxes = []
        scores = []
        for idx in range(bbox_tensor.shape[2]):
            xc, yc, width, height = bbox_tensor[0, 0:4, idx]
            xmin = xc - width / 2
            ymin = yc - height / 2
            xmax = xc + width / 2
            ymax = yc + height / 2
            boxes.append([xmin, ymin, xmax, ymax])
            
            # Assuming the maximum class confidence is the score
            class_confidences = bbox_tensor[0, 4, idx]
            score = np.max(class_confidences)
            scores.append(score)

        boxes = np.array(boxes)
        scores = np.array(scores)
        
        nms_bboxes, nms_scores = self.nms(boxes,scores)
        return nms_bboxes, nms_scores

    def nms(self, boxes, scores):
        # Apply NMS
        indices = cv2.dnn.NMSBoxes(boxes.tolist(), scores.tolist(), score_threshold=0.5, nms_threshold=0.6)

        # Filter the boxes based on NMS
        nms_boxes = boxes[indices]
        nms_scores = scores[indices]

        return nms_boxes, nms_scores