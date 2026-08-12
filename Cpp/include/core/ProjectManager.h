#ifndef IMAGECUT_PROJECTMANAGER_H
#define IMAGECUT_PROJECTMANAGER_H

#include <memory>
#include <QString>
#include "core/ImageDocument.h"

namespace ImageCut {
namespace Core {

class ProjectManager {
public:
    static bool saveProject(const std::shared_ptr<ImageDocument>& document, const QString& filepath);
    static std::shared_ptr<ImageDocument> loadProject(const QString& filepath);
};

} // namespace Core
} // namespace ImageCut

#endif // IMAGECUT_PROJECTMANAGER_H
