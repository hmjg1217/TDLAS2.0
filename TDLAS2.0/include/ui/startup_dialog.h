#pragma once

#include "ui/application_role.h"

#include <QDialog>
#include <QIcon>
#include <QPixmap>

class QCheckBox;
class QComboBox;
class QEvent;
class QLabel;
class QLineEdit;
class QPaintEvent;
class QPushButton;
class QResizeEvent;
class QWidget;

namespace tdlas {

class LoginDialog final : public QDialog {
public:
    explicit LoginDialog(QWidget* parent = nullptr);
    ApplicationRole role() const;
    QString accountName() const;

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    void attemptLogin();
    void togglePasswordVisibility();
    void toggleMaximized();
    void updateResponsiveLayout();
    void updateRoleSelection();
    void setFieldInvalid(QLineEdit* field, bool invalid);

    QIcon logoIcon_;
    QPixmap companyLogoPixmap_;
    QPixmap loginBackgroundPixmap_;
    QLabel* background_ = nullptr;
    QWidget* titleBar_ = nullptr;
    QLabel* companyLogo_ = nullptr;
    QLabel* logo_ = nullptr;
    QLabel* accountIcon_ = nullptr;
    QLabel* passwordIcon_ = nullptr;
    QWidget* roleSelector_ = nullptr;
    QLineEdit* account_ = nullptr;
    QLineEdit* password_ = nullptr;
    QComboBox* role_ = nullptr;
    QLabel* validation_ = nullptr;
    QCheckBox* remember_ = nullptr;
    QPushButton* passwordVisibility_ = nullptr;
    QPushButton* adminRole_ = nullptr;
    QPushButton* customerRole_ = nullptr;
    QPushButton* forgotPassword_ = nullptr;
    QPushButton* login_ = nullptr;
    QPushButton* exit_ = nullptr;
    QPushButton* minimize_ = nullptr;
    QPushButton* maximize_ = nullptr;
    QPushButton* close_ = nullptr;
};

class StartupSplash final : public QWidget {
public:
    explicit StartupSplash(QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QPixmap background_;
};

}
