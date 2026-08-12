#include "ui/panels/LayerPanel.h"
#include "processing/Compositor.h"
#include "utils/ImageUtils.h"
#include "utils/Logger.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMenu>
#include <algorithm>

namespace ImageCut {
namespace UI {

// LayerItemWidget
LayerItemWidget::LayerItemWidget(std::shared_ptr<Core::Layer> layer, QWidget* parent)
    : QWidget(parent), m_layerId(layer->id)
{
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(4, 2, 4, 2);
    layout->setSpacing(6);

    m_btnEye = new QToolButton(this);
    m_btnEye->setText(layer->visible ? "👁" : "🙈");
    m_btnEye->setFixedSize(26, 26);
    m_btnEye->setStyleSheet("QToolButton { border: none; background: transparent; font-size: 14px; }");
    connect(m_btnEye, &QToolButton::clicked, [this]() {
        bool vis = (m_btnEye->text() == "🙈");
        m_btnEye->setText(vis ? "👁" : "🙈");
        emit visibilityChanged(m_layerId, vis);
    });
    layout->addWidget(m_btnEye);

    m_btnLock = new QToolButton(this);
    m_btnLock->setText(layer->locked ? "🔒" : "🔓");
    m_btnLock->setFixedSize(24, 24);
    m_btnLock->setStyleSheet("QToolButton { border: none; background: transparent; font-size: 12px; }");
    connect(m_btnLock, &QToolButton::clicked, [this]() {
        bool lock = (m_btnLock->text() == "🔓");
        m_btnLock->setText(lock ? "🔒" : "🔓");
        emit lockChanged(m_layerId, lock);
    });
    layout->addWidget(m_btnLock);

    m_lblThumb = new QLabel(this);
    m_lblThumb->setFixedSize(32, 32);
    m_lblThumb->setStyleSheet("QLabel { background: #2b2b2b; border: 1px solid #444; border-radius: 4px; }");
    updateThumbnail(layer);
    layout->addWidget(m_lblThumb);

    m_lblName = new QLabel(layer->name, this);
    m_lblName->setStyleSheet("QLabel { font-weight: bold; color: #e0e0e0; }");
    layout->addWidget(m_lblName, 1);

    m_txtName = new QLineEdit(layer->name, this);
    m_txtName->hide();
    connect(m_txtName, &QLineEdit::editingFinished, [this]() {
        QString newName = m_txtName->text().trimmed();
        if (!newName.isEmpty()) {
            m_lblName->setText(newName);
            emit nameChanged(m_layerId, newName);
        }
        m_txtName->hide();
        m_lblName->show();
    });
    layout->addWidget(m_txtName, 1);
}

void LayerItemWidget::updateThumbnail(std::shared_ptr<Core::Layer> layer) {
    if (!layer || layer->image.empty()) {
        m_lblThumb->setText(layer && layer->layerType == "group" ? "📁" : "T");
        m_lblThumb->setAlignment(Qt::AlignCenter);
        return;
    }

    cv::Mat renderMat = Processing::Compositor::renderSingleLayer(*layer);
    if (!renderMat.empty()) {
        QPixmap pix = Utils::ImageUtils::matToQPixmap(renderMat).scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        m_lblThumb->setPixmap(pix);
    } else {
        QPixmap pix = Utils::ImageUtils::matToQPixmap(layer->image).scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        m_lblThumb->setPixmap(pix);
    }
}

void LayerItemWidget::startRename() {
    m_lblName->hide();
    m_txtName->show();
    m_txtName->setFocus();
    m_txtName->selectAll();
}

// LayerManagerPanel
LayerManagerPanel::LayerManagerPanel(QWidget* parent)
    : QWidget(parent)
{
    initUi();
}

void LayerManagerPanel::setDocument(std::shared_ptr<Core::ImageDocument> doc) {
    m_document = doc;
    if (m_document) {
        m_document->addChangeListener([this]() {
            updatePanel();
        });
    }
    updatePanel();
}

void LayerManagerPanel::initUi() {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(8);

    QHBoxLayout* topHbox = new QHBoxLayout();
    topHbox->setSpacing(4);

    btnAdd = new QToolButton(this); btnAdd->setText("➕ Add");
    m_btnDup = new QToolButton(this); m_btnDup->setText("📋 Dup");
    m_btnDel = new QToolButton(this); m_btnDel->setText("🗑 Del");
    m_btnGroup = new QToolButton(this); m_btnGroup->setText("📁 Group");

    topHbox->addWidget(btnAdd);
    topHbox->addWidget(m_btnDup);
    topHbox->addWidget(m_btnDel);
    topHbox->addWidget(m_btnGroup);
    layout->addLayout(topHbox);

    QHBoxLayout* reorderHbox = new QHBoxLayout();
    reorderHbox->setSpacing(4);

    m_btnTop = new QPushButton("Top 🔝", this);
    m_btnUp = new QPushButton("Up 🔺", this);
    m_btnDown = new QPushButton("Down 🔻", this);
    m_btnBot = new QPushButton("Bottom ⏹", this);

    reorderHbox->addWidget(m_btnTop);
    reorderHbox->addWidget(m_btnUp);
    reorderHbox->addWidget(m_btnDown);
    reorderHbox->addWidget(m_btnBot);
    layout->addLayout(reorderHbox);

    m_layerList = new QListWidget(this);
    m_layerList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_layerList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_layerList, &QListWidget::itemSelectionChanged, this, &LayerManagerPanel::onSelectionChanged);
    connect(m_layerList, &QListWidget::itemDoubleClicked, this, &LayerManagerPanel::onItemDoubleClicked);
    connect(m_layerList, &QListWidget::customContextMenuRequested, this, &LayerManagerPanel::showContextMenu);
    layout->addWidget(m_layerList, 1);

