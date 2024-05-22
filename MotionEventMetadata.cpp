#include "MotionEventMetadata.hpp"
#include "Logger.hpp"
#include <cassert>
#include <sstream>

MotionEventMetadata::MotionEventMetadata()
    : motionFramePTS(nullptr), event_type(0), motionScore(0.0), motionEventTime(nullptr)
{
    memset(&unionBox, 0, sizeof(unionBox));
    memset(&deliveryUnionBox, 0, sizeof(deliveryUnionBox));
    for (auto &box : objectBoxs)
    {
        box.boundingBoxXOrd = INVALID_BBOX_ORD;
        box.boundingBoxYOrd = INVALID_BBOX_ORD;
        box.boundingBoxWidth = INVALID_BBOX_ORD;
        box.boundingBoxHeight = INVALID_BBOX_ORD;
    }
}
// Custom Copy Assignment Operator for MotionEventMetadata
MotionEventMetadata &MotionEventMetadata::operator=(const MotionEventMetadata &other)
{
    if (this != &other)
    {
        motionFramePTS = other.motionFramePTS;
        event_type = other.event_type;
        motionScore = other.motionScore;
        deliveryUnionBox = other.deliveryUnionBox;
        unionBox = other.unionBox;
        for (int i = 0; i < UPPER_LIMIT_BLOB_BB; ++i)
        {
            objectBoxs[i] = other.objectBoxs[i];
        }
        motionEventTime = other.motionEventTime;
    }
    return *this;
}

// Define the reset method
void MotionEventMetadata::reset()
{
    motionFramePTS = nullptr;
    event_type = 0;
    motionScore = 0.0;
    deliveryUnionBox = BoundingBox(); // Reset to default constructed BoundingBox
    unionBox = BoundingBox();         // Reset to default constructed BoundingBox

    // Reset all objectBoxs to default constructed BoundingBox
    for (int i = 0; i < UPPER_LIMIT_BLOB_BB; ++i)
    {
        objectBoxs[i] = BoundingBox();
    }

    motionEventTime = nullptr;
}
/**
 * @brief Normalize the bounding box coordinates to the range [0, 1].
 * @param box The bounding box to normalize.
 * @return The normalized bounding box.
 */
std::vector<NormalizedBoundingBox> MotionEventMetadata::getNormalizedBoundingBox(float boxFrameWidth, float boxFrameHeight) const
{
    std::vector<NormalizedBoundingBox> v;

    for (const auto &box : objectBoxs)
    {
        if (box.boundingBoxXOrd == -1)
        {
            continue; // Skip invalid bounding boxes
        }

        NormalizedBoundingBox normalizedBox;
        normalizedBox.x_min = std::clamp(static_cast<float>(box.boundingBoxXOrd) / boxFrameWidth, 0.0f, 1.0f);
        normalizedBox.y_min = std::clamp(static_cast<float>(box.boundingBoxYOrd) / boxFrameHeight, 0.0f, 1.0f);
        normalizedBox.x_max = std::clamp(static_cast<float>(box.boundingBoxXOrd + box.boundingBoxWidth) / boxFrameWidth, 0.0f, 1.0f);
        normalizedBox.y_max = std::clamp(static_cast<float>(box.boundingBoxYOrd + box.boundingBoxHeight) / boxFrameHeight, 0.0f, 1.0f);

        v.push_back(normalizedBox);
    }

    return v;
}

void MotionEventMetadata::parseMessage(MotionEventMetadata *eventMetaData, const rtMessage m)
{
    assert(eventMetaData != nullptr && "eventMetaData should not be NULL");

    rtMessage_GetString(m, "timestamp", &eventMetaData->motionFramePTS);
    rtMessage_GetInt32(m, "event_type", &eventMetaData->event_type);
    rtMessage_GetDouble(m, "motionScore", &eventMetaData->motionScore);
    rtMessage_GetString(m, "currentTime", &eventMetaData->motionEventTime);

    // Parsing unionBox
    rtMessage_GetInt32(m, "boundingBoxXOrd", &eventMetaData->unionBox.boundingBoxXOrd);
    rtMessage_GetInt32(m, "boundingBoxYOrd", &eventMetaData->unionBox.boundingBoxYOrd);
    rtMessage_GetInt32(m, "boundingBoxWidth", &eventMetaData->unionBox.boundingBoxWidth);
    rtMessage_GetInt32(m, "boundingBoxHeight", &eventMetaData->unionBox.boundingBoxHeight);

#ifdef ENABLE_CLASSIFICATION
    // Parsing deliveryUnionBox
    rtMessage_GetInt32(m, "d_boundingBoxXOrd", &eventMetaData->deliveryUnionBox.boundingBoxXOrd);
    rtMessage_GetInt32(m, "d_boundingBoxYOrd", &eventMetaData->deliveryUnionBox.boundingBoxYOrd);
    rtMessage_GetInt32(m, "d_boundingBoxWidth", &eventMetaData->deliveryUnionBox.boundingBoxWidth);
    rtMessage_GetInt32(m, "d_boundingBoxHeight", &eventMetaData->deliveryUnionBox.boundingBoxHeight);
#endif

    // Parsing objectBoxs
    int32_t len = 0;
    rtMessage_GetArrayLength(m, "objectBoxs", &len);
    for (int32_t i = 0; i < len; ++i)
    {
        rtMessage bbox;
        rtMessage_GetMessageItem(m, "objectBoxs", i, &bbox);
        rtMessage_GetInt32(bbox, "boundingBoxXOrd", &eventMetaData->objectBoxs[i].boundingBoxXOrd);
        rtMessage_GetInt32(bbox, "boundingBoxYOrd", &eventMetaData->objectBoxs[i].boundingBoxYOrd);
        rtMessage_GetInt32(bbox, "boundingBoxWidth", &eventMetaData->objectBoxs[i].boundingBoxWidth);
        rtMessage_GetInt32(bbox, "boundingBoxHeight", &eventMetaData->objectBoxs[i].boundingBoxHeight);
        rtMessage_Release(bbox);
    }
}

void MotionEventMetadata::print() const
{
    std::ostringstream logMsg;

    logMsg << "MotionEventMetadata Log: \n";
    logMsg << "Motion Frame PTS: " << (motionFramePTS ? motionFramePTS : "null") << "\n";
    logMsg << "Event Type: " << event_type << "\n";
    logMsg << "Motion Score: " << motionScore << "\n";
    logMsg << "Motion Event Time: " << (motionEventTime ? motionEventTime : "null") << "\n";

    logMsg << "Union Box: " << unionBox.boundingBoxXOrd << ", "
           << unionBox.boundingBoxYOrd << ", "
           << unionBox.boundingBoxWidth << ", "
           << unionBox.boundingBoxHeight << "\n";

#ifdef ENABLE_CLASSIFICATION
    logMsg << "Delivery Union Box: " << deliveryUnionBox.boundingBoxXOrd << ", "
           << deliveryUnionBox.boundingBoxYOrd << ", "
           << deliveryUnionBox.boundingBoxWidth << ", "
           << deliveryUnionBox.boundingBoxHeight << "\n";
#endif

    logMsg << "Object Boxes(motion blobs):\n";
    for (int i = 0; i < UPPER_LIMIT_BLOB_BB; i++)
    {
        logMsg << "  Box " << i << ": " << objectBoxs[i].boundingBoxXOrd << ", "
               << objectBoxs[i].boundingBoxYOrd << ", "
               << objectBoxs[i].boundingBoxWidth << ", "
               << objectBoxs[i].boundingBoxHeight << "\n";
    }

    LOG_INFO(" " << logMsg.str().c_str());
}
