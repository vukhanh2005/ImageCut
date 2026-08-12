#include "ui/CanvasView.h"
#include "processing/Compositor.h"
#include "utils/ImageUtils.h"
#include "utils/Logger.h"
#include <QScrollBar>
#include <QMimeData>
#include <QUrl>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace ImageCut {
namespace UI {

CanvasView::CanvasView(QWidget* parent)
    : QGraphicsView(parent)
{
    m_scene = new QGraphicsScene(this);
    setScene(m_scene);

    setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
    setMouseTracking(true);
    setAcceptDrops(true);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorUnderMouse);
}

void CanvasView::setDocument(std::shared_ptr<Core::ImageDocument> doc) {
    m_document = doc;
    if (m_document) {
        m_document->addChangeListener([this]() {
            updateViewport();
        });
    }
    updateViewport();
    fitInView();
}

void CanvasView::setActiveTool(Tools::BaseTool* tool, const QString& toolName) {
    m_activeTool = tool;
    m_activeToolName = toolName;
    viewport()->update();
}

bool CanvasView::isSelectToolActive() const {
    return m_activeToolName == "Select";
}

void CanvasView::updateViewport(bool fastDrag) {
    if (!m_document) {
        m_scene->clear();
        m_pixmapItem = nullptr;
        return;
    }

    cv::Mat compRgba = Processing::Compositor::compositeDocument(*m_document, true, fastDrag);
    QPixmap pixmap = Utils::ImageUtils::matToQPixmap(compRgba);

    if (!m_pixmapItem) {
        m_scene->clear();
        m_pixmapItem = m_scene->addPixmap(pixmap);
    } else {
        m_pixmapItem->setPixmap(pixmap);
    }

    m_scene->setSceneRect(0, 0, m_document->canvasWidth, m_document->canvasHeight);
    viewport()->update();
}

void CanvasView::fitInView() {
    if (m_scene && !m_scene->sceneRect().isEmpty()) {
        QGraphicsView::fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
        m_zoomFactor = transform().m11();
        emit zoomChangedSignal(m_zoomFactor);
    }
}

void CanvasView::setZoomLevel(double factor) {
    resetTransform();
    scale(factor, factor);
    m_zoomFactor = factor;
    emit zoomChangedSignal(m_zoomFactor);
}

void CanvasView::zoomIn() {
    applyZoomStep(1.2);
}

void CanvasView::zoomOut() {
    applyZoomStep(1.0 / 1.2);
}

void CanvasView::applyZoomStep(double factor) {
    double newZoom = m_zoomFactor * factor;
    if (newZoom >= 0.02 && newZoom <= 50.0) {
        scale(factor, factor);
        m_zoomFactor = transform().m11();
        emit zoomChangedSignal(m_zoomFactor);
    }
}

void CanvasView::panViewport(double dx, double dy) {
    horizontalScrollBar()->setValue(static_cast<int>(horizontalScrollBar()->value() - dx));
    verticalScrollBar()->setValue(static_cast<int>(verticalScrollBar()->value() - dy));
}

void CanvasView::wheelEvent(QWheelEvent* event) {
    double factor = (event->angleDelta().y() > 0) ? 1.15 : (1.0 / 1.15);
    applyZoomStep(factor);
    event->accept();
}

void CanvasView::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Space && !m_isSpacePanning) {
        m_isSpacePanning = true;
        setCursor(Qt::OpenHandCursor);
        event->accept();
        return;
    }
    QGraphicsView::keyPressEvent(event);
}

void CanvasView::keyReleaseEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Space) {
        m_isSpacePanning = false;
        unsetCursor();
        event->accept();
        return;
    }
    QGraphicsView::keyReleaseEvent(event);
}

std::shared_ptr<Core::Layer> CanvasView::getLayerAtPoint(const QPointF& scenePos) const {
    if (!m_document) return nullptr;

    for (auto it = m_document->layers.rbegin(); it != m_document->layers.rend(); ++it) {
        auto lyr = *it;
        if (!lyr || !lyr->visible || lyr->locked) continue;

        auto [poly, center, handles] = getLayerScreenPolygon(lyr);
        if (poly.containsPoint(scenePos, Qt::OddEvenFill)) {
            return lyr;
        }
    }
    return nullptr;
}

