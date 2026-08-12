#ifndef IMAGECUT_BATCHDIALOG_H
#define IMAGECUT_BATCHDIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QProgressBar>
#include <QLabel>
#include <QPushButton>
#include <QStringList>

namespace ImageCut {
namespace UI {

class BatchDialog : public QDialog {
    Q_OBJECT
public:
    explicit BatchDialog(QWidget* parent = nullptr);
    ~BatchDialog() override = default;

private:
    void addFiles();
    void addFolder();
    void clearQueue();
    void browseOutput();
    void startBatch();

    QStringList m_filePaths;
    QListWidget* m_listFiles = nullptr;
    QLineEdit* m_txtOutDir = nullptr;
    QComboBox* m_comboModel = nullptr;
    QComboBox* m_comboFmt = nullptr;
    QProgressBar* m_progressBar = nullptr;
    QLabel* m_lblStatus = nullptr;
    QPushButton* m_btnStart = nullptr;
};

} // namespace UI
} // namespace ImageCut

#endif // IMAGECUT_BATCHDIALOG_H
