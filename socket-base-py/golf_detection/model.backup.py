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



        # set input tensor
        self.interpreter.set_tensor(self.input_details[0]['index'], image.astype(np.float32)) #TODO: investigate np.float32
        
        self.interpreter.invoke()

        # Get the output tensors
        bbox_tensor = self.interpreter.get_tensor(self.output_details[0]['index'])
        mask_tensor = self.interpreter.get_tensor(self.output_details[1]['index'])
                    
        # Postprocess the output
        bboxes, scores, classes = self.postprocess_output(bbox_tensor, mask_tensor)
        
        return bboxes, scores, classes
    
    def preprocess_image(self, image):
        # Preprocess the image here (e.g., resize, normalize, etc.)
        # Return the preprocessed image as a numpy array
        pass
    
    def postprocess_output(self, bbox_tensor, mask_tensor):
        # Convert center coordinates to corner coordinates
        boxes = []
        scores = []
        masks = []
        classes = []
        for idx in range(bbox_tensor.shape[2]):
            xc, yc, width, height = bbox_tensor[0, 0:4, idx]
            xmin = xc - width / 2
            ymin = yc - height / 2
            xmax = xc + width / 2
            ymax = yc + height / 2
            boxes.append([xmin, ymin, xmax, ymax])
            
            # Assuming the maximum class confidence is the score
            class_confidences = bbox_tensor[0, 4:84, idx]
            score = np.max(class_confidences)
            scores.append(score)

            score_idx = np.argmax(class_confidences)
            class_name = self.getclass(score_idx)
            classes.append(class_name)

            mask_coefficients = bbox_tensor[0,84:116,idx]
            masks.append(mask_coefficients)

        boxes = np.array(boxes)
        scores = np.array(scores)
        masks = np.array(masks)
        classes = np.array(classes)
        
        nms_bboxes, nms_scores, nms_classes = self.nms(boxes,scores,classes)
        return nms_bboxes, nms_scores, nms_classes

    def nms(self, boxes, scores, classes):
        # Apply NMS
        indices = cv2.dnn.NMSBoxes(boxes.tolist(), scores.tolist(), score_threshold=0.4, nms_threshold=0.4)

        # Filter the boxes based on NMS
        nms_boxes = boxes[indices[:]]
        nms_scores = scores[indices[:]]
        # nms_masks = masks[indices[:]]
        nms_classes = classes[indices[:]]

        return nms_boxes, nms_scores, nms_classes

    def getclass(self,idx):
        class_dict = {
            0: 'person',
            1: 'bicycle',
            2: 'car',
            3: 'motorcycle',
            4: 'airplane',
            5: 'bus',
            6: 'train',
            7: 'truck',
            8: 'boat',
            9: 'traffic light',
            10: 'fire hydrant',
            11: 'stop sign',
            12: 'parking meter',
            13: 'bench',
            14: 'bird',
            15: 'cat',
            16: 'dog',
            17: 'horse',
            18: 'sheep',
            19: 'cow',
            20: 'elephant',
            21: 'bear',
            22: 'zebra',
            23: 'giraffe',
            24: 'backpack',
            25: 'umbrella',
            26: 'handbag',
            27: 'tie',
            28: 'suitcase',
            29: 'frisbee',
            30: 'skis',
            31: 'snowboard',
            32: 'sports ball',
            33: 'kite',
            34: 'baseball bat',
            35: 'baseball glove',
            36: 'skateboard',
            37: 'surfboard',
            38: 'tennis racket',
            39: 'bottle',
            40: 'wine glass',
            41: 'cup',
            42: 'fork',
            43: 'knife',
            44: 'spoon',
            45: 'bowl',
            46: 'banana',
            47: 'apple',
            48: 'sandwich',
            49: 'orange',
            50: 'broccoli',
            51: 'carrot',
            52: 'hot dog',
            53: 'pizza',
            54: 'donut',
            55: 'cake',
            56: 'chair',
            57: 'couch',
            58: 'potted plant',
            59: 'bed',
            60: 'dining table',
            61: 'toilet',
            62: 'tv',
            63: 'laptop',
            64: 'mouse',
            65: 'remote',
            66: 'keyboard',
            67: 'cell phone',
            68: 'microwave',
            69: 'oven',
            70: 'toaster',
            71: 'sink',
            72: 'refrigerator',
            73: 'book',
            74: 'clock',
            75: 'vase',
            76: 'scissors',
            77: 'teddy bear',
            78: 'hair drier',
            79: 'toothbrush'}
        return class_dict[idx]