#include <QApplication>

#include "ui/main_window.h"

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    application.setOrganizationName(QStringLiteral("TDLAS"));
    application.setApplicationName(QStringLiteral("Hydrogen"));
    application.setApplicationVersion(QStringLiteral("1.6.0"));

    MainWindow window;
    window.show();

    return application.exec();
}
