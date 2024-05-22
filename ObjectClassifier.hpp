#ifndef OBJECT_CLASSIFIER_HPP
#define OBJECT_CLASSIFIER_HPP
#include "ModelProcessor.hpp"
#include <memory>
#include <vector>
#include <string>

namespace camera
{
    namespace camera_ml
    {
        class ObjectClassifier
        {
        public:
            ObjectClassifier(const std::string &modelPath, const std::string device);
            int intializeObjectClassifier();
            DetectionOutput RunObjectClassifier(uint8_t *inputFrame, int inputWidth, int inputHeight);
            TensorFormatSettings getTensorPreprocessingParams();
            static void sortDetectionsByScore(std::vector<BoxPrediction> &detections);
            // Find the detection with the highest score
            static const BoxPrediction &findHighestScoredDetection(const std::vector<BoxPrediction> &detections);

        private:
            std::unique_ptr<ModelProcessor> mModelInterface;
        };
    }
}
#endif // OBJECT_CLASSIFIER_HPP