#include "SurveillanceSystem.hpp"
#include <sys/stat.h>
#include <sys/types.h>
using namespace ::std;
using namespace std::chrono;
namespace camera
{
    namespace camera_ml
    {
        // Method to execute the surveillance analysis and thumbnail generation workflow.
        SurveillanceSystem::SurveillanceSystem(int bufferId, const std::string &eventProps)
        {
            mCameraFrameHandler = std::make_unique<CameraFrameHandler>(bufferId);
            mThumbnailGenerater = std::make_unique<ThumbnailGenerater>(mCameraFrameHandler.get(), eventProps);
            start_detection_time = std::chrono::high_resolution_clock::time_point::min();
            cachedFrame = 0;
            processedFrame = 0;
        }
#ifdef ENABLE_CLASSIFICATION
        SurveillanceSystem::SurveillanceSystem(int bufferId, const std::string &personModelPath, const std::string &deliveryModelPath, const std::string &eventProps, const std::string &device)
            : mMotionPayload(), mSurveillanceFrame(), mObjectClassificationFrame(), mROI(), mDeliveryModelParams(), mPersonModelParams(), classifyObj(false), keepRunning(true), motionDetected(false), mRawFrameInfo(nullptr)
        {
            SurveillanceFrame(bufferId, eventProps);
            mPersonClassifier = std::make_unique<ObjectClassifier>(personModelPath, device);
            mDeliveryClassifier = std::make_unique<ObjectClassifier>(deliveryModelPath, device);
            m_rb = std::make_unique<RingBuffer<ModelData, ModelDataScoreComparator>>(5);
        }
#endif
        void SurveillanceSystem::startSurveillance()
        {
            LOG_INFO("Starting the Surveillance..");
#ifdef ENABLE_CLASSIFICATION
            mPersonClassifier->intializeObjectClassifier();
            mPersonModelParams = getNormalizationParams(mPersonClassifier->getTensorPreprocessingParams());
            mDeliveryClassifier->intializeObjectClassifier();
            mDeliveryModelParams = getNormalizationParams(mDeliveryClassifier->getTensorPreprocessingParams());
            classifierThread = std::thread(&SurveillanceSystem::classifyMotionObjects, this);
            classifierThread.detach();
#endif
        }

        void SurveillanceSystem::captureFrame(int64_t motionFramePTS)
        {
            std::lock_guard<std::mutex> lock(mResourceMutex);
            mRawFrameInfo = mCameraFrameHandler->CaptureFrameFromCamera();
            mSurveillanceFrame.isCaptured = true;
            return;
        }

        void SurveillanceSystem::processFrameMetaData(MotionEventMetadata &metaData, int motionFlags)
        {
            auto currTime = std::chrono::system_clock::now();
            bool hasROISet = (motionFlags & 0x08) != 0;
            int isInsideROI = (motionFlags & 0x04) >> 2;
            bool hasDOISet = (motionFlags & 0x02) != 0;
            int isInsideDOI = motionFlags & 0x01;

            LOG_DEBUG("insideROI:" << isInsideROI << " insideDOI:" << isInsideDOI);
            size_t width, height, y_size, uv_size, total_size;
            {
                std::lock_guard<std::mutex> lock(mResourceMutex);
                width = mRawFrameInfo->width;
                height = mRawFrameInfo->height;
                y_size = width * height;
                uv_size = width * height / 2;
                total_size = y_size + uv_size;

                int unionBoxArea = mSurveillanceFrame.eventData.unionBox.boundingBoxHeight * mSurveillanceFrame.eventData.unionBox.boundingBoxWidth;
                int newUnionBoxArea = metaData.unionBox.boundingBoxHeight * metaData.unionBox.boundingBoxWidth;

                // if motion is detected update the metadata.
                if ((mSurveillanceFrame.isCaptured && metaData.event_type == 4) && (newUnionBoxArea > unionBoxArea) && ((hasROISet && isInsideROI) || (hasDOISet && isInsideDOI) || (!hasROISet && !hasDOISet)))
                {
                    catcheFrameForThumbnail(metaData, width, height, y_size, uv_size, total_size);
// trigger object classification now
#ifdef ENABLE_CLASSIFICATION
                    motionDetected = true;
                    classifyObj = true;
#endif
                }
                else
                {
                    LOG_DEBUG("discarded eventType " << metaData.event_type << " Current UniounBox " << unionBoxArea << " newUnionBoxArea " << newUnionBoxArea << " isInsideROI " << isInsideROI << " hasROISet " << hasROISet << " hasDOISet" << hasDOISet);
                }
#ifdef ENABLE_CLASSIFICATION
                if (classifyObj)
                {
                    catcheFrameForMotionClassification(metaData, width, height, y_size, uv_size, total_size);
                }
#endif
            } // release the lock
// Notify classify thread
#ifdef ENABLE_CLASSIFICATION
            cvClassify.notify_one();
#endif
#ifdef TEST
            if (start_detection_time > std::chrono::high_resolution_clock::time_point::min())
            {
                auto end_time = std::chrono::high_resolution_clock::now();
                std::chrono::duration<double> elapsed = end_time - start_detection_time;

                if (elapsed.count() > 16)
                {
                    LOG_INFO("Time to generate the thumbnail!");
                    // processBufferForDelivery();
                    start_detection_time = std::chrono::high_resolution_clock::time_point::min();
                    auto now = high_resolution_clock::now();
                    auto tend = duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
                    std::string output_image_path = "/opt/image_" + std::to_string(tend) + ".jpg";
                    mCameraFrameHandler->saveBufferAsJpeg(mSurveillanceFrame.getBuffer(), mSurveillanceFrame.width, mSurveillanceFrame.height, output_image_path);
                    mSurveillanceFrame.reset();
                }
            }
#endif
            return;
        }
        void SurveillanceSystem::OnClipGenStart(const char *cvrClipFname)
        {
            LOG_INFO("OnClipGenStart");
            mMotionPayload.reset();
            mMotionPayload.fileName = "/tmp/" + std::string(cvrClipFname) + ".jpeg";
            mMotionPayload.isPayLoadReady = false;
            mMotionPayload.isInitiated = true;
        }