std::tuple<QPolygonF, QPointF, std::map<QString, QPointF>> CanvasView::getLayerScreenPolygon(std::shared_ptr<Core::Layer> lyr) const {
    double w = lyr->width();
    double h = lyr->height();
    double scaledW = w * lyr->scaleX;
    double scaledH = h * lyr->scaleY;

    double cx = lyr->offsetX + scaledW / 2.0;
    double cy = lyr->offsetY + scaledH / 2.0;

    double rad = lyr->rotation * M_PI / 180.0;
    double cosA = std::cos(rad);
    double sinA = std::sin(rad);

    auto rotatePt = [cx, cy, cosA, sinA](double x, double y) -> QPointF {
        double dx = x - cx;
        double dy = y - cy;
        double rx = cx + dx * cosA - dy * sinA;
        double ry = cy + dx * sinA + dy * cosA;
        return QPointF(rx, ry);
    };

    QPointF tl = rotatePt(lyr->offsetX, lyr->offsetY);
    QPointF tr = rotatePt(lyr->offsetX + scaledW, lyr->offsetY);
    QPointF br = rotatePt(lyr->offsetX + scaledW, lyr->offsetY + scaledH);
    QPointF bl = rotatePt(lyr->offsetX, lyr->offsetY + scaledH);

    QPointF tc = rotatePt(lyr->offsetX + scaledW / 2.0, lyr->offsetY);
    QPointF bc = rotatePt(lyr->offsetX + scaledW / 2.0, lyr->offsetY + scaledH);
    QPointF ml = rotatePt(lyr->offsetX, lyr->offsetY + scaledH / 2.0);
    QPointF mr = rotatePt(lyr->offsetX + scaledW, lyr->offsetY + scaledH / 2.0);

    QPointF rotHandle = rotatePt(lyr->offsetX + scaledW / 2.0, lyr->offsetY - 25.0);

    QPolygonF poly;
    poly << tl << tr << br << bl;

    std::map<QString, QPointF> handles;
    handles["tl"] = tl; handles["tc"] = tc; handles["tr"] = tr;
    handles["ml"] = ml; handles["mr"] = mr;
    handles["bl"] = bl; handles["bc"] = bc; handles["br"] = br;
    handles["rot"] = rotHandle;

    return std::make_tuple(poly, QPointF(cx, cy), handles);
}

QString CanvasView::getHandleAtPoint(const QPointF& scenePos) const {
    if (!m_document || !m_document->getActiveLayer()) return "";

    auto lyr = m_document->getActiveLayer();
    if (lyr->locked) return "";

    auto [poly, center, handles] = getLayerScreenPolygon(lyr);
    double handleSize = 10.0 / m_zoomFactor;

    for (const auto& [hname, hpos] : handles) {
        QRectF rect(hpos.x() - handleSize / 2.0, hpos.y() - handleSize / 2.0, handleSize, handleSize);
        if (rect.contains(scenePos)) {
            return hname;
        }
    }
    return "";
}

void CanvasView::mousePressEvent(QMouseEvent* event) {
    if (m_isSpacePanning || event->button() == Qt::MiddleButton) {
        m_panStartPos = event->position();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }

    QPointF scenePos = mapToScene(event->position().toPoint());

    if (isSelectToolActive()) {
        QString handle = getHandleAtPoint(scenePos);
        if (!handle.isEmpty() && m_document && m_document->getActiveLayer()) {
            m_dragMode = handle;
            m_dragStartCanvasPos = scenePos;
            m_dragStartLayerStates.clear();

            for (const auto& lyr : m_document->getActiveLayers()) {
                m_dragStartLayerStates[lyr->id] = std::make_tuple(lyr->offsetX, lyr->offsetY, lyr->scaleX, lyr->scaleY, lyr->rotation);
            }
            event->accept();
            return;
        }

        std::shared_ptr<Core::Layer> targetLayer = nullptr;

        // Priority 1: If click is inside currently active layer, keep active layer selected
        auto activeLyr = m_document ? m_document->getActiveLayer() : nullptr;
        if (activeLyr && activeLyr->visible && !activeLyr->locked) {
            auto [poly, center, handles] = getLayerScreenPolygon(activeLyr);
            if (poly.containsPoint(scenePos, Qt::OddEvenFill)) {
                targetLayer = activeLyr;
            }
        }

        // Priority 2: If active layer was not hit, find topmost layer under cursor
        if (!targetLayer) {
            targetLayer = getLayerAtPoint(scenePos);
            if (targetLayer) {
                bool multi = (event->modifiers() & Qt::ControlModifier) || (event->modifiers() & Qt::ShiftModifier);
                m_document->selectLayer(targetLayer->id, multi);
            }
        }

        // Start dragging target layer
        if (targetLayer && !targetLayer->locked) {
            m_dragMode = "move";
            m_dragStartCanvasPos = scenePos;
            m_dragStartLayerStates.clear();
            for (const auto& lyr : m_document->getActiveLayers()) {
                m_dragStartLayerStates[lyr->id] = std::make_tuple(lyr->offsetX, lyr->offsetY, lyr->scaleX, lyr->scaleY, lyr->rotation);
            }
        }
    }

    if (m_activeTool && m_dragMode.isEmpty()) {
        m_activeTool->mousePress(scenePos, event);
        viewport()->update();
    }

    QGraphicsView::mousePressEvent(event);
}

