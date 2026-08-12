#include "ui/TopBarPanel.h"
#include <QHBoxLayout>

namespace ImageCut {
namespace UI {

TopBarPanel::TopBarPanel(QWidget* parent)
    : QFrame(parent)
{
    setFixedHeight(56);
    setObjectName("top_bar");

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(12, 6, 12, 6);
    layout->setSpacing(10);

    QPushButton* btnOpen = new QPushButton("📁 Open Image", this);
    btnOpen->setToolTip("Open Image File (Ctrl+O)");
    connect(btnOpen, &QPushButton::clicked, this, &TopBarPanel::openSignal);
    layout->addWidget(btnOpen);

    btnUndo = new QPushButton("↩️ Undo", this);
    btnUndo->setToolTip("Undo Last Action (Ctrl+Z)");
    connect(btnUndo, &QPushButton::clicked, this, &TopBarPanel::undoSignal);
    layout->addWidget(btnUndo);

    btnRedo = new QPushButton("↪️ Redo", this);
    btnRedo->setToolTip("Redo (Ctrl+Shift+Z)");
    connect(btnRedo, &QPushButton::clicked, this, &TopBarPanel::redoSignal);
    layout->addWidget(btnRedo);

    btnSnap = new QPushButton("🧲 Snap", this);
    btnSnap->setCheckable(true);
    btnSnap->setChecked(true);
    btnSnap->setToolTip("Toggle Magnet Snapping to Canvas & Layer Edges");
    connect(btnSnap, &QPushButton::toggled, this, &TopBarPanel::toggleSnapSignal);
    layout->addWidget(btnSnap);

    QFrame* sep = new QFrame(this);
    sep->setFrameShape(QFrame::VLine);
    sep->setStyleSheet("color: #2D3748;");
    layout->addWidget(sep);

    btnAutoRemove = new QPushButton("✨ Auto Remove BG", this);
    btnAutoRemove->setObjectName("btn_primary");
    btnAutoRemove->setToolTip("Run AI Model to Remove Background");
    btnAutoRemove->setFixedHeight(38);
    connect(btnAutoRemove, &QPushButton::clicked, this, &TopBarPanel::autoRemoveSignal);
    layout->addWidget(btnAutoRemove);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setFixedWidth(140);
    m_progressBar->setVisible(false);
    m_progressBar->setStyleSheet(
        "QProgressBar { border: 1px solid #3B4252; border-radius: 4px; text-align: center; background-color: #1E202E; color: white; }"
        "QProgressBar::chunk { background-color: #6C5CE7; border-radius: 4px; }"
    );
    layout->addWidget(m_progressBar);

    layout->addStretch();

    QPushButton* btnBatch = new QPushButton("⚡ Batch Removal", this);
    btnBatch->setToolTip("Process Folder of Multiple Images");
    connect(btnBatch, &QPushButton::clicked, this, &TopBarPanel::batchSignal);
    layout->addWidget(btnBatch);

    QPushButton* btnExport = new QPushButton("💾 Export Image", this);
    btnExport->setToolTip("Export Result Image (Ctrl+E)");
    connect(btnExport, &QPushButton::clicked, this, &TopBarPanel::exportSignal);
    layout->addWidget(btnExport);
}

void TopBarPanel::showProgress(int val) {
    if (m_progressBar) {
        m_progressBar->setValue(val);
        m_progressBar->setVisible(true);
    }
}

void TopBarPanel::hideProgress() {
    if (m_progressBar) {
        m_progressBar->setVisible(false);
    }
}

} // namespace UI
} // namespace ImageCut
