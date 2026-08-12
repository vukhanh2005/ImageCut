#ifndef IMAGECUT_STYLE_H
#define IMAGECUT_STYLE_H

#include <QApplication>
#include <QString>
#include <QIcon>
#include <QColor>

namespace ImageCut {
namespace UI {

class Style {
public:
    static void applyTheme(QApplication* app, const QString& themeName = "Dark");
};

class UIIcons {
public:
    static QIcon getIcon(const QString& name, const QColor& color = QColor(200, 210, 225), int size = 24);
};

} // namespace UI
} // namespace ImageCut

#endif // IMAGECUT_STYLE_H
