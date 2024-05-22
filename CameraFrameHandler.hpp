#ifndef __CAMERAFRAMEHANDLER_H__
#define __CAMERAFRAMEHANDLER_H__
#include <cstdint>
#include <iostream>
#include <vector>
#include <memory>
#include <mutex>
#include <algorithm> // For std::min and std::max
#include <opencv2/opencv.hpp>
#include "xStreamerConsumer.h"
#include "MotionEventMetadata.hpp"
#include "Logger.hpp"

namespace camera
{
    namespace camera_ml
    {
        typedef struct NormalizationParams
        {
            float scale;
            int zeroPoint;
            float uBound;
            float lBound;
            int inputWidth;
            int inputHeight;
            int noOfChannels;
            void print() const
            {
                std::cout << "Scale: " << scale << std::endl;
                std::cout << "Zero Point: " << zeroPoint << std::endl;
                std::cout << "Upper Bound: " << uBound << std::endl;
                std::cout << "Lower Bound: " << lBound << std::endl;
                std::cout << "Input Width: " << inputWidth << std::endl;
                std::cout << "Input Height: " << inputHeight << std::endl;
                std::cout << "Number of Channels: " << noOfChannels << std::endl;
            }
        } NormalizationParams;

        class CameraFrameHandler
        {
        public:
            CameraFrameHandler(u16 bufferId);
            frameInfoYUV *CaptureFrameFromCamera();
            std::shared_ptr<uint8_t[]> convertAndResize(uint8_t *raw, int width, int height, int newWidth, int newHeight, BoundingBox *unionBox = nullptr);
            std::shared_ptr<uint8_t[]> normalizeAndResize(uint8_t *raw, int width, int height, NormalizationParams params, BoundingBox *unionBox = nullptr);
            std::shared_ptr<uint8_t[]> resizeNormalizeQuantize(uint8_t *raw, int width, int height, NormalizationParams params, BoundingBox *unionBox = nullptr);
            ScalingParams convertAndStore(uint8_t *raw, int width, int height, int newWidth, int newHeight, const std::string &filePath, BoundingBox *unionBox = nullptr);
            void saveBufferAsJpeg(uint8_t *buffer, int width, int height, const std::string &filePath);
            void saveRGBBufferAsJPEG(const uint8_t *buffer, int width, int height, const std::string &filename);

        private:
            class FrameConverter
            {
            public:
                FrameConverter() {}
                // BoundingBox* box = &mMotionEventMetadata.deliveryUnionBox;
                std::shared_ptr<uint8_t[]> convertAndResize(uint8_t *raw, int width, int height, int newWidth, int newHeight, BoundingBox *unionBox)
                {
                    if (!raw || width <= 0 || height <= 0 || newWidth <= 0 || newHeight <= 0)
                    {
                        LOG_ERROR("Invalid input dimensions or raw data.");
                        return nullptr;
                    }
                    cv::Mat rgbFrame = convertYUVToRGB(raw, width, height);
                    if (rgbFrame.empty())
                    {
                        LOG_ERROR("Failed to convert YUV to RGB.");
                        return nullptr;
                    }
                    cv::Mat resizedFrame;
                    resizeFrame(rgbFrame, resizedFrame, newWidth, newHeight, unionBox);
                    if (resizedFrame.empty())
                    {
                        LOG_ERROR("Failed to resize the frame.");
                        return nullptr;
                    }
                    return allocateAndCopy(resizedFrame);
                }
                std::shared_ptr<uint8_t[]> normalizeAndResize(uint8_t *raw, int width, int height, NormalizationParams params, BoundingBox *unionBox)
                {
                    if (!raw || width <= 0 || height <= 0 || params.inputWidth <= 0 || params.inputHeight <= 0)
                    {
                        LOG_ERROR("Invalid input dimensions or raw data.");
                        return nullptr;
                    }
                    cv::Mat rgbFrame = convertYUVToRGB(raw, width, height);
                    if (rgbFrame.empty())
                    {
                        LOG_ERROR("Failed to convert YUV to RGB.");
                        return nullptr;
                    }
                    cv::Mat resizedFrame;
                    resizeFrame(rgbFrame, resizedFrame, params.inputWidth, params.inputHeight, unionBox);
                    if (resizedFrame.empty())
                    {
                        LOG_ERROR("Failed to resize the frame.");
                        return nullptr;
                    }
                    cv::Mat normalizedFrame;
                    normalizeFrame(resizedFrame, normalizedFrame);

                    return allocateAndCopy(normalizedFrame);
                }

