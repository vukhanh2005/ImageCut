#ifndef IMAGECUT_TOPBARPANEL_H
#define IMAGECUT_TOPBARPANEL_H

#include <QFrame>
#include <QPushButton>
#include <QProgressBar>

namespace ImageCut {
namespace UI {

class TopBarPanel : public QFrame {
    Q_OBJECT
public:
    explicit TopBarPanel(QWidget* parent = nullptr);
    ~TopBarPanel() override = default;

    void showProgress(int val);
    void hideProgress();

signals:
    void openSignal();
    void saveSignal();
    void undoSignal();
    void redoSignal();
    void autoRemoveSignal();
    void batchSignal();
    void exportSignal();
    void toggleSnapSignal(bool enabled);

public:
    QPushButton* btnAutoRemove = nullptr;
    QPushButton* btnUndo = nullptr;
    QPushButton* btnRedo = nullptr;
    QPushButton* btnSnap = nullptr;

private:
    QProgressBar* m_progressBar = nullptr;
};

} // namespace UI
} // namespace ImageCut

#endif // IMAGECUT_TOPBARPANEL_H
