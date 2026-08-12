#include "ui/TopBarPanel.h"
#include "ui/Style.h"
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

    QPushButton* btnOpen = new QPushButton(" Open Image", this);
    btnOpen->setIcon(UIIcons::getIcon("add_image", QColor(220, 230, 245), 20));
    btnOpen->setToolTip("Open Image File (Ctrl+O)");
    connect(btnOpen, &QPushButton::clicked, this, &TopBarPanel::openSignal);
    layout->addWidget(btnOpen);

    QPushButton* btnOpenProj = new QPushButton(" Open Proj", this);
    btnOpenProj->setIcon(UIIcons::getIcon("add_image", QColor(220, 230, 245), 20));
    btnOpenProj->setToolTip("Open Existing Project File (.icproj)");
    connect(btnOpenProj, &QPushButton::clicked, this, &TopBarPanel::openProjectSignal);
    layout->addWidget(btnOpenProj);

    QPushButton* btnSaveProj = new QPushButton(" Save Proj", this);
    btnSaveProj->setIcon(UIIcons::getIcon("export", QColor(220, 230, 245), 20));
    btnSaveProj->setToolTip("Save Current Project File (.icproj) (Ctrl+S)");
    connect(btnSaveProj, &QPushButton::clicked, this, &TopBarPanel::saveProjectSignal);
    layout->addWidget(btnSaveProj);

    btnUndo = new QPushButton(" Undo", this);
    btnUndo->setIcon(UIIcons::getIcon("undo", QColor(220, 230, 245), 20));
    btnUndo->setToolTip("Undo Last Action (Ctrl+Z)");
    connect(btnUndo, &QPushButton::clicked, this, &TopBarPanel::undoSignal);
    layout->addWidget(btnUndo);

    btnRedo = new QPushButton(" Redo", this);
    btnRedo->setIcon(UIIcons::getIcon("redo", QColor(220, 230, 245), 20));
    btnRedo->setToolTip("Redo (Ctrl+Shift+Z)");
    connect(btnRedo, &QPushButton::clicked, this, &TopBarPanel::redoSignal);
    layout->addWidget(btnRedo);

    btnSnap = new QPushButton(" Snap: ON", this);
    btnSnap->setCheckable(true);
    btnSnap->setChecked(true);
    btnSnap->setToolTip("Toggle Magnet Snapping to Canvas & Layer Edges");

    auto updateSnapStyle = [this](bool checked) {
        if (checked) {
            btnSnap->setText(" Snap: ON");
            btnSnap->setStyleSheet(
                "QPushButton { background-color: #6C5CE7; color: white; border: 1px solid #A29BFE; border-radius: 6px; font-weight: bold; padding: 4px 10px; }"
                "QPushButton:hover { background-color: #5B4BC4; }"
            );
        } else {
            btnSnap->setText(" Snap: OFF");
            btnSnap->setStyleSheet(
                "QPushButton { background-color: #2D3748; color: #A0AEC0; border: 1px solid #4A5568; border-radius: 6px; font-weight: normal; padding: 4px 10px; }"
                "QPushButton:hover { background-color: #3A475D; color: white; }"
            );
        }
    };
    updateSnapStyle(true);
    connect(btnSnap, &QPushButton::toggled, [this, updateSnapStyle](bool checked) {
        updateSnapStyle(checked);
        emit toggleSnapSignal(checked);
    });
    layout->addWidget(btnSnap);

    btnRulers = new QPushButton(" Rulers", this);
    btnRulers->setCheckable(true);
    btnRulers->setChecked(false);
    btnRulers->setToolTip("Toggle Top & Left Canvas Rulers (px / mm)");
    connect(btnRulers, &QPushButton::toggled, [this](bool checked) {
        if (checked) {
            btnRulers->setStyleSheet("QPushButton { background-color: #00E6FF; color: black; border-radius: 6px; font-weight: bold; }");
        } else {
            btnRulers->setStyleSheet("");
        }
        emit toggleRulersSignal(checked);
    });
    layout->addWidget(btnRulers);

    btnGrid = new QPushButton(" Grid", this);
    btnGrid->setCheckable(true);
    btnGrid->setChecked(false);
    btnGrid->setToolTip("Toggle Canvas Alignment Grid");
    connect(btnGrid, &QPushButton::toggled, [this](bool checked) {
        if (checked) {
            btnGrid->setStyleSheet("QPushButton { background-color: #00E6FF; color: black; border-radius: 6px; font-weight: bold; }");
        } else {
            btnGrid->setStyleSheet("");
        }
        emit toggleGridSignal(checked);
    });
    layout->addWidget(btnGrid);

    QFrame* sep = new QFrame(this);
    sep->setFrameShape(QFrame::VLine);
    sep->setStyleSheet("color: #2D3748;");
    layout->addWidget(sep);

    btnAutoRemove = new QPushButton(" Auto Remove BG", this);
    btnAutoRemove->setIcon(UIIcons::getIcon("ai_auto", QColor(255, 255, 255), 20));
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

    QPushButton* btnBatch = new QPushButton(" Batch Removal", this);
    btnBatch->setIcon(UIIcons::getIcon("ai_auto", QColor(220, 230, 245), 20));
    btnBatch->setToolTip("Process Folder of Multiple Images");
    connect(btnBatch, &QPushButton::clicked, this, &TopBarPanel::batchSignal);
    layout->addWidget(btnBatch);

    QPushButton* btnExport = new QPushButton(" Export Image", this);
    btnExport->setIcon(UIIcons::getIcon("export", QColor(220, 230, 245), 20));
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
