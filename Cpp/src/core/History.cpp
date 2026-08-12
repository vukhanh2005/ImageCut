#include "core/History.h"
#include "core/ImageDocument.h"
#include "utils/Logger.h"

namespace ImageCut {
namespace Core {

// MaskEditCommand
MaskEditCommand::MaskEditCommand(ImageDocument* document,
                                 const cv::Mat& oldMask,
                                 const cv::Mat& newMask,
                                 const QString& layerId,
                                 const QString& description)
    : m_document(document), m_layerId(layerId), m_description(description)
{
    if (!oldMask.empty()) m_oldMask = oldMask.clone();
    if (!newMask.empty()) m_newMask = newMask.clone();
}

void MaskEditCommand::execute() {
    if (!m_document) return;
    auto target = m_layerId.isEmpty() ? m_document->getActiveLayer() : m_document->getLayerById(m_layerId);
    if (target) {
        target->mask = m_newMask.empty() ? cv::Mat() : m_newMask.clone();
        m_document->notifyChanged();
    }
}

void MaskEditCommand::undo() {
    if (!m_document) return;
    auto target = m_layerId.isEmpty() ? m_document->getActiveLayer() : m_document->getLayerById(m_layerId);
    if (target) {
        target->mask = m_oldMask.empty() ? cv::Mat() : m_oldMask.clone();
        m_document->notifyChanged();
    }
}

// DocumentActionCommand
DocumentActionCommand::DocumentActionCommand(ImageDocument* document,
                                             std::function<void()> undoFn,
                                             std::function<void()> redoFn,
                                             const QString& description)
    : m_document(document), m_undoFn(undoFn), m_redoFn(redoFn), m_description(description)
{}

void DocumentActionCommand::execute() {
    if (m_redoFn) m_redoFn();
    if (m_document) m_document->notifyChanged();
}

void DocumentActionCommand::undo() {
    if (m_undoFn) m_undoFn();
    if (m_document) m_document->notifyChanged();
}

// UndoStack
UndoStack::UndoStack(size_t maxDepth) : m_maxDepth(maxDepth) {}

void UndoStack::push(std::unique_ptr<Command> command) {
    if (!command) return;
    command->execute();
    m_undoStack.push_back(std::move(command));

    if (m_undoStack.size() > m_maxDepth) {
        m_undoStack.erase(m_undoStack.begin());
    }
    m_redoStack.clear();
    notify();
}

void UndoStack::undo() {
    if (!canUndo()) return;
    auto cmd = std::move(m_undoStack.back());
    m_undoStack.pop_back();

    cmd->undo();
    m_redoStack.push_back(std::move(cmd));
    notify();
}

void UndoStack::redo() {
    if (!canRedo()) return;
    auto cmd = std::move(m_redoStack.back());
    m_redoStack.pop_back();

    cmd->execute();
    m_undoStack.push_back(std::move(cmd));
    notify();
}

bool UndoStack::canUndo() const {
    return !m_undoStack.empty();
}

bool UndoStack::canRedo() const {
    return !m_redoStack.empty();
}

void UndoStack::clear() {
    m_undoStack.clear();
    m_redoStack.clear();
    notify();
}

void UndoStack::addChangeListener(std::function<void()> callback) {
    m_onChangeCallbacks.push_back(callback);
}

void UndoStack::notify() {
    for (auto& cb : m_onChangeCallbacks) {
        if (cb) {
            try { cb(); } catch (...) {}
        }
    }
}

} // namespace Core
} // namespace ImageCut
