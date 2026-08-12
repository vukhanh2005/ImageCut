#ifndef IMAGECUT_SETTINGSDIALOG_H
#define IMAGECUT_SETTINGSDIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QPushButton>

namespace ImageCut {
namespace UI {

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget* parent = nullptr);
    ~SettingsDialog() override = default;

private:
    void saveSettings();

    QComboBox* m_comboModel = nullptr;
    QComboBox* m_comboDevice = nullptr;
    QComboBox* m_comboTheme = nullptr;
};

} // namespace UI
} // namespace ImageCut

#endif // IMAGECUT_SETTINGSDIALOG_H
