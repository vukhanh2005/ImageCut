#ifndef IMAGECUT_STYLE_H
#define IMAGECUT_STYLE_H

#include <QApplication>
#include <QString>

namespace ImageCut {
namespace UI {

class Style {
public:
    static void applyTheme(QApplication* app, const QString& themeName = "Dark");
};

} // namespace UI
} // namespace ImageCut

#endif // IMAGECUT_STYLE_H
