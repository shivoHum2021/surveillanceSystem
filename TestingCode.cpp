#ifdef TEST
struct ModelData
{
    uint8_t *modelInput;
    float score;

    ModelData(uint8_t *input, float scr) : modelInput(input), score(scr) {}
    ~ModelData()
    {
        delete[] modelInput; // Properly deallocate memory
    }

    // Disable copy operations to avoid double free
    ModelData(const ModelData &) = delete;
    ModelData &operator=(const ModelData &) = delete;

    // Enable move operations
    ModelData(ModelData &&other) noexcept : modelInput(other.modelInput), score(other.score)
    {
        other.modelInput = nullptr;
    }
    ModelData &operator=(ModelData &&other) noexcept
    {
        if (this != &other)
        {
            delete[] modelInput;
            modelInput = other.modelInput;
            score = other.score;
            other.modelInput = nullptr;
        }
        return *this;
    }
    // Overload the operator<< to handle ModelData directly
    /** std::ostream &operator<<(std::ostream &os, const ModelData &data)
     {
         os << "Score: " << data.score << ", Address: " << static_cast<void *>(data.modelInput);
         return os;
     }*/
};

struct ModelDataScoreComparator
{
    bool operator()(const ModelData &a, const ModelData &b) const
    {
        // This creates a max-heap based on score. Change to a.score < b.score for a min-heap.
        return a.score > b.score;
    }
};
class RingBuffer
{
private:
    std::vector<ModelData> buffer;
    size_t capacity;

public:
    RingBuffer(size_t cap) : capacity(cap)
    {
        buffer.reserve(capacity);
    }
    ~RingBuffer()
    {
        clear(); // Cleanup on destruction
    }

    void add(uint8_t *modelInput, float score)
    {
        if (buffer.size() >= capacity)
        {
            // Remove the element with the lowest score
            std::pop_heap(buffer.begin(), buffer.end(), [](const ModelData &a, const ModelData &b)
                          {
                              return a.score > b.score; // Max-heap based on score
                          });
            buffer.pop_back();
        }
        buffer.emplace_back(modelInput, score);
        std::push_heap(buffer.begin(), buffer.end(), [](const ModelData &a, const ModelData &b)
                       {
                           return a.score > b.score; // Max-heap to keep highest at front
                       });
    }
    const std::vector<ModelData> &getBuffer() const
    {
        return buffer;
    }

    void clear()
    {
        buffer.clear(); // Automatically calls destructors of ModelData
    }

    void print() const
    {
        for (const auto &data : buffer)
        {
            LOG_INFO("Score: " << data.score << ", Address: " << static_cast<void *>(data.modelInput));
        }
    }
};
#endif
/// @brief /////////////////
struct ObjectClassificationFrame
{
    int width;
    int height;
    std::vector<uint8_t> buffer;
    ObjectClassificationFrame() = default;

    // Disable copying to prevent accidental duplication of large buffer data
    ObjectClassificationFrame(const ObjectClassificationFrame &) = delete;
    ObjectClassificationFrame &operator=(const ObjectClassificationFrame &) = delete;

    // Enable move semantics
    ObjectClassificationFrame(ObjectClassificationFrame &&) = default;
    ObjectClassificationFrame &operator=(ObjectClassificationFrame &&) = default;
    // Method to reset all fields to their default values
    void reset()
    {
        width = 0;
        height = 0;
        buffer.clear();
    }
    // Resizes the buffer and initializes new elements to zero
    void resizeBuffer(size_t newSize)
    {
        buffer.resize(newSize, 0);
    }
    // Checks if the buffer is empty
    bool isEmpty() const // Added const to indicate this does not modify the object
    {
        return buffer.empty(); // Directly using vector's empty method
    }
    // Returns a pointer to the buffer data
    uint8_t *getBuffer()
    {
        if (!isEmpty()) // Ensuring there is data before returning the pointer
        {
            return buffer.data(); // Corrected to use data() method
        }
        return nullptr; // Return nullptr if buffer is empty
    }
    // Updates the buffer with new data
    void updateBuffer(const uint8_t *newData, size_t newSize)
    {
        resizeBuffer(newSize);
        std::memcpy(buffer.data(), newData, newSize); // Correct usage of memcpy
    }
};

////////////

struct SurveillanceFrame
{
    int width;
    int height;
    std::vector<uint8_t> buffer;
    MotionEventMetadata eventData;
    bool isCached = false; // Flag to check if the frame has been processed// Default constructor
    bool isCaptured = false;
    SurveillanceFrame() = default;

