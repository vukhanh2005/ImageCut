#ifndef IMAGECUT_TOOLPROPERTIESPANEL_H
#define IMAGECUT_TOOLPROPERTIESPANEL_H

#include <QFrame>
#include <QLabel>
#include <QSpinBox>
#include <QSlider>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QHBoxLayout>
#include <QStackedWidget>
#include <memory>
#include "tools/BaseTool.h"

namespace ImageCut {
namespace UI {

class ToolPropertiesPanel : public QFrame {
    Q_OBJECT
public:
    explicit ToolPropertiesPanel(QWidget* parent = nullptr);
    ~ToolPropertiesPanel() override = default;

    void setTool(Tools::BaseTool* tool, const QString& toolName);

signals:
    void applyCropSignal();

private:
    void initUi();
    QWidget* createBrushWidget();
    QWidget* createMagicWandWidget();
    QWidget* createLassoWidget();
    QWidget* createCropWidget();
    QWidget* createSelectWidget();

    QLabel* m_lblToolIcon = nullptr;
    QLabel* m_lblToolName = nullptr;
    QStackedWidget* m_stackedWidgets = nullptr;

    // Brush controls
    QSpinBox* m_spnBrushSize = nullptr;
    QSlider* m_sldBrushSize = nullptr;
    QSlider* m_sldHardness = nullptr;
    QSlider* m_sldOpacity = nullptr;
    QComboBox* m_cmbBrushMode = nullptr;

    // Magic Wand controls
    QSpinBox* m_spnWandTol = nullptr;
    QSlider* m_sldWandTol = nullptr;
    QCheckBox* m_chkWandContig = nullptr;

    // Lasso controls
    QComboBox* m_cmbLassoMode = nullptr;

    // Crop controls
    QComboBox* m_cmbCropPreset = nullptr;

    Tools::BaseTool* m_currentTool = nullptr;
    QString m_currentToolName;
};

} // namespace UI
} // namespace ImageCut

#endif // IMAGECUT_TOOLPROPERTIESPANEL_H
