#include "processing/Compositor.h"
#include "processing/ColorAdjust.h"
#include "core/MaskProcessor.h"
#include "utils/Logger.h"
#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QFont>
#include <QFontMetrics>
#include <QTransform>
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace ImageCut {
namespace Processing {

cv::Mat Compositor::compositeDocument(const Core::ImageDocument& doc, bool previewMode, bool fastDrag) {
    int canvasW = std::max(1, doc.canvasWidth);
    int canvasH = std::max(1, doc.canvasHeight);

    cv::Mat canvasBg = generateCanvasBackground(doc, canvasH, canvasW);

    auto activeLyr = doc.getActiveLayer();
    if (activeLyr && (doc.maskViewMode == "BlackWhite" || doc.maskViewMode == "Alpha" || doc.maskViewMode == "Overlay")) {
        if (!activeLyr->image.empty()) {
            int h = activeLyr->image.rows;
            int w = activeLyr->image.cols;
            cv::Mat mask = !activeLyr->mask.empty() ? activeLyr->mask.clone() : cv::Mat(h, w, CV_8UC1, cv::Scalar(255));

            if (doc.maskViewMode == "BlackWhite" || doc.maskViewMode == "Alpha") {
                cv::Mat bwMask;
                cv::cvtColor(mask, bwMask, cv::COLOR_GRAY2RGBA);
                return bwMask;
            } else if (doc.maskViewMode == "Overlay") {
                cv::Mat fgRgb = ColorAdjust::applyAdjustments(activeLyr->image, activeLyr->brightness, activeLyr->contrast, activeLyr->saturation);
                cv::Mat overlayRgb = fgRgb.clone();

                std::vector<cv::Mat> channels;
                cv::split(overlayRgb, channels);

                cv::Mat invMask;
                mask.convertTo(invMask, CV_32F, 1.0 / 255.0);
                invMask = 1.0 - invMask;

                cv::Mat rF;
                channels[0].convertTo(rF, CV_32F);
                rF = cv::min(rF + invMask * 180.0, 255.0);
                rF.convertTo(channels[0], CV_8U);

                cv::merge(channels, overlayRgb);

                cv::Mat alphaCh(h, w, CV_8UC1, cv::Scalar(255));
                std::vector<cv::Mat> rgbaChs = { channels[0], channels[1], channels[2], alphaCh };
                cv::Mat res;
                cv::merge(rgbaChs, res);
                return res;
            }
        }
    }

    bool allNormal = true;
    for (const auto& lyr : doc.layers) {
        if (lyr && lyr->visible && lyr->blendMode != "Normal") {
            allNormal = false;
            break;
        }
    }

    if (fastDrag || allNormal) {
        cv::Mat canvasAcc = canvasBg.clone();
        for (const auto& lyr : doc.layers) {
            if (!lyr || !lyr->visible || lyr->opacity <= 0.0) continue;

            cv::Mat layerRgba = renderSingleLayer(*lyr);
            if (layerRgba.empty()) continue;

            cv::Mat patchRgba;
            cv::Rect bbox;
            if (!transformLayerToCanvasRoi(layerRgba, *lyr, canvasW, canvasH, fastDrag, patchRgba, bbox)) {
                continue;
            }

            blendFast8U(canvasAcc, patchRgba, bbox, lyr->opacity);
        }
        return canvasAcc;
    }

    cv::Mat canvasAcc;
    canvasBg.convertTo(canvasAcc, CV_32FC4);

    for (const auto& lyr : doc.layers) {
        if (!lyr || !lyr->visible || lyr->opacity <= 0.0) continue;

        cv::Mat layerRgba = renderSingleLayer(*lyr);
        if (layerRgba.empty()) continue;

        cv::Mat patchRgba;
        cv::Rect bbox;
        if (!transformLayerToCanvasRoi(layerRgba, *lyr, canvasW, canvasH, fastDrag, patchRgba, bbox)) {
            continue;
        }

        blendLayerOntoCanvasRoi(canvasAcc, patchRgba, bbox, lyr->opacity, lyr->blendMode);
    }

    cv::Mat clampedAcc;
    cv::min(cv::max(canvasAcc, 0.0), 255.0, clampedAcc);
    cv::Mat result;
    clampedAcc.convertTo(result, CV_8UC4);
    return result;
}

cv::Mat Compositor::renderSingleLayer(const Core::Layer& lyr) {
    if (lyr.layerType == "image") {
        if (lyr.image.empty()) return cv::Mat();

        if (!lyr.dirty && !lyr.cachedRgba.empty()) {
            return lyr.cachedRgba;
        }

        cv::Mat fgRgb = lyr.image.clone();
        int h = fgRgb.rows;
        int w = fgRgb.cols;

        if (lyr.brightness != 0 || lyr.contrast != 0 || lyr.saturation != 0 ||
            lyr.exposure != 0 || lyr.temperature != 0 || lyr.sharpness != 0) {
            fgRgb = ColorAdjust::applyAdjustments(
                fgRgb, lyr.brightness, lyr.contrast, lyr.saturation,
                lyr.exposure, lyr.temperature, lyr.sharpness
            );
        }

        cv::Mat mask = !lyr.mask.empty() ? lyr.mask.clone() : cv::Mat(h, w, CV_8UC1, cv::Scalar(255));

        if (lyr.expandContractVal != 0) {
            mask = Core::MaskProcessor::expandContract(mask, lyr.expandContractVal);
        }
        if (lyr.smoothKernel > 0) {
            mask = Core::MaskProcessor::smooth(mask, lyr.smoothKernel);
        }
        if (lyr.edgeContrast != 1.0) {
            mask = Core::MaskProcessor::adjustEdgeContrast(mask, lyr.edgeContrast);
        }
        if (lyr.featherRadius > 0.0) {
            mask = Core::MaskProcessor::feather(mask, lyr.featherRadius);
        }
        if (lyr.decontaminate) {
            fgRgb = Core::MaskProcessor::decontaminateColors(fgRgb, mask);
        }

        std::vector<cv::Mat> channels;
        if (fgRgb.channels() == 3) {
            cv::split(fgRgb, channels);
            channels.push_back(mask);
        } else if (fgRgb.channels() == 4) {
            cv::split(fgRgb, channels);
            channels[3] = mask;
        }

        cv::Mat rgbaRes;
        cv::merge(channels, rgbaRes);
        lyr.cachedRgba = rgbaRes;
        lyr.dirty = false;
        return rgbaRes;
    } else if (lyr.layerType == "text") {
        return renderTextLayer(lyr);
    } else if (lyr.layerType == "shape") {
        return renderShapeLayer(lyr);
    }
    return cv::Mat();
}

cv::Mat Compositor::renderTextLayer(const Core::Layer& lyr) {
    QString text = lyr.textContent.isEmpty() ? "Sample Text" : lyr.textContent;
    QFont font(lyr.fontFamily.isEmpty() ? "Arial" : lyr.fontFamily, lyr.fontSize);
    font.setBold(lyr.fontBold);
    font.setItalic(lyr.fontItalic);

    QFontMetrics fm(font);
    QRect textRect = fm.boundingRect(text);

    int margin = 30 + (lyr.textHasStroke ? lyr.textStrokeWidth * 2 : 0) + (lyr.textHasShadow ? 20 : 0);
    int w = std::max(60, textRect.width() + margin * 2);
    int h = std::max(40, textRect.height() + margin * 2);

    QImage img(w, h, QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::transparent);

    QPainter painter(&img);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);

    QRectF drawBox(margin, margin, textRect.width(), textRect.height());

    if (lyr.textHasBg) {
        QRectF bgRect = drawBox.adjusted(-10, -5, 10, 5);
        painter.setPen(Qt::NoPen);
        painter.setBrush(lyr.textBgColor);
        painter.drawRoundedRect(bgRect, 8, 8);
    }

    if (lyr.textHasShadow) {
        QPainterPath shadowPath;
        shadowPath.addText(drawBox.left(), drawBox.top() + fm.ascent(), font, text);
        QTransform trans;
        trans.translate(lyr.textShadowOffsetX, lyr.textShadowOffsetY);
        QPainterPath shiftedShadow = trans.map(shadowPath);
        painter.fillPath(shiftedShadow, lyr.textShadowColor);
    }

    QPainterPath path;
    path.addText(drawBox.left(), drawBox.top() + fm.ascent(), font, text);

    if (lyr.textHasStroke && lyr.textStrokeWidth > 0) {
        QPen strokePen(lyr.textStrokeColor, lyr.textStrokeWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        painter.strokePath(path, strokePen);
    }

    painter.fillPath(path, lyr.textColor);
    painter.end();

    QImage swp = img.convertToFormat(QImage::Format_RGBA8888);
    cv::Mat res(h, w, CV_8UC4);
    std::memcpy(res.data, swp.bits(), swp.sizeInBytes());
    return res;
}

cv::Mat Compositor::renderShapeLayer(const Core::Layer& lyr) {
    int w = 240, h = 240;
    QImage img(w, h, QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::transparent);

    QPainter painter(&img);
    painter.setRenderHints(QPainter::Antialiasing);

    int sw = lyr.strokeWidth;
    QPen pen(sw > 0 ? lyr.strokeColor : Qt::NoPen);
    pen.setWidth(sw);
    pen.setJoinStyle(Qt::RoundJoin);
    painter.setPen(pen);
    painter.setBrush(lyr.fillColor);

    QRectF rect(sw / 2.0 + 10, sw / 2.0 + 10, w - sw - 20, h - sw - 20);

    if (lyr.shapeType == "Rectangle") {
        painter.drawRect(rect);
    } else if (lyr.shapeType == "RoundedRectangle") {
        painter.drawRoundedRect(rect, 20, 20);
    } else if (lyr.shapeType == "Circle") {
        painter.drawEllipse(rect);
    } else if (lyr.shapeType == "Arrow") {
        QPainterPath path;
        double cy = rect.center().y();
        double headW = rect.width() * 0.4;
        double tailH = rect.height() * 0.4;

        path.moveTo(rect.left(), cy - tailH / 2);
        path.lineTo(rect.right() - headW, cy - tailH / 2);
        path.lineTo(rect.right() - headW, rect.top());
        path.lineTo(rect.right(), cy);
        path.lineTo(rect.right() - headW, rect.bottom());
        path.lineTo(rect.right() - headW, cy + tailH / 2);
        path.lineTo(rect.left(), cy + tailH / 2);
        path.closeSubpath();
        painter.drawPath(path);
    } else if (lyr.shapeType == "Star") {
        QPainterPath path;
        QPointF center = rect.center();
        double rOuter = std::min(rect.width(), rect.height()) / 2.0;
        double rInner = rOuter * 0.4;
        int points = 5;

        for (int i = 0; i < points * 2; ++i) {
            double r = (i % 2 == 0) ? rOuter : rInner;
            double angle = i * M_PI / points - M_PI / 2.0;
            double x = center.x() + r * std::cos(angle);
            double y = center.y() + r * std::sin(angle);
            if (i == 0) path.moveTo(x, y);
            else path.lineTo(x, y);
        }
        path.closeSubpath();
        painter.drawPath(path);
    } else if (lyr.shapeType == "SpeechBubble") {
        QPainterPath path;
        QRectF body = rect.adjusted(0, 0, 0, -rect.height() * 0.25);
        path.addRoundedRect(body, 15, 15);

        QPainterPath tail;
        tail.moveTo(body.left() + body.width() * 0.25, body.bottom());
        tail.lineTo(body.left() + body.width() * 0.15, rect.bottom());
        tail.lineTo(body.left() + body.width() * 0.45, body.bottom());
        tail.closeSubpath();

        path = path.united(tail);
        painter.drawPath(path);
    }

    painter.end();

    QImage swp = img.convertToFormat(QImage::Format_RGBA8888);
    cv::Mat res(h, w, CV_8UC4);
    std::memcpy(res.data, swp.bits(), swp.sizeInBytes());
    return res;
}

bool Compositor::transformLayerToCanvasRoi(
    const cv::Mat& layerRgba,
    const Core::Layer& lyr,
    int canvasW,
    int canvasH,
    bool fastDrag,
    cv::Mat& patchRgba,
    cv::Rect& bbox
) {
    int h = layerRgba.rows;
    int w = layerRgba.cols;

    cv::Mat img = layerRgba;
    if (lyr.flipH && lyr.flipV) {
        cv::flip(img, img, -1);
    } else if (lyr.flipH) {
        cv::flip(img, img, 1);
    } else if (lyr.flipV) {
        cv::flip(img, img, 0);
    }

    double srcCx = w / 2.0;
    double srcCy = h / 2.0;

    double scaledW = w * lyr.scaleX;
    double scaledH = h * lyr.scaleY;

    double dstCx = lyr.offsetX + scaledW / 2.0;
    double dstCy = lyr.offsetY + scaledH / 2.0;

    double halfW = scaledW / 2.0;
    double halfH = scaledH / 2.0;

    cv::Point2f corners[4] = {
        cv::Point2f(-halfW, -halfH),
        cv::Point2f( halfW, -halfH),
        cv::Point2f( halfW,  halfH),
        cv::Point2f(-halfW,  halfH)
    };

    double rad = lyr.rotation * M_PI / 180.0;
    double cosA = std::cos(rad);
    double sinA = std::sin(rad);

    float minX = 1e9f, maxX = -1e9f, minY = 1e9f, maxY = -1e9f;
    for (int i = 0; i < 4; ++i) {
        float rx = static_cast<float>(dstCx + corners[i].x * cosA - corners[i].y * sinA);
        float ry = static_cast<float>(dstCy + corners[i].x * sinA + corners[i].y * cosA);
        minX = std::min(minX, rx);
        maxX = std::max(maxX, rx);
        minY = std::min(minY, ry);
        maxY = std::max(maxY, ry);
    }

    int xmin = std::max(0, static_cast<int>(std::floor(minX)) - 1);
    int xmax = std::min(canvasW, static_cast<int>(std::ceil(maxX)) + 1);
    int ymin = std::max(0, static_cast<int>(std::floor(minY)) - 1);
    int ymax = std::min(canvasH, static_cast<int>(std::ceil(maxY)) + 1);

    if (xmin >= xmax || ymin >= ymax) return false;

    int boxW = xmax - xmin;
    int boxH = ymax - ymin;

    cv::Mat T1 = (cv::Mat_<double>(3, 3) << 1, 0, -srcCx, 0, 1, -srcCy, 0, 0, 1);
    cv::Mat S  = (cv::Mat_<double>(3, 3) << lyr.scaleX, 0, 0, 0, lyr.scaleY, 0, 0, 0, 1);
    cv::Mat R  = (cv::Mat_<double>(3, 3) << cosA, -sinA, 0, sinA, cosA, 0, 0, 0, 1);
    cv::Mat T2 = (cv::Mat_<double>(3, 3) << 1, 0, dstCx - xmin, 0, 1, dstCy - ymin, 0, 0, 1);

    cv::Mat M3x3 = T2 * R * S * T1;
    cv::Mat M2x3 = M3x3(cv::Rect(0, 0, 3, 2));

    int flags = fastDrag ? cv::INTER_NEAREST : cv::INTER_LINEAR;
    cv::warpAffine(img, patchRgba, M2x3, cv::Size(boxW, boxH), flags, cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0, 0));
    bbox = cv::Rect(xmin, ymin, boxW, boxH);
    return true;
}

void Compositor::blendLayerOntoCanvasRoi(
    cv::Mat& canvasAcc,
    const cv::Mat& patchRgba,
    const cv::Rect& bbox,
    double opacity,
    const QString& blendMode
) {
    cv::Mat roiAcc = canvasAcc(bbox);

    std::vector<cv::Mat> patchChs;
    cv::split(patchRgba, patchChs);

    cv::Mat srcRgb, srcA;
    std::vector<cv::Mat> srcRgbChs = { patchChs[0], patchChs[1], patchChs[2] };
    cv::merge(srcRgbChs, srcRgb);
    srcRgb.convertTo(srcRgb, CV_32F);

    patchChs[3].convertTo(srcA, CV_32F, opacity / 255.0);

    std::vector<cv::Mat> dstChs;
    cv::split(roiAcc, dstChs);

    cv::Mat dstRgb, dstA;
    std::vector<cv::Mat> dstRgbChs = { dstChs[0], dstChs[1], dstChs[2] };
    cv::merge(dstRgbChs, dstRgb);

    dstChs[3].convertTo(dstA, CV_32F, 1.0 / 255.0);

    cv::Mat srcA3;
    cv::merge(std::vector<cv::Mat>{srcA, srcA, srcA}, srcA3);

    cv::Mat dstA3;
    cv::merge(std::vector<cv::Mat>{dstA, dstA, dstA}, dstA3);

    cv::Mat invSrcA3 = 1.0 - srcA3;
    cv::Mat outA = srcA + dstA.mul(1.0 - srcA);

    cv::Mat outA3;
    cv::merge(std::vector<cv::Mat>{outA, outA, outA}, outA3);
    cv::Mat safeOutA3 = outA3.clone();
    safeOutA3.setTo(1.0, safeOutA3 <= 0.0001);

    cv::Mat outRgb;
    if (blendMode == "Normal") {
        outRgb = (srcRgb.mul(srcA3) + dstRgb.mul(dstA3).mul(invSrcA3)) / safeOutA3;
    } else if (blendMode == "Multiply") {
        cv::Mat blendedRgb = srcRgb.mul(dstRgb) / 255.0;
        outRgb = (blendedRgb.mul(srcA3) + dstRgb.mul(dstA3).mul(invSrcA3)) / safeOutA3;
    } else if (blendMode == "Screen") {
        cv::Mat blendedRgb = 255.0 - (255.0 - srcRgb).mul(255.0 - dstRgb) / 255.0;
        outRgb = (blendedRgb.mul(srcA3) + dstRgb.mul(dstA3).mul(invSrcA3)) / safeOutA3;
    } else {
        outRgb = (srcRgb.mul(srcA3) + dstRgb.mul(dstA3).mul(invSrcA3)) / safeOutA3;
    }

    std::vector<cv::Mat> outChs;
    cv::split(outRgb, outChs);
    outChs.push_back(outA * 255.0);
    cv::merge(outChs, roiAcc);
}

void Compositor::blendFast8U(cv::Mat& canvasRgba, const cv::Mat& patchRgba, const cv::Rect& bbox, double opacity) {
    if (patchRgba.empty() || bbox.width <= 0 || bbox.height <= 0) return;

    cv::Mat roi = canvasRgba(bbox);
    int opInt = static_cast<int>(std::clamp(opacity, 0.0, 1.0) * 255.0);
    if (opInt <= 0) return;

    for (int r = 0; r < bbox.height; ++r) {
        const cv::Vec4b* srcPtr = patchRgba.ptr<cv::Vec4b>(r);
        cv::Vec4b* dstPtr = roi.ptr<cv::Vec4b>(r);
        for (int c = 0; c < bbox.width; ++c) {
            const cv::Vec4b& s = srcPtr[c];
            int sa = (s[3] * opInt + 127) / 255;
            if (sa <= 0) continue;

            cv::Vec4b& d = dstPtr[c];
            if (sa >= 255 || d[3] == 0) {
                d[0] = s[0];
                d[1] = s[1];
                d[2] = s[2];
                d[3] = static_cast<uchar>(sa);
            } else {
                int da = d[3];
                int invSa = 255 - sa;
                int outA = sa + (da * invSa + 127) / 255;
                if (outA > 0) {
                    int rB = (s[0] * sa + d[0] * da * invSa / 255) / outA;
                    int gB = (s[1] * sa + d[1] * da * invSa / 255) / outA;
                    int bB = (s[2] * sa + d[2] * da * invSa / 255) / outA;
                    d[0] = static_cast<uchar>(std::min(255, std::max(0, rB)));
                    d[1] = static_cast<uchar>(std::min(255, std::max(0, gB)));
                    d[2] = static_cast<uchar>(std::min(255, std::max(0, bB)));
                    d[3] = static_cast<uchar>(std::min(255, std::max(0, outA)));
                }
            }
        }
    }
}

cv::Mat Compositor::generateCanvasBackground(const Core::ImageDocument& doc, int h, int w) {
    if (doc.bgType == "Solid") {
        cv::Mat bg(h, w, CV_8UC4, cv::Scalar(doc.bgColor.red(), doc.bgColor.green(), doc.bgColor.blue(), static_cast<int>(doc.bgOpacity * 255)));
        return bg;
    } else if (doc.bgType == "Gradient") {
        cv::Mat bg(h, w, CV_8UC4);
        double r1 = doc.bgColor.red(), g1 = doc.bgColor.green(), b1 = doc.bgColor.blue();
        double r2 = doc.bgColorEnd.red(), g2 = doc.bgColorEnd.green(), b2 = doc.bgColorEnd.blue();
        uint8_t a = static_cast<uint8_t>(doc.bgOpacity * 255);

        for (int y = 0; y < h; ++y) {
            double factor = static_cast<double>(y) / std::max(1, h - 1);
            uint8_t r = static_cast<uint8_t>(r1 * (1.0 - factor) + r2 * factor);
            uint8_t g = static_cast<uint8_t>(g1 * (1.0 - factor) + g2 * factor);
            uint8_t b = static_cast<uint8_t>(b1 * (1.0 - factor) + b2 * factor);

            for (int x = 0; x < w; ++x) {
                bg.at<cv::Vec4b>(y, x) = cv::Vec4b(r, g, b, a);
            }
        }
        return bg;
    } else if (doc.bgType == "Image" && !doc.bgImage.empty()) {
        cv::Mat resized;
        cv::resize(doc.bgImage, resized, cv::Size(w, h));
        if (resized.channels() == 3) {
            cv::Mat alphaCh(h, w, CV_8UC1, cv::Scalar(static_cast<int>(doc.bgOpacity * 255)));
            std::vector<cv::Mat> chs;
            cv::split(resized, chs);
            chs.push_back(alphaCh);
            cv::merge(chs, resized);
        }
        return resized;
    }
    return cv::Mat(h, w, CV_8UC4, cv::Scalar(0, 0, 0, 0));
}

} // namespace Processing
} // namespace ImageCut
