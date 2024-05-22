#ifndef MODEL_PROCESSOR_HPP
#define MODEL_PROCESSOR_HPP
#include <vector>
#include <string>
#include <algorithm>
#include <iostream>
#include "Logger.hpp"

namespace camera
{
    namespace camera_ml
    {
        /**
         * @enum ObjectType
         * @brief Enum representing different types of objects.
         */

        typedef enum
        {
            UNKNOWN = 0,
            PERSON = 1,
            DELIVERY = 2,
            MAX_ENUM = 2,
        } ObjectType;

        typedef enum
        {
            CLASS_UNKNOWN = 0,
            CLASS_PERSON = 1,
            CLASS_DELIVERY = 2,
            CLASS_MAX_ENUM = 2,
        } ObjectClass;
        /**
         * @struct BoxPrediction
         * @brief Structure to represent a prediction bounding box.
        */
       
        struct BoxPrediction
        {
            float y_min;         /**< Minimum y-coordinate of the bounding box. */
            float x_min;         /**< Minimum x-coordinate of the bounding box. */
            float y_max;         /**< Maximum y-coordinate of the bounding box. */
            float x_max;         /**< Maximum x-coordinate of the bounding box. */
            float confidence;    /**< Confidence score of the prediction. */
            ObjectType class_id; /**< Class ID of the detected object. */
        };
        typedef struct TensorFormatSettings
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
        } TensorFormatSettings;

        typedef struct DetectionOutput
        {
            uint32_t noOfBoxes;
            std::vector<BoxPrediction> predictions;

        } DetectionOutput;

        class ModelProcessor
        {
        protected:
            std::string mModelPath;
            std::string mDevice;
            bool isRunWasCalled;
            TensorFormatSettings mTensorFormatSettings;

        public:
            ModelProcessor(const std::string &path, const std::string &device) : mModelPath(path), mDevice(device), mTensorFormatSettings()
            {
            }
            virtual ~ModelProcessor() = default;

            virtual int initializeModelInterface() = 0;
            virtual DetectionOutput runModelInterface(uint8_t *inputFrame) = 0;
            TensorFormatSettings getTensorPreprocessingParams()
            {
                return mTensorFormatSettings;
            }
        };
    }
}
#endif // MODEL_PROCESSOR_HPP