#ifndef IMAGECUT_MASKPANEL_H
#define IMAGECUT_MASKPANEL_H

#include <QWidget>
#include <QComboBox>
#include <QSlider>
#include <QCheckBox>
#include <QLabel>
#include <QHBoxLayout>
#include <functional>
#include <memory>
#include "core/ImageDocument.h"

namespace ImageCut {
namespace UI {

class MaskPanel : public QWidget {
    Q_OBJECT
public:
    explicit MaskPanel(QWidget* parent = nullptr);
    ~MaskPanel() override = default;

    void setDocument(std::shared_ptr<Core::ImageDocument> doc);

private:
    QHBoxLayout* createSliderRow(const QString& labelText, int minVal, int maxVal, int initVal, std::function<void(int)> callback);

    std::shared_ptr<Core::ImageDocument> m_doc;
    QComboBox* m_comboViewMode = nullptr;
    QCheckBox* m_chkDecontam = nullptr;
};

} // namespace UI
} // namespace ImageCut

#endif // IMAGECUT_MASKPANEL_H
