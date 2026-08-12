#include "ui/panels/MaskPanel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>

namespace ImageCut {
namespace UI {

MaskPanel::MaskPanel(QWidget* parent)
    : QWidget(parent)
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(14);
    layout->setAlignment(Qt::AlignTop);

    QGroupBox* grpView = new QGroupBox("Mask View Mode", this);
    QVBoxLayout* vboxView = new QVBoxLayout(grpView);
    m_comboViewMode = new QComboBox(this);
    m_comboViewMode->addItems({ "Normal", "Overlay", "BlackWhite", "Alpha" });
    connect(m_comboViewMode, &QComboBox::currentTextChanged, [this](const QString& mode) {
        if (m_doc) {
            m_doc->maskViewMode = mode;
            m_doc->notifyChanged();
        }
    });
    vboxView->addWidget(m_comboViewMode);
    layout->addWidget(grpView);

    QGroupBox* grpRefine = new QGroupBox("Edge Refinement & Matting", this);
    QVBoxLayout* vboxRefine = new QVBoxLayout(grpRefine);

    vboxRefine->addLayout(createSliderRow("Feather:", 0, 30, 0, [this](int val) {
        if (m_doc && m_doc->getActiveLayer()) {
            m_doc->getActiveLayer()->featherRadius = val;
            m_doc->notifyChanged();
        }
    }));

    vboxRefine->addLayout(createSliderRow("Smoothness:", 0, 15, 0, [this](int val) {
        if (m_doc && m_doc->getActiveLayer()) {
            m_doc->getActiveLayer()->smoothKernel = val;
            m_doc->notifyChanged();
        }
    }));

    vboxRefine->addLayout(createSliderRow("Expand/Contract:", -20, 20, 0, [this](int val) {
        if (m_doc && m_doc->getActiveLayer()) {
            m_doc->getActiveLayer()->expandContractVal = val;
            m_doc->notifyChanged();
        }
    }));

    vboxRefine->addLayout(createSliderRow("Edge Contrast:", 5, 20, 10, [this](int val) {
        if (m_doc && m_doc->getActiveLayer()) {
            m_doc->getActiveLayer()->edgeContrast = val / 10.0;
            m_doc->notifyChanged();
        }
    }));

    m_chkDecontam = new QCheckBox("Color Decontamination (Hair Halo Clean)", this);
    connect(m_chkDecontam, &QCheckBox::toggled, [this](bool checked) {
        if (m_doc && m_doc->getActiveLayer()) {
            m_doc->getActiveLayer()->decontaminate = checked;
            m_doc->notifyChanged();
        }
    });
    vboxRefine->addWidget(m_chkDecontam);

    layout->addWidget(grpRefine);
}

QHBoxLayout* MaskPanel::createSliderRow(const QString& labelText, int minVal, int maxVal, int initVal, std::function<void(int)> callback) {
    QHBoxLayout* hbox = new QHBoxLayout();
    QLabel* lbl = new QLabel(labelText, this);
    QLabel* lblVal = new QLabel(QString::number(initVal), this);
    lblVal->setFixedWidth(30);
    hbox->addWidget(lbl);
    hbox->addWidget(lblVal);

    QSlider* slider = new QSlider(Qt::Horizontal, this);
    slider->setRange(minVal, maxVal);
    slider->setValue(initVal);

    connect(slider, &QSlider::valueChanged, [lblVal, callback](int v) {
        lblVal->setText(QString::number(v));
        if (callback) callback(v);
    });

    hbox->addWidget(slider);
    return hbox;
}

void MaskPanel::setDocument(std::shared_ptr<Core::ImageDocument> doc) {
    m_doc = doc;
    if (m_doc) {
        m_comboViewMode->setCurrentText(m_doc->maskViewMode);
        auto active = m_doc->getActiveLayer();
        if (active) {
            m_chkDecontam->setChecked(active->decontaminate);
        }
    }
}

} // namespace UI
} // namespace ImageCut
