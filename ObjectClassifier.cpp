#include "ObjectClassifier.hpp"
#ifdef USE_TVM
#include "TVMRunner.hpp"
#endif
#ifdef USE_TENSOR_LITE
#include "TensorLiteRunner.hpp"
#endif

using namespace ::camera;
using namespace ::camera::camera_ml;
ObjectClassifier::ObjectClassifier(const std::string &modelPath, const std::string device)
{
#ifdef USE_TVM
    mModelInterface = std::make_unique<TVMRunner>(modelPath, device);
#endif
#ifdef USE_TENSOR_LITE
    mModelInterface = std::make_unique<TensorLiteRunner>(modelPath, device);
#endif
}
int ObjectClassifier::intializeObjectClassifier()
{
    return mModelInterface->initializeModelInterface();
}
DetectionOutput ObjectClassifier::RunObjectClassifier(uint8_t *inputFrame, int inputWidth, int inputHeight)
{
    return mModelInterface->runModelInterface(inputFrame);
}
TensorFormatSettings ObjectClassifier::getTensorPreprocessingParams()
{
    return mModelInterface->getTensorPreprocessingParams();
}
// Sorting using a lambda function
void ObjectClassifier::sortDetectionsByScore(std::vector<BoxPrediction> &detections)
{
    std::sort(detections.begin(), detections.end(),
              [](const BoxPrediction &a, const BoxPrediction &b)
              {
                  return a.confidence > b.confidence; // Sort in descending order
              });
}
// Find the detection with the highest score
const BoxPrediction &ObjectClassifier::findHighestScoredDetection(const std::vector<BoxPrediction> &detections)
{
    auto it = std::max_element(detections.begin(), detections.end(),
                               [](const BoxPrediction &a, const BoxPrediction &b)
                               {
                                   return a.confidence < b.confidence;
                               });
    return *it; // Return the detection with the highest score
}
