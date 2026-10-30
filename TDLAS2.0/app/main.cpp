#include "ui/main_window.h"
#include "ui/startup_dialog.h"
#include <QApplication>
#include <QDir>
#include <QGuiApplication>
#include <QIcon>
#include <QScreen>
#include <QTimer>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    Q_INIT_RESOURCE(resources);
    app.setOrganizationName("DJQUANTUM");
    app.setApplicationName("TDLAS2"); app.setApplicationVersion("2.0.0");
    app.setWindowIcon(QIcon(QStringLiteral(":/icons/tdlas_hydrogen.png")));
    QDir root(QCoreApplication::applicationDirPath());
    // Find the project from a development build or use the portable package directory.
    while (!root.exists("tools/local-env.json")) {
        if (!root.cdUp()) { root = QDir(QCoreApplication::applicationDirPath()); break; }
    }
    QString dataRoot = qEnvironmentVariable("TDLAS_DATA_ROOT", root.filePath("data"));
    const int smoke = app.arguments().indexOf("--smoke-test");
    if (smoke >= 0) {
        tdlas::MainWindow window(dataRoot);
        window.showMaximized();
        const QString output = app.arguments().value(smoke + 1);
        QTimer::singleShot(1200, &window, [&, output] {
            const bool ok = window.isMaximized() && !window.runtime()->active() && !output.isEmpty() && window.grab().save(output);
            app.exit(ok ? 0 : 2);
        });
        return app.exec();
    }

    tdlas::LoginDialog login;
    login.showMaximized();
    if (login.exec() != QDialog::Accepted) return 0;

    tdlas::MainWindow window(dataRoot, nullptr, login.role(), login.accountName());
    tdlas::StartupSplash splash;
    if (const auto* screen = QGuiApplication::primaryScreen()) {
        splash.move(screen->availableGeometry().center() - splash.rect().center());
    }
    splash.show();
    QTimer::singleShot(1500, &window, [&] {
        window.showMaximized();
        splash.close();
    });
    return app.exec();
}
