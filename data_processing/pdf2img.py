import os
from PIL import Image
import PyPDF2
from pdf2image import convert_from_path

def convert_pdf_to_image(pdf_file_path, output_folder, image_type='jpeg'):
    if not os.path.exists(output_folder):
        os.makedirs(output_folder)

    images = convert_from_path(pdf_file_path)
    
    for i, image in enumerate(images):
        image_path = os.path.join(output_folder, f'page_{i + 1}.{image_type}')
        image.save(image_path, image_type.upper())



if __name__ == '__main__':
    pdf_file = '/home/xdoestech/Pictures/sheetmusic/beethoven_s1_page30.pdf'  # Replace with your PDF file path
    output_dir = 'output_images'       # Replace with your desired output directory
    image_format = 'PNG'               # Replace with your desired image format, e.g., 'JPEG', 'PNG', etc.
    convert_pdf_to_image(pdf_file, output_dir, image_format)
