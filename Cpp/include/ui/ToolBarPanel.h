#ifndef IMAGECUT_TOOLBARPANEL_H
#define IMAGECUT_TOOLBARPANEL_H

#include <QFrame>
#include <QPushButton>
#include <QButtonGroup>
#include <map>
#include <QString>

namespace ImageCut {
namespace UI {

class ToolBarPanel : public QFrame {
    Q_OBJECT
public:
    explicit ToolBarPanel(QWidget* parent = nullptr);
    ~ToolBarPanel() override = default;

    void setActiveTool(const QString& toolId);

signals:
    void toolChangedSignal(const QString& toolId);

private:
    QButtonGroup* m_btnGroup = nullptr;
    std::map<QString, QPushButton*> m_buttons;
};

} // namespace UI
} // namespace ImageCut

#endif // IMAGECUT_TOOLBARPANEL_H