    // Disable copying to prevent accidental duplication of large buffer data
    SurveillanceFrame(const SurveillanceFrame &) = delete;
    SurveillanceFrame &operator=(const SurveillanceFrame &) = delete;

    // Enable move semantics
    SurveillanceFrame(SurveillanceFrame &&) = default;
    SurveillanceFrame &operator=(SurveillanceFrame &&) = default;
    // Method to reset all fields to their default values
    void reset()
    {
        width = 0;
        height = 0;
        buffer.clear();
        eventData.reset(); // Assuming MotionEventMetadata can be reset this way
        isCached = false;
        isCaptured = false;
    }
    // Resizes the buffer and initializes new elements to zero
    void resizeBuffer(size_t newSize)
    {
        buffer.resize(newSize, 0);
    }

    // Checks if the buffer is empty
    bool isEmpty() const // Added const to indicate this does not modify the object
    {
        return buffer.empty(); // Directly using vector's empty method
    }

    // Returns a pointer to the buffer data
    uint8_t *getBuffer()
    {
        if (!isEmpty()) // Ensuring there is data before returning the pointer
        {
            return buffer.data(); // Corrected to use data() method
        }
        return nullptr; // Return nullptr if buffer is empty
    }

    // Updates the buffer with new data
    void updateBuffer(const uint8_t *newData, size_t newSize)
    {
        resizeBuffer(newSize);
        std::memcpy(buffer.data(), newData, newSize); // Correct usage of memcpy
    }
};
/////////////////////////////
       struct SurveillanceYUVFrame
        {
            int width;
            int height;
            std::vector<uint8_t> y_channel;
            std::vector<uint8_t> uv_channel;
            // Default constructor
            SurveillanceYUVFrame() : width(0), height(0) {}
            // Copying is not allowed to prevent accidental duplication of large buffer data
            SurveillanceYUVFrame(const SurveillanceYUVFrame &) = delete;
            SurveillanceYUVFrame &operator=(const SurveillanceYUVFrame &) = delete;
            // Allow move semantics
            SurveillanceYUVFrame(SurveillanceYUVFrame &&) = default;
            SurveillanceYUVFrame &operator=(SurveillanceYUVFrame &&) = default;
            // Clear the contents of the buffers
            void clear()
            {
                std::fill(y_channel.begin(), y_channel.end(), 0);
                std::fill(uv_channel.begin(), uv_channel.end(), 0);
            }
            // Resize vector and fill with zeros
            void resizeBuffer()
            {
                y_channel.resize(width * height, 0);                  // Assuming Y channel uses all pixels
                uv_channel.resize((width / 2) * (height / 2) * 2, 0); // Assuming UV channels are subsampled by 2
            }
            // Update buffer content
            void updateBuffer(uint8_t *y_data, uint8_t *uv_data)
            {
                clear();
                resizeBuffer();
                std::memcpy(y_channel.data(), y_data, y_channel.size());
                std::memcpy(uv_channel.data(), uv_data, uv_channel.size());
            }
        };
