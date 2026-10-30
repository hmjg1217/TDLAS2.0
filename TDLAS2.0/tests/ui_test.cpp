#include "ui/main_window.h"
#include "ui/startup_dialog.h"
#include "ui/interactive_chart_view.h"
#include <QtTest>
#include <QComboBox>
#include <QDir>
#include <QLabel>
#include <QLineEdit>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QTabWidget>
#include <QChartView>
#include <QLineSeries>
#include <QValueAxis>
#include <QTableWidget>
#include <QDoubleSpinBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QSpinBox>
#include <QTabBar>
#include <QLegendMarker>
#include <QSettings>
#include <QMessageBox>
#include <QSignalBlocker>
#include <QElapsedTimer>
#include <QColor>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <thread>
#include <tuple>
#include "result_fixture.h"
using namespace tdlas;
namespace {
struct ConfirmationResult { bool shown = false, defaultsToNo = false; };
struct AtomicReleaseGuard {
    std::atomic_bool& value;
    ~AtomicReleaseGuard() { value = true; }
};
ConfirmationResult answerConfirmation(QPushButton* action, QMessageBox::StandardButton answer) {
    ConfirmationResult result; QTimer responder; responder.setInterval(10);
    QObject::connect(&responder, &QTimer::timeout, [&] {
        auto* box = qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
        if (!box) return;
        result.shown = true; result.defaultsToNo = box->defaultButton() == box->button(QMessageBox::No);
        box->button(answer)->click();
    });
    responder.start(); action->click(); return result;
}
double relativeLuminance(const QColor& color) {
    const auto linear = [](double value) {
        return value <= 0.04045 ? value / 12.92 : std::pow((value + 0.055) / 1.055, 2.4);
    };
    return 0.2126 * linear(color.redF()) + 0.7152 * linear(color.greenF()) + 0.0722 * linear(color.blueF());
}
double contrastRatio(const QColor& first, const QColor& second) {
    const double a = relativeLuminance(first), b = relativeLuminance(second);
    return (std::max(a, b) + 0.05) / (std::min(a, b) + 0.05);
}
}
class UiTest : public QObject {
    Q_OBJECT
private slots:
    void asynchronousDaqDiscoveryAndCapabilities() {
        FakeDaqOptions options; options.discoveryDelayMs = 250;
        auto observation = std::make_shared<FakeDaqObservation>();
        MainWindow w(qEnvironmentVariable("TDLAS_TEST_ARTIFACTS", "artifacts") + "/step8-discovery", nullptr,
                     ApplicationRole::Administrator, QStringLiteral("test"), makeFakeDaqBackend(options, observation));
        w.show();
        auto* refresh = w.findChild<QPushButton*>("refreshDevicesButton");
        auto* devices = w.findChild<QComboBox*>("deviceSelector");
        QVERIFY(refresh); QVERIFY(devices); QCOMPARE(devices->count(), 2);
        QElapsedTimer elapsed; elapsed.start(); refresh->click();
        QVERIFY(elapsed.elapsed() < 100); QVERIFY(!refresh->isEnabled());
        QTRY_COMPARE_WITH_TIMEOUT(devices->count(), 3, 2000);
        QTRY_VERIFY_WITH_TIMEOUT(refresh->isEnabled(), 2000);
        QCOMPARE(observation->discoveryCalls.load(), std::uint64_t(1));
        devices->setCurrentIndex(2);
        QCOMPARE(w.findChild<QComboBox*>("deviceSelector")->currentData().toString(), QString("daq:FakeDev1"));
        QVERIFY(w.findChild<QPushButton*>("startButton")->isEnabled());
        QCOMPARE(w.findChild<QComboBox*>("deviceSelector")->currentText(), QString("FakeDev1 · Fake multifunction DAQ"));
    }

