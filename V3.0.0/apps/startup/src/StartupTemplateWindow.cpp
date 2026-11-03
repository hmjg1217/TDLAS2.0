#include "StartupTemplateWindow.h"

#include <QAction>
#include <QDir>
#include <QFileInfo>
#include <QFrame>
#include <QIcon>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QResizeEvent>
#include <QStyle>
#include <QVBoxLayout>

namespace {

constexpr QSize kSplashReferenceSize{1774, 887};
constexpr QSize kLoginReferenceSize{1671, 941};

QIcon makeUserIcon()
{
    QPixmap pixmap(36, 36);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(QColor(QStringLiteral("#6678A8")), 2.4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(QRectF(12.0, 5.0, 12.0, 12.0));
    QPainterPath shoulders;
    shoulders.moveTo(6.5, 31.0);
    shoulders.cubicTo(7.8, 23.0, 12.5, 19.5, 18.0, 19.5);
    shoulders.cubicTo(23.5, 19.5, 28.2, 23.0, 29.5, 31.0);
    painter.drawPath(shoulders);
    return QIcon(pixmap);
}

QIcon makeLockIcon()
{
    QPixmap pixmap(36, 36);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(QColor(QStringLiteral("#6678A8")), 2.4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(QRectF(8.0, 16.0, 20.0, 16.0), 2.5, 2.5);
    QPainterPath shackle;
    shackle.moveTo(12.0, 16.0);
    shackle.lineTo(12.0, 12.0);
    shackle.cubicTo(12.0, 7.0, 15.0, 4.5, 18.0, 4.5);
    shackle.cubicTo(21.0, 4.5, 24.0, 7.0, 24.0, 12.0);
    shackle.lineTo(24.0, 16.0);
    painter.drawPath(shackle);
    return QIcon(pixmap);
}

QPixmap referencePixmap(StartupTemplateWindow::Screen screen)
{
    return QPixmap(screen == StartupTemplateWindow::Screen::Splash
        ? QStringLiteral(":/reference/startup_splash.png")
        : QStringLiteral(":/reference/login_reference.png"));
}

} // namespace

StartupTemplateWindow::StartupTemplateWindow(Screen initialScreen, QWidget *parent)
    : QWidget(parent)
    , screen_(initialScreen)
{
    setObjectName(QStringLiteral("rootCanvas"));
    setWindowTitle(QStringLiteral("TDLAS 测氢系统 - Codex UI 模板"));
    setMinimumSize(960, 540);
    setFocusPolicy(Qt::StrongFocus);
    buildLoginForm();
    setScreen(initialScreen);
}

void StartupTemplateWindow::buildLoginForm()
{
    loginForm_ = new QFrame(this);
    loginForm_->setObjectName(QStringLiteral("loginForm"));

    usernameEdit_ = new QLineEdit(loginForm_);
    usernameEdit_->setObjectName(QStringLiteral("usernameEdit"));
    usernameEdit_->setProperty("uiRole", QStringLiteral("loginField"));
    usernameEdit_->setPlaceholderText(QStringLiteral("Username"));
    usernameEdit_->setAccessibleName(QStringLiteral("账号"));
    usernameEdit_->addAction(makeUserIcon(), QLineEdit::LeadingPosition);

    passwordEdit_ = new QLineEdit(loginForm_);
    passwordEdit_->setObjectName(QStringLiteral("passwordEdit"));
    passwordEdit_->setProperty("uiRole", QStringLiteral("loginField"));
    passwordEdit_->setPlaceholderText(QStringLiteral("Password"));
    passwordEdit_->setAccessibleName(QStringLiteral("密码"));
    passwordEdit_->setEchoMode(QLineEdit::Password);
    passwordEdit_->addAction(makeLockIcon(), QLineEdit::LeadingPosition);

    loginButton_ = new QPushButton(QStringLiteral("Login"), loginForm_);
    loginButton_->setObjectName(QStringLiteral("loginButton"));
    loginButton_->setAccessibleName(QStringLiteral("登录"));
    loginButton_->setDefault(true);
    loginButton_->setAutoDefault(true);

    loginMessage_ = new QLabel(loginForm_);
    loginMessage_->setObjectName(QStringLiteral("loginMessage"));
    loginMessage_->setProperty("severity", QStringLiteral("info"));
    loginMessage_->setAlignment(Qt::AlignCenter);
    loginMessage_->setWordWrap(true);
    loginMessage_->setAccessibleName(QStringLiteral("登录状态"));

    auto *layout = new QVBoxLayout(loginForm_);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(usernameEdit_);
    layout->addSpacing(18);
    layout->addWidget(passwordEdit_);
    layout->addSpacing(34);
    layout->addWidget(loginButton_);
    layout->addSpacing(8);
    layout->addWidget(loginMessage_);

    setTabOrder(usernameEdit_, passwordEdit_);
    setTabOrder(passwordEdit_, loginButton_);

    connect(loginButton_, &QPushButton::clicked, this, &StartupTemplateWindow::submitLogin);
    connect(passwordEdit_, &QLineEdit::returnPressed, this, &StartupTemplateWindow::submitLogin);
}

void StartupTemplateWindow::setScreen(Screen screen)
{
    screen_ = screen;
    loginForm_->setVisible(screen_ == Screen::Login);
    updateWindowForScreen();
    updateLoginFormGeometry();
    update();
    if (screen_ == Screen::Login) {
        usernameEdit_->setFocus(Qt::OtherFocusReason);
    } else {
        setFocus(Qt::OtherFocusReason);
    }
}

void StartupTemplateWindow::setLoginMessage(const QString &message, bool isError)
{
    loginMessage_->setText(message);
    loginMessage_->setProperty("severity", isError ? QStringLiteral("error") : QStringLiteral("info"));
    loginMessage_->style()->unpolish(loginMessage_);
    loginMessage_->style()->polish(loginMessage_);
}

void StartupTemplateWindow::setLoginBusy(bool busy)
{
    usernameEdit_->setEnabled(!busy);
    passwordEdit_->setEnabled(!busy);
    loginButton_->setEnabled(!busy);
    loginButton_->setText(busy ? QStringLiteral("Signing in…") : QStringLiteral("Login"));
}

void StartupTemplateWindow::clearPassword(bool restoreFocus)
{
    passwordEdit_->clear();
    if (restoreFocus) {
        passwordEdit_->setFocus(Qt::OtherFocusReason);
    }
}

bool StartupTemplateWindow::saveCapture(const QString &path)
{
    const QFileInfo fileInfo(path);
    if (!QDir().mkpath(fileInfo.absolutePath())) {
        return false;
    }
    return grab().save(fileInfo.absoluteFilePath(), "PNG");
}

void StartupTemplateWindow::submitLogin()
{
    if (usernameEdit_->text().trimmed().isEmpty() || passwordEdit_->text().isEmpty()) {
        setLoginMessage(QStringLiteral("请输入账号和密码。"), true);
        (usernameEdit_->text().trimmed().isEmpty() ? usernameEdit_ : passwordEdit_)->setFocus();
        return;
    }

    setLoginMessage(QStringLiteral("正在提交登录请求…"), false);
    emit loginSubmitted(usernameEdit_->text().trimmed(), passwordEdit_->text());
}

void StartupTemplateWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_F2) {
        setScreen(screen_ == Screen::Splash ? Screen::Login : Screen::Splash);
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Escape) {
        close();
        event->accept();
        return;
    }
    QWidget::keyPressEvent(event);
}

