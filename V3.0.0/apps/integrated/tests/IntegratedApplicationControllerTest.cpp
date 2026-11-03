#include "AdminTemplateWindow.h"
#include "ClientTemplateWindow.h"
#include "IntegratedApplicationController.h"

#include <QApplication>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

namespace {

bool submitCredentials(
    QApplication &application,
    IntegratedApplicationController &controller,
    const QString &username,
    const QString &password)
{
    StartupTemplateWindow *loginWindow = controller.loginWindow();
    if (!loginWindow) {
        return false;
    }

    auto *usernameEdit = loginWindow->findChild<QLineEdit *>(QStringLiteral("usernameEdit"));
    auto *passwordEdit = loginWindow->findChild<QLineEdit *>(QStringLiteral("passwordEdit"));
    auto *loginButton = loginWindow->findChild<QPushButton *>(QStringLiteral("loginButton"));
    if (!usernameEdit || !passwordEdit || !loginButton) {
        return false;
    }

    usernameEdit->setText(username);
    passwordEdit->setText(password);
    loginButton->click();
    application.processEvents();
    return true;
}

} // namespace

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    application.setQuitOnLastWindowClosed(false);

    {
        IntegratedApplicationController controller(application);
        controller.start();
        if (!submitCredentials(application, controller, QStringLiteral("00"), QStringLiteral("01"))) {
            return 1;
        }
        if (controller.currentRole() != authentication::UserRole::Invalid) {
            return 2;
        }
        StartupTemplateWindow *loginWindow = controller.loginWindow();
        if (!loginWindow || !loginWindow->isVisible()) {
            return 3;
        }
        auto *passwordEdit = loginWindow->findChild<QLineEdit *>(QStringLiteral("passwordEdit"));
        auto *message = loginWindow->findChild<QLabel *>(QStringLiteral("loginMessage"));
        if (!passwordEdit || !passwordEdit->text().isEmpty()) {
            return 4;
        }
        if (!message || message->text() != QStringLiteral("账号或密码错误。")) {
            return 5;
        }
    }
    application.processEvents();

    {
        IntegratedApplicationController controller(application);
        controller.start();
        if (!submitCredentials(application, controller, QStringLiteral("00"), QStringLiteral("00"))) {
            return 6;
        }
        if (controller.currentRole() != authentication::UserRole::Administrator) {
            return 7;
        }
        if (!dynamic_cast<adminui::AdminTemplateWindow *>(controller.roleWindow())) {
            return 8;
        }
        if (!controller.roleWindow()->isVisible()) {
            return 9;
        }
        if (!application.styleSheet().contains(QStringLiteral("adminTemplateRoot"))) {
            return 10;
        }
    }
    application.processEvents();

    {
        IntegratedApplicationController controller(application);
        controller.start();
        if (!submitCredentials(application, controller, QStringLiteral("01"), QStringLiteral("01"))) {
            return 11;
        }
        if (controller.currentRole() != authentication::UserRole::Client) {
            return 12;
        }
        if (!dynamic_cast<clientui::ClientTemplateWindow *>(controller.roleWindow())) {
            return 13;
        }
        if (!controller.roleWindow()->isVisible()) {
            return 14;
        }
        if (!application.styleSheet().contains(QStringLiteral("clientTemplateRoot"))) {
            return 15;
        }
    }

    return 0;
}
