#include "TensorLiteRunner.hpp"
#include "tensorflow/lite/optional_debug_tools.h"
#include <iostream>
namespace camera
{
    namespace camera_ml
    {
        /// @brief
        /// @param path
        /// @param device
        TensorLiteRunner::TensorLiteRunner(const std::string &path, const std::string &device) : ModelProcessor(path, device)
        {
            mInputTensor = nullptr;
        }
        int TensorLiteRunner::initializeModelInterface()
        {
            auto tstart = std::chrono::high_resolution_clock::now();
            if (Load() == 0)
            {
                auto tend = std::chrono::high_resolution_clock::now();
                r_module_load_ms = static_cast<double>((tend - tstart).count()) / 1e6;
                // tflite::PrintInterpreterState(mInterpreter.get(), 32);
                PrintStats();
            }
            return 0;
        }

        DetectionOutput TensorLiteRunner::runModelInterface(uint8_t *inputFrame)
        {
            // Simulated processing
            DetectionOutput results;
            tflite::Interpreter *interpreter = mInterpreter.get();
            if (nullptr != interpreter)
            {
                if (mInputTensor == nullptr)
                {
                    LOG_ERROR("Failed to access input tensor.");
                    return results;
                }
                // printf("modelInput runModelInterface(%p)\n", inputFrame);
                size_t expectedSize = mTensorFormatSettings.inputWidth * mTensorFormatSettings.inputHeight * mTensorFormatSettings.noOfChannels;
                //size_t expectedSize = 224 * 224 * 3;
                std::memset(mInputTensor, 0, expectedSize);
                std::memcpy(mInputTensor, inputFrame, expectedSize);
                // Run inference
                if (interpreter->Invoke() != kTfLiteOk)
                {
                    LOG_ERROR("Failed to invoke TensorFlow Lite interpreter");
                    return results;
                }

                int num_outputs = interpreter->outputs().size();
                LOG_INFO("num_outputs=" << num_outputs);

                if (4 == num_outputs)
                {
                    float *bboxes = interpreter->typed_output_tensor<float>(0);         // [1,10,4]
                    float *classes = interpreter->typed_output_tensor<float>(1);        // [1,10]
                    float *scores = interpreter->typed_output_tensor<float>(2);         // [1,10]
                    float *num_detections = interpreter->typed_output_tensor<float>(3); // [1]

                    int actual_detections = static_cast<int>(*num_detections);
                    LOG_INFO("Number of detections: " << actual_detections);
                    results.noOfBoxes = actual_detections;

                    for (int i = 0; i < actual_detections; ++i)
                    {
                        LOG_DEBUG("Detection " << i + 1 << ": ");
                        LOG_DEBUG("  Class ID: " << classes[i]);
                        LOG_DEBUG("  Score: " << scores[i]);
                        LOG_DEBUG("  BBox: [" << bboxes[i * 4 + 0] << ", " << bboxes[i * 4 + 1] << ", "
                                              << bboxes[i * 4 + 2] << ", " << bboxes[i * 4 + 3] << "]");
                        BoxPrediction prediction;
                        prediction.x_min = bboxes[i * 4 + 0];// * mTensorFormatSettings.inputWidth;
                        prediction.y_min = bboxes[i * 4 + 1];// * mTensorFormatSettings.inputHeight;
                        prediction.x_max = bboxes[i * 4 + 2];// * mTensorFormatSettings.inputWidth;
                        prediction.y_max = bboxes[i * 4 + 3];//* mTensorFormatSettings.inputHeight;
                        switch ((int)classes[i])
                        {
                        case 1:
                            prediction.class_id = PERSON;
                            break;
                        case 2:
                            prediction.class_id = DELIVERY;
                        default:
                            prediction.class_id = UNKNOWN;
                            break;
                        }
                        prediction.confidence = scores[i];
                        results.predictions.push_back(prediction);
                    }
                }
                if (1 == num_outputs)
                {
                    uint8_t *output = interpreter->typed_output_tensor<uint8_t>(0); // [1,2]
                    // Retrieve quantization parameters
                    auto quant_params = interpreter->tensor(interpreter->outputs()[0])->params;
                    float scale = quant_params.scale;
                    int zero_point = quant_params.zero_point;

                    // Dequantize the output
                    float real_output[2]; // Assuming output size is 2
                    for (int i = 0; i < 2; ++i)
                    {
                        real_output[i] = (output[i] - zero_point) * scale;
                    }
                    float sum = real_output[0] + real_output[1];
                    float probabilities[2];

                    for (int i = 0; i < 2; ++i)
                    {
                        probabilities[i] = real_output[i] / sum;
                        BoxPrediction prediction{0};
                        prediction.class_id = DELIVERY;
                        prediction.confidence = probabilities[i];
                        results.predictions.push_back(prediction);
                    }
                }
#if 0
                // Iterate over all output tensors
                for (int i = 0; i < num_outputs; ++i)
                {
                    TfLiteTensor *output_tensor = interpreter->output_tensor(i);
                    if (output_tensor == nullptr)
                    {
                        LOG_ERROR("Output tensor " << i << " is nullptr.");
                        continue;
                    }
                    LOG_INFO("output_tensor[" << i << "].name="<<output_tensor->name);
                    // Assuming output tensor index and result interpretation are known
                    if (output_tensor->type == kTfLiteFloat32)
                    {
                        float *output_data = interpreter->typed_output_tensor<float>(i);
                        int output_size = output_tensor->bytes / sizeof(float);
                        // Copy output tensor to result vector
                        results.insert(results.end(), output_data, output_data + output_size);
                    }
                    else
                    {
                        LOG_ERROR("Output tensor " << i << " is not of type float32.");
                    }
                }
#endif
            }
            return results;
        }

