#include "ui/panels/TransformPanel.h"
#include "processing/AlignUtils.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>

namespace ImageCut {
namespace UI {

TransformPanel::TransformPanel(QWidget* parent)
    : QWidget(parent)
{
    initUi();
}

void TransformPanel::setDocument(std::shared_ptr<Core::ImageDocument> doc) {
    m_doc = doc;
    if (m_doc) {
        m_doc->addChangeListener([this]() {
            updatePanel();
        });
    }
    updatePanel();
}

void TransformPanel::initUi() {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(12);
    layout->setAlignment(Qt::AlignTop);

    // 1. Layer Position & Size Section
    QGroupBox* grpPos = new QGroupBox("Layer Transform", this);
    QGridLayout* gridPos = new QGridLayout(grpPos);
    gridPos->setSpacing(6);

    gridPos->addWidget(new QLabel("X:", this), 0, 0);
    m_spnPosX = new QDoubleSpinBox(this);
    m_spnPosX->setRange(-10000, 10000);
    connect(m_spnPosX, &QDoubleSpinBox::valueChanged, [this](double val) {
        if (m_doc && m_doc->getActiveLayer() && !m_updatingUi) {
            m_doc->getActiveLayer()->offsetX = val;
            m_doc->notifyChanged();
        }
    });
    gridPos->addWidget(m_spnPosX, 0, 1);

    gridPos->addWidget(new QLabel("Y:", this), 0, 2);
    m_spnPosY = new QDoubleSpinBox(this);
    m_spnPosY->setRange(-10000, 10000);
    connect(m_spnPosY, &QDoubleSpinBox::valueChanged, [this](double val) {
        if (m_doc && m_doc->getActiveLayer() && !m_updatingUi) {
            m_doc->getActiveLayer()->offsetY = val;
            m_doc->notifyChanged();
        }
    });
    gridPos->addWidget(m_spnPosY, 0, 3);

    gridPos->addWidget(new QLabel("Scale X:", this), 1, 0);
    m_spnScaleX = new QDoubleSpinBox(this);
    m_spnScaleX->setRange(0.01, 100.0);
    m_spnScaleX->setSingleStep(0.05);
    m_spnScaleX->setValue(1.0);
    connect(m_spnScaleX, &QDoubleSpinBox::valueChanged, [this](double val) {
        if (m_doc && m_doc->getActiveLayer() && !m_updatingUi) {
            auto active = m_doc->getActiveLayer();
            active->scaleX = val;
            if (active->lockAspect) {
                active->scaleY = val;
                m_spnScaleY->blockSignals(true);
                m_spnScaleY->setValue(val);
                m_spnScaleY->blockSignals(false);
            }
            m_doc->notifyChanged();
        }
    });
    gridPos->addWidget(m_spnScaleX, 1, 1);

    gridPos->addWidget(new QLabel("Scale Y:", this), 1, 2);
    m_spnScaleY = new QDoubleSpinBox(this);
    m_spnScaleY->setRange(0.01, 100.0);
    m_spnScaleY->setSingleStep(0.05);
    m_spnScaleY->setValue(1.0);
    connect(m_spnScaleY, &QDoubleSpinBox::valueChanged, [this](double val) {
        if (m_doc && m_doc->getActiveLayer() && !m_updatingUi) {
            auto active = m_doc->getActiveLayer();
            active->scaleY = val;
            if (active->lockAspect) {
                active->scaleX = val;
                m_spnScaleX->blockSignals(true);
                m_spnScaleX->setValue(val);
                m_spnScaleX->blockSignals(false);
            }
            m_doc->notifyChanged();
        }
    });
    gridPos->addWidget(m_spnScaleY, 1, 3);

    m_chkLockAspect = new QCheckBox("Lock Aspect Ratio", this);
    m_chkLockAspect->setChecked(true);
    connect(m_chkLockAspect, &QCheckBox::toggled, [this](bool checked) {
        if (m_doc && m_doc->getActiveLayer()) {
            m_doc->getActiveLayer()->lockAspect = checked;
        }
    });
    gridPos->addWidget(m_chkLockAspect, 2, 0, 1, 4);

    gridPos->addWidget(new QLabel("Rotation:", this), 3, 0);
    m_spnRotation = new QDoubleSpinBox(this);
    m_spnRotation->setRange(-360.0, 360.0);
    m_spnRotation->setSuffix("°");
    connect(m_spnRotation, &QDoubleSpinBox::valueChanged, [this](double val) {
        if (m_doc && m_doc->getActiveLayer() && !m_updatingUi) {
            m_doc->getActiveLayer()->rotation = val;
            m_doc->notifyChanged();
        }
    });
    gridPos->addWidget(m_spnRotation, 3, 1, 1, 3);

    QHBoxLayout* hboxFlips = new QHBoxLayout();
    m_btnFlipH = new QPushButton("↔ Flip H", this);
    m_btnFlipH->setCheckable(true);
    connect(m_btnFlipH, &QPushButton::clicked, [this](bool checked) {
        if (m_doc && m_doc->getActiveLayer()) {
            m_doc->getActiveLayer()->flipH = checked;
            m_doc->notifyChanged();
        }
    });
    hboxFlips->addWidget(m_btnFlipH);

    m_btnFlipV = new QPushButton("↕ Flip V", this);
    m_btnFlipV->setCheckable(true);
    connect(m_btnFlipV, &QPushButton::clicked, [this](bool checked) {
        if (m_doc && m_doc->getActiveLayer()) {
            m_doc->getActiveLayer()->flipV = checked;
            m_doc->notifyChanged();
        }
    });
    hboxFlips->addWidget(m_btnFlipV);
    gridPos->addLayout(hboxFlips, 4, 0, 1, 4);

    layout->addWidget(grpPos);

    // 2. Alignment & Distribution Section
    QGroupBox* grpAlign = new QGroupBox("Alignment & Distribute", this);
    QVBoxLayout* vboxAlign = new QVBoxLayout(grpAlign);
    vboxAlign->setSpacing(6);

    QHBoxLayout* hboxTarget = new QHBoxLayout();
    hboxTarget->addWidget(new QLabel("Target:", this));
    m_cmbAlignTarget = new QComboBox(this);
    m_cmbAlignTarget->addItems({ "Canvas", "Selection" });
    hboxTarget->addWidget(m_cmbAlignTarget, 1);
    vboxAlign->addLayout(hboxTarget);

    QHBoxLayout* hboxA1 = new QHBoxLayout();
    QPushButton* btnAL = new QPushButton("Left ⇤", this);
    QPushButton* btnAC = new QPushButton("Center ↔", this);
    QPushButton* btnAR = new QPushButton("Right ⇥", this);
    connect(btnAL, &QPushButton::clicked, [this]() { if (m_doc) Processing::AlignUtils::alignLayers(*m_doc, "left", m_cmbAlignTarget->currentText()); });
    connect(btnAC, &QPushButton::clicked, [this]() { if (m_doc) Processing::AlignUtils::alignLayers(*m_doc, "center", m_cmbAlignTarget->currentText()); });
    connect(btnAR, &QPushButton::clicked, [this]() { if (m_doc) Processing::AlignUtils::alignLayers(*m_doc, "right", m_cmbAlignTarget->currentText()); });
    for (auto b : { btnAL, btnAC, btnAR }) hboxA1->addWidget(b);
    vboxAlign->addLayout(hboxA1);

    QHBoxLayout* hboxA2 = new QHBoxLayout();
    QPushButton* btnAT = new QPushButton("Top ⟰", this);
    QPushButton* btnAM = new QPushButton("Middle ↕", this);
    QPushButton* btnAB = new QPushButton("Bottom ⟱", this);
    connect(btnAT, &QPushButton::clicked, [this]() { if (m_doc) Processing::AlignUtils::alignLayers(*m_doc, "top", m_cmbAlignTarget->currentText()); });
    connect(btnAM, &QPushButton::clicked, [this]() { if (m_doc) Processing::AlignUtils::alignLayers(*m_doc, "middle", m_cmbAlignTarget->currentText()); });
    connect(btnAB, &QPushButton::clicked, [this]() { if (m_doc) Processing::AlignUtils::alignLayers(*m_doc, "bottom", m_cmbAlignTarget->currentText()); });
    for (auto b : { btnAT, btnAM, btnAB }) hboxA2->addWidget(b);
    vboxAlign->addLayout(hboxA2);

    QHBoxLayout* hboxDist = new QHBoxLayout();
    QPushButton* btnDH = new QPushButton("Distribute H ⫶", this);
    QPushButton* btnDV = new QPushButton("Distribute V ⋯", this);
    connect(btnDH, &QPushButton::clicked, [this]() { if (m_doc) Processing::AlignUtils::distributeLayers(*m_doc, "horizontal"); });
    connect(btnDV, &QPushButton::clicked, [this]() { if (m_doc) Processing::AlignUtils::distributeLayers(*m_doc, "vertical"); });
    hboxDist->addWidget(btnDH);
    hboxDist->addWidget(btnDV);
    vboxAlign->addLayout(hboxDist);

    layout->addWidget(grpAlign);

    // 3. Canvas Size & Presets Section
    QGroupBox* grpCanvas = new QGroupBox("Canvas Presets & Settings", this);
    QGridLayout* gridCanv = new QGridLayout(grpCanvas);

    gridCanv->addWidget(new QLabel("Preset:", this), 0, 0);
    m_cmbPreset = new QComboBox(this);
    m_cmbPreset->addItems({
        "Custom",
        "YouTube Thumbnail (1920×1080)",
        "YouTube Shorts / Reel (1080×1920)",
        "Instagram Post (1080×1080)",
        "Square HD (2048×2048)",
        "HD Standard (1280×720)"
    });
    connect(m_cmbPreset, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int idx) {
        if (idx == 1) { m_spnCanvasW->setValue(1920); m_spnCanvasH->setValue(1080); }
        else if (idx == 2) { m_spnCanvasW->setValue(1080); m_spnCanvasH->setValue(1920); }
        else if (idx == 3) { m_spnCanvasW->setValue(1080); m_spnCanvasH->setValue(1080); }
        else if (idx == 4) { m_spnCanvasW->setValue(2048); m_spnCanvasH->setValue(2048); }
        else if (idx == 5) { m_spnCanvasW->setValue(1280); m_spnCanvasH->setValue(720); }
    });
    gridCanv->addWidget(m_cmbPreset, 0, 1, 1, 3);

