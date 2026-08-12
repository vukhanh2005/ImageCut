#ifndef IMAGECUT_COMPOSITOR_H
#define IMAGECUT_COMPOSITOR_H

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <memory>
#include "core/ImageDocument.h"
#include "core/Layer.h"

namespace ImageCut {
namespace Processing {

class Compositor {
public:
    static cv::Mat compositeDocument(const Core::ImageDocument& doc, bool previewMode = true, bool fastDrag = false);
    static cv::Mat renderSingleLayer(const Core::Layer& lyr);
    static cv::Mat renderTextLayer(const Core::Layer& lyr);
    static cv::Mat renderShapeLayer(const Core::Layer& lyr);

    static bool transformLayerToCanvasRoi(
        const cv::Mat& layerRgba,
        const Core::Layer& lyr,
        int canvasW,
        int canvasH,
        bool fastDrag,
        cv::Mat& patchRgba,
        cv::Rect& bbox
    );

    static void blendLayerOntoCanvasRoi(
        cv::Mat& canvasAcc,
        const cv::Mat& patchRgba,
        const cv::Rect& bbox,
        double opacity,
        const QString& blendMode
    );

    static void blendFast8U(
        cv::Mat& canvasRgba,
        const cv::Mat& patchRgba,
        const cv::Rect& bbox,
        double opacity
    );

    static cv::Mat generateCanvasBackground(const Core::ImageDocument& doc, int h, int w);
};

} // namespace Processing
} // namespace ImageCut

#endif // IMAGECUT_COMPOSITOR_H
