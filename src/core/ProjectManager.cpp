#include "core/ProjectManager.h"
#include "utils/ImageUtils.h"
#include "utils/Logger.h"
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTemporaryDir>

namespace ImageCut {
namespace Core {

bool ProjectManager::saveProject(const std::shared_ptr<ImageDocument>& doc, const QString& filepath) {
    if (!doc || doc->layers.empty()) {
        LOG_ERROR("Cannot save empty document project.");
        return false;
    }

    QString savePath = filepath;
    if (!savePath.endsWith(".bgrem", Qt::CaseInsensitive)) {
        savePath += ".bgrem";
    }

    try {
        QTemporaryDir tempDir;
        if (!tempDir.isValid()) return false;

        QJsonArray layerMetas;
        for (size_t i = 0; i < doc->layers.size(); ++i) {
            auto lyr = doc->layers[i];
            QString imgFilename = QString("layer_%1_img.png").arg(i);
            QString maskFilename = QString("layer_%1_mask.png").arg(i);

            bool hasImg = false;
            if (!lyr->image.empty()) {
                QString imgPath = tempDir.filePath(imgFilename);
                Utils::ImageUtils::saveImage(imgPath, lyr->image);
                hasImg = true;
            }

            bool hasMask = false;
            if (!lyr->mask.empty()) {
                QString maskPath = tempDir.filePath(maskFilename);
                Utils::ImageUtils::saveImage(maskPath, lyr->mask);
                hasMask = true;
            }

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
            lmeta["brightness"] = lyr->brightness;
            lmeta["contrast"] = lyr->contrast;
            lmeta["saturation"] = lyr->saturation;
            lmeta["exposure"] = lyr->exposure;
            lmeta["temperature"] = lyr->temperature;
            lmeta["sharpness"] = lyr->sharpness;
            lmeta["feather_radius"] = lyr->featherRadius;
            lmeta["smooth_kernel"] = lyr->smoothKernel;
            lmeta["expand_contract_val"] = lyr->expandContractVal;
            lmeta["edge_contrast"] = lyr->edgeContrast;
            lmeta["decontaminate"] = lyr->decontaminate;
            lmeta["parent_id"] = lyr->parentId;

            QJsonArray childArray;
            for (const auto& cid : lyr->childrenIds) childArray.append(cid);
            lmeta["children_ids"] = childArray;

            lmeta["text_content"] = lyr->textContent;
            lmeta["font_family"] = lyr->fontFamily;
            lmeta["font_size"] = lyr->fontSize;
            lmeta["font_bold"] = lyr->fontBold;
            lmeta["font_italic"] = lyr->fontItalic;
            lmeta["has_img"] = hasImg;
            lmeta["has_mask"] = hasMask;
            lmeta["img_file"] = imgFilename;
            lmeta["mask_file"] = maskFilename;

            layerMetas.append(lmeta);
        }

        if (!doc->bgImage.empty()) {
            QString bgPath = tempDir.filePath("bg_image.png");
            Utils::ImageUtils::saveImage(bgPath, doc->bgImage);
        }

        QJsonObject meta;
        meta["version"] = 2;
        meta["canvas_width"] = doc->canvasWidth;
        meta["canvas_height"] = doc->canvasHeight;
        meta["bg_type"] = doc->bgType;
        meta["bg_color"] = QJsonArray{ doc->bgColor.red(), doc->bgColor.green(), doc->bgColor.blue() };
        meta["bg_color_end"] = QJsonArray{ doc->bgColorEnd.red(), doc->bgColorEnd.green(), doc->bgColorEnd.blue() };
        meta["bg_blur"] = doc->bgBlur;
        meta["bg_opacity"] = doc->bgOpacity;
        meta["show_grid"] = doc->showGrid;
        meta["grid_size"] = doc->gridSize;
        meta["grid_opacity"] = doc->gridOpacity;
        meta["show_rulers"] = doc->showRulers;
        meta["show_guides"] = doc->showGuides;
        meta["snap_enabled"] = doc->snapEnabled;

        QJsonArray activeArray;
        for (const auto& aid : doc->activeLayerIds) activeArray.append(aid);
        meta["active_layer_ids"] = activeArray;
        meta["layers"] = layerMetas;

        QFile jsonFile(tempDir.filePath("project.json"));
        if (jsonFile.open(QIODevice::WriteOnly)) {
            QJsonDocument jdoc(meta);
            jsonFile.write(jdoc.toJson(QJsonDocument::Indented));
            jsonFile.close();
        }

        LOG_INFO("Project saved to " + savePath.toStdString());
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
        QTemporaryDir tempDir;
        if (!tempDir.isValid()) return nullptr;

        QFile jsonFile(tempDir.filePath("project.json"));
        QJsonObject meta;
        if (jsonFile.open(QIODevice::ReadOnly)) {
            QJsonDocument jdoc = QJsonDocument::fromJson(jsonFile.readAll());
            if (jdoc.isObject()) meta = jdoc.object();
            jsonFile.close();
        }

        int cw = meta.value("canvas_width").toInt(1920);
        int ch = meta.value("canvas_height").toInt(1080);
        auto doc = std::make_shared<ImageDocument>(cv::Mat(), cw, ch);

        doc->bgType = meta.value("bg_type").toString("Transparent");
        QJsonArray bgCol = meta.value("bg_color").toArray();
        if (bgCol.size() == 3) {
            doc->bgColor = QColor(bgCol[0].toInt(), bgCol[1].toInt(), bgCol[2].toInt());
        }
        doc->bgBlur = meta.value("bg_blur").toInt(0);
        doc->bgOpacity = meta.value("bg_opacity").toDouble(1.0);
        doc->showGrid = meta.value("show_grid").toBool(false);
        doc->gridSize = meta.value("grid_size").toInt(20);
        doc->gridOpacity = meta.value("grid_opacity").toDouble(0.3);

        QString bgImgPath = tempDir.filePath("bg_image.png");
        if (QFile::exists(bgImgPath)) {
            doc->bgImage = Utils::ImageUtils::loadImage(bgImgPath);
        }

        QJsonArray layersArr = meta.value("layers").toArray();
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

            if (lmeta.value("has_img").toBool()) {
                QString imgPath = tempDir.filePath(lmeta.value("img_file").toString());
                if (QFile::exists(imgPath)) {
                    lyr->image = Utils::ImageUtils::loadImage(imgPath);
                }
            }

            if (lmeta.value("has_mask").toBool()) {
                QString maskPath = tempDir.filePath(lmeta.value("mask_file").toString());
                if (QFile::exists(maskPath)) {
                    cv::Mat m = Utils::ImageUtils::loadImage(maskPath);
                    if (m.channels() > 1) {
                        cv::cvtColor(m, lyr->mask, cv::COLOR_RGB2GRAY);
                    } else {
                        lyr->mask = m;
                    }
                }
            }

            lyr->offsetX = lmeta.value("offset_x").toDouble(0.0);
            lyr->offsetY = lmeta.value("offset_y").toDouble(0.0);
            lyr->scaleX = lmeta.value("scale_x").toDouble(1.0);
            lyr->scaleY = lmeta.value("scale_y").toDouble(1.0);
            lyr->lockAspect = lmeta.value("lock_aspect").toBool(true);
            lyr->rotation = lmeta.value("rotation").toDouble(0.0);
            lyr->flipH = lmeta.value("flip_h").toBool(false);
            lyr->flipV = lmeta.value("flip_v").toBool(false);
            lyr->brightness = lmeta.value("brightness").toInt(0);
            lyr->contrast = lmeta.value("contrast").toInt(0);
            lyr->saturation = lmeta.value("saturation").toInt(0);
            lyr->exposure = lmeta.value("exposure").toInt(0);
            lyr->temperature = lmeta.value("temperature").toInt(0);
            lyr->sharpness = lmeta.value("sharpness").toInt(0);

            doc->layers.push_back(lyr);
        }

        QJsonArray activeArr = meta.value("active_layer_ids").toArray();
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
