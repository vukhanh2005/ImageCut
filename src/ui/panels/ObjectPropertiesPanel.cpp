#include "ui/panels/ObjectPropertiesPanel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QColorDialog>
#include <QScrollArea>
#include <QFrame>

namespace ImageCut {
namespace UI {

ObjectPropertiesPanel::ObjectPropertiesPanel(QWidget* parent)
    : QWidget(parent)
{
    initUi();
}

void ObjectPropertiesPanel::setDocument(std::shared_ptr<Core::ImageDocument> doc) {
    m_document = doc;
    if (m_document) {
        m_document->addChangeListener([this]() {
            updateProperties();
        });
    }
    updateProperties();
}

void ObjectPropertiesPanel::initUi() {
    QVBoxLayout* mainVbox = new QVBoxLayout(this);
    mainVbox->setContentsMargins(6, 6, 6, 6);
    mainVbox->setSpacing(6);

    QScrollArea* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet("QScrollArea { background-color: transparent; }");

    QWidget* container = new QWidget(scrollArea);
    QVBoxLayout* containerLayout = new QVBoxLayout(container);
    containerLayout->setContentsMargins(4, 4, 4, 4);
    containerLayout->setSpacing(8);

    m_stackedPages = new QStackedWidget(container);
    m_stackedPages->addWidget(createEmptyWidget());          // 0: Empty
    m_stackedPages->addWidget(createTextPropertiesWidget());   // 1: Text
    m_stackedPages->addWidget(createShapePropertiesWidget());  // 2: Shape
    m_stackedPages->addWidget(createImagePropertiesWidget());  // 3: Image

    containerLayout->addWidget(m_stackedPages);
    containerLayout->addStretch();
    container->setLayout(containerLayout);
    scrollArea->setWidget(container);

    mainVbox->addWidget(scrollArea);
}

void ObjectPropertiesPanel::pickColor(const QString& title, QColor initial, std::function<void(const QColor&)> onPicked) {
    QColor c = QColorDialog::getColor(initial, this, title, QColorDialog::ShowAlphaChannel);
    if (c.isValid() && onPicked) {
        onPicked(c);
        if (m_document) m_document->notifyChanged();
    }
}

QWidget* ObjectPropertiesPanel::createEmptyWidget() {
    QWidget* w = new QWidget(this);
    QVBoxLayout* v = new QVBoxLayout(w);
    v->setContentsMargins(16, 32, 16, 16);
    v->setAlignment(Qt::AlignCenter);

    QLabel* icon = new QLabel("🎨", w);
    icon->setStyleSheet("font-size: 36px;");
    icon->setAlignment(Qt::AlignCenter);
    v->addWidget(icon);

    QLabel* lbl = new QLabel("No Object Selected", w);
    lbl->setStyleSheet("color: #E2E8F0; font-size: 14px; font-weight: bold; padding-top: 8px;");
    lbl->setAlignment(Qt::AlignCenter);
    v->addWidget(lbl);

    QLabel* info = new QLabel("Click any layer from the Canvas or Layers list to view & edit properties.", w);
    info->setWordWrap(true);
    info->setStyleSheet("color: #718096; font-size: 11px; padding-top: 4px;");
    info->setAlignment(Qt::AlignCenter);
    v->addWidget(info);

    v->addStretch();
    return w;
}

QWidget* ObjectPropertiesPanel::createTextPropertiesWidget() {
    QWidget* w = new QWidget(this);
    QVBoxLayout* v = new QVBoxLayout(w);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(8);

    // 1. Text Content Group
    QGroupBox* grpText = new QGroupBox("📝 Text Content", w);
    QVBoxLayout* vText = new QVBoxLayout(grpText);
    m_txtContent = new QLineEdit(grpText);
    m_txtContent->setPlaceholderText("Enter text here...");
    connect(m_txtContent, &QLineEdit::textChanged, [this](const QString& text) {
        if (m_updatingUi || !m_document) return;
        auto lyr = m_document->getActiveLayer();
        if (lyr && lyr->layerType == "text") {
            lyr->textContent = text;
            lyr->invalidateCache();
            m_document->notifyChanged();
        }
    });
    vText->addWidget(m_txtContent);
    v->addWidget(grpText);

    // 2. Font & Typography Group
    QGroupBox* grpFont = new QGroupBox("🔤 Typography & Size", w);
    QVBoxLayout* vFont = new QVBoxLayout(grpFont);

    m_cmbFontFamily = new QFontComboBox(grpFont);
    connect(m_cmbFontFamily, &QFontComboBox::currentFontChanged, [this](const QFont& f) {
        if (m_updatingUi || !m_document) return;
        auto lyr = m_document->getActiveLayer();
        if (lyr && lyr->layerType == "text") {
            lyr->fontFamily = f.family();
            lyr->invalidateCache();
            m_document->notifyChanged();
        }
    });
    vFont->addWidget(m_cmbFontFamily);

    QHBoxLayout* hFontOpt = new QHBoxLayout();
    m_spnFontSize = new QSpinBox(grpFont);
    m_spnFontSize->setRange(6, 300);
    m_spnFontSize->setValue(48);
    m_spnFontSize->setSuffix(" pt");
    connect(m_spnFontSize, QOverload<int>::of(&QSpinBox::valueChanged), [this](int val) {
        if (m_updatingUi || !m_document) return;
        auto lyr = m_document->getActiveLayer();
        if (lyr && lyr->layerType == "text") {
            lyr->fontSize = val;
            lyr->invalidateCache();
            m_document->notifyChanged();
        }
    });
    hFontOpt->addWidget(m_spnFontSize, 1);

    m_btnBold = new QPushButton("B", grpFont);
    m_btnBold->setCheckable(true);
    m_btnBold->setFixedSize(30, 28);
    m_btnBold->setStyleSheet("font-weight: bold;");
    connect(m_btnBold, &QPushButton::toggled, [this](bool checked) {
        if (m_updatingUi || !m_document) return;
        auto lyr = m_document->getActiveLayer();
        if (lyr && lyr->layerType == "text") {
            lyr->fontBold = checked;
            lyr->invalidateCache();
            m_document->notifyChanged();
        }
    });
    hFontOpt->addWidget(m_btnBold);

    m_btnItalic = new QPushButton("I", grpFont);
    m_btnItalic->setCheckable(true);
    m_btnItalic->setFixedSize(30, 28);
    m_btnItalic->setStyleSheet("font-style: italic; font-family: serif;");
    connect(m_btnItalic, &QPushButton::toggled, [this](bool checked) {
        if (m_updatingUi || !m_document) return;
        auto lyr = m_document->getActiveLayer();
        if (lyr && lyr->layerType == "text") {
            lyr->fontItalic = checked;
            lyr->invalidateCache();
            m_document->notifyChanged();
        }
    });
    hFontOpt->addWidget(m_btnItalic);

    vFont->addLayout(hFontOpt);
    v->addWidget(grpFont);

    // 3. Text Color & Styling Group
    QGroupBox* grpStyle = new QGroupBox("🎨 Text Color & Effects", w);
    QVBoxLayout* vStyle = new QVBoxLayout(grpStyle);

    QHBoxLayout* hCol = new QHBoxLayout();
    hCol->addWidget(new QLabel("Text Color:", grpStyle));
    m_btnTextColor = new QPushButton(grpStyle);
    m_btnTextColor->setFixedHeight(26);
    connect(m_btnTextColor, &QPushButton::clicked, [this]() {
        if (!m_document) return;
        auto lyr = m_document->getActiveLayer();
        if (!lyr) return;
        pickColor("Select Text Color", lyr->textColor, [lyr](const QColor& c) {
            lyr->textColor = c;
            lyr->invalidateCache();
        });
    });
    hCol->addWidget(m_btnTextColor, 1);
    vStyle->addLayout(hCol);

    // Stroke
    m_chkTextStroke = new QCheckBox("Text Outline / Stroke", grpStyle);
    connect(m_chkTextStroke, &QCheckBox::toggled, [this](bool checked) {
        if (m_updatingUi || !m_document) return;
        auto lyr = m_document->getActiveLayer();
        if (lyr && lyr->layerType == "text") {
            lyr->textHasStroke = checked;
            lyr->invalidateCache();
            m_document->notifyChanged();
        }
    });
    vStyle->addWidget(m_chkTextStroke);

    QHBoxLayout* hStroke = new QHBoxLayout();
    m_btnTextStrokeColor = new QPushButton(grpStyle);
    m_btnTextStrokeColor->setFixedHeight(24);
    connect(m_btnTextStrokeColor, &QPushButton::clicked, [this]() {
        if (!m_document) return;
        auto lyr = m_document->getActiveLayer();
        if (!lyr) return;
        pickColor("Select Stroke Color", lyr->textStrokeColor, [lyr](const QColor& c) {
            lyr->textStrokeColor = c;
            lyr->invalidateCache();
        });
    });
    hStroke->addWidget(m_btnTextStrokeColor, 1);

    m_spnTextStrokeWidth = new QSpinBox(grpStyle);
    m_spnTextStrokeWidth->setRange(1, 30);
    m_spnTextStrokeWidth->setSuffix(" px");
    connect(m_spnTextStrokeWidth, QOverload<int>::of(&QSpinBox::valueChanged), [this](int val) {
        if (m_updatingUi || !m_document) return;
        auto lyr = m_document->getActiveLayer();
        if (lyr && lyr->layerType == "text") {
            lyr->textStrokeWidth = val;
            lyr->invalidateCache();
            m_document->notifyChanged();
        }
    });
    hStroke->addWidget(m_spnTextStrokeWidth);
    vStyle->addLayout(hStroke);

    // Drop Shadow
    m_chkTextShadow = new QCheckBox("Drop Shadow (Bóng đổ)", grpStyle);
    connect(m_chkTextShadow, &QCheckBox::toggled, [this](bool checked) {
        if (m_updatingUi || !m_document) return;
        auto lyr = m_document->getActiveLayer();
        if (lyr && lyr->layerType == "text") {
            lyr->textHasShadow = checked;
            lyr->invalidateCache();
            m_document->notifyChanged();
        }
    });
    vStyle->addWidget(m_chkTextShadow);

    QHBoxLayout* hShadow = new QHBoxLayout();
    m_btnTextShadowColor = new QPushButton(grpStyle);
    m_btnTextShadowColor->setFixedHeight(24);
    connect(m_btnTextShadowColor, &QPushButton::clicked, [this]() {
        if (!m_document) return;
        auto lyr = m_document->getActiveLayer();
        if (!lyr) return;
        pickColor("Select Shadow Color", lyr->textShadowColor, [lyr](const QColor& c) {
            lyr->textShadowColor = c;
            lyr->invalidateCache();
        });
    });
    hShadow->addWidget(m_btnTextShadowColor, 1);

    m_spnTextShadowX = new QSpinBox(grpStyle);
    m_spnTextShadowX->setRange(-50, 50);
    connect(m_spnTextShadowX, QOverload<int>::of(&QSpinBox::valueChanged), [this](int val) {
        if (m_updatingUi || !m_document) return;
        auto lyr = m_document->getActiveLayer();
        if (lyr && lyr->layerType == "text") {
            lyr->textShadowOffsetX = val;
            lyr->invalidateCache();
            m_document->notifyChanged();
        }
    });
    hShadow->addWidget(m_spnTextShadowX);

    m_spnTextShadowY = new QSpinBox(grpStyle);
    m_spnTextShadowY->setRange(-50, 50);
    connect(m_spnTextShadowY, QOverload<int>::of(&QSpinBox::valueChanged), [this](int val) {
        if (m_updatingUi || !m_document) return;
        auto lyr = m_document->getActiveLayer();
        if (lyr && lyr->layerType == "text") {
            lyr->textShadowOffsetY = val;
            lyr->invalidateCache();
            m_document->notifyChanged();
        }
    });
    hShadow->addWidget(m_spnTextShadowY);
    vStyle->addLayout(hShadow);

    // Pill Background
    m_chkTextBg = new QCheckBox("Background Pill (Khung nền)", grpStyle);
    connect(m_chkTextBg, &QCheckBox::toggled, [this](bool checked) {
        if (m_updatingUi || !m_document) return;
        auto lyr = m_document->getActiveLayer();
        if (lyr && lyr->layerType == "text") {
            lyr->textHasBg = checked;
            lyr->invalidateCache();
            m_document->notifyChanged();
        }
    });
    vStyle->addWidget(m_chkTextBg);

    m_btnTextBgColor = new QPushButton(grpStyle);
    m_btnTextBgColor->setFixedHeight(24);
    connect(m_btnTextBgColor, &QPushButton::clicked, [this]() {
        if (!m_document) return;
        auto lyr = m_document->getActiveLayer();
        if (!lyr) return;
        pickColor("Select Background Color", lyr->textBgColor, [lyr](const QColor& c) {
            lyr->textBgColor = c;
            lyr->invalidateCache();
        });
    });
    vStyle->addWidget(m_btnTextBgColor);

    v->addWidget(grpStyle);
    return w;
}

QWidget* ObjectPropertiesPanel::createShapePropertiesWidget() {
    QWidget* w = new QWidget(this);
    QVBoxLayout* v = new QVBoxLayout(w);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(8);

    QGroupBox* grpShape = new QGroupBox("⭐ Shape Options", w);
    QVBoxLayout* vShape = new QVBoxLayout(grpShape);

    vShape->addWidget(new QLabel("Shape Type:", grpShape));
    m_cmbShapeType = new QComboBox(grpShape);
    m_cmbShapeType->addItems({
        "Rectangle", "RoundedRectangle", "Circle", "Triangle", "Diamond",
        "Arrow", "Star", "SpeechBubble", "Heart", "Hexagon", "Octagon", "Shield"
    });
    connect(m_cmbShapeType, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int idx) {
        if (m_updatingUi || !m_document) return;
        auto lyr = m_document->getActiveLayer();
        if (lyr && lyr->layerType == "shape") {
            lyr->shapeType = m_cmbShapeType->itemText(idx);
            lyr->invalidateCache();
            m_document->notifyChanged();
        }
    });
    vShape->addWidget(m_cmbShapeType);

