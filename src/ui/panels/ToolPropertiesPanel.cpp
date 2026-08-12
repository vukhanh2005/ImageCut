#include "ui/panels/ToolPropertiesPanel.h"
#include "tools/BrushTool.h"
#include "tools/MagicWandTool.h"
#include "tools/LassoTool.h"
#include "tools/PolyLassoTool.h"
#include "tools/RefineEdgeTool.h"
#include "tools/CropTool.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFrame>

namespace ImageCut {
namespace UI {

ToolPropertiesPanel::ToolPropertiesPanel(QWidget* parent)
    : QFrame(parent)
{
    setFixedHeight(42);
    setObjectName("tool_properties_panel");
    setStyleSheet(
        "QFrame#tool_properties_panel { background-color: #1A1D27; border-bottom: 1px solid #2D3748; }"
        "QLabel { color: #E2E8F0; font-size: 12px; font-weight: bold; }"
        "QSpinBox, QComboBox { background-color: #26293B; color: white; border: 1px solid #3B4252; border-radius: 4px; padding: 2px 6px; }"
        "QSlider::groove:horizontal { height: 4px; background: #2D3748; border-radius: 2px; }"
        "QSlider::handle:horizontal { background: #6C5CE7; width: 12px; margin: -4px 0; border-radius: 6px; }"
    );

    initUi();
}

void ToolPropertiesPanel::initUi() {
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(14, 2, 14, 2);
    layout->setSpacing(12);

    m_lblToolIcon = new QLabel("🎨", this);
    m_lblToolIcon->setStyleSheet("font-size: 16px;");
    layout->addWidget(m_lblToolIcon);

    m_lblToolName = new QLabel("Select Tool", this);
    m_lblToolName->setStyleSheet("color: #A0AEC0; font-weight: bold; padding-right: 10px;");
    layout->addWidget(m_lblToolName);

    QFrame* sep = new QFrame(this);
    sep->setFrameShape(QFrame::VLine);
    sep->setStyleSheet("color: #2D3748;");
    layout->addWidget(sep);

    m_stackedWidgets = new QStackedWidget(this);
    m_stackedWidgets->addWidget(createSelectWidget());     // 0: Select
    m_stackedWidgets->addWidget(createBrushWidget());      // 1: Brush / Eraser
    m_stackedWidgets->addWidget(createMagicWandWidget());  // 2: MagicWand
    m_stackedWidgets->addWidget(createLassoWidget());      // 3: Lasso
    m_stackedWidgets->addWidget(createCropWidget());       // 4: Crop
    m_stackedWidgets->addWidget(createPolyLassoWidget());  // 5: PolyLasso
    m_stackedWidgets->addWidget(createRefineEdgeWidget()); // 6: RefineEdge

    layout->addWidget(m_stackedWidgets, 1);
}

QWidget* ToolPropertiesPanel::createSelectWidget() {
    QWidget* w = new QWidget(this);
    QHBoxLayout* h = new QHBoxLayout(w);
    h->setContentsMargins(0, 0, 0, 0);
    h->setSpacing(8);

    QLabel* lblInfo = new QLabel("💡 Click & Drag layers to move, rotate, or scale. Hold Shift/Ctrl for multi-select.", w);
    lblInfo->setStyleSheet("color: #718096; font-weight: normal; font-size: 11px;");
    h->addWidget(lblInfo);
    h->addStretch();
    return w;
}

QWidget* ToolPropertiesPanel::createBrushWidget() {
    QWidget* w = new QWidget(this);
    QHBoxLayout* h = new QHBoxLayout(w);
    h->setContentsMargins(0, 0, 0, 0);
    h->setSpacing(12);

    h->addWidget(new QLabel("Size:", w));
    m_spnBrushSize = new QSpinBox(w);
    m_spnBrushSize->setRange(1, 300);
    m_spnBrushSize->setValue(30);
    m_spnBrushSize->setSuffix(" px");
    h->addWidget(m_spnBrushSize);

    m_sldBrushSize = new QSlider(Qt::Horizontal, w);
    m_sldBrushSize->setRange(1, 300);
    m_sldBrushSize->setValue(30);
    m_sldBrushSize->setFixedWidth(100);
    h->addWidget(m_sldBrushSize);

    connect(m_spnBrushSize, QOverload<int>::of(&QSpinBox::valueChanged), [this](int val) {
        m_sldBrushSize->blockSignals(true);
        m_sldBrushSize->setValue(val);
        m_sldBrushSize->blockSignals(false);
        if (m_currentTool) {
            auto btool = dynamic_cast<Tools::MaskBrushTool*>(m_currentTool);
            if (btool) btool->setSize(val);
        }
    });

    connect(m_sldBrushSize, &QSlider::valueChanged, [this](int val) {
        m_spnBrushSize->blockSignals(true);
        m_spnBrushSize->setValue(val);
        m_spnBrushSize->blockSignals(false);
        if (m_currentTool) {
            auto btool = dynamic_cast<Tools::MaskBrushTool*>(m_currentTool);
            if (btool) btool->setSize(val);
        }
    });

    // Quick Size Presets
    for (int sz : { 10, 30, 50, 100 }) {
        QPushButton* btn = new QPushButton(QString("%1px").arg(sz), w);
        btn->setFixedWidth(44);
        btn->setStyleSheet("padding: 2px 4px; font-size: 11px;");
        connect(btn, &QPushButton::clicked, [this, sz]() {
            m_spnBrushSize->setValue(sz);
        });
        h->addWidget(btn);
    }

    QFrame* sep1 = new QFrame(w);
    sep1->setFrameShape(QFrame::VLine);
    sep1->setStyleSheet("color: #2D3748;");
    h->addWidget(sep1);

    h->addWidget(new QLabel("Hardness:", w));
    m_sldHardness = new QSlider(Qt::Horizontal, w);
    m_sldHardness->setRange(0, 100);
    m_sldHardness->setValue(80);
    m_sldHardness->setFixedWidth(80);
    connect(m_sldHardness, &QSlider::valueChanged, [this](int val) {
        if (m_currentTool) {
            auto btool = dynamic_cast<Tools::MaskBrushTool*>(m_currentTool);
            if (btool) btool->setHardness(val / 100.0);
        }
    });
    h->addWidget(m_sldHardness);

    h->addWidget(new QLabel("Opacity:", w));
    m_sldOpacity = new QSlider(Qt::Horizontal, w);
    m_sldOpacity->setRange(0, 100);
    m_sldOpacity->setValue(100);
    m_sldOpacity->setFixedWidth(80);
    connect(m_sldOpacity, &QSlider::valueChanged, [this](int val) {
        if (m_currentTool) {
            auto btool = dynamic_cast<Tools::MaskBrushTool*>(m_currentTool);
            if (btool) btool->setOpacity(val / 100.0);
        }
    });
    h->addWidget(m_sldOpacity);

    h->addStretch();
    return w;
}

QWidget* ToolPropertiesPanel::createMagicWandWidget() {
    QWidget* w = new QWidget(this);
    QHBoxLayout* h = new QHBoxLayout(w);
    h->setContentsMargins(0, 0, 0, 0);
    h->setSpacing(12);

    h->addWidget(new QLabel("Color Tolerance:", w));
    m_spnWandTol = new QSpinBox(w);
    m_spnWandTol->setRange(1, 150);
    m_spnWandTol->setValue(30);
    h->addWidget(m_spnWandTol);

    m_sldWandTol = new QSlider(Qt::Horizontal, w);
    m_sldWandTol->setRange(1, 150);
    m_sldWandTol->setValue(30);
    m_sldWandTol->setFixedWidth(120);
    h->addWidget(m_sldWandTol);

    connect(m_spnWandTol, QOverload<int>::of(&QSpinBox::valueChanged), [this](int val) {
        m_sldWandTol->blockSignals(true);
        m_sldWandTol->setValue(val);
        m_sldWandTol->blockSignals(false);
        if (m_currentTool) {
            auto wtool = dynamic_cast<Tools::MagicWandTool*>(m_currentTool);
            if (wtool) wtool->setTolerance(val);
        }
    });

    connect(m_sldWandTol, &QSlider::valueChanged, [this](int val) {
        m_spnWandTol->blockSignals(true);
        m_spnWandTol->setValue(val);
        m_spnWandTol->blockSignals(false);
        if (m_currentTool) {
            auto wtool = dynamic_cast<Tools::MagicWandTool*>(m_currentTool);
            if (wtool) wtool->setTolerance(val);
        }
    });

    m_chkWandContig = new QCheckBox("Contiguous (Liền kề)", w);
    m_chkWandContig->setChecked(true);
    connect(m_chkWandContig, &QCheckBox::toggled, [this](bool checked) {
        if (m_currentTool) {
            auto wtool = dynamic_cast<Tools::MagicWandTool*>(m_currentTool);
            if (wtool) wtool->setContiguous(checked);
        }
    });
    h->addWidget(m_chkWandContig);

    h->addStretch();
    return w;
}

QWidget* ToolPropertiesPanel::createLassoWidget() {
    QWidget* w = new QWidget(this);
    QHBoxLayout* h = new QHBoxLayout(w);
    h->setContentsMargins(0, 0, 0, 0);
    h->setSpacing(12);

    h->addWidget(new QLabel("Mode:", w));
    m_cmbLassoMode = new QComboBox(w);
    m_cmbLassoMode->addItems({ "Remove (Xóa nền)", "Keep (Giữ lại)" });
    connect(m_cmbLassoMode, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int idx) {
        if (m_currentTool) {
            auto ltool = dynamic_cast<Tools::LassoTool*>(m_currentTool);
            if (ltool) ltool->setMode(idx == 0 ? "Remove" : "Keep");
        }
    });
    h->addWidget(m_cmbLassoMode);