void CanvasView::mouseMoveEvent(QMouseEvent* event) {
    if (!m_panStartPos.isNull()) {
        QPointF delta = event->position() - m_panStartPos;
        m_panStartPos = event->position();
        panViewport(delta.x(), delta.y());
        event->accept();
        return;
    }

    QPointF scenePos = mapToScene(event->position().toPoint());
    m_hoverImgPos = scenePos;

    if (m_dragMode.isEmpty()) {
        if (isSelectToolActive()) {
            QString handle = getHandleAtPoint(scenePos);
            if (handle == "rot") setCursor(Qt::PointingHandCursor);
            else if (handle == "tl" || handle == "br") setCursor(Qt::SizeFDiagCursor);
            else if (handle == "tr" || handle == "bl") setCursor(Qt::SizeBDiagCursor);
            else if (handle == "tc" || handle == "bc") setCursor(Qt::SizeVerCursor);
            else if (handle == "ml" || handle == "mr") setCursor(Qt::SizeHorCursor);
            else unsetCursor();
        } else {
            unsetCursor();
        }
    }

    if (!m_dragMode.isEmpty() && !m_dragStartCanvasPos.isNull() && m_document && m_document->getActiveLayer()) {
        double dx = scenePos.x() - m_dragStartCanvasPos.x();
        double dy = scenePos.y() - m_dragStartCanvasPos.y();
        auto lyr = m_document->getActiveLayer();

        if (m_dragMode == "move") {
            for (auto activeLyr : m_document->getActiveLayers()) {
                if (!activeLyr->locked && m_dragStartLayerStates.find(activeLyr->id) != m_dragStartLayerStates.end()) {
                    auto [initX, initY, sX, sY, rot] = m_dragStartLayerStates[activeLyr->id];
                    activeLyr->offsetX = initX + dx;
                    activeLyr->offsetY = initY + dy;
                }
            }
        } else if (m_dragMode == "rot") {
            auto [initX, initY, sX, sY, initRot] = m_dragStartLayerStates[lyr->id];
            double cx = initX + (lyr->width() * lyr->scaleX) / 2.0;
            double cy = initY + (lyr->height() * lyr->scaleY) / 2.0;

            double angle = std::atan2(scenePos.y() - cy, scenePos.x() - cx);
            double startAngle = std::atan2(m_dragStartCanvasPos.y() - cy, m_dragStartCanvasPos.x() - cx);
            double deltaDeg = (angle - startAngle) * 180.0 / M_PI;
            lyr->rotation = std::fmod(initRot + deltaDeg, 360.0);
        } else if (m_dragMode == "br" || m_dragMode == "tl" || m_dragMode == "tr" || m_dragMode == "bl") {
            auto [initX, initY, initSx, initSy, rot] = m_dragStartLayerStates[lyr->id];
            double origW = lyr->width();
            double origH = lyr->height();

            if (m_dragMode.contains("r")) {
                double newW = std::max(10.0, (origW * initSx) + dx);
                lyr->scaleX = newW / origW;
            } else if (m_dragMode.contains("l")) {
                double newW = std::max(10.0, (origW * initSx) - dx);
                lyr->scaleX = newW / origW;
                lyr->offsetX = initX + dx;
            }

            if (m_dragMode.contains("b")) {
                double newH = std::max(10.0, (origH * initSy) + dy);
                lyr->scaleY = newH / origH;
            } else if (m_dragMode.contains("t")) {
                double newH = std::max(10.0, (origH * initSy) - dy);
                lyr->scaleY = newH / origH;
                lyr->offsetY = initY + dy;
            }

            if (lyr->lockAspect) {
                double avgScale = (lyr->scaleX + lyr->scaleY) / 2.0;
                lyr->scaleX = avgScale;
                lyr->scaleY = avgScale;
            }
        }

        updateViewport(true);
        event->accept();
        return;
    }

    emit mouseMovedSignal(scenePos, QColor(0, 0, 0));

    if (m_activeTool && m_dragMode.isEmpty()) {
        m_activeTool->mouseMove(scenePos, event);
        viewport()->update();
    }

    QGraphicsView::mouseMoveEvent(event);
}

