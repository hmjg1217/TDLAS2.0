#include "StartupTemplateWindow.h"

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QFile>
#include <QTimer>

namespace {

void applyStyleSheet(QApplication &application)
{
    QFile styleFile(QStringLiteral(":/styles/startup.qss"));
    if (styleFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        application.setStyleSheet(QString::fromUtf8(styleFile.readAll()));
    }
}

StartupTemplateWindow::Screen parseScreen(const QString &value)
{
    return value.compare(QStringLiteral("splash"), Qt::CaseInsensitive) == 0
        ? StartupTemplateWindow::Screen::Splash
        : StartupTemplateWindow::Screen::Login;
}

} // namespace

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    QApplication::setApplicationName(QStringLiteral("TDLAS Startup Codex Template"));
    QApplication::setOrganizationName(QStringLiteral("DJQuantum"));
    applyStyleSheet(application);

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("TDLAS 启动与登录 UI 编译模板"));
    parser.addHelpOption();
    parser.addOption(QCommandLineOption(
        QStringList{QStringLiteral("screen")},
        QStringLiteral("选择开发预览状态：splash 或 login。"),
        QStringLiteral("name"),
        QStringLiteral("login")));
    parser.addOption(QCommandLineOption(
        QStringList{QStringLiteral("capture")},
        QStringLiteral("渲染后将窗口保存为 PNG 并退出。"),
        QStringLiteral("path")));
    parser.process(application);

    StartupTemplateWindow window(parseScreen(parser.value(QStringLiteral("screen"))));
    QObject::connect(
        &window,
        &StartupTemplateWindow::loginSubmitted,
        &window,
        [&window](const QString &, const QString &) {
            window.setLoginMessage(QStringLiteral("认证接口未配置；本次未执行登录。"), true);
        });

    window.show();

    const QString capturePath = parser.value(QStringLiteral("capture"));
    if (!capturePath.isEmpty()) {
        QTimer::singleShot(350, &window, [&window, capturePath]() {
            const bool saved = window.saveCapture(capturePath);
            QCoreApplication::exit(saved ? 0 : 2);
        });
    }

    return application.exec();
}
