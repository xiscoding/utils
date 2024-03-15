% Specify the image file name
imageName = "C:/Users/axyzh/Documents/autodrive/bags_matlab/bag_images/frame_92.png"; % Replace with your image file name

% Read the image
originalImage = imread(imageName);

% Check if the image is color
if size(originalImage, 3) == 3
    % Split the image into its RGB channels
    R = originalImage(:,:,1);
    G = originalImage(:,:,2);
    B = originalImage(:,:,3);

    % Perform histogram equalization on each channel
    equalizedR = histeq(R);
    equalizedG = histeq(G);
    equalizedB = histeq(B);

    % Combine the equalized channels back into one image
    equalizedImage = cat(3, equalizedR, equalizedG, equalizedB);
else
    % If the image is already grayscale, just perform histogram equalization
    equalizedImage = histeq(originalImage);
end

% Save the resulting image
outputName = 'equalized_color_image.jpg'; % You can change the file name and format if needed
imwrite(equalizedImage, outputName);

% Display original and equalized images
subplot(1, 2, 1), imshow(originalImage), title('Original Image');
subplot(1, 2, 2), imshow(equalizedImage), title('Color Histogram Equalized Image');