    void cameraControlsDoNotAutoConnect() {
        const QString output = qEnvironmentVariable("TDLAS_TEST_ARTIFACTS", "artifacts"); QDir().mkpath(output);
        MainWindow w(output + "/step9-settings");
        w.show();
        auto* selector = w.findChild<QComboBox*>("cameraDeviceSelector");
        auto* refresh = w.findChild<QPushButton*>("cameraRefreshButton");
        auto* connectButton = w.findChild<QPushButton*>("cameraConnectButton");
        auto* state = w.findChild<QLabel*>("cameraCardState");
        QVERIFY(selector); QVERIFY(refresh); QVERIFY(connectButton); QVERIFY(state);
        QCOMPARE(selector->count(), 1);
        QVERIFY(selector->currentData().toByteArray().isEmpty());
        QVERIFY(refresh->isEnabled()); QVERIFY(!connectButton->isEnabled());
        QVERIFY(state->text().contains("未连接"));
        QVERIFY(state->text().contains("不会自动"));
        auto* preview = w.findChild<QLabel*>("cameraCardPreview");
        auto* devicePanel = w.findChild<QWidget*>("cameraDevicePanel");
        QVERIFY(preview); QVERIFY(devicePanel); QVERIFY(preview->isAncestorOf(devicePanel));
        QVERIFY(w.findChild<QLabel*>("cameraDetails"));
        QVERIFY(w.findChild<QPushButton*>("cameraBrightnessTool"));
        QVERIFY(w.findChild<QPushButton*>("cameraContrastTool"));
        QVERIFY(w.findChild<QPushButton*>("cameraZoomTool"));
        int primaryCameraActions = 0;
        for (auto* button : w.findChildren<QPushButton*>()) if (button->property("cameraAction").toBool()) ++primaryCameraActions;
        QCOMPARE(primaryCameraActions, 3);
        QVERIFY(!w.findChild<QPushButton*>("cameraAction1")->isEnabled());
        QVERIFY(!w.findChild<QPushButton*>("cameraAction2")->isEnabled());
        QVERIFY(w.findChild<QPushButton*>("cameraAction3")->isEnabled());
        QVERIFY(w.findChild<QPushButton*>("cameraAction4")->isEnabled());
        QVERIFY(w.findChild<QPushButton*>("cameraAction4")->property("cameraTool").toBool());
        bool inspected = false;
        QTimer::singleShot(0, &w, [&] {
            auto* dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget()); QVERIFY(dialog); inspected = true;
            QVERIFY(dialog->findChild<QPushButton*>("cameraPanUp")); QVERIFY(dialog->findChild<QPushButton*>("cameraPanDown"));
            QVERIFY(dialog->findChild<QPushButton*>("cameraPanLeft")); QVERIFY(dialog->findChild<QPushButton*>("cameraPanRight"));
            QVERIFY(dialog->findChild<QDoubleSpinBox*>("cameraDigitalZoom"));
            QVERIFY(dialog->findChild<QSpinBox*>("cameraBrightness")); QVERIFY(dialog->findChild<QSpinBox*>("cameraContrast"));
            QVERIFY(dialog->findChild<QSpinBox*>("cameraSaturation")); QVERIFY(dialog->findChild<QSpinBox*>("cameraHue"));
            QVERIFY(dialog->findChild<QSpinBox*>("cameraSharpness")); QVERIFY(dialog->findChild<QDoubleSpinBox*>("cameraGamma"));
            QVERIFY(!dialog->findChild<QDoubleSpinBox*>("cameraHardwareZoom")->isEnabled());
            QVERIFY(dialog->findChild<QLabel*>("cameraTuningScope")->text().contains("录像"));
            QVERIFY(dialog->grab().save(output + "/camera-tuning-controls.png"));
            dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Cancel)->click();
        });
        w.findChild<QPushButton*>("cameraAction4")->click(); QVERIFY(inspected);
    }

    void loginAndRoleWorkspaces() {
        const QString output = qEnvironmentVariable("TDLAS_TEST_ARTIFACTS", "artifacts"); QDir().mkpath(output);
        LoginDialog login; login.resize(1672, 941); login.show(); QTest::qWait(80);
        auto* account = login.findChild<QLineEdit*>("loginAccount");
        auto* password = login.findChild<QLineEdit*>("loginPassword");
        auto* role = login.findChild<QComboBox*>("loginRole");
        auto* submit = login.findChild<QPushButton*>("loginButton");
        QVERIFY(account); QVERIFY(password); QVERIFY(role); QVERIFY(submit); QCOMPARE(role->count(), 2);
        auto* logo = login.findChild<QLabel*>("loginLogo");
        QVERIFY(logo); QVERIFY(!logo->pixmap().isNull());
        auto* companyLogo = login.findChild<QLabel*>("companyLogo");
        QVERIFY(companyLogo); QVERIFY(!companyLogo->pixmap().isNull());
        QVERIFY(login.findChild<QLabel*>("accountLabel"));
        QVERIFY(login.findChild<QLabel*>("passwordLabel"));
        QVERIFY(login.findChild<QLabel*>("localSessionNote")->text().contains("本地界面会话"));
        auto* adminRole = login.findChild<QPushButton*>("adminRoleButton");
        auto* customerRole = login.findChild<QPushButton*>("customerRoleButton");
        auto* forgotPassword = login.findChild<QPushButton*>("forgotPassword");
        QVERIFY(adminRole); QVERIFY(customerRole); QVERIFY(forgotPassword);
        QVERIFY(adminRole->isVisible()); QVERIFY(customerRole->isVisible());
        customerRole->click(); QCOMPARE(login.role(), ApplicationRole::Customer);
        adminRole->click(); QCOMPARE(login.role(), ApplicationRole::Administrator);
        forgotPassword->click();
        QVERIFY(login.findChild<QLabel*>("loginValidation")->text().contains("密码找回"));
        auto* visibility = login.findChild<QPushButton*>("passwordVisibility");
        QVERIFY(visibility); visibility->click(); QCOMPARE(password->echoMode(), QLineEdit::Normal);
        visibility->click(); QCOMPARE(password->echoMode(), QLineEdit::Password);
        submit->click(); QVERIFY(login.findChild<QLabel*>("loginValidation")->text().contains("请输入"));
        account->setText("user"); password->setText("local-session"); role->setCurrentIndex(1);
        QCOMPARE(login.role(), ApplicationRole::Customer); QCOMPARE(login.accountName(), QString("user"));
        QVERIFY(login.grab().save(output + "/login-day.png")); submit->click(); QCOMPARE(login.result(), int(QDialog::Accepted));

        MainWindow customer(output + "/customer-role-settings", nullptr, ApplicationRole::Customer, "user");
        customer.resize(1672, 941); customer.show(); QTest::qWait(80);
        auto* customerTabs = customer.findChild<QTabWidget*>("mainTabs");
        QVERIFY(customerTabs->tabBar()->isHidden());
        for (int page = 1; page < customerTabs->count(); ++page) QVERIFY(!customerTabs->isTabVisible(page));
        QVERIFY(customer.findChild<QLabel*>("headerUser")->text().contains("user"));
        QVERIFY(customer.grab().save(output + "/customer-role-day.png"));

        MainWindow administrator(output + "/admin-role-settings", nullptr, ApplicationRole::Administrator, "admin");
        administrator.show();
        QVERIFY(!administrator.findChild<QTabWidget*>("mainTabs")->tabBar()->isHidden());
        QCOMPARE(administrator.findChild<QTabWidget*>("mainTabs")->count(), 4);
    }

    void algorithmMigrationIsNotMeasurement() {
        MainWindow w(qEnvironmentVariable("TDLAS_TEST_ARTIFACTS", "artifacts") + "/step6-settings");
        w.show();
        auto* notice = w.findChild<QLabel*>("notice");
        QVERIFY(notice->text().contains("旧版算法"));
        QVERIFY(notice->text().contains("标定未确认"));
        auto* device = w.findChild<QComboBox*>("deviceSelector");
        auto* scenario = w.findChild<QComboBox*>("simulationScenario");
        device->setCurrentIndex(1); scenario->setCurrentIndex(2);
        w.findChild<QPushButton*>("startButton")->click();
        QTRY_VERIFY_WITH_TIMEOUT(!w.runtime()->active(), 2000);
        QCOMPARE(w.runtime()->snapshot().stats.acceptedBlocks, std::uint64_t(3));
        QVERIFY(w.runtime()->results().snapshot().entries.empty());
        for (auto* metric : w.findChildren<QLabel*>("peakAbsorbance")) QCOMPARE(metric->text(), QString("—"));
        for (auto* metric : w.findChildren<QLabel*>("intensityPercent")) QCOMPARE(metric->text(), QString("—"));
        for (auto* metric : w.findChildren<QLabel*>("concentration")) QCOMPARE(metric->text(), QString("未校准"));
        QCOMPARE(w.findChild<QTableWidget*>("peakTable")->rowCount(), 0);
    }

    void simulationControlsAndConfirmations() {
        const QString output = qEnvironmentVariable("TDLAS_TEST_ARTIFACTS", "artifacts"); QDir().mkpath(output);
        MainWindow w(output + "/step4-settings"); w.show();
        auto* start = w.findChild<QPushButton*>("startButton"); auto* stop = w.findChild<QPushButton*>("stopButton");
        auto* reset = w.findChild<QPushButton*>("resetButton"); auto* device = w.findChild<QComboBox*>("deviceSelector");
        auto* scenario = w.findChild<QComboBox*>("simulationScenario");
        auto* raw = w.findChild<QWidget*>("rawCard")->findChild<QChartView*>()->chart();
        auto* series = qobject_cast<QLineSeries*>(raw->series().first());
        QVERIFY(!w.runtime()->active()); QVERIFY(!start->isEnabled()); QVERIFY(!stop->isEnabled());
        QCOMPARE(scenario->currentIndex(), 0); QVERIFY(!scenario->isEnabled());
        device->setCurrentIndex(1); start->click();
        QCOMPARE(w.runtime()->state(), RunState::Starting); QVERIFY(stop->isEnabled());
        QVERIFY(!start->isEnabled()); QVERIFY(!reset->isEnabled()); QVERIFY(!device->isEnabled()); QVERIFY(!scenario->isEnabled());
        QTRY_VERIFY_WITH_TIMEOUT(series->count() > 0, 2000);
        w.findChild<QTabWidget*>("mainTabs")->setCurrentIndex(1);
        QVERIFY(w.grab().save(output + "/simulation-running.png"));
        for (auto* metric : w.findChildren<QLabel*>("peakAbsorbance")) QCOMPARE(metric->text(), QString("—"));
        for (auto* metric : w.findChildren<QLabel*>("intensityPercent")) QCOMPARE(metric->text(), QString("—"));
        for (auto* metric : w.findChildren<QLabel*>("concentration")) QCOMPARE(metric->text(), QString("未校准"));
        QCOMPARE(w.findChild<QTableWidget*>("peakTable")->rowCount(), 0);
        stop->click(); QVERIFY(!stop->isEnabled()); QTRY_VERIFY_WITH_TIMEOUT(!w.runtime()->active(), 2000);
        QCOMPARE(w.runtime()->state(), RunState::Stopped); QVERIFY(start->isEnabled()); QVERIFY(reset->isEnabled());
        const auto saved = series->points(); const auto snapshot = w.runtime()->snapshot();
        QTest::qWait(100); QCOMPARE(series->points(), saved);
        auto confirmation = answerConfirmation(reset, QMessageBox::No);
        QVERIFY(confirmation.shown); QVERIFY(confirmation.defaultsToNo); QCOMPARE(series->points(), saved);
        confirmation = answerConfirmation(start, QMessageBox::No); QVERIFY(confirmation.shown); QVERIFY(confirmation.defaultsToNo);
        QVERIFY(!w.runtime()->active()); QCOMPARE(w.runtime()->snapshot().runId, snapshot.runId);
        // An accepted new run clears the previous curve before its delayed first read.
        scenario->setCurrentIndex(3); confirmation = answerConfirmation(start, QMessageBox::Yes);
        QVERIFY(confirmation.shown); QCOMPARE(series->count(), 0);
        QTRY_COMPARE_WITH_TIMEOUT(w.runtime()->state(), RunState::Running, 2000);
        stop->click(); QTRY_VERIFY_WITH_TIMEOUT(!w.runtime()->active(), 2000); QCOMPARE(series->count(), 0);
        // A failed open must not resurrect the old curve or invent metric values.
        scenario->setCurrentIndex(1); start->click(); QTRY_VERIFY_WITH_TIMEOUT(!w.runtime()->active(), 2000);
        QCOMPARE(w.runtime()->state(), RunState::Error); QCOMPARE(series->count(), 0);
        QVERIFY(w.findChild<QLabel*>("notice")->text().contains("打开失败"));
        reset->click(); QCOMPARE(w.runtime()->state(), RunState::Ready); QCOMPARE(scenario->currentIndex(), 0);
        scenario->setCurrentIndex(2); start->click(); QTRY_VERIFY_WITH_TIMEOUT(!w.runtime()->active(), 2000);
        QCOMPARE(w.runtime()->state(), RunState::Error); QCOMPARE(w.runtime()->snapshot().stats.acceptedBlocks, std::uint64_t(3));
        QVERIFY(series->count() > 0); QVERIFY(w.findChild<QLabel*>("notice")->text().contains("读取失败"));
        QVERIFY(w.grab().save(output + "/simulation-read-fault.png"));
        const double rate = w.findChild<QDoubleSpinBox*>("sampleRate")->value();
        confirmation = answerConfirmation(reset, QMessageBox::Yes); QVERIFY(confirmation.shown);
        QCOMPARE(w.runtime()->state(), RunState::Ready); QVERIFY(w.runtime()->error().isEmpty());
        QCOMPARE(series->count(), 0); QCOMPARE(w.runtime()->snapshot().stats.acceptedBlocks, std::uint64_t(0));
        QCOMPARE(device->currentIndex(), 1); QCOMPARE(scenario->currentIndex(), 0);
        QCOMPARE(w.findChild<QDoubleSpinBox*>("sampleRate")->value(), rate);
        QVERIFY(w.grab().save(output + "/simulation-reset.png"));
    }
    void unpaintedDataRequiresConfirmation() {
        MainWindow w(qEnvironmentVariable("TDLAS_TEST_ARTIFACTS", "artifacts") + "/step4-settings");
        auto* series = qobject_cast<QLineSeries*>(w.findChild<QWidget*>("rawCard")->findChild<QChartView*>()->chart()->series().first());
        AcquisitionConfig c; c.deviceId = "fixture"; c.inputChannel = "fixture/ai";
        {
            // Finish worker events without running the UI timer or stopped slot.
            QSignalBlocker blocked(w.runtime());
            QVERIFY(w.runtime()->startWithResults(c, std::make_unique<ResultTestSource>(1), [](const SampleBlock& b) { return std::vector<AcquisitionFrame>{testResult(b)}; }));
            QElapsedTimer elapsed; elapsed.start();
            while (w.runtime()->active() && elapsed.elapsed() < 2000) {
                QTest::qSleep(5); QCoreApplication::sendPostedEvents(w.runtime(), QEvent::MetaCall);
            }
            QVERIFY(!w.runtime()->active()); QCOMPARE(series->count(), 0);
            QCOMPARE(w.findChild<QTableWidget*>("peakTable")->rowCount(), 0);
        }
        auto* reset = w.findChild<QPushButton*>("resetButton");
        const auto no = answerConfirmation(reset, QMessageBox::No); QVERIFY(no.shown); QVERIFY(no.defaultsToNo);
        QCOMPARE(w.runtime()->results().snapshot().entries.size(), std::size_t(1));
        const auto yes = answerConfirmation(reset, QMessageBox::Yes); QVERIFY(yes.shown);
        QVERIFY(w.runtime()->results().snapshot().entries.empty()); QVERIFY(w.runtime()->snapshot().samples.empty());
        QCOMPARE(w.findChild<QTableWidget*>("peakTable")->rowCount(), 0);
    }
    void stopDuringStartingFromUi() {
        MainWindow w(qEnvironmentVariable("TDLAS_TEST_ARTIFACTS", "artifacts") + "/step4-settings"); w.show();
        w.findChild<QComboBox*>("deviceSelector")->setCurrentIndex(1);
        w.findChild<QPushButton*>("startButton")->click(); QCOMPARE(w.runtime()->state(), RunState::Starting);
        auto* stop = w.findChild<QPushButton*>("stopButton"); QVERIFY(stop->isEnabled()); stop->click();
        QVERIFY(!stop->isEnabled()); QTRY_VERIFY_WITH_TIMEOUT(!w.runtime()->active(), 2000);
        QCOMPARE(w.runtime()->state(), RunState::Stopped); QVERIFY(w.runtime()->error().isEmpty());
    }
    void customerDisplayRedesign() {
        const QString output=qEnvironmentVariable("TDLAS_TEST_ARTIFACTS","artifacts") + "/customer-redesign";
        QDir().mkpath(output);
        MainWindow w(output+"/settings"); w.resize(1672,941); w.setNightTheme(false); w.show(); QTest::qWait(100);
        auto* tabs=w.findChild<QTabWidget*>("mainTabs");
        auto* range=w.findChild<QComboBox*>("customerHistoryRange");
        auto* exportAction=w.findChild<QPushButton*>("customerExportDataButton");
        auto* camera=w.findChild<QPushButton*>("customerCameraManage");
        auto* view=w.findChild<InteractiveChartView*>("customerHistoryCardView");
        QVERIFY(range); QVERIFY(exportAction); QVERIFY(camera); QVERIFY(view);
        QVERIFY(!exportAction->isEnabled()); QVERIFY(!w.runtime()->active());
        QVERIFY(w.findChild<QLabel*>("customerCameraCardState")->text().contains("未连接"));
        QCOMPARE(tabs->tabBar()->iconSize(),QSize(24,24));
        if(QGuiApplication::platformName() != "windows") QCOMPARE(w.size(),QSize(1672,941));
        QVERIFY(w.grab().save(output+"/admin-customer-empty-day.png"));
        camera->click(); QCOMPARE(tabs->currentIndex(),3);
        QVERIFY(!w.runtime()->active()); tabs->setCurrentIndex(0);
        AcquisitionConfig c; c.deviceId="simulation"; c.inputChannel="test/input";
        QVERIFY(w.runtime()->startWithResults(c,std::make_unique<ResultTestSource>(481),[](const SampleBlock& b) {
            auto f=testResult(b); f.fv.x.clear(); f.fv.y.clear();
            for(int i=0;i<161;++i) {
                const double x=(i-80)/25.0;
                f.fv.x.push_back(x); f.fv.y.push_back(std::exp(-x*x*3)+0.018*std::sin(i*1.7));
            }
            f.fv.xQuantity="模拟坐标"; f.fv.xUnit="1"; f.fv.yQuantity="模拟 FV"; f.fv.yUnit="1";
            for(auto& y : f.fv.y) y*=1.0+0.1*std::sin(b.sequence*0.1);
            if(b.sequence==240) { f.fv.x.clear(); f.fv.y.clear(); }
            return std::vector<AcquisitionFrame>{f};
        }));
        QTRY_VERIFY_WITH_TIMEOUT(!w.runtime()->active(),5000);
        QTRY_COMPARE_WITH_TIMEOUT(w.findChild<QTableWidget*>("peakTable")->rowCount(),481,5000);
        QVERIFY(exportAction->isEnabled());
        auto* axis=qobject_cast<QValueAxis*>(view->chart()->axes(Qt::Horizontal).first());
        range->setCurrentIndex(1); QCOMPARE(axis->min(),60.0); QCOMPARE(axis->max(),120.0);
        QCOMPARE(view->chart()->series().size(),2); // Missing batch stays a discontinuity.
        const auto run=w.runtime()->snapshot().runId;
        const auto points=qobject_cast<QLineSeries*>(view->chart()->series().last())->points();
        view->setFocus(); QTest::keyClick(view,Qt::Key_Plus); QVERIFY(!view->autoFollow());
        w.findChild<QPushButton*>("customerHistoryCardResetZoom")->click();
        QVERIFY(view->autoFollow()); QCOMPARE(axis->min(),60.0); QCOMPARE(axis->max(),120.0);
        range->setCurrentIndex(0); QCOMPARE(axis->min(),0.0);
        for(bool night : {false,true}) {
            w.setNightTheme(night); tabs->setCurrentIndex(1); tabs->setCurrentIndex(0);
            QCOMPARE(w.runtime()->snapshot().runId,run);
            QCOMPARE(qobject_cast<QLineSeries*>(view->chart()->series().last())->points(),points);
            QTest::qWait(80);
            w.findChild<QLabel*>("headerClock")->setText("2026-10-29  12:00:00");
            w.findChild<QLabel*>("notice")->setText("模拟 / 非测量 · 确定性界面验收数据；物理单位与浓度未校准。");
            QVERIFY(w.grab().save(output+(night ? "/admin-customer-night.png" : "/admin-customer-day.png")));
        }
        w.setNightTheme(false);
    }
    void nativePages() {
        if (QGuiApplication::platformName() != "windows") QSKIP("Native Windows screenshot check is run separately.");
        const QString output = qEnvironmentVariable("TDLAS_TEST_ARTIFACTS", "artifacts"); QDir().mkpath(output);
        LoginDialog login; login.resize(1672, 941); login.show(); QTest::qWait(150);
        QVERIFY(login.grab().save(output + "/native-login-day.png")); login.hide();
        StartupSplash splash; splash.show(); QTest::qWait(100);
        QVERIFY(splash.grab().save(output + "/native-startup-splash.png")); splash.hide();
        MainWindow w(output + "/native-settings"); w.showMaximized(); QTest::qWait(150);
        QVERIFY(w.isMaximized()); QVERIFY(!w.runtime()->active());
        auto* tabs = w.findChild<QTabWidget*>("mainTabs"); QJsonArray geometry;
        for (bool night : {false, true}) {
            w.setNightTheme(night);
            for (int page = 0; page < tabs->count(); ++page) {
                tabs->setCurrentIndex(page); QTest::qWait(120);
                const QString name = QString("native-%1-%2.png").arg(page).arg(night ? "night" : "day");
                QVERIFY(w.grab().save(output + "/" + name));
                geometry.append(QJsonObject{{"screenshot", name}, {"contentWidth", w.centralWidget()->width()},
                    {"contentHeight", w.centralWidget()->height()}, {"devicePixelRatio", w.devicePixelRatioF()}});
            }
        }
        QFile file(output + "/native-geometry.json"); QVERIFY(file.open(QIODevice::WriteOnly)); file.write(QJsonDocument(geometry).toJson());
        w.setNightTheme(false);
    }
    void finalAcceptanceChecklist() {
        const QString output = qEnvironmentVariable("TDLAS_TEST_ARTIFACTS", "artifacts"); QDir().mkpath(output);
        MainWindow w(output + "/step10-settings"); w.resize(1920, 1080); w.show(); w.activateWindow(); QTest::qWait(80);
        auto* workspace = w.findChild<QWidget*>("workspace");
        auto* left = w.findChild<QWidget*>("leftPane");
        auto* right = w.findChild<QWidget*>("rightPane");
        QVERIFY(workspace); QVERIFY(left); QVERIFY(right);
        if (QGuiApplication::platformName() == "windows") {
            // A native top-level window is constrained by the current desktop's
            // available logical geometry; nativePages records that exact size.
            QVERIFY(w.centralWidget()->width() >= 1440); QVERIFY(w.centralWidget()->height() >= 700);
        } else {
            QCOMPARE(w.centralWidget()->size(), QSize(1920, 1080));
        }
        QVERIFY(std::abs(left->width() - workspace->width() * 0.22) <= 1);
        QCOMPARE(left->width() + right->width(), workspace->width());

        for (int i = 1; i <= 11; ++i) QVERIFY(w.findChild<QLabel*>("parameterLabel" + QString::number(i)));
        QVERIFY(!w.findChild<QLabel*>("parameterLabel12"));
        for (const auto* name : {"startButton", "stopButton", "resetButton"}) {
            auto* button = w.findChild<QPushButton*>(name); QVERIFY(button);
            QVERIFY2(button->height() >= 40, qPrintable(QString("%1 rendered height is %2").arg(name).arg(button->height())));
        }
        for (const auto* name : {"peakAbsorbance", "intensityPercent", "concentration"}) {
            auto readings = w.findChildren<QLabel*>(name); QCOMPARE(readings.size(), 1); QVERIFY(!readings.first()->text().isEmpty());
        }
        auto* tabs = w.findChild<QTabWidget*>("mainTabs"); QVERIFY(tabs);
        QCOMPARE(QStringList({tabs->tabText(0), tabs->tabText(1), tabs->tabText(2), tabs->tabText(3)}),
                 QStringList({"客户显示窗口", "调试界面", "峰值数据界面", "摄像界面"}));
        auto* raw = w.findChild<QWidget*>("rawCard");
        for (const auto* name : {"fitCard", "fvCard", "peakCard"}) {
            auto* card = w.findChild<QWidget*>(name); QVERIFY(card);
            QVERIFY(std::abs(card->width() - raw->width()) <= 1); QVERIFY(std::abs(card->height() - raw->height()) <= 1);
        }

        // Keyboard-only start, tab switch and stop must preserve the same run and collected data.
        auto* device = w.findChild<QComboBox*>("deviceSelector"); device->setCurrentIndex(1);
        auto* start = w.findChild<QPushButton*>("startButton"); start->setFocus(); QTest::keyClick(start, Qt::Key_Space);
        QTRY_COMPARE_WITH_TIMEOUT(w.runtime()->state(), RunState::Running, 2000);
        QTRY_VERIFY_WITH_TIMEOUT(w.runtime()->snapshot().stats.acceptedBlocks > 0, 2000);
        const auto runId = w.runtime()->snapshot().runId;
        const auto acceptedBeforeSwitch = w.runtime()->snapshot().stats.acceptedBlocks;
        tabs->tabBar()->setFocus(); QTest::keyClick(tabs->tabBar(), Qt::Key_Right);
        QCOMPARE(tabs->currentIndex(), 1); QCOMPARE(w.runtime()->snapshot().runId, runId);
        QTRY_VERIFY_WITH_TIMEOUT(w.runtime()->snapshot().stats.acceptedBlocks >= acceptedBeforeSwitch, 1000);
        auto* stop = w.findChild<QPushButton*>("stopButton"); stop->setFocus(); QTest::keyClick(stop, Qt::Key_Space);
        QTRY_VERIFY_WITH_TIMEOUT(!w.runtime()->active(), 3000);
        const auto acceptedAfterStop = w.runtime()->snapshot().stats.acceptedBlocks; QVERIFY(acceptedAfterStop > 0);
        auto* theme = w.findChild<QPushButton*>("themeButton"); theme->setFocus(); QTest::keyClick(theme, Qt::Key_Space);
        QCOMPARE(w.runtime()->snapshot().stats.acceptedBlocks, acceptedAfterStop);
        QTest::keyClick(theme, Qt::Key_Space); QCOMPARE(w.runtime()->snapshot().stats.acceptedBlocks, acceptedAfterStop);

        QFile tokenFile(":/design-tokens.json"); QVERIFY(tokenFile.open(QIODevice::ReadOnly));
        const auto themes = QJsonDocument::fromJson(tokenFile.readAll()).object()["themes"].toObject();
        QJsonArray contrastEvidence;
        const QList<std::tuple<QString, QString, double>> checks = {
            {"text", "background", 4.5}, {"textSecondary", "surface", 4.5}, {"disabledText", "disabledBackground", 4.5},
            {"onPrimary", "primary", 4.5}, {"selectedTabText", "selectedTabBackground", 4.5},
            {"controlBorder", "surface", 3.0}, {"focus", "background", 3.0}, {"chartAxis", "chartBackground", 3.0}
        };
        for (const auto& themeName : {QStringLiteral("day"), QStringLiteral("night")}) {
            const auto colors = themes[themeName].toObject();
            for (const auto& [foreground, background, minimum] : checks) {
                const double ratio = contrastRatio(QColor(colors[foreground].toString()), QColor(colors[background].toString()));
                QVERIFY2(ratio >= minimum, qPrintable(QString("%1 %2/%3 contrast %4 < %5").arg(themeName, foreground, background).arg(ratio).arg(minimum)));
                contrastEvidence.append(QJsonObject{{"theme", themeName}, {"foreground", foreground}, {"background", background},
                                                     {"ratio", ratio}, {"minimum", minimum}});
            }
        }
        QFile evidence(output + "/final-acceptance.json"); QVERIFY(evidence.open(QIODevice::WriteOnly));
        evidence.write(QJsonDocument(QJsonObject{{"contentWidth", w.centralWidget()->width()},
            {"contentHeight", w.centralWidget()->height()}, {"leftWidth", left->width()}, {"rightWidth", right->width()},
            {"parameterCount", 11}, {"tabCount", tabs->count()}, {"contrast", contrastEvidence}}).toJson());
        QVERIFY(w.grab().save(output + "/final-keyboard-theme.png"));
    }
    void keyboardScrollAndValidation() {
        const QString output = qEnvironmentVariable("TDLAS_TEST_ARTIFACTS", "artifacts");
        MainWindow w(output + "/test-settings"); w.resize(900, 600); w.show(); w.activateWindow(); QTest::qWait(60);
        auto* device = w.findChild<QComboBox*>("deviceSelector"); device->setCurrentIndex(1); device->setFocus();
        const QStringList order = {"sampleRate", "displaySamples", "inputMaximum", "inputMinimum", "startButton", "resetButton", "themeButton"};
        auto* left = w.findChild<QScrollArea*>("leftPane");
        for (const auto& name : order) {
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
            auto* field = w.findChild<QWidget*>(name); QVERIFY(field);
            QTRY_VERIFY(field->hasFocus());
            if (left->isAncestorOf(field)) {
                const QRect fieldRect(field->mapTo(left->viewport(), QPoint()), field->size());
                QVERIFY2(left->viewport()->rect().contains(fieldRect), qPrintable(QString("%1 field=%2,%3 %4x%5 viewport=%6x%7").arg(name).arg(fieldRect.x()).arg(fieldRect.y()).arg(fieldRect.width()).arg(fieldRect.height()).arg(left->viewport()->width()).arg(left->viewport()->height())));
            }
        }
        QVERIFY(left->verticalScrollBar()->value() > 0);
        auto* upper = w.findChild<QDoubleSpinBox*>("inputMaximum"); upper->setValue(-10);
        QVERIFY(!w.findChild<QPushButton*>("startButton")->isEnabled());
        QVERIFY(w.findChild<QLabel*>("parameterHint")->text().contains("上限必须大于"));
        QVERIFY(upper->property("invalid").toBool()); QVERIFY(!upper->accessibleDescription().isEmpty());
        upper->setValue(10); QVERIFY(w.findChild<QPushButton*>("startButton")->isEnabled()); QVERIFY(!upper->property("invalid").toBool());
        auto* tabs = w.findChild<QTabWidget*>("mainTabs"); tabs->tabBar()->setFocus();
        QTest::keyClick(tabs->tabBar(), Qt::Key_Right); QCOMPARE(tabs->currentIndex(), 1);
        tabs->setCurrentIndex(2);
        auto* toggle = w.findChild<QPushButton*>("togglePeakTable"); toggle->setFocus(); QTest::qWait(60);
        auto* outer = w.findChild<QScrollArea*>("workspaceScroll");
        QVERIFY(outer->viewport()->rect().contains(QRect(toggle->mapTo(outer->viewport(), QPoint()), toggle->size())));
        QTest::keyClick(toggle, Qt::Key_Space); QVERIFY(w.findChild<QTableWidget*>("peakTable")->isHidden());
        QTest::keyClick(toggle, Qt::Key_Space); QVERIFY(!w.findChild<QTableWidget*>("peakTable")->isHidden());
        QVERIFY(!w.runtime()->active());
        tabs->setCurrentIndex(3);
        for (int i = 1; i <= 2; ++i) {
            auto* button = w.findChild<QPushButton*>("cameraAction" + QString::number(i));
            QVERIFY(button); QVERIFY(!button->isEnabled()); QVERIFY(!button->accessibleDescription().isEmpty());
        }
        for (int i = 3; i <= 4; ++i) {
            auto* button = w.findChild<QPushButton*>("cameraAction" + QString::number(i));
            QVERIFY(button); QVERIFY(button->isEnabled()); QVERIFY(!button->accessibleDescription().isEmpty());
        }
        QVERIFY(w.grab().save(output + "/small-keyboard.png"));
    }
    void fitLegendAndThemePersistence() {
        const QString output = qEnvironmentVariable("TDLAS_TEST_ARTIFACTS", "artifacts");
        const QString settings = output + "/theme-settings";
        {
            MainWindow w(settings); w.resize(1920,1080); w.show();
            AcquisitionConfig c; c.deviceId = "test"; c.inputChannel = "test/input";
            QVERIFY(w.runtime()->startWithResults(c, std::make_unique<ResultTestSource>(1), [](const SampleBlock& b) {
                auto f = testResult(b); f.observed.pointValid = {true, false, true}; return std::vector<AcquisitionFrame>{f};
            }));
            QTRY_VERIFY_WITH_TIMEOUT(!w.runtime()->active(), 2000);
            auto* chart = w.findChild<QWidget*>("fitCard")->findChild<QChartView*>()->chart();
            QTRY_COMPARE(chart->series().size(), 3);
            w.findChild<QTabWidget*>("mainTabs")->setCurrentIndex(1);
            for (bool night : {false, true}) {
                w.setNightTheme(night); QTest::qWait(60);
                QCOMPARE(chart->series().size(), 3);
                auto* observed = qobject_cast<QLineSeries*>(chart->series().first());
                auto* fitted = qobject_cast<QLineSeries*>(chart->series().last());
                QCOMPARE(fitted->pen().style(), Qt::DashLine); QCOMPARE(observed->pen().style(), Qt::SolidLine);
                QVERIFY(observed->pen().color() != fitted->pen().color());
                int visible = 0; for (auto* marker : chart->legend()->markers()) if (marker->isVisible()) ++visible;
                QCOMPARE(visible, 2);
                QCOMPARE(w.runtime()->results().snapshot().entries.size(), std::size_t(1));
                QVERIFY(w.grab().save(output + (night ? "/fit-night.png" : "/fit-day.png")));
            }
        }
        MainWindow restored(settings); QCOMPARE(restored.findChild<QPushButton*>("themeButton")->text(), QString("切换日间"));
        restored.setNightTheme(false);
    }
    void chartInteractionDisablesAndRestoresFollow() {
        MainWindow w(qEnvironmentVariable("TDLAS_TEST_ARTIFACTS", "artifacts") + "/step7-chart-settings");
        w.resize(1600, 900); w.show();
        auto* card = w.findChild<QWidget*>("rawCard"); QVERIFY(card);
        auto* view = card->findChild<InteractiveChartView*>("rawCardView"); QVERIFY(view); QVERIFY(view->autoFollow());
        auto* follow = card->findChild<QPushButton*>("rawCardAutoFollow"); QVERIFY(follow); QVERIFY(follow->isChecked());
        QVERIFY(view->accessibleDescription().contains("悬停") || view->accessibleDescription().contains("最近点"));
        view->setFocus(); QTest::keyClick(view, Qt::Key_Plus);
        QVERIFY(!view->autoFollow()); QVERIFY(!follow->isChecked());
        QTest::keyClick(view, Qt::Key_R); QVERIFY(view->autoFollow()); QVERIFY(follow->isChecked());
        QTest::keyClick(view, Qt::Key_Right); QVERIFY(!view->autoFollow());
        follow->click(); QVERIFY(view->autoFollow());
    }
    void commonResultsAndNullBreaks() {
        const QString output = qEnvironmentVariable("TDLAS_TEST_ARTIFACTS", "artifacts");
        MainWindow w(output + "/test-settings"); w.resize(1920, 1080); w.show();
        AcquisitionConfig c; c.deviceId = "test"; c.inputChannel = "test/input";
        QVERIFY(w.runtime()->startWithResults(c, std::make_unique<ResultTestSource>(3), [](const SampleBlock& b) {
            auto f = testResult(b);
            if (b.sequence == 1) { f.fv.x.clear(); f.fv.y.clear(); }
            if (b.sequence == 2) f.fitStatus = "failed";
            return std::vector<AcquisitionFrame>{f};
        }));
        QTRY_VERIFY_WITH_TIMEOUT(!w.runtime()->active(), 2000);
        auto* table = w.findChild<QTableWidget*>("peakTable"); QTRY_COMPARE(table->rowCount(), 3); QCOMPARE(table->item(1, 4)->text(), QString("—"));
        QVERIFY(w.findChild<QPushButton*>("exportDataButton")->isEnabled());
        auto chart = [&](const char* name) { return w.findChild<QWidget*>(name)->findChild<QChartView*>()->chart(); };
        QCOMPARE(chart("peakCard")->series().size(), 2); QCOMPARE(chart("historyCard")->series().size(), 2);
        for (auto* series : chart("historyCard")->series()) QCOMPARE(qobject_cast<QLineSeries*>(series)->count(), 1);
        QCOMPARE(qobject_cast<QLineSeries*>(chart("customerFv")->series().first())->points(), qobject_cast<QLineSeries*>(chart("fvCard")->series().first())->points());
        QCOMPARE(chart("fitCard")->series().size(), 1); QCOMPARE(chart("fitCard")->series().first()->name(), QString("观测"));
        const auto metrics = w.findChildren<QLabel*>("peakAbsorbance"); QCOMPARE(metrics.size(), 1);
        QCOMPARE(metrics[0]->text(), QString("0.25"));
        for (auto* reading : w.findChildren<QLabel*>("concentration")) QCOMPARE(reading->text(), QString("未校准"));
        w.findChild<QTabWidget*>("mainTabs")->setCurrentIndex(2); QTest::qWait(60); QVERIFY(w.grab().save(output + "/result-history-day.png"));
        w.setNightTheme(true); QTest::qWait(60); QVERIFY(w.grab().save(output + "/result-history-night.png"));
        QVERIFY(w.runtime()->reset()); QTRY_COMPARE(table->rowCount(), 0); QTRY_COMPARE(metrics[0]->text(), QString("—"));
        QVERIFY(chart("customerFv")->series().empty()); w.setNightTheme(false);
    }
    void layoutAndThemes() {
        const QString output = qEnvironmentVariable("TDLAS_TEST_ARTIFACTS", "artifacts"); QDir().mkpath(output);
        MainWindow w(output + "/test-settings"); w.setNightTheme(false); w.show();
        auto* tabs = w.findChild<QTabWidget*>("mainTabs"); QVERIFY(tabs); QCOMPARE(tabs->count(), 4);
        QCOMPARE(tabs->tabText(0), QString("客户显示窗口")); QCOMPARE(tabs->tabText(1), QString("调试界面"));
        QCOMPARE(tabs->tabText(2), QString("峰值数据界面")); QCOMPARE(tabs->tabText(3), QString("摄像界面"));
        const QStringList names = {"采集卡（设备识别）", "输出通道", "输出波形", "波形频率", "波形幅值", "波形偏移量", "采集通道", "采样率", "显示窗口读取点数", "量程上限", "量程下限"};
        for (int i = 1; i <= 11; ++i) {
            auto* caption = w.findChild<QLabel*>("parameterLabel" + QString::number(i)); QVERIFY(caption);
            QCOMPARE(caption->text(), names[i - 1]); QVERIFY(caption->buddy()); QCOMPARE(caption->buddy()->accessibleName(), names[i - 1]);
        }
        QVERIFY(!w.findChild<QLabel*>("parameterLabel12")); QVERIFY(!w.runtime()->active());
        QVERIFY(!w.findChild<QPushButton*>("startButton")->isEnabled());
        QJsonArray geometry;
        for (const auto size : {QSize(1920,1080), QSize(1600,900), QSize(1440,900), QSize(1366,768), QSize(2560,1440)}) {
            w.resize(size); tabs->setCurrentIndex(1); QTest::qWait(60);
            QCOMPARE(w.centralWidget()->size(), size);
            auto* workspace = w.findChild<QWidget*>("workspace"); auto* left = w.findChild<QWidget*>("leftPane");
            QVERIFY(std::abs(left->width() - workspace->width() * 0.22) <= 1); QVERIFY(workspace->width() >= 1440);
            auto* raw = w.findChild<QWidget*>("rawCard");
            geometry.append(QJsonObject{{"requestedWidth", size.width()}, {"requestedHeight", size.height()},
                {"actualWidth", w.centralWidget()->width()}, {"actualHeight", w.centralWidget()->height()},
                {"workspaceWidth", workspace->width()}, {"leftWidth", left->width()},
                {"rightWidth", w.findChild<QWidget*>("rightPane")->width()}, {"devicePixelRatio", w.devicePixelRatioF()},
                {"chartWidth", raw->width()}, {"chartHeight", raw->height()}});
            for (const auto* name : {"fitCard", "fvCard", "peakCard"}) {
                auto* card = w.findChild<QWidget*>(name); QVERIFY(std::abs(card->width() - raw->width()) <= 1); QVERIFY(std::abs(card->height() - raw->height()) <= 1);
            }
            QVERIFY(raw->x() < w.findChild<QWidget*>("fitCard")->x());
            QVERIFY(raw->y() < w.findChild<QWidget*>("fvCard")->y());
            QVERIFY(w.findChild<QWidget*>("fvCard")->x() < w.findChild<QWidget*>("peakCard")->x());
            QVERIFY(w.grab().save(output + QString("/debug-%1x%2.png").arg(size.width()).arg(size.height())));
        }
        QFile geometryFile(output + "/geometry.json"); QVERIFY(geometryFile.open(QIODevice::WriteOnly)); geometryFile.write(QJsonDocument(geometry).toJson());
        w.resize(1920,1080); tabs->setCurrentIndex(0); QTest::qWait(60); QVERIFY(w.grab().save(output + "/customer-day.png"));
        w.setNightTheme(true); tabs->setCurrentIndex(1); QTest::qWait(60); QVERIFY(w.grab().save(output + "/debug-night.png"));
        tabs->setCurrentIndex(3); QTest::qWait(60); QVERIFY(w.grab().save(output + "/camera-night.png")); w.setNightTheme(false);
        tabs->setCurrentIndex(2); QTest::qWait(60); QVERIFY(w.grab().save(output + "/history-empty-day.png"));
        tabs->setCurrentIndex(3); QTest::qWait(60); QVERIFY(w.grab().save(output + "/camera-day.png"));
    }
    void simulationAndSafeClose() {
        MainWindow w(qEnvironmentVariable("TDLAS_TEST_ARTIFACTS") + "/test-settings"); w.show();
        w.findChild<QComboBox*>("deviceSelector")->setCurrentIndex(1);
        auto* start = w.findChild<QPushButton*>("startButton"); QVERIFY(start->isEnabled()); start->click();
        QTRY_COMPARE_WITH_TIMEOUT(w.runtime()->state(), RunState::Running, 2000);
        QVERIFY(!start->isEnabled()); QTRY_VERIFY_WITH_TIMEOUT(w.runtime()->snapshot().stats.processedBlocks > 0, 2000);
        w.findChild<QTabWidget*>("mainTabs")->setCurrentIndex(1); QVERIFY(w.runtime()->active());
        w.close(); QTRY_VERIFY_WITH_TIMEOUT(!w.runtime()->active(), 3000); QTRY_VERIFY(!w.isVisible());
    }
    void slowProcessingCloseRemainsResponsive() {
        const QString output = qEnvironmentVariable("TDLAS_TEST_ARTIFACTS", "artifacts"); QDir().mkpath(output);
        MainWindow w(output + "/step5-settings"); w.show();
        AcquisitionConfig c; c.deviceId = "fixture"; c.inputChannel = "fixture/ai"; c.queueCapacitySamples = 3000;
        std::atomic_bool entered{false}, release{false}; AtomicReleaseGuard releaseOnExit{release};
        int ticks = 0; QTimer eventTimer; eventTimer.setInterval(1); connect(&eventTimer, &QTimer::timeout, this, [&] { ++ticks; }); eventTimer.start();
        QVERIFY(w.runtime()->start(c, std::make_unique<ResultTestSource>(3), [&](const SampleBlock&) {
            entered = true;
            while (!release.load()) std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }));
        QTRY_VERIFY_WITH_TIMEOUT(entered.load(), 1000);
        QTRY_VERIFY_WITH_TIMEOUT(w.runtime()->shutdownSnapshot().inputClosed, 1000);
        w.close(); QTest::qWait(100);
        const bool stayedVisible = w.isVisible(); const bool eventLoopAdvanced = ticks > 0;
        const QString diagnostics = w.findChild<QLabel*>("runtimeDiagnostics")->text();
        const bool showedInFlight = diagnostics.contains("正在处理 1 块") && diagnostics.contains("正在排空");
        const bool screenshotSaved = w.grab().save(output + "/slow-close-draining.png");
        release = true;
        QTRY_VERIFY_WITH_TIMEOUT(!w.runtime()->active(), 2000); QTRY_VERIFY_WITH_TIMEOUT(!w.isVisible(), 1000);
        const auto shutdown = w.runtime()->shutdownSnapshot();
        QVERIFY(stayedVisible); QVERIFY(eventLoopAdvanced); QVERIFY(showedInFlight); QVERIFY(screenshotSaved);
        QVERIFY(shutdown.sourceClosedOrder < shutdown.inputCloseOrder);
        QVERIFY(shutdown.inputCloseOrder < shutdown.processingDrainedOrder);
        QVERIFY(w.runtime()->snapshot().stats.terminalComplete());
    }
};
QTEST_MAIN(UiTest)
#include "ui_test.moc"
