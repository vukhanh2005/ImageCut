#include "processing/AlignUtils.h"
#include <algorithm>

namespace ImageCut {
namespace Processing {

void AlignUtils::alignLayers(Core::ImageDocument& doc, const QString& mode, const QString& target) {
    auto activeLayers = doc.getActiveLayers();
    if (activeLayers.empty()) return;

    double cw = doc.canvasWidth;
    double ch = doc.canvasHeight;

    double minX = 0.0, minY = 0.0, selW = cw, selH = ch;
    if (target == "Selection" && activeLayers.size() > 1) {
        minX = 1e9; minY = 1e9;
        double maxX = -1e9, maxY = -1e9;
        for (const auto& l : activeLayers) {
            minX = std::min(minX, l->offsetX);
            minY = std::min(minY, l->offsetY);
            maxX = std::max(maxX, l->offsetX + l->width() * l->scaleX);
            maxY = std::max(maxY, l->offsetY + l->height() * l->scaleY);
        }
        selW = maxX - minX;
        selH = maxY - minY;
    }

    for (auto& l : activeLayers) {
        if (l->locked) continue;
        double lw = l->width() * l->scaleX;
        double lh = l->height() * l->scaleY;

        if (mode == "left") {
            l->offsetX = minX;
        } else if (mode == "center") {
            l->offsetX = minX + (selW - lw) / 2.0;
        } else if (mode == "right") {
            l->offsetX = minX + selW - lw;
        } else if (mode == "top") {
            l->offsetY = minY;
        } else if (mode == "middle") {
            l->offsetY = minY + (selH - lh) / 2.0;
        } else if (mode == "bottom") {
            l->offsetY = minY + selH - lh;
        }
    }
    doc.notifyChanged();
}

void AlignUtils::distributeLayers(Core::ImageDocument& doc, const QString& orientation) {
    auto activeLayers = doc.getActiveLayers();
    std::vector<std::shared_ptr<Core::Layer>> unlocked;
    for (const auto& l : activeLayers) {
        if (!l->locked) unlocked.push_back(l);
    }
    if (unlocked.size() < 3) return;

    if (orientation == "horizontal") {
        std::sort(unlocked.begin(), unlocked.end(), [](const std::shared_ptr<Core::Layer>& a, const std::shared_ptr<Core::Layer>& b) {
            return a->offsetX < b->offsetX;
        });

        auto first = unlocked.front();
        auto last = unlocked.back();

        double startX = first->offsetX + first->width() * first->scaleX;
        double endX = last->offsetX;
        double totalDist = endX - startX;

        double middleWidths = 0.0;
        for (size_t i = 1; i < unlocked.size() - 1; ++i) {
            middleWidths += unlocked[i]->width() * unlocked[i]->scaleX;
        }

        double gap = (totalDist - middleWidths) / (unlocked.size() - 1);
        double currX = startX + gap;

        for (size_t i = 1; i < unlocked.size() - 1; ++i) {
            unlocked[i]->offsetX = currX;
            currX += (unlocked[i]->width() * unlocked[i]->scaleX) + gap;
        }
    } else if (orientation == "vertical") {
        std::sort(unlocked.begin(), unlocked.end(), [](const std::shared_ptr<Core::Layer>& a, const std::shared_ptr<Core::Layer>& b) {
            return a->offsetY < b->offsetY;
        });

        auto first = unlocked.front();
        auto last = unlocked.back();

        double startY = first->offsetY + first->height() * first->scaleY;
        double endY = last->offsetY;
        double totalDist = endY - startY;

        double middleHeights = 0.0;
        for (size_t i = 1; i < unlocked.size() - 1; ++i) {
            middleHeights += unlocked[i]->height() * unlocked[i]->scaleY;
        }

        double gap = (totalDist - middleHeights) / (unlocked.size() - 1);
        double currY = startY + gap;

        for (size_t i = 1; i < unlocked.size() - 1; ++i) {
            unlocked[i]->offsetY = currY;
            currY += (unlocked[i]->height() * unlocked[i]->scaleY) + gap;
        }
    }
    doc.notifyChanged();
}

} // namespace Processing
} // namespace ImageCut
