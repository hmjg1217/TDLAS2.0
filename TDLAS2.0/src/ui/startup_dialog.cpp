#include "ui/startup_dialog.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QEvent>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QResizeEvent>
#include <QScreen>
#include <QSettings>
#include <QSizePolicy>
#include <QStyle>
#include <QVBoxLayout>
#include <QWindow>

#include <algorithm>

static void initializeUiResources()
{
    Q_INIT_RESOURCE(resources);
}

namespace {

constexpr qreal kReferenceWidth = 1672.0;
constexpr qreal kReferenceHeight = 941.0;

QPixmap makeUserPixmap(const QColor& color, bool group)
{
    QPixmap pixmap(28, 28);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(color, 2.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(Qt::NoBrush);

    if (group) {
        painter.drawEllipse(QRectF(5.0, 5.0, 7.0, 7.0));
        painter.drawEllipse(QRectF(16.0, 5.0, 7.0, 7.0));
        painter.drawArc(QRectF(1.5, 13.0, 14.0, 12.0), 8 * 16, 164 * 16);
        painter.drawArc(QRectF(12.5, 13.0, 14.0, 12.0), 8 * 16, 164 * 16);
    } else {
        painter.drawEllipse(QRectF(9.0, 4.0, 10.0, 10.0));
        painter.drawArc(QRectF(4.0, 13.0, 20.0, 13.0), 8 * 16, 164 * 16);
    }
    return pixmap;
}

QIcon makeRoleIcon(bool group)
{
    QIcon icon;
    icon.addPixmap(makeUserPixmap(QColor(QStringLiteral("#49658f")), group),
                   QIcon::Normal,
                   QIcon::Off);
    icon.addPixmap(makeUserPixmap(Qt::white, group), QIcon::Normal, QIcon::On);
    icon.addPixmap(makeUserPixmap(QColor(QStringLiteral("#49658f")), group),
                   QIcon::Active,
                   QIcon::Off);
    icon.addPixmap(makeUserPixmap(Qt::white, group), QIcon::Active, QIcon::On);
    return icon;
}

QPixmap makeLockPixmap(const QColor& color)
{
    QPixmap pixmap(28, 28);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(color, 2.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(QRectF(6.0, 11.0, 16.0, 13.0), 2.0, 2.0);
    painter.drawArc(QRectF(8.0, 3.0, 12.0, 16.0), 0, 180 * 16);
    return pixmap;
}

QPixmap makeEyePixmap(const QColor& color, bool hidden)
{
    QPixmap pixmap(28, 28);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(color, 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(Qt::NoBrush);

    QPainterPath eye;
    eye.moveTo(3.0, 14.0);
    eye.cubicTo(8.0, 7.0, 20.0, 7.0, 25.0, 14.0);
    eye.cubicTo(20.0, 21.0, 8.0, 21.0, 3.0, 14.0);
    painter.drawPath(eye);
    painter.drawEllipse(QRectF(11.0, 11.0, 6.0, 6.0));
    if (hidden) {
        painter.drawLine(QPointF(5.0, 5.0), QPointF(23.0, 23.0));
    }
    return pixmap;
}

QRect scaledRect(const QWidget* widget, int x, int y, int width, int height)
{
    const qreal sx = widget->width() / kReferenceWidth;
    const qreal sy = widget->height() / kReferenceHeight;
    return QRect(qRound(x * sx),
                 qRound(y * sy),
                 qRound(width * sx),
                 qRound(height * sy));
}

} // namespace

namespace tdlas {

LoginDialog::LoginDialog(QWidget* parent)
    : QDialog(parent)
{
    initializeUiResources();
    logoIcon_ = QIcon(QStringLiteral(":/icons/tdlas_hydrogen.ico"));
    if (logoIcon_.isNull()) {
        logoIcon_ = QIcon(QStringLiteral(":/icons/tdlas_hydrogen.png"));
    }
    companyLogoPixmap_.load(QStringLiteral(":/brand/djquantum_company_logo.jpg"));
    // The canonical login artwork is the user's current sample image.  Keep a
    // single resource name so an obsolete generated background cannot silently
    // win because of load order.
    loginBackgroundPixmap_.load(QStringLiteral(":/ui/login_screen.png"));

    setObjectName(QStringLiteral("loginDialog"));
    setWindowTitle(QStringLiteral("TDLAS 测氢系统 · 登录"));
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setMinimumSize(960, 540);
    resize(1280, 720);
    setAccessibleName(QStringLiteral("TDLAS 测氢系统登录"));
    setAccessibleDescription(QStringLiteral("本地界面会话登录，可选择管理员或客户工作台"));

    background_ = new QLabel(this);
    background_->setObjectName(QStringLiteral("loginBackground"));
    background_->setPixmap(loginBackgroundPixmap_);
    background_->setScaledContents(true);
    background_->setAttribute(Qt::WA_TransparentForMouseEvents);
    background_->lower();

    titleBar_ = new QWidget(this);
    titleBar_->setObjectName(QStringLiteral("loginTitleBar"));
    titleBar_->installEventFilter(this);

    minimize_ = new QPushButton(titleBar_);
    minimize_->setObjectName(QStringLiteral("minimizeLogin"));
    minimize_->setToolTip(QStringLiteral("最小化"));
    minimize_->setAccessibleName(QStringLiteral("最小化窗口"));

    maximize_ = new QPushButton(titleBar_);
    maximize_->setObjectName(QStringLiteral("maximizeLogin"));
    maximize_->setToolTip(QStringLiteral("还原或最大化"));
    maximize_->setAccessibleName(QStringLiteral("还原或最大化窗口"));

    close_ = new QPushButton(titleBar_);
    close_->setObjectName(QStringLiteral("closeLogin"));
    close_->setToolTip(QStringLiteral("关闭"));
    close_->setAccessibleName(QStringLiteral("关闭登录窗口"));

    for (QPushButton* button : {minimize_, maximize_, close_}) {
        button->setCursor(Qt::PointingHandCursor);
    }

    // Compatibility objects keep the native fixture and accessibility hooks stable.
    // The uploaded artwork owns their visible rendering.
    companyLogo_ = new QLabel(this);
    companyLogo_->setObjectName(QStringLiteral("companyLogo"));
    companyLogo_->setAccessibleName(QStringLiteral("道纪量子公司标志"));
    companyLogo_->setPixmap(companyLogoPixmap_);
    companyLogo_->hide();

    logo_ = new QLabel(this);
    logo_->setObjectName(QStringLiteral("loginLogo"));
    logo_->setAccessibleName(QStringLiteral("TDLAS 测氢系统标志"));
    logo_->setPixmap(logoIcon_.pixmap(QSize(64, 64)));
    logo_->hide();

    auto* accountLabel = new QLabel(QStringLiteral("账号"), this);
    accountLabel->setObjectName(QStringLiteral("accountLabel"));
    accountLabel->hide();

    account_ = new QLineEdit(this);
    account_->setObjectName(QStringLiteral("loginAccount"));
    account_->setPlaceholderText(QStringLiteral("账号"));
    account_->setAccessibleName(QStringLiteral("账号"));
    account_->setAccessibleDescription(QStringLiteral("必填，本地界面会话账户"));
    account_->setClearButtonEnabled(false);
    account_->setMaxLength(128);

    accountIcon_ = new QLabel(account_);
    accountIcon_->setObjectName(QStringLiteral("accountIcon"));
    accountIcon_->setPixmap(makeUserPixmap(QColor(QStringLiteral("#586c8d")), false));
    accountIcon_->setAlignment(Qt::AlignCenter);
    accountIcon_->setAttribute(Qt::WA_TransparentForMouseEvents);
    auto* accountChevron = new QLabel(QStringLiteral("⌄"), account_);
    accountChevron->setObjectName(QStringLiteral("accountChevron"));
    accountChevron->setAlignment(Qt::AlignCenter);
    accountChevron->setAttribute(Qt::WA_TransparentForMouseEvents);

    auto* passwordLabel = new QLabel(QStringLiteral("密码"), this);
    passwordLabel->setObjectName(QStringLiteral("passwordLabel"));
    passwordLabel->hide();

    password_ = new QLineEdit(this);
    password_->setObjectName(QStringLiteral("loginPassword"));
    password_->setPlaceholderText(QStringLiteral("密码"));
    password_->setAccessibleName(QStringLiteral("密码"));
    password_->setAccessibleDescription(QStringLiteral("必填，允许粘贴"));
    password_->setEchoMode(QLineEdit::Password);
    password_->setMaxLength(128);

    passwordIcon_ = new QLabel(password_);
    passwordIcon_->setObjectName(QStringLiteral("passwordIcon"));
    passwordIcon_->setPixmap(makeLockPixmap(QColor(QStringLiteral("#586c8d"))));
    passwordIcon_->setAlignment(Qt::AlignCenter);
    passwordIcon_->setAttribute(Qt::WA_TransparentForMouseEvents);

    passwordVisibility_ = new QPushButton(password_);
    passwordVisibility_->setObjectName(QStringLiteral("passwordVisibility"));
    passwordVisibility_->setToolTip(QStringLiteral("显示密码"));
    passwordVisibility_->setAccessibleName(QStringLiteral("显示密码"));
    passwordVisibility_->setCursor(Qt::PointingHandCursor);
    passwordVisibility_->setCheckable(true);
    passwordVisibility_->setIcon(
        QIcon(makeEyePixmap(QColor(QStringLiteral("#586c8d")), true)));

    auto* roleLabel = new QLabel(QStringLiteral("用户角色"), this);
    roleLabel->setObjectName(QStringLiteral("roleLabel"));
    roleLabel->hide();

    role_ = new QComboBox(this);
    role_->setObjectName(QStringLiteral("loginRole"));
    role_->addItem(QStringLiteral("管理员"),
                   static_cast<int>(ApplicationRole::Administrator));
    role_->addItem(QStringLiteral("客户"),
                   static_cast<int>(ApplicationRole::Customer));
    role_->setAccessibleName(QStringLiteral("用户角色"));
    role_->hide();

    roleSelector_ = new QWidget(this);
    roleSelector_->setObjectName(QStringLiteral("loginRoleSelector"));
    auto* roleLayout = new QHBoxLayout(roleSelector_);
    roleLayout->setContentsMargins(3, 3, 3, 3);
    roleLayout->setSpacing(0);
    adminRole_ = new QPushButton(QStringLiteral("管理员"), roleSelector_);
    adminRole_->setObjectName(QStringLiteral("adminRoleButton"));
    adminRole_->setAccessibleName(QStringLiteral("管理员角色"));
    adminRole_->setCheckable(true);
    adminRole_->setCursor(Qt::PointingHandCursor);
    adminRole_->setIcon(makeRoleIcon(false));
    adminRole_->setIconSize(QSize(24, 24));
    customerRole_ = new QPushButton(QStringLiteral("客户"), roleSelector_);
    customerRole_->setObjectName(QStringLiteral("customerRoleButton"));
    customerRole_->setAccessibleName(QStringLiteral("客户角色"));
    customerRole_->setCheckable(true);
    customerRole_->setCursor(Qt::PointingHandCursor);
    customerRole_->setIcon(makeRoleIcon(true));
    customerRole_->setIconSize(QSize(24, 24));
    roleLayout->addWidget(adminRole_);
    roleLayout->addWidget(customerRole_);
    auto* roleGroup = new QButtonGroup(this);
    roleGroup->setExclusive(true);
    roleGroup->addButton(adminRole_, 0);
    roleGroup->addButton(customerRole_, 1);

    validation_ = new QLabel(this);
    validation_->setObjectName(QStringLiteral("loginValidation"));
    validation_->setAccessibleName(QStringLiteral("登录错误"));
    validation_->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    validation_->setWordWrap(false);

    remember_ = new QCheckBox(QStringLiteral("记住账号"), this);
    remember_->setObjectName(QStringLiteral("rememberAccount"));
    remember_->setChecked(true);
    remember_->setCursor(Qt::PointingHandCursor);
    remember_->setAccessibleDescription(QStringLiteral("仅在本机保存账号，不保存密码"));

    // The artwork already contains the visible link; this transparent button makes it interactive.
    forgotPassword_ = new QPushButton(this);
    forgotPassword_->setObjectName(QStringLiteral("forgotPassword"));
    forgotPassword_->setAccessibleName(QStringLiteral("忘记密码"));
    forgotPassword_->setCursor(Qt::PointingHandCursor);

    exit_ = new QPushButton(this);
    exit_->setObjectName(QStringLiteral("exitLogin"));
    exit_->hide();

    login_ = new QPushButton(QStringLiteral("登录   →"), this);
    login_->setObjectName(QStringLiteral("loginButton"));
    login_->setToolTip(QStringLiteral("登录"));
    login_->setAccessibleName(QStringLiteral("登录"));
    login_->setCursor(Qt::PointingHandCursor);
    login_->setDefault(true);

    auto* localSessionNote = new QLabel(
        QStringLiteral("当前仅建立本地界面会话，不连接认证服务。"), this);
    localSessionNote->setObjectName(QStringLiteral("localSessionNote"));
    localSessionNote->hide();

    setStyleSheet(QStringLiteral(R"(
        QDialog#loginDialog {
            background: #eaf5ff;
            color: #0c255e;
            font-family: "Microsoft YaHei UI", "Microsoft YaHei";
            font-size: 16px;
        }
        QLabel#loginBackground, QWidget#loginTitleBar {
            background: transparent;
        }
        QPushButton#minimizeLogin, QPushButton#maximizeLogin, QPushButton#closeLogin {
            border: none;
            background: transparent;
            border-radius: 6px;
        }
        QPushButton#minimizeLogin:hover, QPushButton#maximizeLogin:hover {
            background: rgba(203, 228, 250, 150);
        }
        QPushButton#closeLogin:hover {
            background: rgba(207, 55, 67, 190);
        }
        QPushButton#minimizeLogin:focus, QPushButton#maximizeLogin:focus,
        QPushButton#closeLogin:focus {
            border: 2px solid #1d79eb;
        }
        QLineEdit#loginAccount, QLineEdit#loginPassword {
            border: 1px solid #b8c9e2;
            border-radius: 11px;
            background: rgba(249, 252, 255, 248);
            color: #142d63;
            padding: 0 54px;
            font-size: 18px;
            selection-background-color: #137df2;
            selection-color: white;
        }
        QLineEdit#loginAccount:hover, QLineEdit#loginPassword:hover {
            border-color: #70a9ec;
        }
        QLineEdit#loginAccount:focus, QLineEdit#loginPassword:focus {
            border: 2px solid #147df5;
            padding: 0 53px;
        }
        QLineEdit[invalid="true"] {
            border: 2px solid #c62939;
            padding: 0 53px;
        }
        QLabel#accountIcon, QLabel#passwordIcon, QLabel#accountChevron {
            color: #53698d;
            background: transparent;
            font-size: 23px;
        }
        QLabel#loginValidation {
            color: #b42332;
            background: transparent;
            font-size: 12px;
            padding-left: 4px;
        }
        QPushButton#passwordVisibility {
            border: none;
            border-radius: 8px;
            background: transparent;
            padding: 0;
        }
        QPushButton#passwordVisibility:hover, QPushButton#passwordVisibility:focus {
            background: #e6f2ff;
        }
        QWidget#loginRoleSelector {
            background: rgba(239, 246, 255, 248);
            border: 1px solid #c4d2e7;
            border-radius: 12px;
        }
        QPushButton#adminRoleButton, QPushButton#customerRoleButton {
            border: none;
            background: transparent;
            color: #48648e;
            font-size: 18px;
            font-weight: 600;
            padding: 0 18px;
        }
        QPushButton#adminRoleButton {
            border-radius: 9px;
        }
        QPushButton#customerRoleButton {
            border-radius: 9px;
        }
        QPushButton#adminRoleButton:checked, QPushButton#customerRoleButton:checked {
            color: white;
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                                        stop:0 #0967ef, stop:1 #27baf2);
        }
        QPushButton#adminRoleButton:focus, QPushButton#customerRoleButton:focus {
            border: 2px solid #73b6ff;
        }
        QCheckBox#rememberAccount {
            color: #264f89;
            spacing: 9px;
            font-size: 16px;
            background: #f3f9ff;
            padding: 0 8px 0 0;
        }
        QPushButton#forgotPassword {
            border: none;
            background: transparent;
        }
        QPushButton#forgotPassword:hover, QPushButton#forgotPassword:focus {
            border: 1px solid rgba(20, 125, 245, 110);
            border-radius: 6px;
        }
        QPushButton#loginButton {
            border: 1px solid #47c5f2;
            border-radius: 18px;
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                                        stop:0 #0a67f2, stop:0.55 #077df1,
                                        stop:1 #2dbcf2);
            color: white;
            font-size: 22px;
            font-weight: 600;
        }
        QPushButton#loginButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                                        stop:0 #075bdb, stop:0.55 #0872df,
                                        stop:1 #20a9dd);
        }
        QPushButton#loginButton:pressed {
            background: #075fc9;
        }
        QPushButton#loginButton:focus {
            border: 3px solid #84c8ff;
        }
    )"));

    QSettings settings(QStringLiteral("DJQUANTUM"), QStringLiteral("TDLAS2"));
    const QString rememberedAccount = QCoreApplication::applicationName()
            == QStringLiteral("TDLAS2")
        ? settings.value(QStringLiteral("login/rememberedAccount")).toString()
        : QString{};
    if (!rememberedAccount.isEmpty()) {
        account_->setText(rememberedAccount);
        password_->setFocus();
    } else {
        account_->setFocus();
    }

    connect(login_, &QPushButton::clicked, this, &LoginDialog::attemptLogin);
    connect(account_, &QLineEdit::returnPressed, this, &LoginDialog::attemptLogin);
    connect(password_, &QLineEdit::returnPressed, this, &LoginDialog::attemptLogin);
    connect(passwordVisibility_,
            &QPushButton::clicked,
            this,
            &LoginDialog::togglePasswordVisibility);
    connect(exit_, &QPushButton::clicked, this, &QDialog::reject);
    connect(minimize_, &QPushButton::clicked, this, &QWidget::showMinimized);
    connect(maximize_, &QPushButton::clicked, this, &LoginDialog::toggleMaximized);
    connect(close_, &QPushButton::clicked, this, &QDialog::reject);
    connect(adminRole_, &QPushButton::clicked, this, [this] {
        role_->setCurrentIndex(0);
    });
    connect(customerRole_, &QPushButton::clicked, this, [this] {
        role_->setCurrentIndex(1);
    });
    connect(role_,
            qOverload<int>(&QComboBox::currentIndexChanged),
            this,
            [this](int) { updateRoleSelection(); });
    connect(forgotPassword_, &QPushButton::clicked, this, [this] {
        validation_->setText(
            QStringLiteral("当前为本地界面会话，尚未接入密码找回服务。"));
    });
    connect(account_, &QLineEdit::textChanged, this, [this] {
        validation_->clear();
        setFieldInvalid(account_, false);
    });
    connect(password_, &QLineEdit::textChanged, this, [this] {
        validation_->clear();
        setFieldInvalid(password_, false);
    });

    setTabOrder(account_, password_);
    setTabOrder(password_, passwordVisibility_);
    setTabOrder(passwordVisibility_, adminRole_);
    setTabOrder(adminRole_, customerRole_);
    setTabOrder(customerRole_, remember_);
    setTabOrder(remember_, forgotPassword_);
    setTabOrder(forgotPassword_, login_);
    setTabOrder(login_, exit_);
    setTabOrder(exit_, minimize_);
    setTabOrder(minimize_, maximize_);
    setTabOrder(maximize_, close_);

    updateRoleSelection();
    updateResponsiveLayout();
}

