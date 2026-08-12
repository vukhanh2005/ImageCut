#include "ui/dialogs/SettingsDialog.h"
#include "utils/Settings.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>

namespace ImageCut {
namespace UI {

SettingsDialog::SettingsDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Settings");
    setFixedSize(420, 360);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(14);

    QGroupBox* grpAi = new QGroupBox("AI Background Removal Settings", this);
    QVBoxLayout* vboxAi = new QVBoxLayout(grpAi);

    QHBoxLayout* hboxModel = new QHBoxLayout();
    hboxModel->addWidget(new QLabel("Default Model:", this));
    m_comboModel = new QComboBox(this);
    m_comboModel->addItems({ "RMBG-1.4", "U2Net", "Silueta" });
    m_comboModel->setCurrentText(Utils::Settings::getInstance().get("ai_model", "RMBG-1.4"));
    hboxModel->addWidget(m_comboModel);
    vboxAi->addLayout(hboxModel);

    QHBoxLayout* hboxDevice = new QHBoxLayout();
    hboxDevice->addWidget(new QLabel("Inference Hardware:", this));
    m_comboDevice = new QComboBox(this);
    m_comboDevice->addItems({ "Auto", "CUDA", "CPU" });
    m_comboDevice->setCurrentText(Utils::Settings::getInstance().get("ai_device", "Auto"));
    hboxDevice->addWidget(m_comboDevice);
    vboxAi->addLayout(hboxDevice);

    layout->addWidget(grpAi);

    QGroupBox* grpGui = new QGroupBox("Interface & Appearance", this);
    QVBoxLayout* vboxGui = new QVBoxLayout(grpGui);

    QHBoxLayout* hboxTheme = new QHBoxLayout();
    hboxTheme->addWidget(new QLabel("Theme:", this));
    m_comboTheme = new QComboBox(this);
    m_comboTheme->addItems({ "Dark", "Light" });
    m_comboTheme->setCurrentText(Utils::Settings::getInstance().get("theme", "Dark"));
    hboxTheme->addWidget(m_comboTheme);
    vboxGui->addLayout(hboxTheme);

    layout->addWidget(grpGui);

    QHBoxLayout* hboxBtns = new QHBoxLayout();
    hboxBtns->addStretch();

    QPushButton* btnCancel = new QPushButton("Cancel", this);
    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    hboxBtns->addWidget(btnCancel);

    QPushButton* btnSave = new QPushButton("Save Settings", this);
    btnSave->setObjectName("btn_primary");
    connect(btnSave, &QPushButton::clicked, this, &SettingsDialog::saveSettings);
    hboxBtns->addWidget(btnSave);

    layout->addLayout(hboxBtns);
}

void SettingsDialog::saveSettings() {
    Utils::Settings::getInstance().set("ai_model", m_comboModel->currentText());
    Utils::Settings::getInstance().set("ai_device", m_comboDevice->currentText());
    Utils::Settings::getInstance().set("theme", m_comboTheme->currentText());
    accept();
}

} // namespace UI
} // namespace ImageCut
