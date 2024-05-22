#include "ThumbnailGenerater.hpp"
#include "Logger.hpp"
#include <fstream> // For file operations
#include <sstream> // For string stream operations
#include <map>
namespace camera
{
    namespace camera_ml
    {
        ThumbnailGenerater::ThumbnailGenerater(CameraFrameHandler *cameraFrameHandler, const std::string &configFile) : mCameraFrameHandler(cameraFrameHandler)
        {
            // Load configuration from the specified file
            if (!loadConfig(configFile))
            {
                std::cerr << "Failed to load config from: " << configFile << std::endl;
            }
            keepRunning = true;
            readyToUpload = false;
        }

        void ThumbnailGenerater::generateThumbnail(PayLoadMetaData payLoadMetaData)
        {
            {
                std::lock_guard<std::mutex> lock(mUploadMutex);
                readyToUpload = true;
            }
            mUploadCV.notify_one();
        }
        void ThumbnailGenerater::createPayLoad(uint8_t *raw, int inputWidth, int inputHeight, int newWidth, int newHeight, MotionEventMetadata metaData, std::string &clipName)
        {
            PayLoadMetaData payload = PayLoadMetaData();
            ScalingParams params = mCameraFrameHandler->convertAndStore(raw, inputWidth, inputHeight, newWidth, newHeight, clipName, &metaData.unionBox);
            payload.motionTime = metaData.motionEventTime;
            
            payload.unionBox.boundingBoxXOrd = static_cast<int>(metaData.unionBox.boundingBoxXOrd/params.scaleFactor);
            payload.unionBox.boundingBoxYOrd = static_cast<int>(metaData.unionBox.boundingBoxYOrd/params.scaleFactor);
            payload.unionBox.boundingBoxWidth = static_cast<int>(metaData.unionBox.boundingBoxWidth/params.scaleFactor);
            payload.unionBox.boundingBoxHeight = static_cast<int>(metaData.unionBox.boundingBoxHeight/params.scaleFactor);
            BoundingBox relativeBBox = getRelativeBoundingBox(payload.unionBox,params);
            payload.unionBox.boundingBoxXOrd = relativeBBox.boundingBoxXOrd;
            payload.unionBox.boundingBoxYOrd = relativeBBox.boundingBoxYOrd;
            payload.unionBox.boundingBoxWidth = relativeBBox.boundingBoxWidth;
            payload.unionBox.boundingBoxHeight = relativeBBox.boundingBoxHeight;

            for (int i = 0; i < UPPER_LIMIT_BLOB_BB; i++)
            {
                payload.objectBoxes[i].boundingBoxXOrd = static_cast<int>(metaData.objectBoxs[i].boundingBoxXOrd/params.scaleFactor);
                payload.objectBoxes[i].boundingBoxYOrd = static_cast<int>(metaData.objectBoxs[i].boundingBoxYOrd/params.scaleFactor);
                payload.objectBoxes[i].boundingBoxWidth = static_cast<int>(metaData.objectBoxs[i].boundingBoxWidth/params.scaleFactor);
                payload.objectBoxes[i].boundingBoxHeight = static_cast<int>(metaData.objectBoxs[i].boundingBoxHeight/params.scaleFactor);
                relativeBBox = relatigetRelativeBoundingBox(payload.objectBoxes[i],params);
                payload.objectBoxes[i].boundingBoxXOrd = relativeBBox.boundingBoxXOrd;
                payload.objectBoxes[i].boundingBoxYOrd = relativeBBox.boundingBoxYOrd;
                payload.objectBoxes[i].boundingBoxWidth = relativeBBox.boundingBoxWidth;
                payload.objectBoxes[i].boundingBoxHeight = relativeBBox.boundingBoxHeight;
            }
            payload.croppedBoundingBox.boundingBoxXOrd = static_cast<int>((params.point2f.x - (params.size.width/2)));
            payload.croppedBoundingBox.boundingBoxYOrd = static_cast<int>((params.point2f.y - (params.size.height/2)));
            payload.croppedBoundingBox.boundingBoxWidth = params.size.width;
            payload.croppedBoundingBox.boundingBoxHeight = params.size.height;
            payload.fileName = clipName;
        }
        void ThumbnailGenerater::createPayLoad(MotionEventMetadata metaData, uint8_t *raw, std::string &clipName)
        {

            payload.tsDelta = metaData.tsDelta;
        }
        void ThumbnailGenerater::start()
        {
            upLoadThread = std::thread(&ThumbnailGenerater::upload, this);
            upLoadThread.detach();
        }
        uint64_t ThumbnailGenerater::getLastUploadTime()
        {
            return mLastUploadTime;
        }