                std::shared_ptr<uint8_t[]> resizeNormalizeQuantize(uint8_t *raw, int width, int height, NormalizationParams params, BoundingBox *unionBox)
                {
                    if (!raw || width <= 0 || height <= 0 || params.inputWidth <= 0 || params.inputHeight <= 0)
                    {
                        LOG_ERROR("Invalid input dimensions or raw data.");
                        return nullptr;
                    }
                    cv::Mat rgbFrame = convertYUVToRGB(raw, width, height);
                    if (rgbFrame.empty())
                    {
                        LOG_ERROR("Failed to convert YUV to RGB.");
                        return nullptr;
                    }
                    cv::Mat resizedFrame;
                    resizeFrame(rgbFrame, resizedFrame, params.inputWidth, params.inputHeight, unionBox);
                    if (resizedFrame.empty())
                    {
                        LOG_ERROR("Failed to resize the frame.");
                        return nullptr;
                    }
                    cv::Mat normalizedFrame;
                    normalizeFrame(resizedFrame, normalizedFrame);

                    cv::Mat quantizedFrame;
                    quantizeFrame(normalizedFrame, quantizedFrame, params);

                    return allocateAndCopy(quantizedFrame);
                }
                ScalingParams convertAndStore(uint8_t *raw, int width, int height, int newWidth, int newHeight, const std::string &filePath, BoundingBox *unionBox)
                {
                    ScalingParams params;
                    if (!raw || width <= 0 || height <= 0 || newWidth <= 0 || newHeight <= 0)
                    {
                        LOG_ERROR("Invalid input dimensions or raw data.");
                        return;
                    }
                    // cv::Mat rgbFrame = convertYUVToRGB(raw, width, height);
                    cv::Mat yuv(height + height / 2, width, CV_8UC1, raw);
                    cv::Mat rgbFrame(height, width, CV_8UC4);
                    cv::cvtColor(yuv, rgbFrame, cv::COLOR_YUV2BGR_NV12);

                    if (rgbFrame.empty())
                    {
                        LOG_ERROR("Failed to convert YUV to RGB.");
                        return;
                    }
                    cv::Mat resizedFrame;
                    params = resizeFrame(rgbFrame, resizedFrame, newWidth, newHeight, unionBox);
                    if (resizedFrame.empty())
                    {
                        LOG_ERROR("Failed to resize the frame.");
                        return;
                    }
                    std::vector<int> compression_params;
                    compression_params.push_back(cv::IMWRITE_JPEG_QUALITY);
                    compression_params.push_back(95); // Adjust the quality as needed
                    if (!cv::imwrite(filePath, resizedFrame, compression_params))
                    {
                        LOG_ERROR("Failed to save image to " << filePath);
                    }
                    else
                    {
                        LOG_INFO("Image saved with quality 95 to " << filePath);
                    }
                    yuv.release();
                    resizedFrame.release();
                    rgbFrame.release();
                    return params;
                }

            private:
                cv::Mat convertYUVToRGB(uint8_t *raw, int width, int height)
                {
                    cv::Mat yuv(height + height / 2, width, CV_8UC1, raw);
                    cv::Mat rgb(height, width, CV_8UC3);
                    cv::cvtColor(yuv, rgb, cv::COLOR_YUV2RGB_NV12);
                    yuv.release();
                    return rgb;
                }

                ScalingParams resizeFrame(cv::Mat &inputFrame, cv::Mat &outputFrame, int newWidth, int newHeight, BoundingBox *unionBox)
                {
                    try
                    {
                        ScalingParams params;
                        if (unionBox && !unionBox->isEmpty())
                        {
                            double scaleFactor = 1.0;
                            cv::Rect currentBox(unionBox->boundingBoxXOrd, unionBox->boundingBoxYOrd, unionBox->boundingBoxWidth, unionBox->boundingBoxHeight);

                            cv::Size cropSize = CameraFrameHandler::getResizedCropSize(currentBox, newWidth, newHeight, &scaleFactor);
                            LOG_INFO("Resizing scale for the thumbnail is ");
                            if (scaleFactor != 1.0)
                            {
                                cv::Size rescaleSize(static_cast<int>(inputFrame.cols / scaleFactor), static_cast<int>(inputFrame.rows / scaleFactor));
                                cv::resize(inputFrame, inputFrame, rescaleSize);

                                // Resize the union blob with scaleFactor
                                currentBox.width = static_cast<int>(unionBox->boundingBoxWidth / scaleFactor);
                                currentBox.height = static_cast<int>(unionBox->boundingBoxHeight / scaleFactor);
                                unionBox.x = static_cast<int>(unionBox->boundingBoxXOrd / scaleFactor);
                                unionBox.y = static_cast<int>(unionBox->boundingBoxYOrd / scaleFactor);
                            }
                            cv::Point2f orgCenter = CameraFrameHandler::getActualCentroid(currentBox);
                            cv::Point2f alignedCenter = CameraFrameHandler::alignCentroid(orgCenter, inputFrame, cropSize);
                            cv::getRectSubPix(inputFrame, cropSize, alignedCenter, outputFrame);
                            CropSize size(cropSize.width, cropSize.height);
                            Point center(alignedCenter.x, alignedCenter.y);
                            params.scaleFactor = scaleFactor;
                            params.size = size;
                            params.point2f = center;
                        }
                        else
                        {
                            cv::resize(inputFrame, outputFrame, cv::Size(newWidth, newHeight));
                        }
                    }
                    catch (const cv::Exception &e)
                    {
                        LOG_ERROR("OpenCV error: " << e.what());
                        outputFrame = cv::Mat(); // Return an empty matrix in case of an error
                    }
                    catch (const std::exception &e)
                    {
                        LOG_ERROR("Standard error: " << e.what());
                        outputFrame = cv::Mat(); // Return an empty matrix in case of an error
                    }
                    catch (...)
                    {
                        LOG_ERROR("Unknown error occurred");
                        outputFrame = cv::Mat(); // Return an empty matrix in case of an error
                    }
                    return params;
                }

