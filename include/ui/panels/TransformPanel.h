#ifndef IMAGECUT_TRANSFORMPANEL_H
#define IMAGECUT_TRANSFORMPANEL_H

#include <QWidget>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QComboBox>
#include <memory>
#include "core/ImageDocument.h"

namespace ImageCut {
namespace UI {

class TransformPanel : public QWidget {
    Q_OBJECT
public:
    explicit TransformPanel(QWidget* parent = nullptr);
    ~TransformPanel() override = default;

    void setDocument(std::shared_ptr<Core::ImageDocument> doc);
    void updatePanel();

signals:
    void applyCropSignal();

private:
    void initUi();

    std::shared_ptr<Core::ImageDocument> m_doc;
    bool m_updatingUi = false;

    QDoubleSpinBox* m_spnPosX = nullptr;
    QDoubleSpinBox* m_spnPosY = nullptr;
    QDoubleSpinBox* m_spnScaleX = nullptr;
    QDoubleSpinBox* m_spnScaleY = nullptr;
    QCheckBox* m_chkLockAspect = nullptr;
    QDoubleSpinBox* m_spnRotation = nullptr;

    QPushButton* m_btnFlipH = nullptr;
    QPushButton* m_btnFlipV = nullptr;

    QComboBox* m_cmbAlignTarget = nullptr;
    QComboBox* m_cmbPreset = nullptr;
    QSpinBox* m_spnCanvasW = nullptr;
    QSpinBox* m_spnCanvasH = nullptr;

    QCheckBox* m_chkGrid = nullptr;
    QCheckBox* m_chkSnap = nullptr;
};

} // namespace UI
} // namespace ImageCut

#endif // IMAGECUT_TRANSFORMPANEL_H
