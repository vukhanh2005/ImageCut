#ifndef IMAGECUT_IMAGEPANEL_H
#define IMAGECUT_IMAGEPANEL_H

#include <QWidget>
#include <QSlider>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <map>
#include <memory>
#include "core/ImageDocument.h"

namespace ImageCut {
namespace UI {

class ImagePanel : public QWidget {
    Q_OBJECT
public:
    explicit ImagePanel(QWidget* parent = nullptr);
    ~ImagePanel() override = default;

    void setDocument(std::shared_ptr<Core::ImageDocument> doc);
    void resetAll();

private:
    QHBoxLayout* createSliderRow(const QString& labelText, const QString& key, int minVal, int maxVal, int initVal);

    std::shared_ptr<Core::ImageDocument> m_doc;
    std::map<QString, std::pair<QSlider*, QLabel*>> m_sliders;
};

} // namespace UI
} // namespace ImageCut

#endif // IMAGECUT_IMAGEPANEL_H
