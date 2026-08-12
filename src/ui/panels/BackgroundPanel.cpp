#include "ui/panels/BackgroundPanel.h"
#include "utils/ImageUtils.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QColorDialog>
#include <QFileDialog>

namespace ImageCut {
namespace UI {

BackgroundPanel::BackgroundPanel(QWidget* parent)
    : QWidget(parent)
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(14);
    layout->setAlignment(Qt::AlignTop);

    QGroupBox* grpType = new QGroupBox("Background Type", this);
    QVBoxLayout* vboxType = new QVBoxLayout(grpType);
    m_comboBgType = new QComboBox(this);
    m_comboBgType->addItems({ "Transparent", "Solid", "Image", "Gradient" });
    connect(m_comboBgType, &QComboBox::currentTextChanged, [this](const QString& type) {
        if (m_doc) {
            m_doc->bgType = type;
            m_doc->notifyChanged();
        }
        updateVisibility();
    });
    vboxType->addWidget(m_comboBgType);
    layout->addWidget(grpType);

    m_grpColor = new QGroupBox("Solid Color", this);
    QVBoxLayout* vboxColor = new QVBoxLayout(m_grpColor);
    QPushButton* btnPickColor = new QPushButton("🎨 Pick Color", this);
    connect(btnPickColor, &QPushButton::clicked, [this]() {
        if (!m_doc) return;
        QColor initial = m_doc->bgColor;
        QColorDialog* dlg = new QColorDialog(initial, this);
        dlg->setWindowTitle("Select Background Color");
        dlg->setOption(QColorDialog::ShowAlphaChannel, true);
        dlg->setAttribute(Qt::WA_DeleteOnClose);

        connect(dlg, &QColorDialog::currentColorChanged, [this](const QColor& color) {
            if (color.isValid() && m_doc) {
                m_doc->bgColor = color;
                m_doc->notifyChanged();
            }
        });

        connect(dlg, &QColorDialog::colorSelected, [this](const QColor& color) {
            if (color.isValid() && m_doc) {
                m_doc->bgColor = color;
                m_doc->notifyChanged();
            }
        });

        connect(dlg, &QColorDialog::rejected, [this, initial]() {
            if (m_doc) {
                m_doc->bgColor = initial;
                m_doc->notifyChanged();
            }
        });

        dlg->open();
    });
    vboxColor->addWidget(btnPickColor);

    QHBoxLayout* hboxPresets = new QHBoxLayout();
    struct Preset { QString name; QString hex; };
    std::vector<Preset> presets = { {"White", "#FFFFFF"}, {"Black", "#000000"}, {"Blue", "#3B82F6"}, {"Green", "#22C55E"} };
    for (const auto& p : presets) {
        QPushButton* btnP = new QPushButton(p.name, this);
        btnP->setStyleSheet(QString("background-color: %1; color: %2; border-radius: 4px;").arg(p.hex).arg(p.hex == "#FFFFFF" ? "#000000" : "#FFFFFF"));
        connect(btnP, &QPushButton::clicked, [this, p]() {
            if (m_doc) {
                m_doc->bgColor = QColor(p.hex);
                m_doc->notifyChanged();
            }
        });
        hboxPresets->addWidget(btnP);
    }
    vboxColor->addLayout(hboxPresets);
    layout->addWidget(m_grpColor);

    m_grpImage = new QGroupBox("Background Image", this);
    QVBoxLayout* vboxImg = new QVBoxLayout(m_grpImage);
    QPushButton* btnImportBg = new QPushButton("🖼️ Import Image", this);
    connect(btnImportBg, &QPushButton::clicked, [this]() {
        QString path = QFileDialog::getOpenFileName(this, "Open Background Image", "", "Images (*.png *.jpg *.jpeg *.webp *.bmp)");
        if (!path.isEmpty() && m_doc) {
            cv::Mat bgArr = Utils::ImageUtils::loadImage(path);
            if (!bgArr.empty()) {
                m_doc->bgImage = bgArr;
                m_doc->bgType = "Image";
                m_comboBgType->setCurrentText("Image");
                m_doc->notifyChanged();
            }
        }
    });
    vboxImg->addWidget(btnImportBg);
    layout->addWidget(m_grpImage);

    QGroupBox* grpBlur = new QGroupBox("Background Blur", this);
    QVBoxLayout* vboxBlur = new QVBoxLayout(grpBlur);
    QHBoxLayout* hboxLbl = new QHBoxLayout();
    hboxLbl->addWidget(new QLabel("Blur Amount:", this));
    m_lblBlurVal = new QLabel("0 px", this);
    hboxLbl->addWidget(m_lblBlurVal);
    vboxBlur->addLayout(hboxLbl);

    m_sliderBlur = new QSlider(Qt::Horizontal, this);
    m_sliderBlur->setRange(0, 50);
    m_sliderBlur->setValue(0);
    connect(m_sliderBlur, &QSlider::valueChanged, [this](int val) {
        m_lblBlurVal->setText(QString("%1 px").arg(val));
        if (m_doc) {
            m_doc->bgBlur = val;
            m_doc->notifyChanged();
        }
    });
    vboxBlur->addWidget(m_sliderBlur);
    layout->addWidget(grpBlur);

    updateVisibility();
}

void BackgroundPanel::setDocument(std::shared_ptr<Core::ImageDocument> doc) {
    m_doc = doc;
    if (m_doc) {
        m_comboBgType->setCurrentText(m_doc->bgType);
        m_sliderBlur->setValue(m_doc->bgBlur);
        updateVisibility();
    }
}

void BackgroundPanel::updateVisibility() {
    QString bgType = m_comboBgType->currentText();
    m_grpColor->setVisible(bgType == "Solid" || bgType == "Gradient");
    m_grpImage->setVisible(bgType == "Image");
}

} // namespace UI
} // namespace ImageCut
