import cv2
import cv2.aruco as aruco
import matplotlib.pyplot as plt
import matplotlib.patches as patches
import numpy as np


def detect_aruco_and_scan(image_path):
    # Load the image
    image = cv2.imread(image_path)
    
    # Convert to grayscale
    gray = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)
    
    # Initialize the detector parameters using default values
    aruco_dict = aruco.getPredefinedDictionary(aruco.DICT_4X4_250)
    
    # For versions that do not recognize DetectorParameters_create, use DetectorParameters
    parameters = aruco.DetectorParameters()

    detector = aruco.ArucoDetector(aruco_dict, parameters)

    corners, ids, rejectedImgPoints = detector.detectMarkers(gray)

    # Detect the markers in the image
    # corners, ids, rejectedImgPoints = aruco.detectMarkers(gray, aruco_dict, parameters=parameters)
    
    fig, ax = plt.subplots(1, figsize=(12, 8))
    ax.imshow(image)
    
    if ids is not None:
        print(f"Detected ArUco marker IDs: {ids.flatten()}")
        print(f'ArUco marker corners: {corners}')
        for corner in corners:
            # corner is a list with a single array of four corners
            points = corner[0]
            # Extract the top-left, top-right, bottom-right, and bottom-left points
            tl, tr, br, bl = points
            # Calculate the width and height of the rectangle
            width = np.linalg.norm(tr - tl)
            height = np.linalg.norm(tr - br)
            # Calculate the angle of the rectangle
            angle = np.arctan2(tr[1] - tl[1], tr[0] - tl[0])
            angle = np.degrees(angle)
            # Create a rectangle patch
            rect = patches.Rectangle((tl[0], tl[1]), width, height, angle=angle, linewidth=1, edgecolor='r', facecolor='none')
            ax.add_patch(rect)
            print(tl,tr,br,bl)


    else:
        print("No ArUco markers detected.")
    
    plt.savefig('detected_aruco_markers.jpg')
    plt.show()


# Example usage
image_path = 'arcu_image.jpg'
# image_path = 'makers.jpg'
detect_aruco_and_scan(image_path)