ApplicationRole LoginDialog::role() const
{
    return static_cast<ApplicationRole>(role_->currentData().toInt());
}

QString LoginDialog::accountName() const
{
    return account_->text().trimmed();
}

bool LoginDialog::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == titleBar_) {
        if (event->type() == QEvent::MouseButtonPress) {
            auto* mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::LeftButton
                && !isMaximized()
                && windowHandle() != nullptr) {
                windowHandle()->startSystemMove();
                return true;
            }
        } else if (event->type() == QEvent::MouseButtonDblClick) {
            auto* mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::LeftButton) {
                toggleMaximized();
                return true;
            }
        }
    }
    return QDialog::eventFilter(watched, event);
}

void LoginDialog::resizeEvent(QResizeEvent* event)
{
    QDialog::resizeEvent(event);
    updateResponsiveLayout();
}

void LoginDialog::updateResponsiveLayout()
{
    if (background_ == nullptr) {
        return;
    }
    background_->setGeometry(rect());
    background_->lower();

    titleBar_->setGeometry(scaledRect(this, 14, 17, 1644, 70));
    minimize_->setGeometry(scaledRect(this, 1452, 16, 64, 58).translated(-titleBar_->x(),
                                                                        -titleBar_->y()));
    maximize_->setGeometry(scaledRect(this, 1515, 16, 64, 58).translated(-titleBar_->x(),
                                                                        -titleBar_->y()));
    close_->setGeometry(scaledRect(this, 1578, 16, 64, 58).translated(-titleBar_->x(),
                                                                     -titleBar_->y()));

    account_->setGeometry(scaledRect(this, 1115, 325, 455, 64));
    password_->setGeometry(scaledRect(this, 1115, 406, 455, 64));
    roleSelector_->setGeometry(scaledRect(this, 1115, 529, 455, 62));
    validation_->setGeometry(scaledRect(this, 1115, 473, 455, 26));
    remember_->setGeometry(scaledRect(this, 1115, 600, 210, 50));
    forgotPassword_->setGeometry(scaledRect(this, 1420, 600, 150, 50));
    login_->setGeometry(scaledRect(this, 1115, 673, 455, 67));

    const int accountHeight = account_->height();
    accountIcon_->setGeometry(15, 0, 40, accountHeight);
    if (auto* chevron = account_->findChild<QLabel*>(QStringLiteral("accountChevron"))) {
        chevron->setGeometry(account_->width() - 52, 0, 38, accountHeight);
    }
    const int passwordHeight = password_->height();
    passwordIcon_->setGeometry(15, 0, 40, passwordHeight);
    passwordVisibility_->setGeometry(password_->width() - 58,
                                     qMax(4, (passwordHeight - 46) / 2),
                                     46,
                                     qMin(46, passwordHeight - 8));

    titleBar_->raise();
    account_->raise();
    password_->raise();
    roleSelector_->raise();
    validation_->raise();
    remember_->raise();
    forgotPassword_->raise();
    login_->raise();
}

