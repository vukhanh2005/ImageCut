#ifndef IMAGECUT_BATCHWORKER_H
#define IMAGECUT_BATCHWORKER_H

#include <QThread>
#include <QString>
#include <QStringList>

namespace ImageCut {
namespace Workers {

class BatchWorker : public QThread {
    Q_OBJECT
public:
    BatchWorker(
        const QStringList& filePaths,
        const QString& outputDir,
        const QString& modelName = "RMBG-1.4",
        const QString& outputFormat = "PNG",
        int quality = 95
    );
    ~BatchWorker() override = default;

    void cancel();

signals:
    void progress(int currentIndex, int totalCount, const QString& currentFilename);
    void finished(int successCount, int failCount);
    void error(const QString& errorMsg);

protected:
    void run() override;

private:
    QStringList m_filePaths;
    QString m_outputDir;
    QString m_modelName;
    QString m_outputFormat;
    int m_quality;
    bool m_isCancelled = false;
};

} // namespace Workers
} // namespace ImageCut

#endif // IMAGECUT_BATCHWORKER_H