        void SurveillanceSystem::OnClipGenEnd(const char *cvrClipFname)
        {
            LOG_INFO("OnClipGenEnd");
            if (mMotionPayload.isPayLoadReady && mMotionPayload.isInitiated)
            {
                bool ignoreEvent = false;
                auto now = std::chrono::system_clock::now();
                uint64_t now_secs = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
                if (now_secs - mThumbnailGenerater->getLastUploadTime() < mThumbnailGenerater->getQuiteTime())
                {
                    ignoreEvent = true;
                    LOG_INFO("Skipping Motion events! curr time" << now_secs << " previous motion upload time " << mThumbnailGenerater->getLastUploadTime());
                }
                if (!ignoreEvent)
                {
                }
                mMotionPayload.isPayLoadReady = false;
                mMotionPayload.isInitiated = false;
            }
#ifdef ENABLE_CLASSIFICATION
            classifyObj = false;
#endif
            LOG_INFO("Number of time new frame cached; " << cachedFrame << " No of frame processed for person: " << processedFrame);
            bool isStore = false;
            struct stat statbuf;
            if (stat("/tmp/.store", &statbuf) == 0)
            {
                LOG_INFO("Frame will be stored in jpg format for debugging");
                isStore = true;
            }
            if (isStore)
            {
                {
                    std::lock_guard<std::mutex> resourceLock(mResourceMutex);

                    if (mSurveillanceFrame.isCached && !mSurveillanceFrame.isEmpty())
                    {
                        uint8_t *yBuffer = mSurveillanceFrame.getBuffer();
                        if (yBuffer)
                        {
                            mCameraFrameHandler->convertAndStore(yBuffer,mSurveillanceFrame.width, mSurveillanceFrame.height,400, 300,mMotionPayload.fileName,&mSurveillanceFrame.eventData.unionBox);
                            mCameraFrameHandler->saveBufferAsJpeg(yBuffer, mSurveillanceFrame.width, mSurveillanceFrame.height, mMotionPayload.fileName + "_1");
                        }
                    }
                    mSurveillanceFrame.reset();
#ifdef ENABLE_CLASSIFICATION
                    mObjectClassificationFrame.reset();
#endif
                    cachedFrame = 0;
                    processedFrame = 0;
                }
            }
        }

        void SurveillanceSystem::catcheFrame(FrameBase &frame, const MotionEventMetadata &metaData, size_t width, size_t height, size_t y_size, size_t uv_size, size_t total_size)
        {
            frame.resizeBuffer(total_size);

            uint8_t *buffer = frame.getBuffer();
            if (buffer != nullptr)
            {
                std::memcpy(buffer, mRawFrameInfo->y_addr, y_size);
                if (mRawFrameInfo->uv_addr)
                {
                    std::memcpy(buffer + y_size, mRawFrameInfo->uv_addr, uv_size);
                }
                frame.height = height;
                frame.width = width;
                frame.isCached = true;
            }
            else
            {
                LOG_INFO("Buffer is empty, unable to process frame.");
            }
        }

