#ifndef __SERVEILLANCESYSTEM_H__
#define __SERVEILLANCESYSTEM_H__

#include "CameraFrameHandler.hpp"
#include "ThumbnailGenerater.hpp"
#ifdef ENABLE_CLASSIFICATION
#include "ObjectClassifier.hpp"
#include "RingBuffer.hpp"
#include "MotionEventMetadata.hpp"
#include "PredictionProcessor.hpp"
#include <optional>
#endif
#include <iostream>
#include <vector>
#include <algorithm> // For std::sort
#include <cstring>   // For memcpy
#include <cstdint>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
namespace camera
{
    namespace camera_ml
    {
        typedef enum
        {
            CVR_CLIP_GEN_START = 0,
            CVR_CLIP_GEN_END,
            CVR_CLIP_GEN_UNKNOWN
        } CVR_CLIP_GEN_STATUS;

        typedef enum
        {
            CVR_UPLOAD_OK = 0,
            CVR_UPLOAD_FAIL,
            CVR_UPLOAD_CONNECT_ERR,
            CVR_UPLOAD_SEND_ERR,
            CVR_UPLOAD_RESPONSE_ERROR,
            CVR_UPLOAD_TIMEOUT,
            CVR_UPLOAD_MAX,
            CVR_UPLOAD_CURL_ERR
        } CVR_UPLOAD_STATUS;
#ifdef ENABLE_CLASSIFICATION
        struct ModelData
        {
            std::shared_ptr<uint8_t[]> modelInput;
            float score;
            // Construct with raw pointer and take ownership immediately
            ModelData(uint8_t *input, float scr) : modelInput(input), score(scr) {}
            // Optional: Constructor directly taking a shared_ptr for more explicit shared ownership scenarios
            ModelData(std::shared_ptr<uint8_t[]> input, float scr) : modelInput(std::move(input)), score(scr) {}
            // Overload the stream insertion operator to print details about ModelData
            friend std::ostream &operator<<(std::ostream &os, const ModelData &data)
            {
                os << "Score: " << data.score << ", Address: " << static_cast<void *>(data.modelInput.get());
                return os;
            }
        };

        struct ModelDataScoreComparator
        {
            bool operator()(const ModelData &a, const ModelData &b) const
            {
                // This creates a max-heap based on score. Change to a.score < b.score for a min-heap.
                return a.score > b.score;
            }
        };
#endif
        class FrameBase
        {

        public:
            int width;
            int height;
            bool isCached;
            // Virtual destructor
            virtual ~FrameBase()
            {
                free(buffer);
            }

            // Disable copying to prevent accidental duplication of large buffer data
            FrameBase(const FrameBase &) = delete;
            FrameBase &operator=(const FrameBase &) = delete;

            // Enable move semantics
            FrameBase(FrameBase &&other) noexcept
                : width(other.width), height(other.height), buffer(other.buffer), bufferSize(other.bufferSize), isCached(other.isCached)
            {
                other.buffer = nullptr;
                other.bufferSize = 0;
                other.isCached = false;
            }

            FrameBase &operator=(FrameBase &&other) noexcept
            {
                if (this != &other)
                {
                    free(buffer);

                    width = other.width;
                    height = other.height;
                    buffer = other.buffer;
                    bufferSize = other.bufferSize;

                    other.buffer = nullptr;
                    other.bufferSize = 0;
                    other.isCached = false;
                }
                return *this;
            }

            // Method to reset all fields to their default values
            virtual void reset()
            {
                width = 0;
                height = 0;
                isCached = false;
                if (buffer)
                {
                    memset(buffer, 0, bufferSize);
                }
            }

            // Resizes the buffer and initializes new elements to zero
            bool resizeBuffer(size_t newSize)
            {
                if (newSize == bufferSize)
                {
                    return true; // No change needed
                }

                uint8_t *newBuffer = static_cast<uint8_t *>(realloc(buffer, newSize));
                if (!newBuffer)
                {
                    std::cerr << "Memory allocation failed for resizeBuffer" << std::endl;
                    return false;
                }

                if (newSize > bufferSize)
                {
                    // Initialize the newly allocated memory to zero
                    memset(newBuffer + bufferSize, 0, newSize - bufferSize);
                }

                buffer = newBuffer;
                bufferSize = newSize;
                return true;
            }

            // Checks if the buffer is empty
            bool isEmpty() const
            {
                return bufferSize == 0;
            }

            // Returns a pointer to the buffer data
            uint8_t *getBuffer()
            {
                return buffer;
            }

            // Updates the buffer with new data
            bool updateBuffer(const uint8_t *newData, size_t newSize)
            {
                if (!resizeBuffer(newSize))
                {
                    return false;
                }
                if (buffer)
                {
                    std::memcpy(buffer, newData, newSize);
                }
                return true;
            }

