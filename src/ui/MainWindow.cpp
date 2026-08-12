#include "ui/MainWindow.h"
#include "core/ProjectManager.h"
#include "tools/SelectTool.h"
#include "tools/BrushTool.h"
#include "tools/MagicWandTool.h"
#include "tools/LassoTool.h"
#include "tools/PolyLassoTool.h"
#include "tools/RefineEdgeTool.h"
#include "tools/EyedropperTool.h"
#include "tools/CropTool.h"
#include "workers/InferenceWorker.h"
#include "utils/ImageUtils.h"
#include "utils/Settings.h"
#include "utils/Logger.h"
#include "ui/dialogs/ExportDialog.h"
#include "ui/dialogs/BatchDialog.h"
#include "ui/dialogs/SettingsDialog.h"

#include <QMenuBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFileInfo>

namespace ImageCut {
namespace UI {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("ImageCut — Multi-Layer Image Compositing & Background Remover (C++)");
    resize(1400, 900);

    m_document = std::make_shared<Core::ImageDocument>();

    initMenuBar();
    initUiLayout();
    initTools();

    m_document->addChangeListener([this]() {
        if (m_actUndo) m_actUndo->setEnabled(m_document->undoStack.canUndo());
        if (m_actRedo) m_actRedo->setEnabled(m_document->undoStack.canRedo());
        if (m_canvas) m_canvas->updateViewport();
    });

