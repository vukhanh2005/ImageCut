#ifndef IMAGECUT_EXPORTWORKER_H
#define IMAGECUT_EXPORTWORKER_H

#include <QThread>
#include <QString>
#include <memory>
#include "core/ImageDocument.h"

namespace ImageCut {
namespace Workers {

class ExportWorker : public QThread {
    Q_OBJECT
public:
    ExportWorker(
        std::shared_ptr<Core::ImageDocument> document,
        const QString& outputPath,
        const QString& formatStr = "PNG",
        int quality = 95,
        int width = 0,
        int height = 0
    );
    ~ExportWorker() override = default;

signals:
    void finished(const QString& outputPath);
    void error(const QString& errorMsg);

protected:
    void run() override;

private:
    std::shared_ptr<Core::ImageDocument> m_doc;
    QString m_outputPath;
    QString m_formatStr;
    int m_quality;
    int m_targetWidth;
    int m_targetHeight;
};

} // namespace Workers
} // namespace ImageCut

#endif // IMAGECUT_EXPORTWORKER_H