void LoginDialog::updateRoleSelection()
{
    const bool administrator = role_->currentIndex() != 1;
    adminRole_->setChecked(administrator);
    customerRole_->setChecked(!administrator);
}

void LoginDialog::setFieldInvalid(QLineEdit* field, bool invalid)
{
    field->setProperty("invalid", invalid);
    field->style()->unpolish(field);
    field->style()->polish(field);
    field->update();
}

void LoginDialog::attemptLogin()
{
    const QString account = account_->text().trimmed();
    setFieldInvalid(account_, account.isEmpty());
    setFieldInvalid(password_, password_->text().isEmpty());
    if (account.isEmpty()) {
        validation_->setText(QStringLiteral("请输入账户。"));
        account_->setFocus();
        return;
    }
    if (password_->text().isEmpty()) {
        validation_->setText(QStringLiteral("请输入密码。"));
        password_->setFocus();
        return;
    }

    if (QCoreApplication::applicationName() == QStringLiteral("TDLAS2")) {
        QSettings settings(QStringLiteral("DJQUANTUM"), QStringLiteral("TDLAS2"));
        if (remember_->isChecked()) {
            settings.setValue(QStringLiteral("login/rememberedAccount"), account);
        } else {
            settings.remove(QStringLiteral("login/rememberedAccount"));
        }
    }
    validation_->clear();
    accept();
}