QRect StartupTemplateWindow::fittedImageRect() const
{
    const QSize referenceSize = screen_ == Screen::Splash ? kSplashReferenceSize : kLoginReferenceSize;
    QSize fitted = referenceSize;
    fitted.scale(size(), Qt::KeepAspectRatio);
    return QRect(
        (width() - fitted.width()) / 2,
        (height() - fitted.height()) / 2,
        fitted.width(),
        fitted.height());
}

void StartupTemplateWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.fillRect(rect(), screen_ == Screen::Splash ? QColor(QStringLiteral("#F4FAFF")) : QColor(QStringLiteral("#DCEFFC")));

    const QPixmap pixmap = referencePixmap(screen_);
    if (!pixmap.isNull()) {
        painter.drawPixmap(fittedImageRect(), pixmap, pixmap.rect());
    }
}

void StartupTemplateWindow::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateLoginFormGeometry();
}

void StartupTemplateWindow::updateLoginFormGeometry()
{
    if (screen_ != Screen::Login || !loginForm_) {
        return;
    }

    const QRect imageRect = fittedImageRect();
    const qreal sx = imageRect.width() / static_cast<qreal>(kLoginReferenceSize.width());
    const qreal sy = imageRect.height() / static_cast<qreal>(kLoginReferenceSize.height());

    // ESTIMATED from Ref S-02. Coordinates are tied to the fixed visual composition,
    // while the controls inside this region still use a semantic QVBoxLayout.
    const QRectF referenceFormRect(1080.0, 500.0, 474.0, 300.0);
    const QRect formRect(
        imageRect.left() + qRound(referenceFormRect.left() * sx),
        imageRect.top() + qRound(referenceFormRect.top() * sy),
        qRound(referenceFormRect.width() * sx),
        qRound(referenceFormRect.height() * sy));
    loginForm_->setGeometry(formRect);

    const int fieldHeight = qMax(42, qRound(66.0 * sy));
    const int buttonHeight = qMax(42, qRound(61.0 * sy));
    usernameEdit_->setFixedHeight(fieldHeight);
    passwordEdit_->setFixedHeight(fieldHeight);
    loginButton_->setFixedHeight(buttonHeight);
}

void StartupTemplateWindow::updateWindowForScreen()
{
    if (!isVisible()) {
        resize(screen_ == Screen::Splash ? QSize(1500, 750) : QSize(1504, 847));
    }
}