                void normalizeFrame(cv::Mat &inputFrame, cv::Mat &outputFrame)
                {
                    inputFrame.convertTo(outputFrame, CV_32FC3, 1.0 / 255);
                }

                void quantizeFrame(cv::Mat &inputFrame, cv::Mat &outputFrame, NormalizationParams params)
                {

                    outputFrame.create(inputFrame.size(), CV_8UC3);
                    for (int i = 0; i < inputFrame.rows; ++i)
                    {
                        for (int j = 0; j < inputFrame.cols; ++j)
                        {
                            for (int c = 0; c < 3; ++c)
                            {
                                float pixel = inputFrame.at<cv::Vec3f>(i, j)[c];
                                float transformed = pixel * 1.9921875 - 1.0;
                                uint8_t quantized = static_cast<uint8_t>(std::max(0.0f, std::min(255.0f, static_cast<float>(128.0 + transformed / 0.0078125))));
                                outputFrame.at<cv::Vec3b>(i, j)[c] = quantized;
                            }
                        }
                    }
                }

                std::shared_ptr<uint8_t[]> allocateAndCopy(cv::Mat &frame)
                {
                    size_t numBytes = frame.total() * frame.elemSize();
                    if (numBytes == 0)
                    {
                        LOG_ERROR("Error: No data in output frame.");
                        return nullptr;
                    }

                    std::shared_ptr<uint8_t[]> output(new (std::nothrow) uint8_t[numBytes], std::default_delete<uint8_t[]>());
                    if (!output)
                    {
                        LOG_ERROR("Error: Memory allocation failed for output buffer.");
                        return nullptr;
                    }

                    memcpy(output.get(), frame.data, numBytes);
                    return output;
                }
            };

            class FrameReader
            {
            public:
                FrameReader(u16 bufferId) : mBufferId(bufferId)
                {
                    mConsumer = std::make_unique<XStreamerConsumer>();
                    if (!mConsumer)
                    {
                        LOG_ERROR("Error:creating instance of xstreamerConsumer.");
                        return;
                    }
                    if (0 != mConsumer->RAWInit(bufferId))
                    {
                        LOG_ERROR("Failed to initialize resources for reading raw frames.");
                        return;
                    }
                    mFrameContainer = mConsumer->GetRAWFrameContainer();
                    if (!mFrameContainer)
                    {
                        LOG_ERROR("Failed to create applicaton buffer");
                        return;
                    }
                }

                frameInfoYUV *readFrame()
                {
                    mConsumer->ReadRAWFrame(mBufferId, (u16)FORMAT_YUV, mFrameContainer);
                    return mFrameContainer;
                }

            private:
                u16 mBufferId;
                std::unique_ptr<XStreamerConsumer> mConsumer;
                frameInfoYUV *mFrameContainer;
            };
            u16 mBufferId;
            std::mutex mResourceMutex;
            std::unique_ptr<FrameConverter> mFrameConverter;
            std::unique_ptr<FrameReader> mFrameReader;
            static cv::Point2f getActualCentroid(cv::Rect boundRect);
            static cv::Point2f alignCentroid(cv::Point2f orgCenter, cv::Mat origFrame, cv::Size cropSize);
            static cv::Rect getRelativeBoundingBox(cv::Rect boundRect, cv::Size cropSize, cv::Point2f allignedCenter);
            static cv::Size getResizedCropSize(cv::Rect boundRect, int w, int h, double *resizeScale);
        };
    }
}
#endif // __CAMERAFRAMEHANDLER_H__