        uint32_t ThumbnailGenerater::getQuiteTime()
        {
            return mQuiteInterval;
        }

        bool ThumbnailGenerater::loadConfig(const std::string &configFile)
        {
            std::ifstream file(configFile);
            if (!file.is_open())
            {
                return false;
            }
            std::map<std::string, std::string> properties;
            std::string line;

            while (std::getline(file, line))
            {
                std::istringstream iss(line);
                std::string key, value;

                if (std::getline(iss, key, '=') && std::getline(iss, value))
                {
                    properties[key] = value;
                }
            }

            file.close();

            // Example of accessing the properties
            for (const auto &kv : properties)
            {
                std::cout << kv.first << " = " << kv.second << std::endl;
            }

            // Access specific properties
            int enabled = std::stoi(properties["enabled"]);
            isEnabled = (enabled != 0);
            mHeight = std::stoi(properties["height"]);
            mWidth = std::stoi(properties["width"]);
            mQuality = std::stoi(properties["quality"]);
            mUploadUrl = properties["url"];
            mAuth = properties["auth"];
            mQuiteInterval = 120;
            // Output parsed values
            LOG_INFO("Enabled: " << std::boolalpha << isEnabled);
            LOG_INFO("Height: " << mHeight);
            LOG_INFO("Width: " << mWidth);
            LOG_INFO("Quality: " << mQuality);
            LOG_INFO("URL: " << maskString(mUploadUrl));
            LOG_INFO("Auth: " << maskString(mAuth));
            return true;
        }

        void ThumbnailGenerater::upload()
        {
            while (keepRunning)
            {
                {
                    std::unique_lock<std::mutex> lock(mUploadMutex);
                    mUploadCV.wait(lock, [this]
                                   { return readyToUpload.load(); });
                    readyToUpload = false;
                }
            }
        }
        BoundingBox ThumbnailGenerater::getRelativeBoundingBox(BoundingBox boundRect,ScalingParams params)
        {
            BoundingBox newBBox; // to store the new bounding box co-ordinate
            // to find the relative x-ordinate and width of the bounding box in the smart thumbnail image
            if (boundRect.boundingBoxWidth >= params.size.width)
            {
                newBBox.boundingBoxXOrd = 0;
                newBBox.boundingBoxWidth = params.size.width; // restrict the width of the relative bounding box to width of the final cropped image
            }
            else
            {
                float deltaX = params.point2f.x - boundRect.boundingBoxXOrd;
                newBBox.boundingBoxXOrd = static_cast<int>(params.size.width / 2 - deltaX);
                newBBox.boundingBoxWidth = boundRect.boundingBoxWidth;
            }
            // to find the relative y-ordinate and height of the bounding box in the smart thumbnail image
            if (boundRect.boundingBoxHeight >= params.size.height)
            {
                newBBox.boundingBoxYOrd = 0;
                newBBox.boundingBoxHeight = params.size.height; // restrict the height of the relative bounding box to height of the final cropped image
            }
            else
            {
                float deltaY = params.point2f.y - boundRect.boundingBoxYOrd;
                newBBox.boundingBoxYOrd = static_cast<int>(params.size.height / 2 - deltaY);
                newBBox.boundingBoxHeight = boundRect.boundingBoxHeight;
            }
            return newBBox;
        }
    }
}
