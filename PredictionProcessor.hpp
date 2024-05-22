#ifndef PREDICTION_PROCESSOR_HPP
#define PREDICTION_PROCESSOR_HPP
#include "MotionEventMetadata.hpp"
#include "ModelProcessor.hpp"
#include <vector>
#include <optional>

using namespace ::camera;
using namespace camera::camera_ml;
/**
 * @class PredictionProcessor
 * @brief Class to process predictions and check if they fall within specified bounding boxes or ROI.
 */
class PredictionProcessor
{
public:
    /**
     * @brief Constructor for PredictionProcessor.
     * @param objectBoxes Vector of bounding boxes.
     * @param roi Region of interest as a polygon.
     * @param isNormalized Flag indicating if the coordinates are normalized.
     * @param frameWidth Width of the frame (only used if coordinates are not normalized).
     * @param frameHeight Height of the frame (only used if coordinates are not normalized).
     */
    PredictionProcessor(const std::vector<NormalizedBoundingBox> &objectBoxes, const ROI &roi);

    /**
     * @brief Process the output predictions and return the best prediction based on object type.
     * @param predictions Vector of predictions.
     * @param type The type of object to check for.
     * @return The best prediction if it meets the criteria, otherwise std::nullopt.
     */
    std::optional<BoxPrediction> processOutput(const std::vector<BoxPrediction> &predictions);

private:
    std::vector<NormalizedBoundingBox> mObjectBoxes; /**< Vector of bounding boxes to check against. */
    ROI mROI;                              /**< Region of interest as a polygon. */
    /**
     * @brief Check if a BoxPrediction is inside a given bounding box.
     * @param box The BoxPrediction to check.
     * @param boundingBox The bounding box to check against.
     * @return True if the BoxPrediction is inside the bounding box, false otherwise.
     */
    bool isInsideBoundingBox(const BoxPrediction &box, const NormalizedBoundingBox &boundingBox) const;

    /**
     * @brief Check if a point is inside a polygon using the ray-casting algorithm.
     * @param x The x-coordinate of the point.
     * @param y The y-coordinate of the point.
     * @param polygon The polygon to check against.
     * @return True if the point is inside the polygon, false otherwise.
     */
    bool isPointInPolygon(float x, float y, const std::vector<Point> &polygon) const;

    /**
     * @brief Check if a BoxPrediction is inside the ROI polygon.
     * @param box The BoxPrediction to check.
     * @param roi The ROI polygon to check against.
     * @return True if the BoxPrediction is inside the ROI, false otherwise.
     */
    bool isInsideROI(const BoxPrediction &box) const;

    /**
     * @brief Check if a BoxPrediction is inside any of the bounding boxes or the ROI.
     * @param box The BoxPrediction to check.
     * @return True if the BoxPrediction is inside any bounding box or the ROI, false otherwise.
     */
    bool isPredictionInsideAnyBox(const BoxPrediction &box);
};

#endif // PREDICTION_PROCESSOR_HPP
