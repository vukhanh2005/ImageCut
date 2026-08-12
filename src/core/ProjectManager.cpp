#include "core/ProjectManager.h"
#include "utils/ImageUtils.h"
#include "utils/Logger.h"
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QByteArray>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <vector>

namespace ImageCut {
namespace Core {

static QString matToBase64Png(const cv::Mat& mat) {
    if (mat.empty()) return "";
    std::vector<uchar> buf;
    cv::imencode(".png", mat, buf);
    QByteArray bytes(reinterpret_cast<const char*>(buf.data()), static_cast<qsizetype>(buf.size()));
    return QString::fromLatin1(bytes.toBase64());
}

static cv::Mat base64PngToMat(const QString& b64Str) {
    if (b64Str.isEmpty()) return cv::Mat();
    QByteArray bytes = QByteArray::fromBase64(b64Str.toLatin1());
    std::vector<uchar> buf(bytes.begin(), bytes.end());
    if (buf.empty()) return cv::Mat();
    return cv::imdecode(buf, cv::IMREAD_UNCHANGED);
}

static QJsonArray colorToJson(const QColor& c) {
    return QJsonArray{ c.red(), c.green(), c.blue(), c.alpha() };
}

static QColor jsonToColor(const QJsonArray& arr, const QColor& def = QColor(0,0,0)) {
    if (arr.size() >= 3) {
        int a = (arr.size() >= 4) ? arr[3].toInt() : 255;
        return QColor(arr[0].toInt(), arr[1].toInt(), arr[2].toInt(), a);
    }
    return def;
}

bool ProjectManager::saveProject(const std::shared_ptr<ImageDocument>& doc, const QString& filepath) {
    if (!doc || doc->layers.empty()) {
        LOG_ERROR("Cannot save empty document project.");
        return false;
    }

    QString savePath = filepath;
    if (!savePath.endsWith(".icproj", Qt::CaseInsensitive) && !savePath.endsWith(".bgrem", Qt::CaseInsensitive)) {
        savePath += ".icproj";
    }

    try {
        QJsonObject root;
        root["format"] = "ImageCut_Project";
        root["version"] = 3;
        root["canvas_width"] = doc->canvasWidth;
        root["canvas_height"] = doc->canvasHeight;
        root["bg_type"] = doc->bgType;
        root["bg_color"] = colorToJson(doc->bgColor);
        root["bg_color_end"] = colorToJson(doc->bgColorEnd);
        root["bg_blur"] = doc->bgBlur;
        root["bg_opacity"] = doc->bgOpacity;
        root["show_grid"] = doc->showGrid;
        root["grid_size"] = doc->gridSize;
        root["grid_opacity"] = doc->gridOpacity;
        root["show_rulers"] = doc->showRulers;
        root["show_guides"] = doc->showGuides;
        root["snap_enabled"] = doc->snapEnabled;

        if (!doc->bgImage.empty()) {
            root["bg_image_b64"] = matToBase64Png(doc->bgImage);
        }

        QJsonArray layerArray;
        for (size_t i = 0; i < doc->layers.size(); ++i) {
            auto lyr = doc->layers[i];
            if (!lyr) continue;

            QJsonObject lmeta;
            lmeta["id"] = lyr->id;
            lmeta["name"] = lyr->name;
            lmeta["layer_type"] = lyr->layerType;
            lmeta["opacity"] = lyr->opacity;
            lmeta["visible"] = lyr->visible;
            lmeta["locked"] = lyr->locked;
            lmeta["blend_mode"] = lyr->blendMode;
            lmeta["offset_x"] = lyr->offsetX;
            lmeta["offset_y"] = lyr->offsetY;
            lmeta["scale_x"] = lyr->scaleX;
            lmeta["scale_y"] = lyr->scaleY;
            lmeta["lock_aspect"] = lyr->lockAspect;
            lmeta["rotation"] = lyr->rotation;
            lmeta["flip_h"] = lyr->flipH;
            lmeta["flip_v"] = lyr->flipV;

            // Text Layer properties
            lmeta["text_content"] = lyr->textContent;
            lmeta["font_family"] = lyr->fontFamily;
            lmeta["font_size"] = lyr->fontSize;
            lmeta["font_bold"] = lyr->fontBold;
            lmeta["font_italic"] = lyr->fontItalic;
            lmeta["text_color"] = colorToJson(lyr->textColor);

            lmeta["text_has_stroke"] = lyr->textHasStroke;
            lmeta["text_stroke_color"] = colorToJson(lyr->textStrokeColor);
            lmeta["text_stroke_width"] = lyr->textStrokeWidth;

            lmeta["text_has_shadow"] = lyr->textHasShadow;
            lmeta["text_shadow_color"] = colorToJson(lyr->textShadowColor);
            lmeta["text_shadow_offset_x"] = lyr->textShadowOffsetX;
            lmeta["text_shadow_offset_y"] = lyr->textShadowOffsetY;

            lmeta["text_has_bg"] = lyr->textHasBg;
            lmeta["text_bg_color"] = colorToJson(lyr->textBgColor);

            // Shape Layer properties
            lmeta["shape_type"] = lyr->shapeType;
            lmeta["fill_color"] = colorToJson(lyr->fillColor);
            lmeta["stroke_color"] = colorToJson(lyr->strokeColor);
            lmeta["stroke_width"] = lyr->strokeWidth;

            // Color Adjustments
            lmeta["brightness"] = lyr->brightness;
            lmeta["contrast"] = lyr->contrast;
            lmeta["saturation"] = lyr->saturation;
            lmeta["exposure"] = lyr->exposure;
            lmeta["temperature"] = lyr->temperature;
            lmeta["sharpness"] = lyr->sharpness;

            // Feather & Edge
            lmeta["feather_radius"] = lyr->featherRadius;
            lmeta["smooth_kernel"] = lyr->smoothKernel;
            lmeta["expand_contract_val"] = lyr->expandContractVal;
            lmeta["edge_contrast"] = lyr->edgeContrast;
            lmeta["decontaminate"] = lyr->decontaminate;

            // Parent & Child relations
            lmeta["parent_id"] = lyr->parentId;
            QJsonArray childArray;
            for (const auto& cid : lyr->childrenIds) childArray.append(cid);
            lmeta["children_ids"] = childArray;

            // Image & Mask Data Base64
            if (!lyr->image.empty()) {
                lmeta["img_b64"] = matToBase64Png(lyr->image);
            }
            if (!lyr->mask.empty()) {
                lmeta["mask_b64"] = matToBase64Png(lyr->mask);
            }

            layerArray.append(lmeta);
        }

        root["layers"] = layerArray;

        QJsonArray activeArray;
        for (const auto& aid : doc->activeLayerIds) activeArray.append(aid);
        root["active_layer_ids"] = activeArray;

        QFile file(savePath);
        if (!file.open(QIODevice::WriteOnly)) {
            LOG_ERROR("Failed to write to file: " + savePath.toStdString());
            return false;
        }

        QJsonDocument jdoc(root);
        file.write(jdoc.toJson(QJsonDocument::Compact));
        file.close();

        LOG_INFO("Project saved successfully to " + savePath.toStdString());
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR(std::string("Error saving project: ") + e.what());
        return false;
    }
}

std::shared_ptr<ImageDocument> ProjectManager::loadProject(const QString& filepath) {
    if (!QFile::exists(filepath)) {
        LOG_ERROR("Project file not found: " + filepath.toStdString());
        return nullptr;
    }

    try {
        QFile file(filepath);
        if (!file.open(QIODevice::ReadOnly)) {
            LOG_ERROR("Cannot open project file for reading: " + filepath.toStdString());
            return nullptr;
        }

        QByteArray data = file.readAll();
        file.close();

        QJsonDocument jdoc = QJsonDocument::fromJson(data);
        if (!jdoc.isObject()) {
            LOG_ERROR("Invalid JSON format in project file.");
            return nullptr;
        }

        QJsonObject root = jdoc.object();
        int cw = root.value("canvas_width").toInt(1920);
        int ch = root.value("canvas_height").toInt(1080);
        auto doc = std::make_shared<ImageDocument>(cv::Mat(), cw, ch);

        doc->bgType = root.value("bg_type").toString("Transparent");
        doc->bgColor = jsonToColor(root.value("bg_color").toArray(), QColor(255, 255, 255));
        doc->bgColorEnd = jsonToColor(root.value("bg_color_end").toArray(), QColor(240, 240, 240));
        doc->bgBlur = root.value("bg_blur").toInt(0);
        doc->bgOpacity = root.value("bg_opacity").toDouble(1.0);
        doc->showGrid = root.value("show_grid").toBool(false);
        doc->gridSize = root.value("grid_size").toInt(20);
        doc->gridOpacity = root.value("grid_opacity").toDouble(0.3);
        doc->showRulers = root.value("show_rulers").toBool(false);
        doc->showGuides = root.value("show_guides").toBool(true);
        doc->snapEnabled = root.value("snap_enabled").toBool(true);

        if (root.contains("bg_image_b64")) {
            doc->bgImage = base64PngToMat(root.value("bg_image_b64").toString());
        }

        QJsonArray layersArr = root.value("layers").toArray();
        doc->layers.clear();

        for (const auto& lval : layersArr) {
            QJsonObject lmeta = lval.toObject();
            auto lyr = std::make_shared<Layer>(
                lmeta.value("name").toString("Layer"),
                cv::Mat(),
                lmeta.value("layer_type").toString("image"),
                lmeta.value("opacity").toDouble(1.0),
                lmeta.value("visible").toBool(true),
                lmeta.value("locked").toBool(false),
                lmeta.value("blend_mode").toString("Normal"),
                lmeta.value("id").toString()
            );

            // Transform properties
            lyr->offsetX = lmeta.value("offset_x").toDouble(0.0);
            lyr->offsetY = lmeta.value("offset_y").toDouble(0.0);
            lyr->scaleX = lmeta.value("scale_x").toDouble(1.0);
            lyr->scaleY = lmeta.value("scale_y").toDouble(1.0);
            lyr->lockAspect = lmeta.value("lock_aspect").toBool(true);
            lyr->rotation = lmeta.value("rotation").toDouble(0.0);
            lyr->flipH = lmeta.value("flip_h").toBool(false);
            lyr->flipV = lmeta.value("flip_v").toBool(false);

            // Text properties
            lyr->textContent = lmeta.value("text_content").toString("");
            lyr->fontFamily = lmeta.value("font_family").toString("Segoe UI");
            lyr->fontSize = lmeta.value("font_size").toInt(48);
            lyr->fontBold = lmeta.value("font_bold").toBool(false);
            lyr->fontItalic = lmeta.value("font_italic").toBool(false);
            lyr->textColor = jsonToColor(lmeta.value("text_color").toArray(), QColor(255, 255, 255));

            lyr->textHasStroke = lmeta.value("text_has_stroke").toBool(false);
            lyr->textStrokeColor = jsonToColor(lmeta.value("text_stroke_color").toArray(), QColor(0, 0, 0));
            lyr->textStrokeWidth = lmeta.value("text_stroke_width").toInt(3);

            lyr->textHasShadow = lmeta.value("text_has_shadow").toBool(false);
            lyr->textShadowColor = jsonToColor(lmeta.value("text_shadow_color").toArray(), QColor(0, 0, 0, 160));
            lyr->textShadowOffsetX = lmeta.value("text_shadow_offset_x").toInt(4);
            lyr->textShadowOffsetY = lmeta.value("text_shadow_offset_y").toInt(4);

            lyr->textHasBg = lmeta.value("text_has_bg").toBool(false);
            lyr->textBgColor = jsonToColor(lmeta.value("text_bg_color").toArray(), QColor(0, 0, 0, 180));

            // Shape properties
            lyr->shapeType = lmeta.value("shape_type").toString("Rectangle");
            lyr->fillColor = jsonToColor(lmeta.value("fill_color").toArray(), QColor(0, 120, 215, 255));
            lyr->strokeColor = jsonToColor(lmeta.value("stroke_color").toArray(), QColor(255, 255, 255, 255));
            lyr->strokeWidth = lmeta.value("stroke_width").toInt(2);

            // Color Adjustments
            lyr->brightness = lmeta.value("brightness").toInt(0);
            lyr->contrast = lmeta.value("contrast").toInt(0);
            lyr->saturation = lmeta.value("saturation").toInt(0);
            lyr->exposure = lmeta.value("exposure").toInt(0);
            lyr->temperature = lmeta.value("temperature").toInt(0);
            lyr->sharpness = lmeta.value("sharpness").toInt(0);

            // Feather & Edge
            lyr->featherRadius = lmeta.value("feather_radius").toDouble(0.0);
            lyr->smoothKernel = lmeta.value("smooth_kernel").toInt(0);
            lyr->expandContractVal = lmeta.value("expand_contract_val").toInt(0);
            lyr->edgeContrast = lmeta.value("edge_contrast").toDouble(1.0);
            lyr->decontaminate = lmeta.value("decontaminate").toBool(false);

            // Parent & Child relations
            lyr->parentId = lmeta.value("parent_id").toString("");
            QJsonArray childArray = lmeta.value("children_ids").toArray();
            for (const auto& cid : childArray) lyr->childrenIds.push_back(cid.toString());

            // Image & Mask Data Base64
            if (lmeta.contains("img_b64")) {
                lyr->image = base64PngToMat(lmeta.value("img_b64").toString());
            }
            if (lmeta.contains("mask_b64")) {
                cv::Mat m = base64PngToMat(lmeta.value("mask_b64").toString());
                if (!m.empty()) {
                    if (m.channels() > 1) {
                        cv::cvtColor(m, lyr->mask, cv::COLOR_RGB2GRAY);
                    } else {
                        lyr->mask = m;
                    }
                }
            }

            doc->layers.push_back(lyr);
        }

        QJsonArray activeArr = root.value("active_layer_ids").toArray();
        doc->activeLayerIds.clear();
        for (const auto& aid : activeArr) {
            doc->activeLayerIds.push_back(aid.toString());
        }
        if (doc->activeLayerIds.empty() && !doc->layers.empty()) {
            doc->activeLayerIds = { doc->layers.back()->id };
        }

        doc->notifyChanged();
        LOG_INFO("Project loaded successfully from " + filepath.toStdString());
        return doc;
    } catch (const std::exception& e) {
        LOG_ERROR(std::string("Error loading project: ") + e.what());
        return nullptr;
    }
}

} // namespace Core
} // namespace ImageCut