//////////////////////////////

 void SurveillanceSystem::startSurveillance()
        {
            mPersonClassifier->intializeObjectClassifier();
            mPersonModelParams = getNormalizationParams(mPersonClassifier->getTensorPreprocessingParams());
            mDeliveryClassifier->intializeObjectClassifier();
            mDeliveryModelParams = getNormalizationParams(mDeliveryClassifier->getTensorPreprocessingParams());
            LOG_INFO("Starting the Surveillance..");
            classifierThread = std::thread(&SurveillanceSystem::classifyMotionObjects, this);
            classifierThread.detach();
#if 0
            int counter = 0;
            int count = 0;
            do
            {
                counter++;
                frameInfoYUV *raw = mCameraFrameHandler->CaptureFrameFromCamera();
                // uint8_t *modelInput = mCameraFrameHandler->normalizeAndResize(raw, 224, 224);
                uint8_t *modelInput = mCameraFrameHandler->normalizeAndResize(raw, 224, 224);

                if (modelInput)
                {
                    LOG_INFO("modelInput(" << static_cast<void *>(modelInput) << ")");
                    auto bestPrediction = processInput(modelInput, PERSON);
                    if (bestPrediction.has_value())
                    {
                        uint8_t *rawInput = mCameraFrameHandler->resizeNormalizeQuantize(raw, mDeliveryModelParams);
                        if (rawInput)
                        {
                            LOG_INFO("Caching (" << static_cast<void *>(rawInput) << ") for delivery!!")
                            std::shared_ptr<uint8_t[]> dModelInput(rawInput); // Wrap the raw pointer in a smart pointer
                            m_rb->add(ModelData(std::move(dModelInput), bestPrediction->confidence));
                        }
                    }
                    LOG_INFO("Clean modelInput(" << static_cast<void *>(modelInput) << ")");
                    delete[] modelInput;
                }

                if (counter < 16)
                {
                    sleep(1);
                }
                else
                {
                    m_rb->print();
                    for (const auto &data : m_rb->getBuffer())
                    {
                        if (data.modelInput)
                        {
                            auto bestPrediction = processInput(data.modelInput.get(), DELIVERY);
                            struct stat statbuf;
                            if (stat("/tmp/.store", &statbuf) < 0)
                            {
                                if (count < 5)
                                {

                                    auto now = high_resolution_clock::now();
                                    auto tend = duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
                                    std::string output_image_path = "/opt/image_" + std::to_string(tend) + ".jpg";
                                    mCameraFrameHandler->saveBufferAsJpeg(data.modelInput.get(), mDeliveryModelParams.inputWidth, mDeliveryModelParams.inputHeight, output_image_path);
                                    count++;
                                }
                            }
                            if (bestPrediction.has_value())
                            {
                                LOG_INFO("DELIVERY DETECTED!!!  Confidence: " << bestPrediction.value().confidence);
                                break;
                            }
                        }
                    }
                    m_rb->clear();
                    sleep(20);
                    counter = 0;
                }
            } while (true);
#endif
        }
