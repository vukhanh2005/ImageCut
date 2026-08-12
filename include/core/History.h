#ifndef IMAGECUT_HISTORY_H
#define IMAGECUT_HISTORY_H

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <opencv2/core.hpp>
#include <QString>

namespace ImageCut {
namespace Core {

class ImageDocument;

class Command {
public:
    virtual ~Command() = default;
    virtual void execute() = 0;
    virtual void undo() = 0;
    virtual QString getDescription() const { return "Command"; }
};

class MaskEditCommand : public Command {
public:
    MaskEditCommand(ImageDocument* document,
                    const cv::Mat& oldMask,
                    const cv::Mat& newMask,
                    const QString& layerId = "",
                    const QString& description = "Edit Mask");

    void execute() override;
    void undo() override;
    QString getDescription() const override { return m_description; }

private:
    ImageDocument* m_document;
    QString m_layerId;
    cv::Mat m_oldMask;
    cv::Mat m_newMask;
    QString m_description;
};

class DocumentActionCommand : public Command {
public:
    DocumentActionCommand(ImageDocument* document,
                          std::function<void()> undoFn,
                          std::function<void()> redoFn,
                          const QString& description = "Action");

    void execute() override;
    void undo() override;
    QString getDescription() const override { return m_description; }

private:
    ImageDocument* m_document;
    std::function<void()> m_undoFn;
    std::function<void()> m_redoFn;
    QString m_description;
};

class UndoStack {
public:
    explicit UndoStack(size_t maxDepth = 30);
    ~UndoStack() = default;

    void push(std::unique_ptr<Command> command);
    void undo();
    void redo();

    bool canUndo() const;
    bool canRedo() const;
    void clear();

    void addChangeListener(std::function<void()> callback);

private:
    void notify();

    size_t m_maxDepth;
    std::vector<std::unique_ptr<Command>> m_undoStack;
    std::vector<std::unique_ptr<Command>> m_redoStack;
    std::vector<std::function<void()>> m_onChangeCallbacks;
};

} // namespace Core
} // namespace ImageCut

#endif // IMAGECUT_HISTORY_H