void CanvasView::mouseReleaseEvent(QMouseEvent* event) {
    if (!m_panStartPos.isNull()) {
        m_panStartPos = QPointF();
        if (m_isSpacePanning) setCursor(Qt::OpenHandCursor);
        else unsetCursor();
        event->accept();
        return;
    }

    if (!m_dragMode.isEmpty()) {
        if (m_document && !m_dragStartLayerStates.empty()) {
            std::map<QString, std::tuple<double, double, double, double, double>> startStates = m_dragStartLayerStates;
            std::map<QString, std::tuple<double, double, double, double, double>> endStates;
            bool changed = false;

            for (const auto& lyr : m_document->getActiveLayers()) {
                endStates[lyr->id] = std::make_tuple(lyr->offsetX, lyr->offsetY, lyr->scaleX, lyr->scaleY, lyr->rotation);
                if (startStates.find(lyr->id) != startStates.end() && startStates[lyr->id] != endStates[lyr->id]) {
                    changed = true;
                }
            }

            if (changed) {
                auto undoFn = [doc = m_document, startStates]() {
                    for (const auto& [lid, state] : startStates) {
                        auto l = doc->getLayerById(lid);
                        if (l) {
                            auto [x, y, sx, sy, r] = state;
                            l->offsetX = x; l->offsetY = y;
                            l->scaleX = sx; l->scaleY = sy;
                            l->rotation = r;
                        }
                    }
                };

                auto redoFn = [doc = m_document, endStates]() {
                    for (const auto& [lid, state] : endStates) {
                        auto l = doc->getLayerById(lid);
                        if (l) {
                            auto [x, y, sx, sy, r] = state;
                            l->offsetX = x; l->offsetY = y;
                            l->scaleX = sx; l->scaleY = sy;
                            l->rotation = r;
                        }
                    }
                };

                auto cmd = std::make_unique<Core::DocumentActionCommand>(m_document.get(), undoFn, redoFn, "Transform Layer");
                m_document->undoStack.push(std::move(cmd));
            }
        }

        m_dragMode.clear();
        m_dragStartCanvasPos = QPointF();
        m_dragStartLayerStates.clear();
        updateViewport(false);
        if (m_document) m_document->notifyChanged();
        event->accept();
        return;
    }

    QPointF scenePos = mapToScene(event->position().toPoint());
    if (m_activeTool) {
        m_activeTool->mouseRelease(scenePos, event);
        viewport()->update();
    }

    QGraphicsView::mouseReleaseEvent(event);
}

