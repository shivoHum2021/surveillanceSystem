#ifndef MOTION_EVENT_METADATA_H
#define MOTION_EVENT_METADATA_H
#include <rtMessage.h>
#include <rtConnection.h>
#include <rtLog.h>
#include <cstring> // For std::memset
#include <string>  // For std::string
#include <vector>
// Define the UPPER_LIMIT_BLOB_BB and INVALID_BBOX_ORD appropriately
constexpr int UPPER_LIMIT_BLOB_BB = 5;
constexpr int INVALID_BBOX_ORD = -1;
// #define ENABLE_CLASSIFICATION

/**
 * @struct Point
 * @brief Structure to represent a point with x and y coordinates.
 */
struct Point
{
    float x; /**< x-coordinate of the point. */
    float y; /**< y-coordinate of the point. */
    Point(float x, float y) : x(x), y(y) {}
};

/**
 * @struct ROI
 * @brief Structure to represent a region of interest as a list of points.
 */
struct ROI
{
    std::vector<Point> coordinates; /**< Vector of points representing the ROI polygon. */
};

struct CropSize
{
    int width;
    int height;
    CropSize() : width(0), height(0) {}
    CropSize(int width, int height) : width(width), height(height) {}
};

struct ScalingParams
{
    double scalingFactor;
    Point point2f;
    CropSize size;
    ScalingParams() : scalingFactor(1.0), point2f(), size() {}
    ScalingParams(double scalingFactor, Point point2f.CropSize size) : scalingFactor(scalingFactor), point2f(point2f), size(size) {}
};
/**
 * @struct NormalizedBoundingBox
 * @brief Structure to represent a normalized bounding box.
 */
struct NormalizedBoundingBox
{
    float x_min; /**< Minimum x-coordinate of the bounding box. */
    float y_min; /**< Minimum y-coordinate of the bounding box. */
    float x_max; /**< Maximum x-coordinate of the bounding box. */
    float y_max; /**< Maximum y-coordinate of the bounding box. */
    NormalizedBoundingBox() : x_min(0), y_min(0), x_max(0), y_max(0) {}

    /**
     * @brief Check if the bounding box is empty.
     * @return True if the bounding box is empty, false otherwise.
     */
    bool isEmpty() const
    {
        return x_min == 0 && y_min == 0 && x_max == 0 && y_max == 0;
    }
};
/**
 * @struct BoundingBox
 * @brief Structure to represent a absolute bounding box.
 */
struct BoundingBox
{
    int boundingBoxXOrd;
    int boundingBoxYOrd;
    int boundingBoxWidth;
    int boundingBoxHeight;
    BoundingBox() : boundingBoxXOrd(0), boundingBoxYOrd(0), boundingBoxWidth(0), boundingBoxHeight(0) {}
    /**
     * @brief Check if the bounding box is empty.
     * @return True if the bounding box is empty, false otherwise.
     */
    bool isEmpty() const
    {
        return boundingBoxWidth == 0 || boundingBoxHeight == 0;
    }
};

class MotionEventMetadata
{
public:
    MotionEventMetadata();
    static void parseMessage(MotionEventMetadata *smInfo, const rtMessage m);
    MotionEventMetadata &operator=(const MotionEventMetadata &other);
    void print() const;
    void reset();

    char const *motionFramePTS;
    int32_t event_type;
    uint64_t tsDelta;
    double motionScore;
    BoundingBox deliveryUnionBox;
    BoundingBox unionBox;
    BoundingBox objectBoxs[UPPER_LIMIT_BLOB_BB];
    char const *motionEventTime;
    std::vector<NormalizedBoundingBox> getNormalizedBoundingBox(float boxFrameWidth = 320.0f, float boxFrameHeight = 240.0f) const;
};

#endif // MOTION_EVENT_METADATA_H