        int TensorLiteRunner::Load(void)
        {
            mModel = tflite::FlatBufferModel::BuildFromFile(mModelPath.c_str());
            if (!mModel)
            {
                LOG_ERROR("Failed to load model");
                return -1;
            }
            tflite::ops::builtin::BuiltinOpResolver resolver;
            tflite::InterpreterBuilder builder(*mModel, resolver);
            builder(&mInterpreter);
            if (!mInterpreter)
            {
                LOG_ERROR("Failed to create interpreter");
                return -1;
            }

            if (mInterpreter->AllocateTensors() != kTfLiteOk)
            {
                LOG_ERROR("Failed to allocate tensors");
                return -1;
            }
            // Ensure the input tensor type and format
            TfLiteTensor *input_tensor = mInterpreter.get()->input_tensor(0);
            if (input_tensor->type != kTfLiteUInt8)
            {
                LOG_ERROR("Model expects tensor of type uin8_t, but got different type");
                return -1; // Return empty vector if input tensor type is not float
            }

            // Check input tensor shape not mandatory though
            if (input_tensor->dims->size != 4 ||
                input_tensor->dims->data[1] != 224 ||
                input_tensor->dims->data[2] != 224 ||
                input_tensor->dims->data[3] != 3)
            {
                LOG_ERROR("Model input tensor has unexpected shape.");
                return -1;
            }

            mTensorFormatSettings.inputWidth = input_tensor->dims->data[1];
            mTensorFormatSettings.inputHeight = input_tensor->dims->data[2];
            mTensorFormatSettings.noOfChannels = input_tensor->dims->data[3];

            // Fetch and log the quantization parameters
            if (input_tensor->quantization.type == kTfLiteAffineQuantization)
            {
                auto *quant_params = reinterpret_cast<TfLiteAffineQuantization *>(input_tensor->quantization.params);
                if (quant_params && quant_params->scale && quant_params->zero_point)
                {
                    float scale = quant_params->scale->data[0];
                    int zero_point = quant_params->zero_point->data[0];
                    LOG_INFO("Quantization parameters - Scale: " + std::to_string(scale) + ", Zero Point: " + std::to_string(zero_point));
                    mTensorFormatSettings.scale = scale;
                    mTensorFormatSettings.zeroPoint = zero_point;
                    int q_min = 0;   // Minimum possible value for uint8
                    int q_max = 255; // Maximum possible value for uint8
                    mTensorFormatSettings.uBound = scale * (q_max - zero_point);
                    mTensorFormatSettings.lBound = scale * (q_min - zero_point);
                }
                else
                {
                    LOG_ERROR("Failed to retrieve quantization parameters.");
                    return -1;
                }
            }
            else
            {
                LOG_ERROR("Expected quantized input tensor.");
                return -1;
            }

            mInputTensor = mInterpreter.get()->typed_input_tensor<uint8_t>(0);
            LOG_INFO("model is loaded!!");
            return 0;
        }