    gridCanv->addWidget(new QLabel("Width:", this), 1, 0);
    m_spnCanvasW = new QSpinBox(this);
    m_spnCanvasW->setRange(10, 16000);
    m_spnCanvasW->setValue(1920);
    gridCanv->addWidget(m_spnCanvasW, 1, 1);

    gridCanv->addWidget(new QLabel("Height:", this), 1, 2);
    m_spnCanvasH = new QSpinBox(this);
    m_spnCanvasH->setRange(10, 16000);
    m_spnCanvasH->setValue(1080);
    gridCanv->addWidget(m_spnCanvasH, 1, 3);

    QPushButton* btnResizeCanv = new QPushButton("📐 Resize Canvas", this);
    connect(btnResizeCanv, &QPushButton::clicked, [this]() {
        if (m_doc) m_doc->setCanvasSize(m_spnCanvasW->value(), m_spnCanvasH->value());
    });
    gridCanv->addWidget(btnResizeCanv, 2, 0, 1, 4);

    m_chkGrid = new QCheckBox("Show Grid", this);
    connect(m_chkGrid, &QCheckBox::toggled, [this](bool checked) {
        if (m_doc) { m_doc->showGrid = checked; m_doc->notifyChanged(); }
    });
    m_chkSnap = new QCheckBox("Snap to Grid/Edges", this);
    m_chkSnap->setChecked(true);
    connect(m_chkSnap, &QCheckBox::toggled, [this](bool checked) {
        if (m_doc) m_doc->snapEnabled = checked;
    });

    gridCanv->addWidget(m_chkGrid, 3, 0, 1, 2);
    gridCanv->addWidget(m_chkSnap, 3, 2, 1, 2);

    layout->addWidget(grpCanvas);
}

void TransformPanel::updatePanel() {
    if (!m_doc || m_updatingUi) return;
    m_updatingUi = true;

    try {
        m_spnCanvasW->setValue(m_doc->canvasWidth);
        m_spnCanvasH->setValue(m_doc->canvasHeight);
        m_chkGrid->setChecked(m_doc->showGrid);
        m_chkSnap->setChecked(m_doc->snapEnabled);

        auto active = m_doc->getActiveLayer();
        if (active) {
            m_spnPosX->setValue(active->offsetX);
            m_spnPosY->setValue(active->offsetY);
            m_spnScaleX->setValue(active->scaleX);
            m_spnScaleY->setValue(active->scaleY);
            m_chkLockAspect->setChecked(active->lockAspect);
            m_spnRotation->setValue(active->rotation);
            m_btnFlipH->setChecked(active->flipH);
            m_btnFlipV->setChecked(active->flipV);
        }
    } catch (...) {}
    m_updatingUi = false;
}

} // namespace UI
} // namespace ImageCut
