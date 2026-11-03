#include "IntegratedApplicationController.h"

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QTimer>

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    QApplication::setApplicationName(QStringLiteral("TDLAS Hydrogen UI"));
    QApplication::setOrganizationName(QStringLiteral("DJQuantum"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("TDLAS 测氢系统整合界面"));
    parser.addHelpOption();
    parser.addOption(QCommandLineOption(
        QStringList{QStringLiteral("capture")},
        QStringLiteral("保存登录页候选截图后退出。"),
        QStringLiteral("path")));
    parser.process(application);

    IntegratedApplicationController controller(application);
    controller.start();

    const QString capturePath = parser.value(QStringLiteral("capture"));
    if (!capturePath.isEmpty()) {
        StartupTemplateWindow *loginWindow = controller.loginWindow();
        QTimer::singleShot(350, loginWindow, [loginWindow, capturePath]() {
            const bool saved = loginWindow->saveCapture(capturePath);
            QCoreApplication::exit(saved ? 0 : 2);
        });
    }

    return application.exec();
}
