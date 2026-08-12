#include "ui/dialogs/BatchDialog.h"
#include "workers/BatchWorker.h"
#include "utils/Logger.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QDir>
#include <QDirIterator>

namespace ImageCut {
namespace UI {

BatchDialog::BatchDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Batch Background Removal");
    setFixedSize(560, 480);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);

    QGroupBox* grpIn = new QGroupBox("Input Images Queue", this);
    QVBoxLayout* vboxIn = new QVBoxLayout(grpIn);
    m_listFiles = new QListWidget(this);
    vboxIn->addWidget(m_listFiles);

    QHBoxLayout* hboxInBtns = new QHBoxLayout();
    QPushButton* btnAddFiles = new QPushButton("➕ Add Files...", this);
    connect(btnAddFiles, &QPushButton::clicked, this, &BatchDialog::addFiles);
    hboxInBtns->addWidget(btnAddFiles);

    QPushButton* btnAddDir = new QPushButton("📁 Add Folder...", this);
    connect(btnAddDir, &QPushButton::clicked, this, &BatchDialog::addFolder);
    hboxInBtns->addWidget(btnAddDir);

    QPushButton* btnClear = new QPushButton("🗑️ Clear Queue", this);
    connect(btnClear, &QPushButton::clicked, this, &BatchDialog::clearQueue);
    hboxInBtns->addWidget(btnClear);
    vboxIn->addLayout(hboxInBtns);
    layout->addWidget(grpIn);

    QGroupBox* grpOut = new QGroupBox("Batch Settings", this);
    QVBoxLayout* vboxOut = new QVBoxLayout(grpOut);

    QHBoxLayout* hboxOutDir = new QHBoxLayout();
    hboxOutDir->addWidget(new QLabel("Output Dir:", this));
    m_txtOutDir = new QLineEdit(this);
    m_txtOutDir->setText(QDir::homePath() + "/BackgroundRemover_Output");
    hboxOutDir->addWidget(m_txtOutDir);

    QPushButton* btnBrowseOut = new QPushButton("Browse...", this);
    connect(btnBrowseOut, &QPushButton::clicked, this, &BatchDialog::browseOutput);
    hboxOutDir->addWidget(btnBrowseOut);
    vboxOut->addLayout(hboxOutDir);

    QHBoxLayout* hboxOpts = new QHBoxLayout();
    hboxOpts->addWidget(new QLabel("AI Model:", this));
    m_comboModel = new QComboBox(this);
    m_comboModel->addItems({ "RMBG-1.4", "U2Net", "Silueta" });
    hboxOpts->addWidget(m_comboModel);

    hboxOpts->addWidget(new QLabel("Format:", this));
    m_comboFmt = new QComboBox(this);
    m_comboFmt->addItems({ "PNG", "JPG", "WEBP" });
    hboxOpts->addWidget(m_comboFmt);
    vboxOut->addLayout(hboxOpts);

    layout->addWidget(grpOut);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setValue(0);
    layout->addWidget(m_progressBar);

    m_lblStatus = new QLabel("Ready to process.", this);
    layout->addWidget(m_lblStatus);

    QHBoxLayout* hboxActions = new QHBoxLayout();
    hboxActions->addStretch();
    m_btnStart = new QPushButton("⚡ Start Batch", this);
    m_btnStart->setObjectName("btn_primary");
    connect(m_btnStart, &QPushButton::clicked, this, &BatchDialog::startBatch);
    hboxActions->addWidget(m_btnStart);

    QPushButton* btnClose = new QPushButton("Close", this);
    connect(btnClose, &QPushButton::clicked, this, &QDialog::accept);
    hboxActions->addWidget(btnClose);
    layout->addLayout(hboxActions);
}

void BatchDialog::addFiles() {
    QStringList files = QFileDialog::getOpenFileNames(this, "Select Images", "", "Images (*.png *.jpg *.jpeg *.webp *.bmp *.tiff)");
    for (const auto& f : files) {
        if (!m_filePaths.contains(f)) {
            m_filePaths.append(f);
            QFileInfo info(f);
            m_listFiles->addItem(info.fileName());
        }
    }
    m_lblStatus->setText(QString("%1 files queued.").arg(m_filePaths.size()));
}

void BatchDialog::addFolder() {
    QString folder = QFileDialog::getExistingDirectory(this, "Select Image Directory");
    if (!folder.isEmpty()) {
        int count = 0;
        QDirIterator it(folder, QStringList() << "*.png" << "*.jpg" << "*.jpeg" << "*.webp" << "*.bmp" << "*.tiff", QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            QString fullP = it.next();
            if (!m_filePaths.contains(fullP)) {
                m_filePaths.append(fullP);
                m_listFiles->addItem(it.fileName());
                count++;
            }
        }
        m_lblStatus->setText(QString("Added %1 files. Total queued: %2").arg(count).arg(m_filePaths.size()));
    }
}

void BatchDialog::clearQueue() {
    m_filePaths.clear();
    m_listFiles->clear();
    m_lblStatus->setText("Queue cleared.");
    m_progressBar->setValue(0);
}

void BatchDialog::browseOutput() {
    QString folder = QFileDialog::getExistingDirectory(this, "Select Output Directory");
    if (!folder.isEmpty()) {
        m_txtOutDir->setText(folder);
    }
}

void BatchDialog::startBatch() {
    if (m_filePaths.isEmpty()) {
        m_lblStatus->setText("Error: Queue is empty.");
        return;
    }

    m_btnStart->setEnabled(false);
    auto worker = new Workers::BatchWorker(m_filePaths, m_txtOutDir->text(), m_comboModel->currentText(), m_comboFmt->currentText());
    connect(worker, &Workers::BatchWorker::progress, [this](int idx, int total, const QString& filename) {
        int pct = static_cast<int>((idx / static_cast<double>(total)) * 100);
        m_progressBar->setValue(pct);
        m_lblStatus->setText(QString("Processing (%1/%2): %3").arg(idx).arg(total).arg(filename));
    });
    connect(worker, &Workers::BatchWorker::finished, [this, worker](int success, int fail) {
        m_btnStart->setEnabled(true);
        m_lblStatus->setText(QString("Batch completed! Success: %1, Failed: %2").arg(success).arg(fail));
        worker->deleteLater();
    });
    worker->start();
}

} // namespace UI
} // namespace ImageCut
