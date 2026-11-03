#include "ClientTemplateWindow.h"

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QFile>
#include <QTimer>

namespace {

void applyStyleSheet(QApplication &application)
{
    QFile styleFile(QStringLiteral(":/styles/client.qss"));
    if (styleFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        application.setStyleSheet(QString::fromUtf8(styleFile.readAll()));
    }
}

clientui::ClientTemplateWindow::Screen parseScreen(const QString &value)
{
    const QString normalized = value.trimmed().toLower();
    if (normalized == QStringLiteral("parameters")) {
        return clientui::ClientTemplateWindow::Screen::Parameters;
    }
    if (normalized == QStringLiteral("camera")) {
        return clientui::ClientTemplateWindow::Screen::Camera;
    }
    if (normalized == QStringLiteral("camera-motion")) {
        return clientui::ClientTemplateWindow::Screen::CameraMotion;
    }
    return clientui::ClientTemplateWindow::Screen::Main;
}

} // namespace

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    QApplication::setApplicationName(QStringLiteral("TDLAS Client Codex Template"));
    QApplication::setOrganizationName(QStringLiteral("DJQuantum"));
    applyStyleSheet(application);

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("TDLAS 客户 UI 可编译模板"));
    parser.addHelpOption();
    parser.addOption(QCommandLineOption(
        QStringList{QStringLiteral("screen")},
        QStringLiteral("选择预览状态：main、parameters、camera、camera-motion。"),
        QStringLiteral("name"),
        QStringLiteral("main")));
    parser.addOption(QCommandLineOption(
        QStringList{QStringLiteral("capture")},
        QStringLiteral("渲染后将窗口保存为 PNG 并退出。"),
        QStringLiteral("path")));
    parser.process(application);

    const auto requestedScreen = parseScreen(parser.value(QStringLiteral("screen")));
    clientui::ClientTemplateWindow window(requestedScreen);
    window.show();
    window.setScreen(requestedScreen);

    const QString capturePath = parser.value(QStringLiteral("capture"));
    if (!capturePath.isEmpty()) {
        QTimer::singleShot(650, &window, [&window, capturePath]() {
            const bool saved = window.saveCapture(capturePath);
            QCoreApplication::exit(saved ? 0 : 2);
        });
    }

    return application.exec();
}
