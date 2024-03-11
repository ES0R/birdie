


from PIL import Image
import os 
import matplotlib.pyplot as plt

class ImageLoader:
    def __init__(self, image_path):
        self.image_path = image_path
        self.image = None

    def load_image(self):
        """Load the image from the specified path."""
        try:
            self.image = Image.open(self.image_path)
            print(f"Image loaded successfully: {self.image_path}")
        except IOError:
            print(f"Failed to load image: {self.image_path}")

    def show_image(self):
        """Display the loaded image."""
        if self.image is not None:
            self.image.show()
        else:
            print("No image loaded to display.")

    def plot_and_save_image(self, save_path):
        """Plot the image using matplotlib and save the plot."""
        if self.image is not None:
            plt.imshow(self.image)
            plt.axis('off')  # Hide axis
            plt.title('Loaded Image')
            plt.savefig(save_path)
            plt.show()
            print(f"Image plot saved to: {save_path}")
        else:
            print("No image loaded to plot and save.")

# Usage
if __name__ == "__main__":
    print(f"path: {os.getcwd()}")
    image_loader = ImageLoader("golf_01.png")  # Specify the correct path to your image
    image_loader.load_image()
    image_loader.show_image()
    image_loader.plot_and_save_image("plotted_image.png")  # Saves the plot in the current working directory