        void SurveillanceSystem::catcheFrameForThumbnail(const MotionEventMetadata &metaData, size_t width, size_t height, size_t y_size, size_t uv_size, size_t total_size)
        {
            LOG_DEBUG("Processing metadata for thumbnail");
            mSurveillanceFrame.reset();
            catcheFrame(mSurveillanceFrame, metaData, width, height, y_size, uv_size, total_size);
            mSurveillanceFrame.eventData = metaData;
            cachedFrame++;
            mSurveillanceFrame.eventData.print();
        }
#ifdef ENABLE_CLASSIFICATION
        NormalizationParams SurveillanceSystem::getNormalizationParams(TensorFormatSettings settings)
        {
            NormalizationParams params{};
            params.inputHeight = settings.inputHeight;
            params.inputWidth = settings.inputWidth;
            params.lBound = settings.lBound;
            params.uBound = settings.uBound;
            params.noOfChannels = settings.noOfChannels;
            params.scale = settings.scale;
            params.zeroPoint = settings.zeroPoint;
            return params;
        }
        void SurveillanceSystem::catcheFrameForMotionClassification(const MotionEventMetadata &metaData, size_t width, size_t height, size_t y_size, size_t uv_size, size_t total_size)
        {
            LOG_DEBUG("Processing metadata for motion classification");
            mObjectClassificationFrame.reset();
            catcheFrame(mObjectClassificationFrame, metaData, width, height, y_size, uv_size, total_size);
            mObjectClassificationFrame.mDeleveryUnionBox = metaData.deliveryUnionBox;
            mObjectClassificationFrame.mObjectBoxes = metaData.getNormalizedBoundingBox();
        }

        std::optional<BoxPrediction> SurveillanceSystem::processInput(const std::shared_ptr<uint8_t[]> &modelInput, ObjectType type)
        {
            LOG_INFO("modelInput(" << static_cast<void *>(modelInput.get()) << ")");

            try
            {
                switch (type)
                {
                case PERSON:
                {
                    DetectionOutput modelOutput = mPersonClassifier->RunObjectClassifier(modelInput.get(), mPersonModelParams.inputWidth, mPersonModelParams.inputHeight);
                    return processOutput(modelOutput.predictions, type);
                }
                case DELIVERY:
                {
                    DetectionOutput modelOutput = mDeliveryClassifier->RunObjectClassifier(modelInput.get(), mDeliveryModelParams.inputWidth, mDeliveryModelParams.inputHeight);
                    return processOutput(modelOutput.predictions, type);
                }
                default:
                    LOG_ERROR("Invalid ObjectType provided");
                    return std::nullopt;
                }
            }
            catch (const std::exception &e)
            {
                LOG_ERROR("Exception during processInput: " << e.what());
                return std::nullopt;
            }
            catch (...)
            {
                LOG_ERROR("Unknown error occurred during processInput");
                return std::nullopt;
            }
        }