    QLabel* lblInfo = new QLabel("💡 Draw freehand path around subject to erase/keep area.", w);
    lblInfo->setStyleSheet("color: #718096; font-weight: normal; font-size: 11px;");
    h->addWidget(lblInfo);

    h->addStretch();
    return w;
}

QWidget* ToolPropertiesPanel::createPolyLassoWidget() {
    QWidget* w = new QWidget(this);
    QHBoxLayout* h = new QHBoxLayout(w);
    h->setContentsMargins(0, 0, 0, 0);
    h->setSpacing(12);

    h->addWidget(new QLabel("Mode:", w));
    m_cmbPolyLassoMode = new QComboBox(w);
    m_cmbPolyLassoMode->addItems({ "Keep (Giữ lại vùng chọn)", "Remove (Xóa nền vùng chọn)" });
    connect(m_cmbPolyLassoMode, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int idx) {
        if (m_currentTool) {
            auto pltool = dynamic_cast<Tools::PolyLassoTool*>(m_currentTool);
            if (pltool) pltool->setMode(idx == 0 ? "Keep" : "Remove");
        }
    });
    h->addWidget(m_cmbPolyLassoMode);

    QLabel* lblInfo = new QLabel("💡 Click points to outline polygon. Click start point (🟢) or press Enter to finish.", w);
    lblInfo->setStyleSheet("color: #00E6FF; font-weight: normal; font-size: 11px;");
    h->addWidget(lblInfo);

