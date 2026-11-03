#pragma once

#include <QWidget>

class QFrame;
class QLabel;
class QLineEdit;
class QPushButton;

class StartupTemplateWindow final : public QWidget
{
    Q_OBJECT

public:
    enum class Screen {
        Splash,
        Login
    };

    explicit StartupTemplateWindow(Screen initialScreen, QWidget *parent = nullptr);

    void setScreen(Screen screen);
    void setLoginMessage(const QString &message, bool isError = false);
    void setLoginBusy(bool busy);
    void clearPassword(bool restoreFocus = false);
    [[nodiscard]] bool saveCapture(const QString &path);

signals:
    void loginSubmitted(const QString &username, const QString &password);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    [[nodiscard]] QRect fittedImageRect() const;
    void buildLoginForm();
    void updateLoginFormGeometry();
    void updateWindowForScreen();
    void submitLogin();

    Screen screen_;
    QFrame *loginForm_ = nullptr;
    QLineEdit *usernameEdit_ = nullptr;
    QLineEdit *passwordEdit_ = nullptr;
    QPushButton *loginButton_ = nullptr;
    QLabel *loginMessage_ = nullptr;
};
