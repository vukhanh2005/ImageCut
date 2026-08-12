#ifndef IMAGECUT_MAINWINDOW_H
#define IMAGECUT_MAINWINDOW_H

#include <QMainWindow>
#include <QAction>
#include <QLabel>
#include <QStatusBar>
#include <QTabWidget>
#include <memory>
#include <map>
#include "core/ImageDocument.h"
#include "tools/BaseTool.h"
#include "ui/CanvasView.h"
#include "ui/TopBarPanel.h"
#include "ui/ToolBarPanel.h"
#include "ui/panels/LayerPanel.h"
#include "ui/panels/TransformPanel.h"
#include "ui/panels/MaskPanel.h"
#include "ui/panels/ImagePanel.h"
#include "ui/panels/BackgroundPanel.h"
#include "ui/panels/ToolPropertiesPanel.h"
#include "ui/panels/ObjectPropertiesPanel.h"

namespace ImageCut {
namespace UI {

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override = default;

    void importImageFiles(const QStringList& filePaths);
    void selectToolByName(const QString& toolName);

protected:
    void keyPressEvent(QKeyEvent* event) override;

private:
    void initMenuBar();
    void initUiLayout();
    void initTools();
    void bindDocumentToPanels();

    void actionImportImages();
    void actionOpenSingleImage();
    void actionOpenProject();
    void actionSaveProject();
    void actionExport();
    void actionUndo();
    void actionRedo();
    void actionCopyLayer();
    void actionPasteLayer();
    void actionDuplicateLayer();
    void actionDeleteLayer();
    void actionAddTextLayer();
    void actionAddShapeLayer();
    void actionAutoRemove();
    void actionBatch();
    void actionSettings();

    std::shared_ptr<Core::ImageDocument> m_document;
    std::vector<std::shared_ptr<Core::Layer>> m_copiedLayers;
    std::map<QString, std::unique_ptr<Tools::BaseTool>> m_tools;

    CanvasView* m_canvas = nullptr;
    TopBarPanel* m_topBar = nullptr;
    ToolBarPanel* m_toolBar = nullptr;
    ToolPropertiesPanel* m_panelToolProperties = nullptr;
    QTabWidget* m_rightTabs = nullptr;

    LayerManagerPanel* m_panelLayers = nullptr;
    ObjectPropertiesPanel* m_panelObjectProperties = nullptr;
    TransformPanel* m_panelTransform = nullptr;
    MaskPanel* m_panelMask = nullptr;
    ImagePanel* m_panelImage = nullptr;
    BackgroundPanel* m_panelBg = nullptr;

    QAction* m_actUndo = nullptr;
    QAction* m_actRedo = nullptr;

    QLabel* m_lblStatusMsg = nullptr;
    QLabel* m_lblStatusDim = nullptr;
    QLabel* m_lblStatusColor = nullptr;
    QLabel* m_lblStatusZoom = nullptr;
};

} // namespace UI
} // namespace ImageCut

#endif // IMAGECUT_MAINWINDOW_H