    h->addStretch();
    return w;
}

QWidget* ToolPropertiesPanel::createRefineEdgeWidget() {
    QWidget* w = new QWidget(this);
    QHBoxLayout* h = new QHBoxLayout(w);
    h->setContentsMargins(0, 0, 0, 0);
    h->setSpacing(12);

    h->addWidget(new QLabel("Brush Size:", w));
    m_spnRefineSize = new QSpinBox(w);
    m_spnRefineSize->setRange(5, 150);
    m_spnRefineSize->setValue(25);
    m_spnRefineSize->setSuffix(" px");
    h->addWidget(m_spnRefineSize);

    m_sldRefineSize = new QSlider(Qt::Horizontal, w);
    m_sldRefineSize->setRange(5, 150);
    m_sldRefineSize->setValue(25);
    m_sldRefineSize->setFixedWidth(100);
    h->addWidget(m_sldRefineSize);

    connect(m_spnRefineSize, QOverload<int>::of(&QSpinBox::valueChanged), [this](int val) {
        m_sldRefineSize->blockSignals(true);
        m_sldRefineSize->setValue(val);
        m_sldRefineSize->blockSignals(false);
        if (m_currentTool) {
            auto rtool = dynamic_cast<Tools::RefineEdgeTool*>(m_currentTool);
            if (rtool) rtool->setSize(val);
        }
    });

    connect(m_sldRefineSize, &QSlider::valueChanged, [this](int val) {
        m_spnRefineSize->blockSignals(true);
        m_spnRefineSize->setValue(val);
        m_spnRefineSize->blockSignals(false);
        if (m_currentTool) {
            auto rtool = dynamic_cast<Tools::RefineEdgeTool*>(m_currentTool);
            if (rtool) rtool->setSize(val);
        }
    });

    QFrame* sep1 = new QFrame(w);
    sep1->setFrameShape(QFrame::VLine);
    sep1->setStyleSheet("color: #2D3748;");
    h->addWidget(sep1);

    h->addWidget(new QLabel("Hair Radius:", w));
    m_spnRefineRadius = new QSpinBox(w);
    m_spnRefineRadius->setRange(1, 25);
    m_spnRefineRadius->setValue(8);
    h->addWidget(m_spnRefineRadius);

    m_sldRefineRadius = new QSlider(Qt::Horizontal, w);
    m_sldRefineRadius->setRange(1, 25);
    m_sldRefineRadius->setValue(8);
    m_sldRefineRadius->setFixedWidth(80);
    h->addWidget(m_sldRefineRadius);

    connect(m_spnRefineRadius, QOverload<int>::of(&QSpinBox::valueChanged), [this](int val) {
        m_sldRefineRadius->blockSignals(true);
        m_sldRefineRadius->setValue(val);
        m_sldRefineRadius->blockSignals(false);
        if (m_currentTool) {
            auto rtool = dynamic_cast<Tools::RefineEdgeTool*>(m_currentTool);
            if (rtool) rtool->setRadius(val);
        }
    });

    connect(m_sldRefineRadius, &QSlider::valueChanged, [this](int val) {
        m_spnRefineRadius->blockSignals(true);
        m_spnRefineRadius->setValue(val);
        m_spnRefineRadius->blockSignals(false);
        if (m_currentTool) {
            auto rtool = dynamic_cast<Tools::RefineEdgeTool*>(m_currentTool);
            if (rtool) rtool->setRadius(val);
        }
    });

    m_chkDecontaminate = new QCheckBox("Color Decontamination (Tẩy ám màu nền)", w);
    m_chkDecontaminate->setChecked(true);
    connect(m_chkDecontaminate, &QCheckBox::toggled, [this](bool checked) {
        if (m_currentTool) {
            auto rtool = dynamic_cast<Tools::RefineEdgeTool*>(m_currentTool);
            if (rtool) rtool->setDecontaminate(checked);
        }
    });
    h->addWidget(m_chkDecontaminate);

    h->addStretch();
    return w;
}

