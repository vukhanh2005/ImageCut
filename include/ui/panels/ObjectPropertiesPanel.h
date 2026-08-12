#ifndef IMAGECUT_OBJECTPROPERTIESPANEL_H
#define IMAGECUT_OBJECTPROPERTIESPANEL_H

#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QFontComboBox>
#include <QPushButton>
#include <QCheckBox>
#include <QSlider>
#include <QStackedWidget>
#include <QGroupBox>
#include <functional>
#include <memory>
#include "core/ImageDocument.h"

namespace ImageCut {
namespace UI {

class ObjectPropertiesPanel : public QWidget {
    Q_OBJECT
public:
    explicit ObjectPropertiesPanel(QWidget* parent = nullptr);
    ~ObjectPropertiesPanel() override = default;

    void setDocument(std::shared_ptr<Core::ImageDocument> doc);
    void updateProperties();

private:
    void initUi();
    QWidget* createEmptyWidget();
    QWidget* createTextPropertiesWidget();
    QWidget* createShapePropertiesWidget();
    QWidget* createImagePropertiesWidget();

    void pickColor(const QString& title, QColor initial, std::function<void(const QColor&)> onPicked);

    std::shared_ptr<Core::ImageDocument> m_document;
    bool m_updatingUi = false;

    QStackedWidget* m_stackedPages = nullptr;

    // Text Widgets
    QLineEdit* m_txtContent = nullptr;
    QFontComboBox* m_cmbFontFamily = nullptr;
    QSpinBox* m_spnFontSize = nullptr;
    QPushButton* m_btnBold = nullptr;
    QPushButton* m_btnItalic = nullptr;
    QPushButton* m_btnTextColor = nullptr;

    QCheckBox* m_chkTextStroke = nullptr;
    QPushButton* m_btnTextStrokeColor = nullptr;
    QSpinBox* m_spnTextStrokeWidth = nullptr;

    QCheckBox* m_chkTextShadow = nullptr;
    QPushButton* m_btnTextShadowColor = nullptr;
    QSpinBox* m_spnTextShadowX = nullptr;
    QSpinBox* m_spnTextShadowY = nullptr;

    QCheckBox* m_chkTextBg = nullptr;
    QPushButton* m_btnTextBgColor = nullptr;

    // Shape Widgets
    QComboBox* m_cmbShapeType = nullptr;
    QPushButton* m_btnShapeFill = nullptr;
    QPushButton* m_btnShapeStroke = nullptr;
    QSpinBox* m_spnShapeStrokeWidth = nullptr;

    // Image Widgets
    QLineEdit* m_txtLayerName = nullptr;
    QComboBox* m_cmbBlendMode = nullptr;
    QSlider* m_sldOpacity = nullptr;
    QLabel* m_lblOpacityVal = nullptr;

    QSlider* m_sldBrightness = nullptr;
    QSlider* m_sldContrast = nullptr;
    QSlider* m_sldSaturation = nullptr;
    QSlider* m_sldExposure = nullptr;
};

} // namespace UI
} // namespace ImageCut

#endif // IMAGECUT_OBJECTPROPERTIESPANEL_H