        std::optional<BoxPrediction> SurveillanceSystem::processOutput(const std::vector<BoxPrediction> &predictions, ObjectType type)
        {
            if (predictions.empty())
            {
                LOG_ERROR("mDetection failed or no output returned.");
                return std::nullopt;
            }

            for (const BoxPrediction &prediction : predictions)
            {
                LOG_INFO("BoxPrediction: "
                         << "y_min=" << prediction.y_min << ", "
                         << "x_min=" << prediction.x_min << ", "
                         << "y_max=" << prediction.y_max << ", "
                         << "x_max=" << prediction.x_max << ", "
                         << "confidence=" << prediction.confidence);
            }

            switch (type)
            {
            case PERSON:
            {
                auto objectBoxes = mObjectClassificationFrame.mObjectBoxes;
                PredictionProcessor processor(objectBoxes, mROI);
                auto prediction = processor.processOutput(predictions);
                float confidenceThreshold = 0.60;

                if (prediction.has_value())
                {
                    if (prediction.value().confidence >= confidenceThreshold)
                    {
                        LOG_INFO("Person Detection above confidence threshold.");
                        return prediction;
                    }
                    LOG_INFO("Person Detection below confidence threshold.");
                }
            }
            break;
            case DELIVERY:
            {
                float confidenceThreshold = 0.87;
                BoxPrediction prediction = ObjectClassifier::findHighestScoredDetection(predictions);
                if (prediction.confidence >= confidenceThreshold)
                {
                    LOG_INFO("Delivery Detection above confidence threshold.");
                    return prediction;
                }
                LOG_INFO("Delivery Detection below confidence threshold.");
            }
            default:
                break;
            }
            return std::nullopt;

#if 0
            // Let's say each detection outputs 6 values: [class_id, score, x_min, y_min, x_max, y_max]
            int num_values_per_detection = 6;
            int num_detections = modelOutput.size();

            for (int i = 0; i < num_detections; ++i)
            {
                int offset = i * num_values_per_detection;
                float x_min = modelOutput[offset];
                float y_min = modelOutput[offset + 1];
                float x_max = modelOutput[offset + 2];
                float y_max = modelOutput[offset + 3];
                float class_id = modelOutput[offset + 4];
                float score = modelOutput[offset + 5];

                LOG_INFO("Detection " << i + 1 << ": Class ID = " << class_id
                                      << ", Score = " << score
                                      << ", BBox = [" << x_min << ", " << y_min << ", " << x_max << ", " << y_max << "]");

                if (score > m_threshold)
                {
                    LOG_INFO("High confidence detection above threshold.");
                }
                else
                {
                    LOG_INFO("Detection below confidence threshold.");
                }
            }
#endif
        }
        void SurveillanceSystem::classifyMotionObjects()
        {
            auto lastActivityTime = high_resolution_clock::now();
            while (keepRunning)
            {
                LOG_INFO("Waiting for the next notification");
                // Wait for a new frame to be cached
                lastActivityTime = high_resolution_clock::now();
                {
                    unique_lock<mutex> lock(mResourceMutex);
                    cvClassify.wait(lock, [this]
                                    { return motionDetected.load(); });
                    motionDetected = false;
                }
                auto now = high_resolution_clock::now();
                LOG_INFO("Starting the object classification (" << std::boolalpha << classifyObj << ") after " << (duration_cast<seconds>(now - lastActivityTime)).count() << " secs of inactivity");
                milliseconds sleepDuration(1000);
                while (classifyObj)
                {
                    sleepDuration = milliseconds(1000);
                    lastActivityTime = high_resolution_clock::now();
                    {
                        std::lock_guard<std::mutex> resourceLock(mResourceMutex);
                        if (mObjectClassificationFrame.isCached)
                        {
                            processFrameForPerson();
                        }

                    } // Unlock as soon as possible
                    auto timeTaken = duration_cast<milliseconds>(high_resolution_clock::now() - lastActivityTime);
                    LOG_INFO("Time taken for the person detection " << timeTaken.count() << " msecs");
                    sleepDuration -= timeTaken;
                    if (sleepDuration.count() < 0)
                    {
                        sleepDuration = milliseconds(0);
                    }
                    this_thread::sleep_for(sleepDuration);
                }
                LOG_INFO("Done with object calssification(Person) " << std::boolalpha << classifyObj << " It took " << duration_cast<seconds>(high_resolution_clock::now() - now).count() << " secs");
                processFrameForDelivery();
            }
        }
        void SurveillanceSystem::cacheFrameForDelivery(uint8_t *yBuffer, BoxPrediction predictedPerson)
        {
            auto rawInput = mCameraFrameHandler->convertAndResize(yBuffer, mObjectClassificationFrame.width, mObjectClassificationFrame.height, mDeliveryModelParams.inputWidth, mDeliveryModelParams.inputHeight, &mObjectClassificationFrame.mDeleveryUnionBox);
            if (rawInput)
            {
                // std::shared_ptr<uint8_t[]> dModelInput(rawInput); // Properly managing memory
                m_rb->add(ModelData(std::move(rawInput), predictedPerson.confidence));
                LOG_INFO("Caching for delivery: " << static_cast<void *>(yBuffer));
            }
        }
        void SurveillanceSystem::processFrameForPerson()
        {
            LOG_INFO("Processing cached frame for person detection!");
            if (!mObjectClassificationFrame.isEmpty())
            {
                uint8_t *yBuffer = mObjectClassificationFrame.getBuffer();
                if (yBuffer)
                {
                    auto modelInput = mCameraFrameHandler->resizeNormalizeQuantize(yBuffer, mObjectClassificationFrame.width, mObjectClassificationFrame.height, mPersonModelParams, &mObjectClassificationFrame.mDeleveryUnionBox);
                    if (modelInput)
                    {
                        processedFrame++;
                        auto bestPrediction = processInput(modelInput, PERSON);
                        if (bestPrediction)
                        {
                            cacheFrameForDelivery(yBuffer, *bestPrediction);
                        }
                        // delete[] modelInput;
                    }
                }
            }
            else
            {
                LOG_INFO("Empty Buffer");
            }
            LOG_INFO("Processing cached frame complete.");
        }
        void SurveillanceSystem::processFrameForDelivery()
        {
            int count = 0;
            bool isStore = false;
            struct stat statbuf;
            if (stat("/tmp/.store", &statbuf) == 0)
            {
                LOG_INFO("Frame will be stored in jpg format for debugging");
                isStore = true;
            }
            for (const auto &data : m_rb->getBuffer())
            {
                if (data.modelInput)
                {
                    auto bestPrediction = processInput(data.modelInput, DELIVERY);
                    struct stat statbuf;
                    if (isStore)
                    {
                        if (count < 5)
                        {
                            auto now = std::chrono::high_resolution_clock::now();
                            auto tend = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
                            std::string output_image_path = "/opt/image_" + std::to_string(tend) + ".jpg";
                            mCameraFrameHandler->saveRGBBufferAsJPEG(data.modelInput.get(), mDeliveryModelParams.inputWidth, mDeliveryModelParams.inputHeight, output_image_path);
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
        }
#endif
    }

}
