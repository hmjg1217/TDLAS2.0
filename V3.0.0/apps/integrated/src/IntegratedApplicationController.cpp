#include "IntegratedApplicationController.h"

#include "AdminTemplateWindow.h"
#include "ClientTemplateWindow.h"

#include <QApplication>
#include <QFile>
#include <QWidget>

namespace {

QString readStyleSheet(const QString &resourcePath)
{
    QFile styleFile(resourcePath);
    if (!styleFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }
    return QString::fromUtf8(styleFile.readAll());
}

void applyStyleSheet(QApplication &application, const QString &resourcePath)
{
    application.setStyleSheet(readStyleSheet(resourcePath));
}

QWidget *createRoleWindow(authentication::UserRole role)
{
    if (role == authentication::UserRole::Administrator) {
        return new adminui::AdminTemplateWindow(adminui::AdminTemplateWindow::Screen::Main);
    }
    if (role == authentication::UserRole::Client) {
        return new clientui::ClientTemplateWindow(clientui::ClientTemplateWindow::Screen::Main);
    }
    return nullptr;
}

QString stylePathForRole(authentication::UserRole role)
{
    return role == authentication::UserRole::Administrator
        ? QStringLiteral(":/styles/admin.qss")
        : QStringLiteral(":/styles/client.qss");
}

} // namespace

IntegratedApplicationController::IntegratedApplicationController(
    QApplication &application,
    QObject *parent)
    : QObject(parent)
    , application_(application)
{
}

IntegratedApplicationController::~IntegratedApplicationController()
{
    delete roleWindow_.data();
    delete loginWindow_.data();
}

void IntegratedApplicationController::start()
{
    if (loginWindow_ || roleWindow_) {
        return;
    }

    currentRole_ = authentication::UserRole::Invalid;
    applyStyleSheet(application_, QStringLiteral(":/styles/startup.qss"));

    loginWindow_ = new StartupTemplateWindow(StartupTemplateWindow::Screen::Login);
    loginWindow_->setAttribute(Qt::WA_DeleteOnClose);
    connect(
        loginWindow_,
        &StartupTemplateWindow::loginSubmitted,
        this,
        &IntegratedApplicationController::handleLogin);
    loginWindow_->show();
}

authentication::UserRole IntegratedApplicationController::currentRole() const
{
    return currentRole_;
}

StartupTemplateWindow *IntegratedApplicationController::loginWindow() const
{
    return loginWindow_.data();
}

QWidget *IntegratedApplicationController::roleWindow() const
{
    return roleWindow_.data();
}

void IntegratedApplicationController::handleLogin(
    const QString &username,
    const QString &password)
{
    const authentication::UserRole role = authenticator_.authenticate(username, password);
    if (role == authentication::UserRole::Invalid) {
        loginWindow_->setLoginMessage(QStringLiteral("账号或密码错误。"), true);
        loginWindow_->clearPassword(true);
        return;
    }

    loginWindow_->setLoginBusy(true);
    loginWindow_->clearPassword();
    loginWindow_->hide();
    applyStyleSheet(application_, stylePathForRole(role));

    roleWindow_ = createRoleWindow(role);
    if (!roleWindow_) {
        applyStyleSheet(application_, QStringLiteral(":/styles/startup.qss"));
        loginWindow_->setLoginBusy(false);
        loginWindow_->setLoginMessage(QStringLiteral("无法打开目标界面。"), true);
        loginWindow_->show();
        return;
    }

    currentRole_ = role;
    roleWindow_->setAttribute(Qt::WA_DeleteOnClose);
    roleWindow_->show();
    roleWindow_->raise();
    roleWindow_->activateWindow();
    loginWindow_->close();
}
