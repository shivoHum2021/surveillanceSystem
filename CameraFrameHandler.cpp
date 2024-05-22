#include "CameraFrameHandler.hpp"
#define GET_MIN(a, b) ((a) < (b) ? (a) : (b))
#define GET_MAX(a, b) ((a > b) ? a : b)
using namespace ::camera;
using namespace ::camera::camera_ml;

CameraFrameHandler::CameraFrameHandler(u16 bufferId)
{
    mFrameReader = std::make_unique<FrameReader>(bufferId);
    mFrameConverter = std::make_unique<FrameConverter>();
}
frameInfoYUV *CameraFrameHandler::CaptureFrameFromCamera()
{
    return mFrameReader->readFrame();
}
std::shared_ptr<uint8_t[]> CameraFrameHandler::convertAndResize(uint8_t *raw, int width, int height, int newWidth, int newHeight, BoundingBox *unionBox)
{
    return mFrameConverter->convertAndResize(raw, width, height, newWidth, newHeight, unionBox);
}
/**
 * @brief Normalizes and resizes an image from YUV to RGB format.
 *
 * This function takes a YUV frame, converts it to RGB format, resizes it to the specified dimensions,
 * normalizes the pixel values to a [0, 1] range, then applies a quantization transformation to map
 * the normalized values back to a uint8 range according to a specific quantization formula.
 * The quantization is designed to prepare the image for processing by a machine learning model
 * that expects input data to be in a specific range and format.
 *
 * @param raw Pointer to the frameInfoYUV structure containing the YUV frame data and metadata.
 * @param outputWidth The desired width of the output image after resizing.
 * @param outputHeight The desired height of the output image after resizing.
 *
 * @return A pointer to the uint8_t array containing the processed image data. The caller
 *         is responsible for freeing this memory.
 *
 * @note The function assumes the input YUV format is NV12 and the output needs to be RGB.
 *       The caller must ensure that the input data format and desired output format are
 *       correctly specified and compatible with this function's processing.
 *
 * @exception std::bad_alloc If memory allocation for the output array fails.
 */
std::shared_ptr<uint8_t[]> CameraFrameHandler::normalizeAndResize(uint8_t *raw, int width, int height, NormalizationParams params, BoundingBox *unionBox)
{
    return mFrameConverter->normalizeAndResize(raw, width, height, params, unionBox);
}

std::shared_ptr<uint8_t[]> CameraFrameHandler::resizeNormalizeQuantize(uint8_t *raw, int width, int height, NormalizationParams params, BoundingBox *unionBox)
{
    return mFrameConverter->resizeNormalizeQuantize(raw, width, height, params, unionBox);
}

ScalingParams CameraFrameHandler::convertAndStore(uint8_t *raw, int width, int height, int newWidth, int newHeight, const std::string &filePath, BoundingBox *unionBox)
{
    return mFrameConverter->convertAndStore(raw, width, height, newWidth, newHeight, filePath, unionBox);
}

void CameraFrameHandler::saveBufferAsJpeg(uint8_t *buffer, int width, int height, const std::string &filePath)
{
    std::cerr << "saveBufferAsJpeg OK1" << " width=" << width << "height " << height << std::endl;

    cv::Mat yuvImage(height + height / 2, width, CV_8UC1, buffer); // Assuming YUV420p
    cv::Mat convertedImage = cv::Mat(width, height, CV_8UC4);
    cv::cvtColor(yuvImage, convertedImage, cv::COLOR_YUV2BGR_NV12); // Proper YUV to BGR conversion

    // Write the image to a file
    std::vector<int> compression_params;
    compression_params.push_back(cv::IMWRITE_JPEG_QUALITY);
    compression_params.push_back(95); // Adjust the quality as needed
    if (!cv::imwrite(filePath, convertedImage, compression_params))
    {
        std::cerr << "Failed to write image to " << filePath << std::endl;
    }
    else
    {
        std::cout << "Image saved successfully to " << filePath << std::endl;
    }
}

void CameraFrameHandler::saveRGBBufferAsJPEG(const uint8_t *buffer, int width, int height, const std::string &filename)
{
    cv::Mat img(height, width, CV_8UC3, const_cast<uint8_t*>buffer); // Create a Mat object from the buffer

    if (!cv::imwrite(filename, img))
    {
        std::cerr << "Failed to save image to " << filename << std::endl;
    }
    else
    {
        std::cout << "Image saved to " << filename << std::endl;
    }
}
/**
 * @brief Calculates the actual centroid of a given bounding rectangle.
 *
 * This method computes the centroid (center point) of a given bounding rectangle, adjusting the coordinates
 * according to a specified factor and adding some height padding.
 *
 * @param boundRect The bounding rectangle for which the centroid is calculated.
 * @return cv::Point2f The calculated centroid point.
 */
cv::Point2f CameraFrameHandler::getActualCentroid(cv::Rect boundRect)
{
    cv::Point2f pts;

    // Calculate the centroid of the bounding rectangle
    float xPoint = boundRect.x + boundRect.width / 2.0f;
    float yPoint = boundRect.y + boundRect.height / 2.0f;

    pts.x = xPoint;
    pts.y = yPoint;

    return pts;
}
/**
 * @brief Aligns the centroid of the given point within the original frame according to the crop size.
 *
 * This method adjusts the coordinates of the centroid (original center) to ensure it is properly aligned
 * within the bounds of the cropped area of the original frame. The adjustments are made to keep the centroid
 * within the frame dimensions while accounting for the specified crop size.
 *
 * @param orgCenter The original center point (centroid) to be aligned.
 * @param origFrame The original frame from which the crop is taken.
 * @param cropSize The size of the crop area.
 * @return cv::Point2f The adjusted centroid point after alignment.
 *
 * @note The method uses `GET_MAX` and `GET_MIN` macros to ensure the centroid remains within the valid range.
 */