QWidget* ToolPropertiesPanel::createCropWidget() {
    QWidget* w = new QWidget(this);
    QHBoxLayout* h = new QHBoxLayout(w);
    h->setContentsMargins(0, 0, 0, 0);
    h->setSpacing(12);

    h->addWidget(new QLabel("Aspect Ratio:", w));
    m_cmbCropPreset = new QComboBox(w);
    m_cmbCropPreset->addItems({ "Free", "1:1 Square", "16:9 Landscape", "9:16 Portrait / Story", "4:3 Standard" });
    h->addWidget(m_cmbCropPreset);

    QPushButton* btnApply = new QPushButton("✔️ Apply Crop", w);
    btnApply->setStyleSheet("background-color: #00B894; color: white; font-weight: bold; padding: 3px 12px; border-radius: 4px;");
    connect(btnApply, &QPushButton::clicked, [this]() {
        if (m_currentTool) {
            auto ctool = dynamic_cast<Tools::CropTool*>(m_currentTool);
            if (ctool) ctool->applyCrop();
        }
    });
    h->addWidget(btnApply);

    h->addStretch();
    return w;
}

void ToolPropertiesPanel::setTool(Tools::BaseTool* tool, const QString& toolName) {
    m_currentTool = tool;
    m_currentToolName = toolName;

    if (toolName == "Select") {
        m_lblToolIcon->setText("🖐️");
        m_lblToolName->setText("Select & Move Tool");
        m_stackedWidgets->setCurrentIndex(0);
    } else if (toolName == "Brush") {
        m_lblToolIcon->setText("🖌️");
        m_lblToolName->setText("Restore Brush Tool");
        m_stackedWidgets->setCurrentIndex(1);
        auto btool = dynamic_cast<Tools::MaskBrushTool*>(tool);
        if (btool) {
            btool->setMode("Restore");
            m_spnBrushSize->setValue(btool->getSize());
        }
    } else if (toolName == "Eraser") {
        m_lblToolIcon->setText("🧹");
        m_lblToolName->setText("Eraser Brush Tool");
        m_stackedWidgets->setCurrentIndex(1);
        auto btool = dynamic_cast<Tools::MaskBrushTool*>(tool);
        if (btool) {
            btool->setMode("Eraser");
            m_spnBrushSize->setValue(btool->getSize());
        }
    } else if (toolName == "RefineEdge") {
        m_lblToolIcon->setText("💇");
        m_lblToolName->setText("Refine Edge Hair Matting Tool");
        m_stackedWidgets->setCurrentIndex(6);
        auto rtool = dynamic_cast<Tools::RefineEdgeTool*>(tool);
        if (rtool) {
            m_spnRefineSize->setValue(rtool->getSize());
            m_spnRefineRadius->setValue(rtool->getRadius());
            m_chkDecontaminate->setChecked(rtool->isDecontaminate());
        }
    } else if (toolName == "MagicWand") {
        m_lblToolIcon->setText("🪄");
        m_lblToolName->setText("Magic Wand Tool");
        m_stackedWidgets->setCurrentIndex(2);
        auto wtool = dynamic_cast<Tools::MagicWandTool*>(tool);
        if (wtool) {
            m_spnWandTol->setValue(wtool->getTolerance());
            m_chkWandContig->setChecked(wtool->isContiguous());
        }
    } else if (toolName == "Lasso") {
        m_lblToolIcon->setText("🪢");
        m_lblToolName->setText("Lasso Tool");
        m_stackedWidgets->setCurrentIndex(3);
    } else if (toolName == "PolyLasso") {
        m_lblToolIcon->setText("🪡");
        m_lblToolName->setText("Point-to-Point Polygonal Lasso");
        m_stackedWidgets->setCurrentIndex(5);
        auto pltool = dynamic_cast<Tools::PolyLassoTool*>(tool);
        if (pltool) {
            m_cmbPolyLassoMode->setCurrentIndex(pltool->getMode() == "Keep" ? 0 : 1);
        }
    } else if (toolName == "Crop") {
        m_lblToolIcon->setText("✂️");
        m_lblToolName->setText("Crop Tool");
        m_stackedWidgets->setCurrentIndex(4);
    }
}

} // namespace UI
} // namespace ImageCut