        protected:
            uint8_t *buffer;
            size_t bufferSize;
            // Protected constructor for base class
            FrameBase() : width(0), height(0), buffer(nullptr), bufferSize(0), isCached(false) {}
        };

        class SurveillanceFrame : public FrameBase
        {
        public:
            MotionEventMetadata eventData;
            bool isCaptured;

            // Constructor
            SurveillanceFrame()
                : FrameBase(), eventData(), isCaptured(false) {}

            // Move constructor
            SurveillanceFrame(SurveillanceFrame &&other) noexcept
                : FrameBase(std::move(other)), eventData(std::move(other.eventData)), isCaptured(other.isCaptured)
            {
                other.isCaptured = false;
            }

            // Move assignment operator
            SurveillanceFrame &operator=(SurveillanceFrame &&other) noexcept
            {
                if (this != &other)
                {
                    FrameBase::operator=(std::move(other));
                    eventData = std::move(other.eventData);
                    isCached = other.isCached;
                    isCaptured = other.isCaptured;
                    other.isCaptured = false;
                }
                return *this;
            }

            // Method to reset all fields to their default values
            void reset() override
            {
                FrameBase::reset();
                eventData.reset();
                isCached = false;
                isCaptured = false;
            }
        };
#ifdef ENABLE_CLASSIFICATION
        class ObjectClassificationFrame : public FrameBase
        {
        public:
            // Constructor
            ObjectClassificationFrame() : FrameBase(), mDeleveryUnionBox(), mObjectBoxes() {}
            BoundingBox mDeleveryUnionBox;
            std::vector<NormalizedBoundingBox> mObjectBoxes;
            // Method to reset all fields to their default values
            void reset() override
            {
                FrameBase::reset();
                mDeleveryUnionBox = BoundingBox();
                mObjectBoxes.clear();
                mObjectBoxes.shrink_to_fit();
            }
        };
#endif
        class SurveillanceSystem
        {
        public:
            SurveillanceSystem(int bufferId, const std::string &modelPath);
#ifdef ENABLE_CLASSIFICATION
            SurveillanceSystem(int bufferId, const std::string &modelPath, const std::string &modelPath1, const std::string &eventProps, const std::string &device);
            ~SurveillanceSystem()
            {
                keepRunning = false;
                cvClassify.notify_one();
            };
#endif
            void startSurveillance();
            void captureFrame(int64_t motionFramePTS);
            void processFrameMetaData(MotionEventMetadata &metaData, int motionFlags);
            void OnClipGenStart(const char *cvrClipFname);
            void OnClipGenEnd(const char *cvrClipFname);

        private:
            void catcheFrame(FrameBase &frame, const MotionEventMetadata &metaData, size_t width, size_t height, size_t y_size, size_t uv_size, size_t total_size);
            void catcheFrameForThumbnail(const MotionEventMetadata &metaData, size_t width, size_t height, size_t y_size, size_t uv_size, size_t total_size);

            std::unique_ptr<CameraFrameHandler> mCameraFrameHandler;
            std::unique_ptr<ThumbnailGenerater> mThumbnailGenerater;
            // std::unique_ptr<RingBuffer> m_rb;
            SurveillanceFrame mSurveillanceFrame;
            ROI mROI;
            PayLoadMetaData mMotionPayload;
            frameInfoYUV *mRawFrameInfo;
            std::mutex mResourceMutex;
            std::chrono::high_resolution_clock::time_point start_detection_time;
            std::chrono::steady_clock::time_point last_processed_time;
#ifdef ENABLE_CLASSIFICATION
            NormalizationParams getNormalizationParams(TensorFormatSettings settings);
            void catcheFrameForMotionClassification(const MotionEventMetadata &metaData, size_t width, size_t height, size_t y_size, size_t uv_size, size_t total_size);
            std::optional<BoxPrediction> processInput(const std::shared_ptr<uint8_t[]> &modelInput, ObjectType type);
            std::optional<BoxPrediction> processOutput(const std::vector<BoxPrediction> &modelOutput, ObjectType type);
            void classifyMotionObjects();
            void cacheFrameForDelivery(uint8_t *yBuffer, BoxPrediction predictedPerson);
            void processFrameForDelivery();
            void processFrameForPerson();

            std::unique_ptr<ObjectClassifier> mDeliveryClassifier;
            std::unique_ptr<ObjectClassifier> mPersonClassifier;
            std::unique_ptr<RingBuffer<ModelData, ModelDataScoreComparator>> m_rb;
            NormalizationParams mDeliveryModelParams;
            NormalizationParams mPersonModelParams;
            ObjectClassificationFrame mObjectClassificationFrame;
            std::condition_variable cvClassify;
            std::atomic<bool> motionDetected;
            std::atomic<bool> classifyObj;
            std::thread classifierThread;
            std::atomic<bool> keepRunning;

#endif
            int cachedFrame;
            int processedFrame;
            static float m_threshold;
        };
    }
}
#endif // __SERVEILLANCESYSTEM_H__
