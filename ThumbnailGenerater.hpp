#ifndef __THUMBNAILGENERATER_H__
#define __THUMBNAILGENERATER_H__
#include "CameraFrameHandler.hpp"
#include "MotionEventMetadata.hpp"
#include <cstdint>
#include <iostream>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <atomic>
constexpr int CONFIG_STRING_MAX = 256;
namespace camera
{
    namespace camera_ml
    {
        struct PayLoadMetaData
        {
            std::string fileName = "";
            uint64_t motionEventTime;
            uint64_t tsDelta;
#ifdef ENABLE_DING
            uint64_t dingtstamp;
#endif
            uint64_t motionTime;
            char motionLog[CONFIG_STRING_MAX];
            char doiMotionLog[CONFIG_STRING_MAX];
            BoundingBox objectBoxes[UPPER_LIMIT_BLOB_BB];
            BoundingBox unionBox; 
            BoundingBox croppedBoundingBox;     
            bool isPayLoadReady = false;
            bool isInitiated = false;
#ifdef ENABLE_CLASSIFICATION
            json_t *detectionResult;
            bool deliveryDetected;
#endif
            // Constructor to initialize with default values
            PayLoadMetaData()
            {
                reset();
            }

            // Method to reset all members to their default values
            void reset()
            {
                fileName.clear();
                motionEventTime = 0;
#ifdef _HAS_DING_
                dingtstamp = 0;
#endif
#ifdef _OBJ_DETECTION_
                // Reset detectionResult if necessary
                if (detectionResult) 
                {
                     freeJson(detectionResult);
                }
                detectionResult = nullptr;
                deliveryDetected = false;
#endif
                motionTime = 0;
                memset(motionLog, 0, CONFIG_STRING_MAX);
                memset(doiMotionLog, 0, CONFIG_STRING_MAX);
                for (int i = 0; i < UPPER_LIMIT_BLOB_BB; ++i)
                {
                    objectBoxs[i] = BoundingBox();
                }
                unionBox = BoundingBox();
                croppedBoundingBox = BoundingBox();
                tsDelta = 0;
            }
        };
        
        class ThumbnailGenerater
        {
        public:
            ThumbnailGenerater(CameraFrameHandler *cameraFrameHandler, const std::string &configFile);
            void generateThumbnail(PayLoadMetaData payLoadMetaData);
            void createPayLoad(uint8_t *raw,int inputWidth,int inputHeight,int newWidth,int newHeight,MotionEventMetadata metaData,std::string &clipName);
            void start();
            uint64_t getLastUploadTime();
            uint32_t getQuiteTime();

        private:
            bool loadConfig(const std::string &configFile);
            void upload();
            BoundingBox getRelativeBoundingBox(BoundingBox box,ScalingParams params);
            CameraFrameHandler *mCameraFrameHandler;
            std::thread upLoadThread;
            std::atomic<bool> keepRunning;
            std::mutex mUploadMutex;
            std::atomic<bool> readyToUpload;
            std::condition_variable mUploadCV;
            uint32_t mQuiteInterval;
            uint32_t mLastUploadTime;
            std::string mUploadUrl;
            std::string mAuth;
            bool isEnabled;
            uint32_t mHeight;
            uint32_t mWidth;
            uint32_t mQuality;
        };
    }
    inline std::string maskString(const std::string &input)
    {
        return std::string(input.length(), '*');
    }
}
#endif // __THUMBNAILGENERATER_H__