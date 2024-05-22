// TensorLiteRunner.hpp
#ifndef TENSOR_LITE_RUNNER_HPP
#define TENSOR_LITE_RUNNER_HPP

#include "ModelProcessor.hpp"
#include <tensorflow/lite/interpreter.h>
#include <tensorflow/lite/model.h>
#include <tensorflow/lite/kernels/register.h>
#include <chrono>
namespace camera
{
    namespace camera_ml
    {
        struct TensorInfo
        {
            int index;
            TfLiteType type;
            bool found;
            size_t size;
            TensorInfo() : index(-1), type(kTfLiteNoType), found(false) ,size(){}
        };
        class TensorLiteRunner : public ModelProcessor
        {
        public:
            TensorLiteRunner(const std::string &path, const std::string &device);
            int initializeModelInterface() override;
            DetectionOutput runModelInterface(uint8_t *inputFrame) override;

        private:
            std::unique_ptr<tflite::Interpreter> mInterpreter;
            std::unique_ptr<tflite::FlatBufferModel> mModel;
            uint8_t* mInputTensor;
            float* mOutputTensor;
            int r_module_load_ms{0};
            int Load(void);
            size_t GetTensorSize(tflite::Interpreter *interpreter, int tensor_index);
            TensorInfo GetTensorInfoByName(tflite::Interpreter* interpreter, const std::string& tensor_name);
            bool compare_confidence(const BoxPrediction &a, const BoxPrediction &b);
            float box_iou(const BoxPrediction &a, const BoxPrediction &b);
            std::vector<BoxPrediction> non_maximum_suppression(const std::vector<BoxPrediction>& boxes, float iou_threshold, float score_threshold);
            void PrintStats(void);
        };
    }
}
#endif // TENSOR_LITE_RUNNER_HPP
