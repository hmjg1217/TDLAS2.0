#pragma once

#include <QWidget>

#include <array>

class QButtonGroup;
class QFrame;
class QKeyEvent;
class QLabel;
class QPushButton;
class QResizeEvent;
class QStackedWidget;

namespace clientui {

class ClientTemplateWindow final : public QWidget
{
public:
    enum class Screen {
        Main,
        Parameters,
        Camera,
        CameraMotion
    };

    explicit ClientTemplateWindow(Screen initialScreen, QWidget *parent = nullptr);

    void setScreen(Screen screen);
    [[nodiscard]] bool saveCapture(const QString &path);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void buildShell();
    QWidget *buildTopBar();
    QWidget *buildSideRail();
    QWidget *buildTabBar();
    QWidget *buildMainPage();
    QWidget *buildCameraPage();
    void buildParameterDrawer();
    void buildMovementPanel();
    void updateOverlays();
    void selectPageButton(int index);
    void showTemplateNotice(const QString &text);

    Screen screen_ = Screen::Main;
    QStackedWidget *pages_ = nullptr;
    QButtonGroup *tabGroup_ = nullptr;
    QFrame *parameterDrawer_ = nullptr;
    QFrame *movementPanel_ = nullptr;
    QWidget *cameraPage_ = nullptr;
    QLabel *templateNotice_ = nullptr;
    QPushButton *homeRail_ = nullptr;
    QPushButton *parameterRail_ = nullptr;
    std::array<QPushButton *, 2> tabButtons_{};
};

} // namespace clientui