    QHBoxLayout* hFill = new QHBoxLayout();
    hFill->addWidget(new QLabel("Fill Color:", grpShape));
    m_btnShapeFill = new QPushButton(grpShape);
    m_btnShapeFill->setFixedHeight(26);
    connect(m_btnShapeFill, &QPushButton::clicked, [this]() {
        if (!m_document) return;
        auto lyr = m_document->getActiveLayer();
        if (!lyr) return;
        pickColor("Select Fill Color", lyr->fillColor, [lyr](const QColor& c) {
            lyr->fillColor = c;
            lyr->invalidateCache();
        });
    });
    hFill->addWidget(m_btnShapeFill, 1);
    vShape->addLayout(hFill);

    QHBoxLayout* hStroke = new QHBoxLayout();
    hStroke->addWidget(new QLabel("Stroke Color:", grpShape));
    m_btnShapeStroke = new QPushButton(grpShape);
    m_btnShapeStroke->setFixedHeight(26);
    connect(m_btnShapeStroke, &QPushButton::clicked, [this]() {
        if (!m_document) return;
        auto lyr = m_document->getActiveLayer();
        if (!lyr) return;
        pickColor("Select Stroke Color", lyr->strokeColor, [lyr](const QColor& c) {
            lyr->strokeColor = c;
            lyr->invalidateCache();
        });
    });
    hStroke->addWidget(m_btnShapeStroke, 1);
    vShape->addLayout(hStroke);