cv::Point2f CameraFrameHandler::alignCentroid(cv::Point2f orgCenter, cv::Mat origFrame, cv::Size cropSize)
{
    cv::Point2f pts;

    // Calculate the necessary shifts to align the centroid
    float shiftX = (orgCenter.x + cropSize.width / 2) - origFrame.cols;
    float adjustedX = orgCenter.x - std::max(0.0f, shiftX);
    float shiftXleft = adjustedX - cropSize.width / 2;
    float adjustedXfinal = adjustedX - std::min(0.0f, shiftXleft);

    float shiftY = (orgCenter.y + cropSize.height / 2) - origFrame.rows;
    float adjustedY = orgCenter.y - std::max(0.0f, shiftY);
    float shiftYdown = adjustedY - cropSize.height / 2;
    float adjustedYfinal = adjustedY - std::min(0.0f, shiftYdown);

    // Set the final adjusted points
    pts.x = adjustedXfinal;
    pts.y = adjustedYfinal;

#if 0
    std::cout << "\n\n Original Center { " << orgCenter.x << ", " << orgCenter.y << " } " 
              << "Aligned Center: { " << adjustedXfinal << ", " << adjustedYfinal << " } \n";
    std::cout << " Cropping Resolution {W, H}: { " << cropSize.width << ", " << cropSize.height << " } " 
              << "Original frame Resolution {W, H}: { " << origFrame.cols << ", " << origFrame.rows << " } \n";
    std::cout << " Intermediate Adjustments {shiftX, adjustedX, shiftXleft}: { " << shiftX << ", " << adjustedX << ", " << shiftXleft << " } \n";
    std::cout << " Intermediate Adjustments {shiftY, adjustedY, shiftYdown}: { " << shiftY << ", " << adjustedY << ", " << shiftYdown << " } \n\n";
#endif

    return pts;
}
/**
 * @brief Calculates the relative bounding box within a cropped image.
 *
 * This method determines the relative bounding box coordinates within a cropped image based on the
 * original bounding rectangle, the crop size, and the aligned center point. It ensures that the bounding
 * box fits within the dimensions of the cropped image.
 *
 * @param boundRect The original bounding rectangle.
 * @param cropSize The size of the crop area.
 * @param alignedCenter The aligned center point within the cropped area.
 * @return cv::Rect The calculated relative bounding box.
 */

cv::Rect CameraFrameHandler::getRelativeBoundingBox(cv::Rect boundRect, cv::Size cropSize, cv::Point2f allignedCenter)
{
    cv::Rect newBBox; // to store the new bounding box co-ordinate
    // to find the relative x-ordinate and width of the bounding box in the smart thumbnail image
    if (boundRect.width >= cropSize.width)
    {
        newBBox.x = 0;
        newBBox.width = cropSize.width; // restrict the width of the relative bounding box to width of the final cropped image
    }
    else
    {
        float deltaX = allignedCenter.x - boundRect.x;
        newBBox.x = static_cast<int>(cropSize.width / 2 - deltaX);
        newBBox.width = boundRect.width;
    }
    // to find the relative y-ordinate and height of the bounding box in the smart thumbnail image
    if (boundRect.height >= cropSize.height)
    {
        newBBox.y = 0;
        newBBox.height = cropSize.height; // restrict the height of the relative bounding box to height of the final cropped image
    }
    else
    {
        float deltaY = allignedCenter.y - boundRect.y;
        newBBox.y = static_cast<int>(cropSize.height / 2 - deltaY);
        newBBox.height = boundRect.height;
    }
    return newBBox;
}
/**
 * @brief Calculates the crop size for a given bounding rectangle and scales the frame if necessary.
 *
 * This method determines the size of the crop for a given bounding rectangle within a frame,
 * adjusting the size of the frame if the bounding rectangle is larger than the specified width and height.
 * The resize scale is updated accordingly to ensure the bounding rectangle fits within the specified dimensions.
 *
 * @param boundRect The bounding rectangle for which the crop size is calculated.
 * @param w The target width for the crop.
 * @param h The target height for the crop.
 * @param resizeScale Pointer to a double where the resize scale will be stored.
 * @return cv::Size The calculated crop size.
 *
 * @note As per RDKC-10175, to crop the thumbnail from the frame, if the union blob size
 *       is greater than the thumbnail size, the frame is resized so that the union blob fits in the thumbnail.
 */
cv::Size CameraFrameHandler::getResizedCropSize(cv::Rect currentBox, int newWidth, int newHeight, double *scaleFactor)
{
    *scaleFactor = 1.0; // std::min(static_cast<double>(newWidth) / currentBox.width, static_cast<double>(newHeight) / currentBox.height);
    /*
     * As per RDKC-10175, to crop the thumbnail from the frame, where the union blob size is
     * greater than the thumbnail size, the frame resized so that the union blob fits in the
     * thumbnail.
     */
    if ((currentBox.width > newWidth) || (currentBox.height > newHeight))
    {
        // Calculate the resizing scale for the frame
        *scaleFactor = std::max(static_cast<double>(currentBox.width) / newWidth, static_cast<double>(currentBox.height) / newHeight);
    }
    return cv::Size(newWidth, newHeight);
}