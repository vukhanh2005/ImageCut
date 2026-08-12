#include "ui/panels/ImagePanel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>

namespace ImageCut {
namespace UI {

ImagePanel::ImagePanel(QWidget* parent)
    : QWidget(parent)
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(14);
    layout->setAlignment(Qt::AlignTop);

    QGroupBox* grpAdj = new QGroupBox("Color & Tone Adjustments", this);
    QVBoxLayout* vboxAdj = new QVBoxLayout(grpAdj);

    vboxAdj->addLayout(createSliderRow("Brightness:", "brightness", -100, 100, 0));
    vboxAdj->addLayout(createSliderRow("Contrast:", "contrast", -100, 100, 0));
    vboxAdj->addLayout(createSliderRow("Saturation:", "saturation", -100, 100, 0));
    vboxAdj->addLayout(createSliderRow("Exposure:", "exposure", -100, 100, 0));
    vboxAdj->addLayout(createSliderRow("Temperature:", "temperature", -100, 100, 0));
    vboxAdj->addLayout(createSliderRow("Sharpness:", "sharpness", 0, 100, 0));

    QPushButton* btnReset = new QPushButton("🔄 Reset Adjustments", this);
    connect(btnReset, &QPushButton::clicked, this, &ImagePanel::resetAll);
    vboxAdj->addWidget(btnReset);

    layout->addWidget(grpAdj);
}

QHBoxLayout* ImagePanel::createSliderRow(const QString& labelText, const QString& key, int minVal, int maxVal, int initVal) {
    QHBoxLayout* hbox = new QHBoxLayout();
    QLabel* lbl = new QLabel(labelText, this);
    QLabel* lblVal = new QLabel(QString::number(initVal), this);
    lblVal->setFixedWidth(35);
    hbox->addWidget(lbl);
    hbox->addWidget(lblVal);

    QSlider* slider = new QSlider(Qt::Horizontal, this);
    slider->setRange(minVal, maxVal);
    slider->setValue(initVal);

    connect(slider, &QSlider::valueChanged, [this, key, lblVal](int v) {
        lblVal->setText(QString::number(v));
        if (m_doc && m_doc->getActiveLayer()) {
            auto active = m_doc->getActiveLayer();
            if (key == "brightness") active->brightness = v;
            else if (key == "contrast") active->contrast = v;
            else if (key == "saturation") active->saturation = v;
            else if (key == "exposure") active->exposure = v;
            else if (key == "temperature") active->temperature = v;
            else if (key == "sharpness") active->sharpness = v;
            active->invalidateCache();
            m_doc->notifyChanged();
        }
    });

    m_sliders[key] = { slider, lblVal };
    hbox->addWidget(slider);
    return hbox;
}

void ImagePanel::setDocument(std::shared_ptr<Core::ImageDocument> doc) {
    m_doc = doc;
    if (m_doc && m_doc->getActiveLayer()) {
        auto active = m_doc->getActiveLayer();
        if (m_sliders.find("brightness") != m_sliders.end()) { m_sliders["brightness"].first->setValue(active->brightness); }
        if (m_sliders.find("contrast") != m_sliders.end()) { m_sliders["contrast"].first->setValue(active->contrast); }
        if (m_sliders.find("saturation") != m_sliders.end()) { m_sliders["saturation"].first->setValue(active->saturation); }
        if (m_sliders.find("exposure") != m_sliders.end()) { m_sliders["exposure"].first->setValue(active->exposure); }
        if (m_sliders.find("temperature") != m_sliders.end()) { m_sliders["temperature"].first->setValue(active->temperature); }
        if (m_sliders.find("sharpness") != m_sliders.end()) { m_sliders["sharpness"].first->setValue(active->sharpness); }
    }
}

void ImagePanel::resetAll() {
    for (auto& [key, pair] : m_sliders) {
        pair.first->setValue(0);
        pair.second->setText("0");
    }
    if (m_doc && m_doc->getActiveLayer()) {
        auto active = m_doc->getActiveLayer();
        active->brightness = 0; active->contrast = 0; active->saturation = 0;
        active->exposure = 0; active->temperature = 0; active->sharpness = 0;
        active->invalidateCache();
        m_doc->notifyChanged();
    }
}

} // namespace UI
} // namespace ImageCut
