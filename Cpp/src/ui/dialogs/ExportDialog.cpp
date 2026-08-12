#include "ui/dialogs/ExportDialog.h"
#include "workers/ExportWorker.h"
#include "utils/Settings.h"
#include "utils/Logger.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QDir>
#include <QMessageBox>

namespace ImageCut {
namespace UI {

ExportDialog::ExportDialog(std::shared_ptr<Core::ImageDocument> doc, QWidget* parent)
    : QDialog(parent), m_doc(doc)
{
    setWindowTitle("Export Image");
    setFixedSize(480, 420);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(14);

    QGroupBox* grpFile = new QGroupBox("Export Destination", this);
    QVBoxLayout* vboxFile = new QVBoxLayout(grpFile);
    QHBoxLayout* hboxPath = new QHBoxLayout();
    m_txtPath = new QLineEdit(this);
    QString defaultDir = Utils::Settings::getInstance().get("export_folder", QDir::homePath());
    m_txtPath->setText(defaultDir + "/export_nobg.png");
    hboxPath->addWidget(m_txtPath);

    QPushButton* btnBrowse = new QPushButton("Browse...", this);
    connect(btnBrowse, &QPushButton::clicked, [this]() {
        QString fmt = m_comboFormat->currentText();
        QString filter = QString("%1 Images (*.%2)").arg(fmt).arg(fmt.toLower());
        QString path = QFileDialog::getSaveFileName(this, "Save Image", m_txtPath->text(), filter);
        if (!path.isEmpty()) m_txtPath->setText(path);
    });
    hboxPath->addWidget(btnBrowse);
    vboxFile->addLayout(hboxPath);
    layout->addWidget(grpFile);

    QGroupBox* grpFmt = new QGroupBox("Format & Quality", this);
    QVBoxLayout* vboxFmt = new QVBoxLayout(grpFmt);

    QHBoxLayout* hboxFmt = new QHBoxLayout();
    hboxFmt->addWidget(new QLabel("Format:", this));
    m_comboFormat = new QComboBox(this);
    m_comboFormat->addItems({ "PNG", "JPG", "WEBP" });
    connect(m_comboFormat, &QComboBox::currentTextChanged, [this](const QString& fmt) {
        bool isLossy = (fmt == "JPG" || fmt == "WEBP");
        m_sliderQuality->setEnabled(isLossy);
    });
    hboxFmt->addWidget(m_comboFormat);
    vboxFmt->addLayout(hboxFmt);

    QHBoxLayout* hboxQual = new QHBoxLayout();
    hboxQual->addWidget(new QLabel("Quality:", this));
    m_lblQualVal = new QLabel("95%", this);
    hboxQual->addWidget(m_lblQualVal);
    m_sliderQuality = new QSlider(Qt::Horizontal, this);
    m_sliderQuality->setRange(1, 100);
    m_sliderQuality->setValue(95);
    connect(m_sliderQuality, &QSlider::valueChanged, [this](int v) {
        m_lblQualVal->setText(QString("%1%").arg(v));
    });
    hboxQual->addWidget(m_sliderQuality);
    vboxFmt->addLayout(hboxQual);
    layout->addWidget(grpFmt);

    QGroupBox* grpDim = new QGroupBox("Resolution / Dimensions & Presets", this);
    QVBoxLayout* vboxDim = new QVBoxLayout(grpDim);

    QHBoxLayout* hboxPreset = new QHBoxLayout();
    hboxPreset->addWidget(new QLabel("Preset:", this));
    m_comboPreset = new QComboBox(this);
    m_comboPreset->addItems({
        "Current Canvas Size",
        "YouTube Thumbnail (1920×1080)",
        "YouTube Shorts / Reels (1080×1920)",
        "Instagram Post (1080×1080)",
        "Instagram Story (1080×1920)",
        "Custom"
    });
    connect(m_comboPreset, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int idx) {
        int origW = m_doc ? m_doc->canvasWidth : 1920;
        int origH = m_doc ? m_doc->canvasHeight : 1080;
        if (idx == 0) { m_spinW->setValue(origW); m_spinH->setValue(origH); }
        else if (idx == 1) { m_spinW->setValue(1920); m_spinH->setValue(1080); }
        else if (idx == 2) { m_spinW->setValue(1080); m_spinH->setValue(1920); }
        else if (idx == 3) { m_spinW->setValue(1080); m_spinH->setValue(1080); }
        else if (idx == 4) { m_spinW->setValue(1080); m_spinH->setValue(1920); }
    });
    hboxPreset->addWidget(m_comboPreset, 1);
    vboxDim->addLayout(hboxPreset);

    QHBoxLayout* hboxSpins = new QHBoxLayout();
    int origW = m_doc ? m_doc->canvasWidth : 1920;
    int origH = m_doc ? m_doc->canvasHeight : 1080;

    hboxSpins->addWidget(new QLabel("Width:", this));
    m_spinW = new QSpinBox(this);
    m_spinW->setRange(1, 10000);
    m_spinW->setValue(origW);
    hboxSpins->addWidget(m_spinW);

    hboxSpins->addWidget(new QLabel("Height:", this));
    m_spinH = new QSpinBox(this);
    m_spinH->setRange(1, 10000);
    m_spinH->setValue(origH);
    hboxSpins->addWidget(m_spinH);
    vboxDim->addLayout(hboxSpins);

    m_chkAspect = new QCheckBox("Lock Aspect Ratio", this);
    m_chkAspect->setChecked(true);
    vboxDim->addWidget(m_chkAspect);
    layout->addWidget(grpDim);

    QHBoxLayout* hboxBtns = new QHBoxLayout();
    hboxBtns->addStretch();
    QPushButton* btnCancel = new QPushButton("Cancel", this);
    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    hboxBtns->addWidget(btnCancel);

    m_btnExport = new QPushButton("💾 Export Now", this);
    m_btnExport->setObjectName("btn_primary");
    connect(m_btnExport, &QPushButton::clicked, this, &ExportDialog::doExport);
    hboxBtns->addWidget(m_btnExport);
    layout->addLayout(hboxBtns);
}

void ExportDialog::doExport() {
    if (!m_doc) return;
    QString path = m_txtPath->text();
    QString fmt = m_comboFormat->currentText();
    int qual = m_sliderQuality->value();
    int w = m_spinW->value();
    int h = m_spinH->value();

    m_btnExport->setEnabled(false);
    auto worker = new Workers::ExportWorker(m_doc, path, fmt, qual, w, h);
    
    // Safely delete worker object ONLY after QThread has completely stopped running!
    connect(worker, &QThread::finished, worker, &QObject::deleteLater);

    connect(worker, &Workers::ExportWorker::exportFinished, this, [this](const QString& outPath) {
        LOG_INFO("Export completed: " + outPath.toStdString());
        accept();
    }, Qt::QueuedConnection);

    connect(worker, &Workers::ExportWorker::error, this, [this](const QString& err) {
        LOG_ERROR("Export Error: " + err.toStdString());
        m_btnExport->setEnabled(true);
        QMessageBox::critical(this, "Export Failed", "Could not export image:\n" + err);
    }, Qt::QueuedConnection);

    worker->start();
}

} // namespace UI
} // namespace ImageCut
