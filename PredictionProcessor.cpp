#include "PredictionProcessor.hpp"
#include <algorithm>
#include <iostream>

/**
 * @brief Constructor for PredictionProcessor.
 * @param objectBoxes Vector of bounding boxes.
 * @param roi Region of interest as a polygon.
 * @param isNormalized Flag indicating if the coordinates are normalized.
 * @param frameWidth Width of the frame (only used if coordinates are not normalized).
 * @param frameHeight Height of the frame (only used if coordinates are not normalized).
 */
PredictionProcessor::PredictionProcessor(const std::vector<NormalizedBoundingBox> &objectBoxes, const ROI &roi)
    : mObjectBoxes(objectBoxes), mROI(roi) {}

/**
 * @brief Check if a BoxPrediction is inside a given bounding box.
 * @param box The BoxPrediction to check.
 * @param boundingBox The bounding box to check against.
 * @return True if the BoxPrediction is inside the bounding box, false otherwise.
 */
bool PredictionProcessor::isInsideBoundingBox(const BoxPrediction &box, const NormalizedBoundingBox &boundingBox) const
{
    return (box.x_min >= boundingBox.x_min &&
            box.y_min >= boundingBox.y_min &&
            box.x_max <= boundingBox.x_max &&
            box.y_max <= boundingBox.y_max);
}

/**
 * @brief Check if a point is inside a polygon using the ray-casting algorithm.
 * @param x The x-coordinate of the point.
 * @param y The y-coordinate of the point.
 * @param polygon The polygon to check against.
 * @return True if the point is inside the polygon, false otherwise.
 */
bool PredictionProcessor::isPointInPolygon(float x, float y, const std::vector<Point> &polygon) const
{
    bool inside = false;
    int n = polygon.size();
    for (int i = 0, j = n - 1; i < n; j = i++)
    {
        if (((polygon[i].y > y) != (polygon[j].y > y)) &&
            (x < (polygon[j].x - polygon[i].x) * (y - polygon[i].y) / (polygon[j].y - polygon[i].y) + polygon[i].x))
        {
            inside = !inside;
        }
    }
    return inside;
}

/**
 * @brief Check if a BoxPrediction is inside the ROI polygon.
 * @param box The BoxPrediction to check.
 * @param roi The ROI polygon to check against.
 * @return True if the BoxPrediction is inside the ROI, false otherwise.
 */
bool PredictionProcessor::isInsideROI(const BoxPrediction &box) const
{
    if(mROI.coordinates.empty())
    {
        LOG_INFO("No ROI filter");
        return true;
    }
    // Check all four corners of the bounding box
    return (isPointInPolygon(box.x_min, box.y_min, mROI.coordinates) ||
            isPointInPolygon(box.x_min, box.y_max, mROI.coordinates) ||
            isPointInPolygon(box.x_max, box.y_min, mROI.coordinates) ||
            isPointInPolygon(box.x_max, box.y_max, mROI.coordinates));
}

/**
 * @brief Check if a BoxPrediction is inside any of the bounding boxes or the ROI.
 * @param box The BoxPrediction to check.
 * @return True if the BoxPrediction is inside any bounding box or the ROI, false otherwise.
 */
bool PredictionProcessor::isPredictionInsideAnyBox(const BoxPrediction &box)
{
    for (const auto &bbox : mObjectBoxes)
    {
        if (isInsideBoundingBox(box, bbox))
        {
            return true;
        }
    }
}

/**
 * @brief Process the output predictions and return the best prediction based on object type.
 * @param predictions Vector of predictions.
 * @param type The type of object to check for.
 * @return The best prediction if it meets the criteria, otherwise std::nullopt.
 */
std::optional<BoxPrediction> PredictionProcessor::processOutput(const std::vector<BoxPrediction> &predictions)
{
    if (predictions.empty())
    {
        std::cerr << "mDetection failed or no output returned." << std::endl;
        return std::nullopt;
    }
    for (const auto &prediction : predictions)
    {
        if (isPredictionInsideAnyBox(prediction))
        {
            if(isInsideROI(prediction))
            {
                return prediction;
            }
        }
    }
    return std::nullopt;
}