void CanvasView::drawForeground(QPainter* painter, const QRectF& rect) {
    QGraphicsView::drawForeground(painter, rect);

    if (!m_document) return;

    double cw = m_document->canvasWidth;
    double ch = m_document->canvasHeight;

    // 1. Canvas Boundary Frame (Neon Cyan + Amber Gold Corners)
    QPen framePen(QColor(0, 210, 255), 2.0 / m_zoomFactor);
    painter->setPen(framePen);
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(QRectF(0, 0, cw, ch));

    double bracketLen = std::min(40.0, std::max(12.0, 24.0 / m_zoomFactor));
    QPen bracketPen(QColor(255, 170, 0), std::max(1.5, 3.0 / m_zoomFactor));
    painter->setPen(bracketPen);

    painter->drawLine(QPointF(0, 0), QPointF(bracketLen, 0));
    painter->drawLine(QPointF(0, 0), QPointF(0, bracketLen));

    painter->drawLine(QPointF(cw, 0), QPointF(cw - bracketLen, 0));
    painter->drawLine(QPointF(cw, 0), QPointF(cw, bracketLen));

    painter->drawLine(QPointF(cw, ch), QPointF(cw - bracketLen, ch));
    painter->drawLine(QPointF(cw, ch), QPointF(cw, ch - bracketLen));

    painter->drawLine(QPointF(0, ch), QPointF(bracketLen, ch));
    painter->drawLine(QPointF(0, ch), QPointF(0, ch - bracketLen));

    // Floating Dimension Badge
    QString badgeText = QString(" Canvas: %1 × %2 px ").arg(static_cast<int>(cw)).arg(static_cast<int>(ch));
    QFont badgeFont = painter->font();
    badgeFont.setPointSize(10);
    badgeFont.setBold(true);
    painter->setFont(badgeFont);

    QFontMetrics metrics(badgeFont);
    int textW = metrics.horizontalAdvance(badgeText) + 12;
    int textH = metrics.height() + 6;

    QRectF badgeRect(0, -textH - (6.0 / m_zoomFactor), textW, textH);
    painter->setPen(Qt::NoPen);
    painter->setBrush(QBrush(QColor(0, 120, 215, 230)));
    painter->drawRoundedRect(badgeRect, 4, 4);

    painter->setPen(QPen(QColor(255, 255, 255)));
    painter->drawText(badgeRect, Qt::AlignCenter, badgeText);

    // 2. Render Grid if enabled
    if (m_document->showGrid) {
        int gridSize = m_document->gridSize;
        QPen gridPen(QColor(255, 255, 255, static_cast<int>(m_document->gridOpacity * 255)));
        gridPen.setStyle(Qt::DotLine);
        painter->setPen(gridPen);
        for (int x = 0; x < cw; x += gridSize) painter->drawLine(x, 0, x, ch);
        for (int y = 0; y < ch; y += gridSize) painter->drawLine(0, y, cw, y);
    }

    // 3. Selection Bounding Box & Handles
    if (!m_document->activeLayerIds.empty()) {
        bool isSelectMode = isSelectToolActive();
        for (const auto& lyr : m_document->getActiveLayers()) {
            auto [poly, center, handles] = getLayerScreenPolygon(lyr);

            QPen pen(QColor(0, 120, 215), 1.5 / m_zoomFactor);
            pen.setStyle(lyr->locked ? Qt::DashLine : Qt::SolidLine);
            painter->setPen(pen);
            painter->setBrush(Qt::NoBrush);
            painter->drawPolygon(poly);

            if (!lyr->locked && isSelectMode) {
                painter->setPen(QPen(QColor(0, 120, 215), 1.0 / m_zoomFactor));
                painter->drawLine(handles["tc"], handles["rot"]);

                double handleSz = 8.0 / m_zoomFactor;
                painter->setPen(QPen(QColor(255, 255, 255), 1.0 / m_zoomFactor));
                painter->setBrush(QBrush(QColor(0, 120, 215)));

                for (const auto& [hname, hpos] : handles) {
                    if (hname == "rot") {
                        painter->setBrush(QBrush(QColor(255, 140, 0)));
                        painter->drawEllipse(hpos, handleSz / 1.5, handleSz / 1.5);
                        painter->setBrush(QBrush(QColor(0, 120, 215)));
                    } else {
                        QRectF hrect(hpos.x() - handleSz / 2.0, hpos.y() - handleSz / 2.0, handleSz, handleSz);
                        painter->drawRect(hrect);
                    }
                }
            }
        }
    }

    // 4. Tool Overlay
    if (m_activeTool) {
        m_activeTool->drawOverlay(painter);
    }
}

void CanvasView::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void CanvasView::dropEvent(QDropEvent* event) {
    QStringList filePaths;
    for (const QUrl& url : event->mimeData()->urls()) {
        QString localPath = url.toLocalFile();
        if (!localPath.isEmpty()) {
            filePaths.append(localPath);
        }
    }

    if (filePaths.size() == 1) {
        emit imageDroppedSignal(filePaths.first());
    } else if (filePaths.size() > 1) {
        emit imagesDroppedSignal(filePaths);
    }
}

} // namespace UI
} // namespace ImageCut