///////////////////
  // BoundingBox* box = &mMotionEventMetadata.deliveryUnionBox;
                uint8_t *convertAndResize(uint8_t *raw, int width, int height, int newWidth, int newHeight, BoundingBox *unionBox)
                {
                    cv::Mat yuv(height + height / 2, width, CV_8UC1, raw);
                    cv::Mat convertedFrame(height, width, CV_8UC3);
                    cv::cvtColor(yuv, convertedFrame, cv::COLOR_YUV2RGB_NV12); // Adjust according to your actual YUV format
                    cv::Mat resizedFrame;
                    if (unionBox && !unionBox->isEmpty())
                    {
                        double scaleFactor = 1;
                        cv::Rect currentBox(unionBox->boundingBoxXOrd, unionBox->boundingBoxYOrd, unionBox->boundingBoxWidth, unionBox->boundingBoxHeight);
                        cv::Size cropSize = CameraFrameHandler::getResizedCropSize(currentBox, newWidth, newHeight, &scaleFactor);

                        if (scaleFactor != 1)
                        {
                            LOG_INFO("Resizing scale for the thumbnail is" << std::to_string(scaleFactor));
                            cv::Size rescaleSize = cv::Size(convertedFrame.cols / scaleFactor, convertedFrame.rows / scaleFactor);
                            // resize the frame to fit the union blob in the thumbnail
                            cv::resize(convertedFrame, convertedFrame, rescaleSize);
                            // Resize the union blob also accordingly
                            unionBox->boundingBoxWidth = unionBox->boundingBoxWidth / scaleFactor;
                            unionBox->boundingBoxHeight = unionBox->boundingBoxHeight / scaleFactor;
                            unionBox->boundingBoxXOrd = unionBox->boundingBoxXOrd / scaleFactor;
                            unionBox->boundingBoxYOrd = unionBox->boundingBoxYOrd / scaleFactor;
                            currentBox = cv::Rect(unionBox->boundingBoxXOrd, unionBox->boundingBoxYOrd, unionBox->boundingBoxWidth, unionBox->boundingBoxHeight);
                        }
                        else
                        {
                            LOG_INFO("Not resizing the thumbnail");
                        }
                    }
                    cv::resize(convertedFrame, resizedFrame, cv::Size(newWidth, newHeight));
                    // Allocate memory that persists outside this function scope
                    size_t numBytes = resizedFrame.total() * resizedFrame.elemSize();
                    if (numBytes == 0)
                    {
                        std::cerr << "Error: No data in output frame." << std::endl;
                        return nullptr;
                    }
                    uint8_t *modelInput = new (std::nothrow) uint8_t[numBytes]; // Use nothrow to return nullptr on failure
                    if (!modelInput)
                    {
                        std::cerr << "Error: Memory allocation failed for model input." << std::endl;
                        return nullptr;
                    }
                    memcpy(modelInput, resizedFrame.data, numBytes);
                    return modelInput;
                }
                uint8_t *normalizeAndResize(uint8_t *raw, int width, int height, NormalizationParams params, BoundingBox *unionBox)
                {
                    // params.print();
                    cv::Mat yuv(height + height / 2, width, CV_8UC1, raw);
                    cv::Mat convertedFrame(height, width, CV_8UC3);
                    cv::cvtColor(yuv, convertedFrame, cv::COLOR_YUV2RGB_NV12); // Adjust according to your actual YUV format
                    double scaleFactor = 1;
                    cv::Mat resizedFrame;
                    // Resize the image
                    if (unionBox && !unionBox->isEmpty())
                    {
                        double scaleFactor = 1;
                        cv::Rect currentBox(unionBox->boundingBoxXOrd, unionBox->boundingBoxYOrd, unionBox->boundingBoxWidth, unionBox->boundingBoxHeight);
                        cv::Size cropSize = CameraFrameHandler::getResizedCropSize(currentBox, params.inputWidth, params.inputHeight, &scaleFactor);

                        if (scaleFactor != 1)
                        {
                            LOG_INFO("Resizing scale for the thumbnail is" << std::to_string(scaleFactor));
                            cv::Size rescaleSize = cv::Size(convertedFrame.cols / scaleFactor, convertedFrame.rows / scaleFactor);
                            // resize the frame to fit the union blob in the thumbnail
                            cv::resize(convertedFrame, convertedFrame, rescaleSize);
                            // Resize the union blob also accordingly
                            unionBox->boundingBoxWidth = unionBox->boundingBoxWidth / scaleFactor;
                            unionBox->boundingBoxHeight = unionBox->boundingBoxHeight / scaleFactor;
                            unionBox->boundingBoxXOrd = unionBox->boundingBoxXOrd / scaleFactor;
                            unionBox->boundingBoxYOrd = unionBox->boundingBoxYOrd / scaleFactor;
                            currentBox = cv::Rect(unionBox->boundingBoxXOrd, unionBox->boundingBoxYOrd, unionBox->boundingBoxWidth, unionBox->boundingBoxHeight);
                        }
                        else
                        {
                            LOG_INFO("Not resizing the thumbnail");
                        }
                    }
                    cv::resize(convertedFrame, resizedFrame, cv::Size(params.inputWidth, params.inputHeight));
                    // Normalize to [0, 1]H
                    cv::Mat normalizedFrame;
                    resizedFrame.convertTo(normalizedFrame, CV_32FC3, 1.0 / 255);
                    // Prepare output Mat
                    cv::Mat outputFrame(normalizedFrame.size(), CV_8UC3);
                    // Adjust according to the quantization formula to map to [0, 255] again correctly
                    // quantization: linear -1 ≤ 0.0078125 * (q - 128) ≤ 0.9921875
                    // Convert from [0, 1] to [-1, 0.9921875]
                    for (int i = 0; i < normalizedFrame.rows; ++i)
                    {
                        for (int j = 0; j < normalizedFrame.cols; ++j)
                        {
                            for (int c = 0; c < 3; ++c)
                            {
                                float pixel = normalizedFrame.at<cv::Vec3f>(i, j)[c];
                                // Map to the target range
                                // Scaling: Multiplying by 1.9921875 essentially stretches the [0, 1] range to [0, 1.9921875].
                                // Shifting: Subtracting 1.0 from the scaled value shifts the range down to [-1, 0.9921875].
                                float transformed = pixel * 1.9921875 - 1.0; // Scale and shift
                                // Transform to match quantization expectation
                                uint8_t quantized = static_cast<uint8_t>(std::max(0.0f, std::min(255.0f, static_cast<float>(128.0 + transformed / 0.0078125))));
                                outputFrame.at<cv::Vec3b>(i, j)[c] = quantized;
                            }
                        }
                    }
                    // Allocate memory that persists outside this function scope
                    size_t numBytes = outputFrame.total() * outputFrame.elemSize();
                    if (numBytes == 0)
                    {
                        std::cerr << "Error: No data in output frame." << std::endl;
                        return nullptr;
                    }
                    // Allocate memory for model input
                    uint8_t *modelInput = new (std::nothrow) uint8_t[numBytes]; // Use nothrow to return nullptr on failure
                    if (!modelInput)
                    {
                        std::cerr << "Error: Memory allocation failed for model input." << std::endl;
                        return nullptr;
                    }
                    memcpy(modelInput, outputFrame.data, numBytes);
#if 0
                    for (size_t i = 0; i < std::min(numBytes, static_cast<size_t>(10)); ++i)
                    {
                        std::cerr << "modelInput[" << i << "] = " << static_cast<int>(modelInput[i]) << std::endl;
                    }
#endif
                    return modelInput;
                }

                uint8_t *resizeNormalizeQuantize(uint8_t *raw, int width, int height, NormalizationParams params, BoundingBox *unionBox)
                {
                    // params.print();
                    //  Convert YUV to RGB using OpenCV
                    cv::Mat yuv(height + height / 2, width, CV_8UC1, raw);
                    cv::Mat rgb(height, width, CV_8UC3);
                    ;
                    cv::cvtColor(yuv, rgb, cv::COLOR_YUV2RGB_NV12); // Adjust this based on actual YUV format

                    if (unionBox && !unionBox->isEmpty())
                    {
                        double scaleFactor = 1;
                        cv::Rect currentBox(unionBox->boundingBoxXOrd, unionBox->boundingBoxYOrd, unionBox->boundingBoxWidth, unionBox->boundingBoxHeight);
                        cv::Size cropSize = CameraFrameHandler::getResizedCropSize(currentBox, params.inputWidth, params.inputHeight, &scaleFactor);

                        if (scaleFactor != 1)
                        {
                            LOG_INFO("Resizing scale for the thumbnail is" << std::to_string(scaleFactor));
                            cv::Size rescaleSize = cv::Size(rgb.cols / scaleFactor, rgb.rows / scaleFactor);
                            // resize the frame to fit the union blob in the thumbnail
                            cv::resize(rgb, rgb, rescaleSize);
                            // Resize the union blob also accordingly
                            unionBox->boundingBoxWidth = unionBox->boundingBoxWidth / scaleFactor;
                            unionBox->boundingBoxHeight = unionBox->boundingBoxHeight / scaleFactor;
                            unionBox->boundingBoxXOrd = unionBox->boundingBoxXOrd / scaleFactor;
                            unionBox->boundingBoxYOrd = unionBox->boundingBoxYOrd / scaleFactor;
                            currentBox = cv::Rect(unionBox->boundingBoxXOrd, unionBox->boundingBoxYOrd, unionBox->boundingBoxWidth, unionBox->boundingBoxHeight);
                        }
                        else
                        {
                            LOG_INFO("Not resizing the thumbnail");
                        }
                    }
                    // Resize the image to the expected dimensions
                    cv::Mat resized;
                    cv::resize(rgb, resized, cv::Size(params.inputWidth, params.inputHeight));

                    // Allocate the output buffer
                    size_t numBytes = resized.total() * resized.elemSize();
                    uint8_t *output = new (std::nothrow) uint8_t[numBytes];
                    if (!output)
                    {
                        std::cerr << "Memory allocation failed for output buffer" << std::endl;
                        return nullptr;
                    }

                    // Normalize and quantize according to the parameters
                    for (int i = 0; i < resized.rows; i++)
                    {
                        for (int j = 0; j < resized.cols; j++)
                        {
                            for (int c = 0; c < 3; c++)
                            { // Process each color channel
                                uint8_t pixel = resized.at<cv::Vec3b>(i, j)[c];
                                // Normalize to [0, 1]
                                float normalized = pixel / 255.0f;
                                // Scale to [lBound, uBound] and adjust based on zeroPoint and scale
                                float scaled = (normalized * (params.uBound - params.lBound) + params.lBound - params.zeroPoint * params.scale) / params.scale;
                                // Quantize to uint8
                                uint8_t quantized = static_cast<uint8_t>(std::min(std::max(0.0f, scaled), 255.0f));
                                resized.at<cv::Vec3b>(i, j)[c] = quantized;
                            }
                        }
                    }
                    // Copy resized data to output
                    memcpy(output, resized.data, numBytes);
                    return output;
                }

 class FrameConverter
            {
            public:
                FrameConverter() {}
                // BoundingBox* box = &mMotionEventMetadata.deliveryUnionBox;
                uint8_t *convertAndResize(uint8_t *raw, int width, int height, int newWidth, int newHeight, BoundingBox *unionBox)
                {
                    if (!raw || width <= 0 || height <= 0 || newWidth <= 0 || newHeight <= 0)
                    {
                        std::cerr << "Invalid input dimensions or raw data." << std::endl;
                        return nullptr;
                    }
                    cv::Mat rgbFrame = convertYUVToRGB(raw, width, height);
                    if (rgbFrame.empty())
                    {
                        std::cerr << "Failed to convert YUV to RGB." << std::endl;
                        return nullptr;
                    }
                    cv::Mat resizedFrame;
                    resizeFrame(rgbFrame, resizedFrame, newWidth, newHeight, unionBox);

                    return allocateAndCopy(resizedFrame);
                }
                uint8_t *normalizeAndResize(uint8_t *raw, int width, int height, NormalizationParams params, BoundingBox *unionBox)
                {
                    if (!raw || width <= 0 || height <= 0 || params.inputWidth <= 0 || params.inputHeight <= 0)
                    {
                        std::cerr << "Invalid input dimensions or raw data." << std::endl;
                        return nullptr;
                    }
                    cv::Mat rgbFrame = convertYUVToRGB(raw, width, height);
                    if (rgbFrame.empty())
                    {
                        std::cerr << "Failed to convert YUV to RGB." << std::endl;
                        return nullptr;
                    }
                    cv::Mat resizedFrame;
                    resizeFrame(rgbFrame, resizedFrame, params.inputWidth, params.inputHeight, unionBox);

                    cv::Mat normalizedFrame;
                    normalizeFrame(resizedFrame, normalizedFrame);

                    return allocateAndCopy(normalizedFrame);
                }

                uint8_t *resizeNormalizeQuantize(uint8_t *raw, int width, int height, NormalizationParams params, BoundingBox *unionBox)
                {
                    if (!raw || width <= 0 || height <= 0 || params.inputWidth <= 0 || params.inputHeight <= 0)
                    {
                        std::cerr << "Invalid input dimensions or raw data." << std::endl;
                        return nullptr;
                    }
                    cv::Mat rgbFrame = convertYUVToRGB(raw, width, height);
                    if (rgbFrame.empty())
                    {
                        std::cerr << "Failed to convert YUV to RGB." << std::endl;
                        return nullptr;
                    }
                    cv::Mat resizedFrame;
                    resizeFrame(rgbFrame, resizedFrame, params.inputWidth, params.inputHeight, unionBox);

                    cv::Mat normalizedFrame;
                    normalizeFrame(resizedFrame, normalizedFrame);

                    cv::Mat quantizedFrame;
                    quantizeFrame(normalizedFrame, quantizedFrame, params);

                    return allocateAndCopy(quantizedFrame);
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

                void resizeFrame(cv::Mat &inputFrame, cv::Mat &outputFrame, int newWidth, int newHeight, BoundingBox *unionBox)
                {
                    if (unionBox && !unionBox->isEmpty())
                    {
                        double scaleFactor = 1;
                        cv::Rect currentBox(unionBox->boundingBoxXOrd, unionBox->boundingBoxYOrd, unionBox->boundingBoxWidth, unionBox->boundingBoxHeight);
                        cv::Size cropSize = CameraFrameHandler::getResizedCropSize(currentBox, newWidth, newHeight, &scaleFactor);

                        if (scaleFactor != 1)
                        {
                            std::cout << "Resizing scale for the thumbnail is " << scaleFactor << std::endl;
                            cv::Size rescaleSize(inputFrame.cols / scaleFactor, inputFrame.rows / scaleFactor);
                            cv::resize(inputFrame, inputFrame, rescaleSize);

                            unionBox->boundingBoxWidth /= scaleFactor;
                            unionBox->boundingBoxHeight /= scaleFactor;
                            unionBox->boundingBoxXOrd /= scaleFactor;
                            unionBox->boundingBoxYOrd /= scaleFactor;
                        }
                    }

                    cv::resize(inputFrame, outputFrame, cv::Size(newWidth, newHeight));
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

                uint8_t *allocateAndCopy(cv::Mat &frame)
                {
                    size_t numBytes = frame.total() * frame.elemSize();
                    if (numBytes == 0)
                    {
                        std::cerr << "Error: No data in output frame." << std::endl;
                        return nullptr;
                    }

                    uint8_t *output = new (std::nothrow) uint8_t[numBytes];
                    if (!output)
                    {
                        std::cerr << "Error: Memory allocation failed for output buffer." << std::endl;
                        return nullptr;
                    }

                    memcpy(output, frame.data, numBytes);
                    return output;
                }
            };