    connect(m_canvas, &CanvasView::imageDroppedSignal, [this](const QString& path) {
        importImageFiles({ path });
    });
    connect(m_canvas, &CanvasView::imagesDroppedSignal, this, &MainWindow::importImageFiles);
}

void MainWindow::initMenuBar() {
    QMenuBar* mb = menuBar();

    // File Menu
    QMenu* menuFile = mb->addMenu("&File");

    QAction* actImport = menuFile->addAction("📥 &Import Image(s)...");
    actImport->setShortcut(QKeySequence("Ctrl+I"));
    connect(actImport, &QAction::triggered, this, &MainWindow::actionImportImages);

    QAction* actOpen = menuFile->addAction("&Open Single Image...");
    actOpen->setShortcut(QKeySequence("Ctrl+O"));
    connect(actOpen, &QAction::triggered, this, &MainWindow::actionOpenSingleImage);

    menuFile->addSeparator();

    QAction* actOpenProj = menuFile->addAction("📁 Open Project (.icproj)...");
    actOpenProj->setShortcut(QKeySequence("Ctrl+Shift+O"));
    connect(actOpenProj, &QAction::triggered, this, &MainWindow::actionOpenProject);

    QAction* actSaveProj = menuFile->addAction("💾 Save Project (.icproj)");
    actSaveProj->setShortcut(QKeySequence("Ctrl+S"));
    connect(actSaveProj, &QAction::triggered, this, &MainWindow::actionSaveProject);

    menuFile->addSeparator();

    QAction* actExport = menuFile->addAction("&Export Image...");
    actExport->setShortcut(QKeySequence("Ctrl+E"));
    connect(actExport, &QAction::triggered, this, &MainWindow::actionExport);

    menuFile->addSeparator();
    QAction* actExit = menuFile->addAction("E&xit");
    connect(actExit, &QAction::triggered, this, &QWidget::close);

    // Edit Menu
    QMenu* menuEdit = mb->addMenu("&Edit");

    m_actUndo = menuEdit->addAction("&Undo");
    m_actUndo->setShortcut(QKeySequence("Ctrl+Z"));
    connect(m_actUndo, &QAction::triggered, this, &MainWindow::actionUndo);

    m_actRedo = menuEdit->addAction("&Redo");
    m_actRedo->setShortcut(QKeySequence("Ctrl+Shift+Z"));
    connect(m_actRedo, &QAction::triggered, this, &MainWindow::actionRedo);

    menuEdit->addSeparator();

    QAction* actCopy = menuEdit->addAction("Copy Layer(s)");
    actCopy->setShortcut(QKeySequence("Ctrl+C"));
    connect(actCopy, &QAction::triggered, this, &MainWindow::actionCopyLayer);

    QAction* actPaste = menuEdit->addAction("Paste Layer(s)");
    actPaste->setShortcut(QKeySequence("Ctrl+V"));
    connect(actPaste, &QAction::triggered, this, &MainWindow::actionPasteLayer);

    QAction* actDup = menuEdit->addAction("Duplicate Layer(s)");
    actDup->setShortcut(QKeySequence("Ctrl+D"));
    connect(actDup, &QAction::triggered, this, &MainWindow::actionDuplicateLayer);

    QAction* actDel = menuEdit->addAction("Delete Selected Layer(s)");
    actDel->setShortcut(QKeySequence("Delete"));
    connect(actDel, &QAction::triggered, this, &MainWindow::actionDeleteLayer);

    // Layer Menu
    QMenu* menuLayer = mb->addMenu("&Layer");

    QAction* actTop = menuLayer->addAction("Move to Top");
    actTop->setShortcut(QKeySequence("Ctrl+Shift+]"));
    connect(actTop, &QAction::triggered, [this]() { if (m_document) m_document->moveLayerTop(); });

    QAction* actUp = menuLayer->addAction("Move Up");
    actUp->setShortcut(QKeySequence("Ctrl+]"));
    connect(actUp, &QAction::triggered, [this]() { if (m_document) m_document->moveLayerUp(); });

    QAction* actDown = menuLayer->addAction("Move Down");
    actDown->setShortcut(QKeySequence("Ctrl+["));
    connect(actDown, &QAction::triggered, [this]() { if (m_document) m_document->moveLayerDown(); });

    QAction* actBot = menuLayer->addAction("Move to Bottom");
    actBot->setShortcut(QKeySequence("Ctrl+Shift+["));
    connect(actBot, &QAction::triggered, [this]() { if (m_document) m_document->moveLayerBottom(); });

    menuLayer->addSeparator();

    QAction* actGroup = menuLayer->addAction("Group Selected Layers");
    actGroup->setShortcut(QKeySequence("Ctrl+G"));
    connect(actGroup, &QAction::triggered, [this]() { if (m_document) m_document->groupLayers(m_document->activeLayerIds); });

    QAction* actAddText = menuLayer->addAction("➕ Add Text Layer");
    connect(actAddText, &QAction::triggered, this, &MainWindow::actionAddTextLayer);

    QAction* actAddShape = menuLayer->addAction("➕ Add Shape Layer");
    connect(actAddShape, &QAction::triggered, this, &MainWindow::actionAddShapeLayer);

    // View Menu
    QMenu* menuView = mb->addMenu("&View");
    QAction* actZoomIn = menuView->addAction("🔍 Zoom &In");
    actZoomIn->setShortcuts({ QKeySequence("Ctrl++"), QKeySequence("Ctrl+=") });
    connect(actZoomIn, &QAction::triggered, [this]() { if (m_canvas) m_canvas->zoomIn(); });

    QAction* actZoomOut = menuView->addAction("🔍 Zoom &Out");
    actZoomOut->setShortcut(QKeySequence("Ctrl+-"));
    connect(actZoomOut, &QAction::triggered, [this]() { if (m_canvas) m_canvas->zoomOut(); });

    menuView->addSeparator();

    QAction* actZoomFit = menuView->addAction("🎯 Fit to Screen");
    actZoomFit->setShortcut(QKeySequence("Ctrl+0"));
    connect(actZoomFit, &QAction::triggered, [this]() { if (m_canvas) m_canvas->fitInView(); });

    QAction* actZoom100 = menuView->addAction("100% Actual Size");
    actZoom100->setShortcut(QKeySequence("Ctrl+1"));
    connect(actZoom100, &QAction::triggered, [this]() { if (m_canvas) m_canvas->setZoomLevel(1.0); });

    // Tools Menu
    QMenu* menuTools = mb->addMenu("&Tools");
    QAction* actRemoveBg = menuTools->addAction("✨ Auto Remove Background (Selected Layer)");
    connect(actRemoveBg, &QAction::triggered, this, &MainWindow::actionAutoRemove);

    QAction* actBatch = menuTools->addAction("⚡ Batch Background Removal...");
    connect(actBatch, &QAction::triggered, this, &MainWindow::actionBatch);

    // Settings Menu
    QMenu* menuSettings = mb->addMenu("&Settings");
    QAction* actPref = menuSettings->addAction("Preferences...");
    connect(actPref, &QAction::triggered, this, &MainWindow::actionSettings);
}

void MainWindow::initUiLayout() {
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout* mainVbox = new QVBoxLayout(centralWidget);
    mainVbox->setContentsMargins(0, 0, 0, 0);
    mainVbox->setSpacing(0);

    m_topBar = new TopBarPanel(this);
    connect(m_topBar, &TopBarPanel::openSignal, this, &MainWindow::actionImportImages);
    connect(m_topBar, &TopBarPanel::openProjectSignal, this, &MainWindow::actionOpenProject);
    connect(m_topBar, &TopBarPanel::saveProjectSignal, this, &MainWindow::actionSaveProject);
    connect(m_topBar, &TopBarPanel::undoSignal, this, &MainWindow::actionUndo);
    connect(m_topBar, &TopBarPanel::redoSignal, this, &MainWindow::actionRedo);
    connect(m_topBar, &TopBarPanel::autoRemoveSignal, this, &MainWindow::actionAutoRemove);
    connect(m_topBar, &TopBarPanel::batchSignal, this, &MainWindow::actionBatch);
    connect(m_topBar, &TopBarPanel::exportSignal, this, &MainWindow::actionExport);
    connect(m_topBar, &TopBarPanel::toggleSnapSignal, [this](bool enabled) {
        if (m_document) m_document->snapEnabled = enabled;
    });
    connect(m_topBar, &TopBarPanel::toggleRulersSignal, [this](bool enabled) {
        if (m_document) {
            m_document->showRulers = enabled;
            if (m_canvas && m_canvas->viewport()) m_canvas->viewport()->update();
        }
    });
    connect(m_topBar, &TopBarPanel::toggleGridSignal, [this](bool enabled) {
        if (m_document) {
            m_document->showGrid = enabled;
            if (m_canvas && m_canvas->viewport()) m_canvas->viewport()->update();
        }
    });
    mainVbox->addWidget(m_topBar);

    m_panelToolProperties = new ToolPropertiesPanel(this);
    mainVbox->addWidget(m_panelToolProperties);

    QHBoxLayout* contentHbox = new QHBoxLayout();
    contentHbox->setContentsMargins(0, 0, 0, 0);
    contentHbox->setSpacing(0);

    m_toolBar = new ToolBarPanel(this);
    connect(m_toolBar, &ToolBarPanel::toolChangedSignal, this, &MainWindow::selectToolByName);
    contentHbox->addWidget(m_toolBar);

    m_canvas = new CanvasView(this);
    connect(m_canvas, &CanvasView::mouseMovedSignal, [this](const QPointF& scenePos, const QColor&) {
        m_lblStatusColor->setText(QString("X: %1, Y: %2").arg(static_cast<int>(scenePos.x())).arg(static_cast<int>(scenePos.y())));
        m_lblStatusZoom->setText(QString("Zoom: %1%").arg(static_cast<int>(m_canvas->transform().m11() * 100)));
    });
    connect(m_canvas, &CanvasView::zoomChangedSignal, [this](double factor) {
        m_lblStatusZoom->setText(QString("Zoom: %1%").arg(static_cast<int>(factor * 100)));
    });
    contentHbox->addWidget(m_canvas, 1);

    m_rightTabs = new QTabWidget(this);
    m_rightTabs->setFixedWidth(340);

    m_panelLayers = new LayerManagerPanel(this);
    connect(m_panelLayers->btnAdd, &QToolButton::clicked, this, &MainWindow::actionImportImages);

    m_panelTransform = new TransformPanel(this);
    m_panelMask = new MaskPanel(this);
    m_panelImage = new ImagePanel(this);
    m_panelBg = new BackgroundPanel(this);

    m_rightTabs->addTab(m_panelLayers, "Layers");
    m_rightTabs->addTab(m_panelTransform, "Transform");
    m_rightTabs->addTab(m_panelMask, "Mask");
    m_rightTabs->addTab(m_panelImage, "Image");
    m_rightTabs->addTab(m_panelBg, "Background");

    contentHbox->addWidget(m_rightTabs);
    mainVbox->addLayout(contentHbox, 1);

    QStatusBar* sb = statusBar();
    m_lblStatusMsg = new QLabel("Ready. Import images to start compositing.", this);
    m_lblStatusDim = new QLabel("Canvas: 1920x1080", this);
    m_lblStatusColor = new QLabel("X: 0, Y: 0", this);

    QWidget* zoomBar = new QWidget(this);
    QHBoxLayout* zoomLayout = new QHBoxLayout(zoomBar);
    zoomLayout->setContentsMargins(0, 0, 0, 0);
    zoomLayout->setSpacing(4);

    QPushButton* btnZOut = new QPushButton("➖", this);
    btnZOut->setFixedSize(24, 22);
    connect(btnZOut, &QPushButton::clicked, [this]() { m_canvas->zoomOut(); });

    m_lblStatusZoom = new QLabel("Zoom: 100%", this);
    m_lblStatusZoom->setStyleSheet("font-weight: bold; padding: 0 4px;");

    QPushButton* btnZIn = new QPushButton("➕", this);
    btnZIn->setFixedSize(24, 22);
    connect(btnZIn, &QPushButton::clicked, [this]() { m_canvas->zoomIn(); });

    QPushButton* btnZFit = new QPushButton("Fit 🔍", this);
    btnZFit->setFixedHeight(22);
    connect(btnZFit, &QPushButton::clicked, [this]() { m_canvas->fitInView(); });

    QPushButton* btnZ100 = new QPushButton("100% 🎯", this);
    btnZ100->setFixedHeight(22);
    connect(btnZ100, &QPushButton::clicked, [this]() { m_canvas->setZoomLevel(1.0); });

    zoomLayout->addWidget(btnZOut);
    zoomLayout->addWidget(m_lblStatusZoom);
    zoomLayout->addWidget(btnZIn);
    zoomLayout->addWidget(btnZFit);
    zoomLayout->addWidget(btnZ100);

    sb->addWidget(m_lblStatusMsg, 1);
    sb->addPermanentWidget(m_lblStatusColor);
    sb->addPermanentWidget(m_lblStatusDim);
    sb->addPermanentWidget(zoomBar);

    bindDocumentToPanels();
}

void MainWindow::bindDocumentToPanels() {
    if (m_canvas) m_canvas->setDocument(m_document);
    if (m_panelLayers) m_panelLayers->setDocument(m_document);
    if (m_panelTransform) m_panelTransform->setDocument(m_document);
    if (m_panelMask) m_panelMask->setDocument(m_document);
    if (m_panelImage) m_panelImage->setDocument(m_document);
    if (m_panelBg) m_panelBg->setDocument(m_document);

    if (m_document) {
        auto updateUndoState = [this]() {
            bool canU = m_document->undoStack.canUndo();
            bool canR = m_document->undoStack.canRedo();
            if (m_actUndo) m_actUndo->setEnabled(canU);
            if (m_actRedo) m_actRedo->setEnabled(canR);
            if (m_topBar && m_topBar->btnUndo) m_topBar->btnUndo->setEnabled(canU);
            if (m_topBar && m_topBar->btnRedo) m_topBar->btnRedo->setEnabled(canR);
        };
        m_document->undoStack.addChangeListener(updateUndoState);
        updateUndoState();
    }
}

void MainWindow::initTools() {
    m_tools["Select"] = std::make_unique<Tools::SelectMoveTool>(m_canvas);
    m_tools["Brush"] = std::make_unique<Tools::MaskBrushTool>(m_canvas, "Restore");
    m_tools["Eraser"] = std::make_unique<Tools::MaskBrushTool>(m_canvas, "Eraser");
    m_tools["RefineEdge"] = std::make_unique<Tools::RefineEdgeTool>(m_canvas);
    m_tools["MagicWand"] = std::make_unique<Tools::MagicWandTool>(m_canvas);

    auto eyeTool = std::make_unique<Tools::EyedropperTool>(m_canvas);
    connect(eyeTool.get(), &Tools::EyedropperTool::colorPickedSignal, [this](const QColor& color) {
        if (m_document) {
            auto lyr = m_document->getActiveLayer();
            if (lyr) {
                if (lyr->layerType == "text") lyr->textColor = color;
                else if (lyr->layerType == "shape") lyr->fillColor = color;
                lyr->invalidateCache();
                m_document->notifyChanged();
            }
        }
        m_lblStatusColor->setText(QString("Picked: %1").arg(color.name().toUpper()));
    });
    m_tools["Eyedropper"] = std::move(eyeTool);

    m_tools["Lasso"] = std::make_unique<Tools::LassoTool>(m_canvas, "Remove");
    m_tools["PolyLasso"] = std::make_unique<Tools::PolyLassoTool>(m_canvas, "Keep");
    m_tools["Crop"] = std::make_unique<Tools::CropTool>(m_canvas);

    selectToolByName("Select");
}

void MainWindow::keyPressEvent(QKeyEvent* event) {
    int key = event->key();
    Qt::KeyboardModifiers mods = event->modifiers();

    if (mods == Qt::ControlModifier && key == Qt::Key_Z) {
        actionUndo();
    } else if ((mods == (Qt::ControlModifier | Qt::ShiftModifier) && key == Qt::Key_Z) || (mods == Qt::ControlModifier && key == Qt::Key_Y)) {
        actionRedo();
    } else if (key == Qt::Key_B) {
        m_toolBar->setActiveTool("Brush");
        selectToolByName("Brush");
    } else if (key == Qt::Key_E) {
        m_toolBar->setActiveTool("Eraser");
        selectToolByName("Eraser");
    } else if (key == Qt::Key_R) {
        m_toolBar->setActiveTool("RefineEdge");
        selectToolByName("RefineEdge");
    } else if (key == Qt::Key_W) {
        m_toolBar->setActiveTool("MagicWand");
        selectToolByName("MagicWand");
    } else if (key == Qt::Key_I) {
        m_toolBar->setActiveTool("Eyedropper");
        selectToolByName("Eyedropper");
    } else if (key == Qt::Key_L) {
        m_toolBar->setActiveTool("Lasso");
        selectToolByName("Lasso");
    } else if (key == Qt::Key_P) {
        m_toolBar->setActiveTool("PolyLasso");
        selectToolByName("PolyLasso");
    } else if (key == Qt::Key_C) {
        m_toolBar->setActiveTool("Crop");
        selectToolByName("Crop");
    } else if (key == Qt::Key_H) {
        m_toolBar->setActiveTool("Select");
        selectToolByName("Select");
    } else if (mods == Qt::ControlModifier && key == Qt::Key_0) {
        m_canvas->fitInView();
    } else if (mods == Qt::ControlModifier && key == Qt::Key_1) {
        m_canvas->setZoomLevel(1.0);
    } else if (mods == Qt::ControlModifier && key == Qt::Key_D) {
        actionDuplicateLayer();
    } else if (key == Qt::Key_Delete || key == Qt::Key_Backspace) {
        actionDeleteLayer();
    } else {
        QMainWindow::keyPressEvent(event);
    }
}

void MainWindow::selectToolByName(const QString& toolName) {
    if (m_tools.find(toolName) != m_tools.end()) {
        auto tool = m_tools[toolName].get();
        m_canvas->setActiveTool(tool, toolName);
        if (m_panelToolProperties) {
            m_panelToolProperties->setTool(tool, toolName);
        }
        m_lblStatusMsg->setText("Active Tool: " + toolName);
    }
}

void MainWindow::importImageFiles(const QStringList& filePaths) {
    std::vector<std::shared_ptr<Core::Layer>> added;
    for (const auto& path : filePaths) {
        try {
            cv::Mat arr = Utils::ImageUtils::loadImage(path);
            if (!arr.empty()) {
                QFileInfo info(path);
                auto lyr = m_document->addImageLayer(arr, info.fileName());
                if (lyr) added.push_back(lyr);
            }
        } catch (const std::exception& e) {
            LOG_ERROR("Error importing image " + path.toStdString() + ": " + e.what());
        }
    }

    if (!added.empty()) {
        m_canvas->fitInView();
        m_lblStatusDim->setText(QString("Canvas: %1x%2").arg(m_document->width()).arg(m_document->height()));
        m_lblStatusMsg->setText(QString("Imported %1 layer(s)").arg(added.size()));
    }
}

void MainWindow::actionImportImages() {
    QStringList paths = QFileDialog::getOpenFileNames(this, "Import Images", "", "Images (*.png *.jpg *.jpeg *.webp *.bmp *.tiff)");
    if (!paths.isEmpty()) {
        importImageFiles(paths);
    }
}

void MainWindow::actionOpenSingleImage() {
    QString path = QFileDialog::getOpenFileName(this, "Open Image", "", "Images (*.png *.jpg *.jpeg *.webp *.bmp *.tiff)");
    if (!path.isEmpty()) {
        importImageFiles({ path });
    }
}

void MainWindow::actionOpenProject() {
    QString path = QFileDialog::getOpenFileName(this, "Open ImageCut Project", "", "ImageCut Project Files (*.icproj *.bgrem);;All Files (*.*)");
    if (!path.isEmpty()) {
        auto doc = Core::ProjectManager::loadProject(path);
        if (doc) {
            m_document = doc;
            bindDocumentToPanels();
            QFileInfo info(path);
            m_lblStatusMsg->setText("Loaded Project: " + info.fileName());
        } else {
            QMessageBox::critical(this, "Project Load Error", "Could not load project file.");
        }
    }
}

void MainWindow::actionSaveProject() {
    if (!m_document || m_document->layers.empty()) {
        QMessageBox::warning(this, "No Image", "Please import an image before saving a project.");
        return;
    }

    QString path = QFileDialog::getSaveFileName(this, "Save ImageCut Project", "project.icproj", "ImageCut Project Files (*.icproj);;Legacy (*.bgrem)");
    if (!path.isEmpty()) {
        bool ok = Core::ProjectManager::saveProject(m_document, path);
        if (ok) {
            QFileInfo info(path);
            m_lblStatusMsg->setText("Project saved: " + info.fileName());
        } else {
            QMessageBox::critical(this, "Error", "Failed to save project.");
        }
    }
}

void MainWindow::actionUndo() {
    if (m_document && m_document->undoStack.canUndo()) {
        m_document->undoStack.undo();
    }
}

void MainWindow::actionRedo() {
    if (m_document && m_document->undoStack.canRedo()) {
        m_document->undoStack.redo();
    }
}

void MainWindow::actionCopyLayer() {
    if (m_document && !m_document->activeLayerIds.empty()) {
        m_copiedLayers.clear();
        for (const auto& lyr : m_document->getActiveLayers()) {
            m_copiedLayers.push_back(lyr->clone());
        }
        m_lblStatusMsg->setText(QString("Copied %1 layer(s)").arg(m_copiedLayers.size()));
    }
}

void MainWindow::actionPasteLayer() {
    if (m_document && !m_copiedLayers.empty()) {
        std::vector<std::shared_ptr<Core::Layer>> pasted;
        for (const auto& lyr : m_copiedLayers) {
            auto dup = lyr->clone();
            dup->offsetX += 30;
            dup->offsetY += 30;
            m_document->addLayer(dup);
            pasted.push_back(dup);
        }
        m_document->activeLayerIds.clear();
        for (const auto& l : pasted) m_document->activeLayerIds.push_back(l->id);
        m_document->notifyChanged();
        m_lblStatusMsg->setText(QString("Pasted %1 layer(s)").arg(pasted.size()));
    }
}

void MainWindow::actionDuplicateLayer() {
    if (m_document) m_document->duplicateLayers();
}

void MainWindow::actionDeleteLayer() {
    if (m_document && !m_document->activeLayerIds.empty()) {
        m_document->removeLayers(m_document->activeLayerIds);
    }
}

void MainWindow::actionAddTextLayer() {
    if (m_document) {
        auto lyr = std::make_shared<Core::Layer>(QString("Text %1").arg(m_document->layers.size() + 1), cv::Mat(), "text");
        lyr->offsetX = (m_document->canvasWidth - 200) / 2.0;
        lyr->offsetY = (m_document->canvasHeight - 100) / 2.0;
        m_document->addLayer(lyr);
    }
}

void MainWindow::actionAddShapeLayer() {
    if (!m_document) return;

    QMenu menu(this);
    menu.setTitle("Choose Shape");

    struct ShapeChoice { QString type; QString label; };
    std::vector<ShapeChoice> choices = {
        { "Rectangle", "⬛ Rectangle (Hình chữ nhật)" },
        { "RoundedRectangle", "🔲 Rounded Rectangle (Bo góc)" },
        { "Circle", "⚪ Circle / Ellipse (Hình tròn)" },
        { "Triangle", "🔺 Triangle (Hình tam giác)" },
        { "Diamond", "🔷 Diamond (Hình thoi)" },
        { "Arrow", "➡️ Arrow (Mũi tên chỉ hướng)" },
        { "Star", "⭐ Star (Ngôi sao 5 cánh)" },
        { "SpeechBubble", "💬 Speech Bubble (Bóng thoại)" },
        { "Heart", "❤️ Heart (Hình trái tim)" },
        { "Hexagon", "⬡ Hexagon (Hình lục giác)" },
        { "Octagon", "🛑 Octagon (Hình bát giác)" },
        { "Shield", "🛡️ Shield (Hình cái khiên)" }
    };

    for (const auto& ch : choices) {
        QAction* act = menu.addAction(ch.label);
        connect(act, &QAction::triggered, [this, ch]() {
            auto lyr = std::make_shared<Core::Layer>(QString("%1 Layer").arg(ch.type), cv::Mat(), "shape");
            lyr->shapeType = ch.type;
            lyr->offsetX = (m_document->canvasWidth - 240) / 2.0;
            lyr->offsetY = (m_document->canvasHeight - 240) / 2.0;
            m_document->addLayer(lyr);
        });
    }

    menu.exec(QCursor::pos());
}

void MainWindow::actionAutoRemove() {
    auto active = m_document ? m_document->getActiveLayer() : nullptr;
    if (!active || active->image.empty()) {
        LOG_WARN("actionAutoRemove called but no valid image layer is selected.");
        QMessageBox::warning(this, "No Layer Selected", "Please select an image layer to run AI Background Removal.");
        return;
    }

    LOG_INFO(QString("[AutoRemove] Triggered on layer '%1' (ID: %2, Dim: %3x%4, Channels: %5)")
        .arg(active->name)
        .arg(active->id)
        .arg(active->width())
        .arg(active->height())
        .arg(active->image.channels()).toStdString());

    m_topBar->showProgress(10);
    m_topBar->btnAutoRemove->setEnabled(false);

    QString modelName = Utils::Settings::getInstance().get("ai_model", "RMBG-1.4");
    QString device = Utils::Settings::getInstance().get("ai_device", "Auto");

    auto worker = new Workers::BackgroundRemovalWorker(active->image, "AI", modelName, device);
    
    // Safely delete worker object ONLY after QThread has completely stopped running!
    connect(worker, &QThread::finished, worker, &QObject::deleteLater);
    
    connect(worker, &Workers::BackgroundRemovalWorker::progress, m_topBar, &TopBarPanel::showProgress, Qt::QueuedConnection);
    
    connect(worker, &Workers::BackgroundRemovalWorker::resultReady, this, [this, active](const cv::Mat& mask) {
        LOG_INFO("[AutoRemove] Worker resultReady callback executing on Main GUI Thread.");
        m_topBar->hideProgress();
        m_topBar->btnAutoRemove->setEnabled(true);
        if (m_document && !mask.empty()) {
            m_document->updateMask(mask, active->id, "AI Background Removal");
            LOG_INFO("[AutoRemove] Mask applied to layer successfully.");
        } else {
            LOG_WARN("[AutoRemove] Generated mask was empty or document released.");
        }
        m_lblStatusMsg->setText("AI Background Removal complete on selected layer!");
    }, Qt::QueuedConnection);
    
    connect(worker, &Workers::BackgroundRemovalWorker::error, this, [this](const QString& err) {
        LOG_ERROR("[AutoRemove] Worker error callback: " + err.toStdString());
        m_topBar->hideProgress();
        m_topBar->btnAutoRemove->setEnabled(true);
        QMessageBox::critical(this, "Background Removal Error", "Failed to run background removal:\n" + err);
    }, Qt::QueuedConnection);
    
    LOG_INFO("[AutoRemove] Starting BackgroundRemovalWorker QThread...");
    worker->start();
}

void MainWindow::actionBatch() {
    BatchDialog dlg(this);
    dlg.exec();
}

void MainWindow::actionExport() {
    if (!m_document || m_document->layers.empty()) {
        QMessageBox::warning(this, "Empty Document", "There are no layers to export.");
        return;
    }
    ExportDialog dlg(m_document, this);
    dlg.exec();
}

void MainWindow::actionSettings() {
    SettingsDialog dlg(this);
    dlg.exec();
}

} // namespace UI
} // namespace ImageCut
