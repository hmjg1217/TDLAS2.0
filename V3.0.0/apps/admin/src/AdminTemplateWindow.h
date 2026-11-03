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
class QTableWidget;

namespace adminui {

class AdminTemplateWindow final : public QWidget
{
    Q_OBJECT

public:
    enum class Screen {
        Main,
        Parameters,
        Debug,
        PeakData,
        Camera,
        CameraMotion
    };

    explicit AdminTemplateWindow(Screen initialScreen, QWidget *parent = nullptr);

    void setScreen(Screen screen);
    [[nodiscard]] bool saveCapture(const QString &path);

signals:
    void actionRequested(const QString &actionId);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void buildShell();
    QWidget *buildTopBar();
    QWidget *buildSideRail();
    QWidget *buildTabBar();
    QWidget *buildMainPage();
    QWidget *buildDebugPage();
    QWidget *buildPeakPage();
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
    std::array<QPushButton *, 4> tabButtons_{};
};

} // namespace adminui
