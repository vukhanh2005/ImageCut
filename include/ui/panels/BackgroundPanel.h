#ifndef IMAGECUT_BACKGROUNDPANEL_H
#define IMAGECUT_BACKGROUNDPANEL_H

#include <QWidget>
#include <QComboBox>
#include <QPushButton>
#include <QSlider>
#include <QLabel>
#include <QGroupBox>
#include <memory>
#include "core/ImageDocument.h"

namespace ImageCut {
namespace UI {

class BackgroundPanel : public QWidget {
    Q_OBJECT
public:
    explicit BackgroundPanel(QWidget* parent = nullptr);
    ~BackgroundPanel() override = default;

    void setDocument(std::shared_ptr<Core::ImageDocument> doc);

private:
    void updateVisibility();

    std::shared_ptr<Core::ImageDocument> m_doc;
    QComboBox* m_comboBgType = nullptr;
    QGroupBox* m_grpColor = nullptr;
    QGroupBox* m_grpImage = nullptr;
    QSlider* m_sliderBlur = nullptr;
    QLabel* m_lblBlurVal = nullptr;
};

} // namespace UI
} // namespace ImageCut

#endif // IMAGECUT_BACKGROUNDPANEL_H