    QVBoxLayout* optsLayout = new QVBoxLayout();
    optsLayout->setSpacing(6);

    QHBoxLayout* opacHdr = new QHBoxLayout();
    opacHdr->addWidget(new QLabel("Opacity:", this));
    m_lblOpacVal = new QLabel("100%", this);
    opacHdr->addStretch(1);
    opacHdr->addWidget(m_lblOpacVal);
    optsLayout->addLayout(opacHdr);

    m_sliderOpacity = new QSlider(Qt::Horizontal, this);
    m_sliderOpacity->setRange(0, 100);
    m_sliderOpacity->setValue(100);
    connect(m_sliderOpacity, &QSlider::valueChanged, [this](int val) {
        if (m_document && m_document->getActiveLayer()) {
            m_lblOpacVal->setText(QString("%1%").arg(val));
            m_document->getActiveLayer()->opacity = val / 100.0;
            m_document->notifyChanged();
        }
    });
    optsLayout->addWidget(m_sliderOpacity);

    QHBoxLayout* bmHdr = new QHBoxLayout();
    bmHdr->addWidget(new QLabel("Blend Mode:", this));
    m_cmbBlendMode = new QComboBox(this);
    m_cmbBlendMode->addItems({ "Normal", "Multiply", "Screen", "Overlay", "Darken", "Lighten", "Add", "Difference" });
    connect(m_cmbBlendMode, &QComboBox::currentTextChanged, [this](const QString& mode) {
        if (m_document && m_document->getActiveLayer()) {
            m_document->getActiveLayer()->blendMode = mode;
            m_document->notifyChanged();
        }
    });
    bmHdr->addWidget(m_cmbBlendMode, 1);
    optsLayout->addLayout(bmHdr);

    layout->addLayout(optsLayout);

    connect(m_btnDup, &QToolButton::clicked, [this]() { if (m_document) m_document->duplicateLayers(); });
    connect(m_btnDel, &QToolButton::clicked, [this]() { if (m_document) m_document->removeLayers(m_document->activeLayerIds); });
    connect(m_btnGroup, &QToolButton::clicked, [this]() { if (m_document) m_document->groupLayers(m_document->activeLayerIds); });