    QHBoxLayout* hStrokeW = new QHBoxLayout();
    hStrokeW->addWidget(new QLabel("Stroke Width:", grpShape));
    m_spnShapeStrokeWidth = new QSpinBox(grpShape);
    m_spnShapeStrokeWidth->setRange(0, 50);
    m_spnShapeStrokeWidth->setSuffix(" px");
    connect(m_spnShapeStrokeWidth, QOverload<int>::of(&QSpinBox::valueChanged), [this](int val) {
        if (m_updatingUi || !m_document) return;
        auto lyr = m_document->getActiveLayer();
        if (lyr && lyr->layerType == "shape") {
            lyr->strokeWidth = val;
            lyr->invalidateCache();
            m_document->notifyChanged();
        }
    });
    hStrokeW->addWidget(m_spnShapeStrokeWidth, 1);
    vShape->addLayout(hStrokeW);

    v->addWidget(grpShape);
    return w;
}

QWidget* ObjectPropertiesPanel::createImagePropertiesWidget() {
    QWidget* w = new QWidget(this);
    QVBoxLayout* v = new QVBoxLayout(w);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(8);

    QGroupBox* grpImg = new QGroupBox("📷 Image Layer Info", w);
    QVBoxLayout* vImg = new QVBoxLayout(grpImg);

    vImg->addWidget(new QLabel("Layer Name:", grpImg));
    m_txtLayerName = new QLineEdit(grpImg);
    connect(m_txtLayerName, &QLineEdit::textChanged, [this](const QString& text) {
        if (m_updatingUi || !m_document) return;
        auto lyr = m_document->getActiveLayer();
        if (lyr) {
            lyr->name = text;
            m_document->notifyChanged();
        }
    });
    vImg->addWidget(m_txtLayerName);

    QHBoxLayout* hOpac = new QHBoxLayout();
    hOpac->addWidget(new QLabel("Opacity:", grpImg));
    m_sldOpacity = new QSlider(Qt::Horizontal, grpImg);
    m_sldOpacity->setRange(0, 100);
    connect(m_sldOpacity, &QSlider::valueChanged, [this](int val) {
        if (m_lblOpacityVal) m_lblOpacityVal->setText(QString("%1%").arg(val));
        if (m_updatingUi || !m_document) return;
        auto lyr = m_document->getActiveLayer();
        if (lyr) {
            lyr->opacity = val / 100.0;
            m_document->notifyChanged();
        }
    });
    hOpac->addWidget(m_sldOpacity, 1);

    m_lblOpacityVal = new QLabel("100%", grpImg);
    m_lblOpacityVal->setFixedWidth(36);
    hOpac->addWidget(m_lblOpacityVal);
    vImg->addLayout(hOpac);

    v->addWidget(grpImg);
    return w;
}

void ObjectPropertiesPanel::updateProperties() {
    if (!m_document) {
        m_stackedPages->setCurrentIndex(0);
        return;
    }

    auto lyr = m_document->getActiveLayer();
    if (!lyr) {
        m_stackedPages->setCurrentIndex(0);
        return;
    }

    m_updatingUi = true;

    if (lyr->layerType == "text") {
        m_stackedPages->setCurrentIndex(1);
        m_txtContent->setText(lyr->textContent);
        m_cmbFontFamily->setCurrentFont(QFont(lyr->fontFamily));
        m_spnFontSize->setValue(lyr->fontSize);
        m_btnBold->setChecked(lyr->fontBold);
        m_btnItalic->setChecked(lyr->fontItalic);

        m_btnTextColor->setStyleSheet(QString("background-color: %1; border: 1px solid #4A5568; border-radius: 4px;").arg(lyr->textColor.name()));

        m_chkTextStroke->setChecked(lyr->textHasStroke);
        m_btnTextStrokeColor->setStyleSheet(QString("background-color: %1; border: 1px solid #4A5568; border-radius: 4px;").arg(lyr->textStrokeColor.name()));
        m_spnTextStrokeWidth->setValue(lyr->textStrokeWidth);

        m_chkTextShadow->setChecked(lyr->textHasShadow);
        m_btnTextShadowColor->setStyleSheet(QString("background-color: %1; border: 1px solid #4A5568; border-radius: 4px;").arg(lyr->textShadowColor.name()));
        m_spnTextShadowX->setValue(lyr->textShadowOffsetX);
        m_spnTextShadowY->setValue(lyr->textShadowOffsetY);

        m_chkTextBg->setChecked(lyr->textHasBg);
        m_btnTextBgColor->setStyleSheet(QString("background-color: %1; border: 1px solid #4A5568; border-radius: 4px;").arg(lyr->textBgColor.name()));
    } else if (lyr->layerType == "shape") {
        m_stackedPages->setCurrentIndex(2);
        int idx = m_cmbShapeType->findText(lyr->shapeType);
        if (idx >= 0) m_cmbShapeType->setCurrentIndex(idx);
        m_btnShapeFill->setStyleSheet(QString("background-color: %1; border: 1px solid #4A5568; border-radius: 4px;").arg(lyr->fillColor.name()));
        m_btnShapeStroke->setStyleSheet(QString("background-color: %1; border: 1px solid #4A5568; border-radius: 4px;").arg(lyr->strokeColor.name()));
        m_spnShapeStrokeWidth->setValue(lyr->strokeWidth);
    } else {
        m_stackedPages->setCurrentIndex(3);
        m_txtLayerName->setText(lyr->name);
        int opVal = static_cast<int>(lyr->opacity * 100);
        m_sldOpacity->setValue(opVal);
        if (m_lblOpacityVal) m_lblOpacityVal->setText(QString("%1%").arg(opVal));
    }

    m_updatingUi = false;
}

} // namespace UI
} // namespace ImageCut