        size_t TensorLiteRunner::GetTensorSize(tflite::Interpreter *interpreter, int tensor_index)
        {
            TfLiteTensor *tensor = interpreter->tensor(tensor_index);
            const TfLiteIntArray *dims = tensor->dims; // Tensor dimension array

            size_t size_in_bytes = 1;
            for (int i = 0; i < dims->size; ++i)
            {
                size_in_bytes *= dims->data[i];
            }

            // Multiply by the size of the tensor's data type
            switch (tensor->type)
            {
            case kTfLiteFloat32:
                size_in_bytes *= sizeof(float); // float is typically 4 bytes
                break;
            case kTfLiteInt32:
                size_in_bytes *= sizeof(int32_t); // int32_t is typically 4 bytes
                break;
            case kTfLiteUInt8:
                size_in_bytes *= sizeof(uint8_t); // uint8_t is 1 byte
                break;
            case kTfLiteInt8:
                size_in_bytes *= sizeof(int8_t); // int8_t is 1 byte
                break;
            // Add other cases as necessary
            default:
                LOG_ERROR("Unsupported tensor data type.");
                return 0;
            }

            return size_in_bytes;
        }

        TensorInfo TensorLiteRunner::GetTensorInfoByName(tflite::Interpreter *interpreter, const std::string &tensor_name)
        {
            TensorInfo info;
            std::cout << "Looking for tensor with name " << tensor_name.c_str() << std::endl;
            const size_t num_subgraphs = interpreter->subgraphs_size();
            printf("No of subgraph %d\n", num_subgraphs);
            for (int i = 0; i < num_subgraphs; ++i)
            {
                const tflite::Subgraph &subgraph = *(interpreter->subgraph(i));
                printf("Number of tensor %d\n", subgraph.tensors_size());
                for (size_t tensor_index = 0; tensor_index < subgraph.tensors_size(); tensor_index++)
                {
                    const TfLiteTensor *tensor = subgraph.tensor(static_cast<int>(tensor_index));
                    if (nullptr != tensor->name)
                    {
                        std::string tmp(tensor->name);
                        if (tensor_name == tmp)
                        {
                            if (tensor->type != kTfLiteUInt8)
                            {
                                std::cerr << "Model expects tensor of type uint8_t, but got different type" << std::endl;
                            }
                            // Check input tensor shape
                            if (tensor->dims->size != 4 ||
                                tensor->dims->data[1] != 224 ||
                                tensor->dims->data[2] != 224 ||
                                tensor->dims->data[3] != 3)
                            {
                                std::cerr << "Model input tensor has unexpected shape." << std::endl;
                            }
                            info.index = tensor_index;
                            info.type = tensor->type;
                            info.found = true;
                            info.size = GetTensorSize(interpreter, tensor_index);
                            return info;
                        }
                    }
                }
            }
            return info; // Return default info if not found
        }
        void TensorLiteRunner::PrintStats(void)
        {
            LOG_INFO("Performance Stats:" << mModelPath);
            LOG_INFO("    Module Load              :" << r_module_load_ms << " ms");
            LOG_INFO("Total Load Time     :" << r_module_load_ms << " ms");
        }

        /**
         * based on the properties of the TFLite_Detection_PostProcess node in our model,
         * the Intersection over Union (IoU) threshold for Non-Maximum Suppression (NMS) is indeed handled internally by the model.
         * The value is set to 0.6, meaning that this part of the filtering process is already configured and does not
         * require additional external implementation unless we wish to apply a different IoU threshold for some reason.
         * So all the methods below are experimental.
         */
        bool TensorLiteRunner::compare_confidence(const BoxPrediction &a, const BoxPrediction &b)
        {
            return a.confidence > b.confidence;
        }

        float TensorLiteRunner::box_iou(const BoxPrediction &a, const BoxPrediction &b)
        {
            float x1 = std::max(a.x_min, b.x_min);
            float y1 = std::max(a.y_min, b.y_min);
            float x2 = std::min(a.x_max, b.x_max);
            float y2 = std::min(a.y_max, b.y_max);

            float interArea = std::max(0.0f, x2 - x1) * std::max(0.0f, y2 - y1);
            float unionArea = (a.x_max - a.x_min) * (a.y_max - a.y_min) + (b.x_max - b.x_min) * (b.y_max - b.y_min) - interArea;

            return interArea / unionArea;
        }
        std::vector<BoxPrediction> TensorLiteRunner::non_maximum_suppression(const std::vector<BoxPrediction> &boxes, float iou_threshold, float score_threshold)
        {
            std::vector<BoxPrediction> nms_boxes;
            std::vector<bool> selected(boxes.size(), false);

            for (int i = 0; i < boxes.size(); ++i)
            {
                if (!selected[i] && boxes[i].confidence > score_threshold)
                {
                    nms_boxes.push_back(boxes[i]);
                    for (int j = i + 1; j < boxes.size(); ++j)
                    {
                        if (!selected[j] && box_iou(boxes[i], boxes[j]) > iou_threshold)
                        {
                            selected[j] = true;
                        }
                    }
                }
            }
            return nms_boxes;
        }

    } // ml
}