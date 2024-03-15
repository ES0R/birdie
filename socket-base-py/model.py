import tflite_runtime.interpreter as tflite
# import cv2
import numpy as np


class Model:
    def __init__(self, model_path):
        self.interpreter = tflite.Interpreter(model_path=model_path)
        self.interpreter.allocate_tensors()

        # Get input and output tensors.
        self.input_details = self.interpreter.get_input_details()
        self.output_details = self.interpreter.get_output_details()


        # interpreter = tflite.Interpreter(model_path='/content/yolov8n-seg_int8.tflite')

    def run_inference(self, image, process_output=False):
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
        postprocessed_output = self.postprocess_output(bbox_tensor, mask_tensor) if self.postprocess_output else None
        

        if process_output != None:
            return postprocessed_output
        else:
            return bbox_tensor, mask_tensor
    
    def preprocess_image(self, image):
        # Preprocess the image here (e.g., resize, normalize, etc.)
        # Return the preprocessed image as a numpy array
        pass
    
    def postprocess_output(self, bbox_tensor, mask_tensor):
        # Postprocess the output here (e.g., decode, filter, etc.)
        # Return the postprocessed output
        pass