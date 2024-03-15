import cv2

camera_stream_url = "http://10.197.216.166:8081"

# Sometimes, it might be necessary to pass in additional options to OpenCV's VideoCapture
# For instance, forcing the use of MJPEG if that's what the stream is using.
cap = cv2.VideoCapture(camera_stream_url, cv2.CAP_FFMPEG)

# If that doesn't work, you can try forcing the use of MJPEG:
#cap = cv2.VideoCapture(camera_stream_url + "/mjpeg", cv2.CAP_FFMPEG)

if not cap.isOpened():
    print("Cannot open camera stream.")
    exit()

ret, frame = cap.read()
if ret:
    image_path = 'captured_image_2.jpg'
    cv2.imwrite(image_path, frame)
    print(f"Image captured and saved at {image_path}")
else:
    print("Failed to capture image from the stream.")

cap.release()