    connect(m_btnTop, &QPushButton::clicked, [this]() { if (m_document) m_document->moveLayerTop(); });
    connect(m_btnUp, &QPushButton::clicked, [this]() { if (m_document) m_document->moveLayerUp(); });
    connect(m_btnDown, &QPushButton::clicked, [this]() { if (m_document) m_document->moveLayerDown(); });
    connect(m_btnBot, &QPushButton::clicked, [this]() { if (m_document) m_document->moveLayerBottom(); });
}

void LayerManagerPanel::updatePanel() {
    if (!m_document || m_isUpdatingUi) return;

    m_isUpdatingUi = true;
    m_layerList->blockSignals(true);
    try {
        m_layerList->clear();

        for (auto it = m_document->layers.rbegin(); it != m_document->layers.rend(); ++it) {
            auto lyr = *it;
            QListWidgetItem* item = new QListWidgetItem(m_layerList);
            item->setSizeHint(QSize(0, 36));
            item->setData(Qt::UserRole, lyr->id);

            LayerItemWidget* widget = new LayerItemWidget(lyr, this);
            connect(widget, &LayerItemWidget::visibilityChanged, [this](const QString& lid, bool vis) {
                auto l = m_document->getLayerById(lid);
                if (l) { l->visible = vis; m_document->notifyChanged(); }
            });
            connect(widget, &LayerItemWidget::lockChanged, [this](const QString& lid, bool lock) {
                auto l = m_document->getLayerById(lid);
                if (l) { l->locked = lock; m_document->notifyChanged(); }
            });
            connect(widget, &LayerItemWidget::nameChanged, [this](const QString& lid, const QString& name) {
                auto l = m_document->getLayerById(lid);
                if (l) { l->name = name; m_document->notifyChanged(); }
            });

            m_layerList->setItemWidget(item, widget);
            if (std::find(m_document->activeLayerIds.begin(), m_document->activeLayerIds.end(), lyr->id) != m_document->activeLayerIds.end()) {
                item->setSelected(true);
            }
        }

        auto active = m_document->getActiveLayer();
        if (active) {
            m_sliderOpacity->blockSignals(true);
            m_sliderOpacity->setValue(static_cast<int>(active->opacity * 100));
            m_lblOpacVal->setText(QString("%1%").arg(static_cast<int>(active->opacity * 100)));
            m_sliderOpacity->blockSignals(false);

            m_cmbBlendMode->blockSignals(true);
            m_cmbBlendMode->setCurrentText(active->blendMode);
            m_cmbBlendMode->blockSignals(false);
        }
    } catch (...) {}
    m_layerList->blockSignals(false);
    m_isUpdatingUi = false;
}

void LayerManagerPanel::onSelectionChanged() {
    if (m_isUpdatingUi || !m_document) return;
    std::vector<QString> selIds;
    for (auto item : m_layerList->selectedItems()) {
        QString lid = item->data(Qt::UserRole).toString();
        if (!lid.isEmpty()) selIds.push_back(lid);
    }
    if (!selIds.empty()) {
        m_document->activeLayerIds = selIds;
        m_document->notifyChanged();
    }
}

void LayerManagerPanel::onItemDoubleClicked(QListWidgetItem* item) {
    QWidget* w = m_layerList->itemWidget(item);
    LayerItemWidget* widget = qobject_cast<LayerItemWidget*>(w);
    if (widget) {
        widget->startRename();
    }
}

void LayerManagerPanel::showContextMenu(const QPoint& pos) {
    QListWidgetItem* item = m_layerList->itemAt(pos);
    if (!item || !m_document) return;

    QString lid = item->data(Qt::UserRole).toString();
    auto lyr = m_document->getLayerById(lid);
    if (!lyr) return;

    QMenu menu(this);
    QAction* actRename = menu.addAction("✏ Rename");
    QAction* actDup = menu.addAction("📋 Duplicate");
    QAction* actDel = menu.addAction("🗑 Delete");
    menu.addSeparator();
    QAction* actTop = menu.addAction("⏫ Move to Top");
    QAction* actUp = menu.addAction("▲ Move Up");
    QAction* actDown = menu.addAction("▼ Move Down");
    QAction* actBot = menu.addAction("⏬ Move to Bottom");
    menu.addSeparator();
    QAction* actGroup = menu.addAction("📁 Group Selected");
    QAction* actUngroup = menu.addAction("📂 Ungroup");

    QAction* selectedAction = menu.exec(m_layerList->mapToGlobal(pos));
    if (selectedAction == actRename) {
        QWidget* w = m_layerList->itemWidget(item);
        LayerItemWidget* widget = qobject_cast<LayerItemWidget*>(w);
        if (widget) widget->startRename();
    } else if (selectedAction == actDup) {
        m_document->duplicateLayers({ lid });
    } else if (selectedAction == actDel) {
        m_document->removeLayers({ lid });
    } else if (selectedAction == actTop) {
        m_document->moveLayerTop(lid);
    } else if (selectedAction == actUp) {
        m_document->moveLayerUp(lid);
    } else if (selectedAction == actDown) {
        m_document->moveLayerDown(lid);
    } else if (selectedAction == actBot) {
        m_document->moveLayerBottom(lid);
    } else if (selectedAction == actGroup) {
        m_document->groupLayers(m_document->activeLayerIds);
    } else if (selectedAction == actUngroup) {
        m_document->ungroupLayer(lid);
    }
}

} // namespace UI
} // namespace ImageCut
