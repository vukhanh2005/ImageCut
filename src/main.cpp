#include <QApplication>
#include "ui/MainWindow.h"
#include "ui/Style.h"
#include "utils/Settings.h"
#include "utils/Logger.h"

int main(int argc, char *argv[]) {
    ImageCut::Utils::Logger::getInstance().init("logs/app.log");
    LOG_INFO("Starting Personal Background Remover & Image Editor (C++ Native)...");

    QApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    QApplication app(argc, argv);
    app.setApplicationName("BackgroundRemover");
    app.setOrganizationName("ImageCut");

    QString themeName = ImageCut::Utils::Settings::getInstance().get("theme", "Dark");
    ImageCut::UI::Style::applyTheme(&app, themeName);

    ImageCut::UI::MainWindow window;
    window.show();

    return app.exec();
}
