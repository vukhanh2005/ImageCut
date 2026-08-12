#ifndef IMAGECUT_LAYERPANEL_H
#define IMAGECUT_LAYERPANEL_H

#include <QWidget>
#include <QListWidget>
#include <QSlider>
#include <QComboBox>
#include <QLabel>
#include <QToolButton>
#include <QPushButton>
#include <QLineEdit>
#include <memory>
#include "core/ImageDocument.h"

namespace ImageCut {
namespace UI {

class LayerItemWidget : public QWidget {
    Q_OBJECT
public:
    explicit LayerItemWidget(std::shared_ptr<Core::Layer> layer, QWidget* parent = nullptr);
    ~LayerItemWidget() override = default;

    void updateThumbnail(std::shared_ptr<Core::Layer> layer);
    void startRename();

signals:
    void visibilityChanged(const QString& layerId, bool visible);
    void lockChanged(const QString& layerId, bool locked);
    void nameChanged(const QString& layerId, const QString& newName);

private:
    QString m_layerId;
    QToolButton* m_btnEye = nullptr;
    QToolButton* m_btnLock = nullptr;
    QLabel* m_lblThumb = nullptr;
    QLabel* m_lblName = nullptr;
    QLineEdit* m_txtName = nullptr;
};

class LayerManagerPanel : public QWidget {
    Q_OBJECT
public:
    explicit LayerManagerPanel(QWidget* parent = nullptr);
    ~LayerManagerPanel() override = default;

    void setDocument(std::shared_ptr<Core::ImageDocument> doc);
    void updatePanel();

    QToolButton* btnAdd = nullptr;

private:
    void initUi();
    void onSelectionChanged();
    void onItemDoubleClicked(QListWidgetItem* item);
    void showContextMenu(const QPoint& pos);

    std::shared_ptr<Core::ImageDocument> m_document;
    bool m_isUpdatingUi = false;

    QListWidget* m_layerList = nullptr;
    QSlider* m_sliderOpacity = nullptr;
    QLabel* m_lblOpacVal = nullptr;
    QComboBox* m_cmbBlendMode = nullptr;

    QToolButton* m_btnGroup = nullptr;
    QToolButton* m_btnDup = nullptr;
    QToolButton* m_btnDel = nullptr;

    QPushButton* m_btnTop = nullptr;
    QPushButton* m_btnUp = nullptr;
    QPushButton* m_btnDown = nullptr;
    QPushButton* m_btnBot = nullptr;
};

} // namespace UI
} // namespace ImageCut

#endif // IMAGECUT_LAYERPANEL_H
