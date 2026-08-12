#ifndef IMAGECUT_ALIGNUTILS_H
#define IMAGECUT_ALIGNUTILS_H

#include <QString>
#include "core/ImageDocument.h"

namespace ImageCut {
namespace Processing {

class AlignUtils {
public:
    static void alignLayers(Core::ImageDocument& doc, const QString& mode, const QString& target = "Canvas");
    static void distributeLayers(Core::ImageDocument& doc, const QString& orientation = "horizontal");
};

} // namespace Processing
} // namespace ImageCut

#endif // IMAGECUT_ALIGNUTILS_H