void LoginDialog::togglePasswordVisibility()
{
    const bool currentlyVisible = password_->echoMode() == QLineEdit::Normal;
    const bool makeVisible = !currentlyVisible;
    password_->setEchoMode(makeVisible ? QLineEdit::Normal : QLineEdit::Password);
    passwordVisibility_->setToolTip(makeVisible ? QStringLiteral("隐藏密码")
                                                : QStringLiteral("显示密码"));
    passwordVisibility_->setAccessibleName(passwordVisibility_->toolTip());
    passwordVisibility_->setChecked(makeVisible);
    passwordVisibility_->setIcon(QIcon(makeEyePixmap(
        QColor(QStringLiteral("#586c8d")), !makeVisible)));
    password_->setFocus();
}

void LoginDialog::toggleMaximized()
{
    if (isMaximized()) {
        showNormal();
        resize(1280, 720);
        if (const QScreen* targetScreen = screen()) {
            move(targetScreen->availableGeometry().center() - rect().center());
        }
    } else {
        showMaximized();
    }
}

StartupSplash::StartupSplash(QWidget* parent)
    : QWidget(parent, Qt::SplashScreen | Qt::FramelessWindowHint)
{
    initializeUiResources();
    background_.load(QStringLiteral(":/ui/startup_splash.png"));
    if (const QScreen* targetScreen = QGuiApplication::primaryScreen()) {
        const QSize maximumSize = targetScreen->availableGeometry().size() * 0.95;
        if (background_.width() > maximumSize.width()
            || background_.height() > maximumSize.height()) {
            background_ = background_.scaled(
                maximumSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }
    }
    setObjectName(QStringLiteral("startupSplash"));
    setFixedSize(background_.isNull() ? QSize(1000, 520) : background_.size());
    setAttribute(Qt::WA_TranslucentBackground);
}

void StartupSplash::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    if (background_.isNull()) {
        painter.fillRect(rect(), QColor(QStringLiteral("#f2f9ff")));
        return;
    }
    painter.drawPixmap(rect(), background_, background_.rect());
}

} // namespace tdlas
