#ifndef IMAGECUT_EXPORTDIALOG_H
#define IMAGECUT_EXPORTDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QSlider>
#include <QLabel>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <memory>
#include "core/ImageDocument.h"

namespace ImageCut {
namespace UI {

class ExportDialog : public QDialog {
    Q_OBJECT
public:
    ExportDialog(std::shared_ptr<Core::ImageDocument> doc, QWidget* parent = nullptr);
    ~ExportDialog() override = default;

private:
    void initUi();
    void doExport();

    std::shared_ptr<Core::ImageDocument> m_doc;

    QLineEdit* m_txtPath = nullptr;
    QComboBox* m_comboFormat = nullptr;
    QSlider* m_sliderQuality = nullptr;
    QLabel* m_lblQualVal = nullptr;

    QComboBox* m_comboPreset = nullptr;
    QSpinBox* m_spinW = nullptr;
    QSpinBox* m_spinH = nullptr;
    QCheckBox* m_chkAspect = nullptr;
    QPushButton* m_btnExport = nullptr;
};

} // namespace UI
} // namespace ImageCut

#endif // IMAGECUT_EXPORTDIALOG_H
