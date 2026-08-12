#include "ui/ToolBarPanel.h"
#include <QVBoxLayout>
#include <vector>

namespace ImageCut {
namespace UI {

ToolBarPanel::ToolBarPanel(QWidget* parent)
    : QFrame(parent)
{
    setFixedWidth(54);
    setObjectName("toolbar_panel");

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 8, 4, 8);
    layout->setSpacing(6);
    layout->setAlignment(Qt::AlignTop);

    m_btnGroup = new QButtonGroup(this);
    m_btnGroup->setExclusive(true);

    struct ToolItem {
        QString id;
        QString tooltip;
        QString iconStr;
    };

    std::vector<ToolItem> tools = {
        { "Select", "Select & Pan Tool (H / Space)", "✋" },
        { "Brush", "Restore Brush Tool (B)", "🖌️" },
        { "Eraser", "Eraser Tool (E)", "🧹" },
        { "RefineEdge", "Refine Edge Hair Matting Tool (R)", "💇" },
        { "MagicWand", "Magic Wand Color Select (W)", "🪄" },
        { "Lasso", "Freehand Lasso Tool (L)", "✂️" },
        { "PolyLasso", "Point-to-Point Polygon Keep/Remove Tool (P)", "🪡" },
        { "Crop", "Crop Canvas Tool (C)", "🖼️" }
    };

    for (const auto& item : tools) {
        QPushButton* btn = new QPushButton(item.iconStr, this);
        btn->setObjectName("btn_tool");
        btn->setCheckable(true);
        btn->setToolTip(item.tooltip);
        btn->setFixedSize(44, 44);

        m_btnGroup->addButton(btn);
        m_buttons[item.id] = btn;
        layout->addWidget(btn);

        connect(btn, &QPushButton::clicked, [this, item]() {
            emit toolChangedSignal(item.id);
        });
    }

    if (m_buttons.find("Select") != m_buttons.end()) {
        m_buttons["Select"]->setChecked(true);
    }
}

void ToolBarPanel::setActiveTool(const QString& toolId) {
    if (m_buttons.find(toolId) != m_buttons.end()) {
        m_buttons[toolId]->setChecked(true);
    }
}

} // namespace UI
} // namespace ImageCut
