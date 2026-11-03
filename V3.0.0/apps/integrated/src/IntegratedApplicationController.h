#pragma once

#include "StartupTemplateWindow.h"
#include "TemporaryCredentialAuthenticator.h"

#include <QObject>
#include <QPointer>

class QApplication;
class QWidget;

class IntegratedApplicationController final : public QObject
{
public:
    explicit IntegratedApplicationController(
        QApplication &application,
        QObject *parent = nullptr);
    ~IntegratedApplicationController() override;

    void start();

    [[nodiscard]] authentication::UserRole currentRole() const;
    [[nodiscard]] StartupTemplateWindow *loginWindow() const;
    [[nodiscard]] QWidget *roleWindow() const;

private:
    void handleLogin(const QString &username, const QString &password);

    QApplication &application_;
    authentication::TemporaryCredentialAuthenticator authenticator_;
    authentication::UserRole currentRole_ = authentication::UserRole::Invalid;
    QPointer<StartupTemplateWindow> loginWindow_;
    QPointer<QWidget> roleWindow_;
};
