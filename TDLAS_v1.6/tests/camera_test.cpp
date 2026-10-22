#include "camera/camera_image.h"
#include "camera/uvc_controls.h"
#include "ui/camera_window.h"
#include "ui/main_window.h"

#include <QApplication>
#include <QCameraDevice>
#include <QCheckBox>
#include <QComboBox>
#include <QDebug>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileDialog>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QMediaDevices>
#include <QMediaPlayer>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QTemporaryDir>
#include <QThread>
#include <QTimer>
#include <QVideoSink>
#include <functional>
#include <cstdio>

namespace {
bool require(bool condition, const char *message)
{
    if (!condition) qCritical() << message;
    return condition;
}

bool waitUntil(const std::function<bool()> &condition, int timeout = 12000)
{
    QElapsedTimer elapsed;
    elapsed.start();
    while (!condition() && elapsed.elapsed() < timeout) {
        QApplication::processEvents(QEventLoop::AllEvents, 20);
        QThread::msleep(5);
    }
    QApplication::processEvents();
    return condition();
}

bool selectSavePath(QPushButton *button, const QString &path)
{
    bool selected = false;
    QTimer selector;
    selector.setInterval(10);
    QObject::connect(&selector, &QTimer::timeout, &selector, [&]() {
        for (auto *widget : QApplication::topLevelWidgets()) {
            if (auto *dialog = qobject_cast<QFileDialog *>(widget)) {
                if (!dialog->isVisible()) continue;
                if (dialog->fileMode() == QFileDialog::Directory) dialog->setDirectory(path);
                else dialog->selectFile(path);
                selected = QMetaObject::invokeMethod(dialog, "accept", Qt::QueuedConnection);
                selector.stop();
                return;
            }
        }
    });
    QTimer timeout;
    timeout.setSingleShot(true);
    QObject::connect(&timeout, &QTimer::timeout, &timeout, []() {
        for (auto *widget : QApplication::topLevelWidgets())
            if (auto *dialog = qobject_cast<QFileDialog *>(widget)) dialog->reject();
    });
    selector.start();
    timeout.start(5000);
    button->click();
    return selected;
}

void processFor(int milliseconds)
{
    QElapsedTimer timer;
    timer.start();
    waitUntil([&]() { return timer.elapsed() >= milliseconds; }, milliseconds + 1000);
}

bool directoryTests(CameraWindow &window)
{
    QTemporaryDir photos;
    QTemporaryDir videos;
    if (!require(selectSavePath(window.findChild<QPushButton *>(QStringLiteral("cameraDirectoryButton")), photos.path()),
                 "Photo directory selection failed")
        || !require(selectSavePath(window.findChild<QPushButton *>(QStringLiteral("cameraVideoDirectoryButton")), videos.path()),
                    "Video directory selection failed")) return false;
    QSettings().sync();
    CameraWindow reopened;
    return require(QDir::fromNativeSeparators(reopened.findChild<QLabel *>(QStringLiteral("cameraPhotoDirectory"))->text()) == photos.path(),
                   "Photo directory was not remembered")
        && require(QDir::fromNativeSeparators(reopened.findChild<QLabel *>(QStringLiteral("cameraVideoDirectory"))->text()) == videos.path(),
                   "Video directory was not remembered independently");
}

bool playableVideo(const QString &path, qint64 minimumDuration)
{
    QMediaPlayer player;
    QVideoSink sink;
    player.setVideoSink(&sink);
    bool decoded = false;
    QObject::connect(&sink, &QVideoSink::videoFrameChanged, &sink,
        [&](const QVideoFrame &frame) { decoded = decoded || !frame.toImage().isNull(); });
    player.setSource(QUrl::fromLocalFile(path));
    player.play();
    const bool valid = waitUntil([&]() { return decoded || player.error() != QMediaPlayer::NoError; })
        && require(player.error() == QMediaPlayer::NoError, qPrintable(player.errorString()))
        && require(decoded && player.duration() >= minimumDuration, "Recorded camera MP4 is not playable or is too short");
    qInfo() << "Saved camera video:" << path << "duration ms:" << player.duration() << "bytes:" << QFileInfo(path).size();
    player.stop();
    player.setSource({});
    return valid;
}

bool imageTests()
{
    QTemporaryDir output;
    QImage image(40, 20, QImage::Format_RGB32);
    image.fill(Qt::red);
    for (int y = 0; y < 20; ++y)
        for (int x = 20; x < 40; ++x) image.setPixelColor(x, y, Qt::blue);
    const QImage identity = tdlas::cameraPhotograph(image, {}, 0);
    if (!require(identity == image, "Identity transform changes image pixels")) return false;
    const QImage rotated = tdlas::cameraPhotograph(image, {}, 90);
    if (!require(rotated.size() == QSize(20, 40), "90-degree rotation must swap dimensions")
        || !require(rotated.pixelColor(10, 5) == QColor(Qt::red)
            && rotated.pixelColor(10, 35) == QColor(Qt::blue), "Clockwise rotation has wrong orientation")) return false;
    const QImage translated = tdlas::cameraPhotograph(image, QPoint(25, 0), 0);
    if (!require(translated.pixelColor(5, 10) == QColor(Qt::black), "Translation must fill empty canvas with black")
        || !require(translated.pixelColor(15, 10) == QColor(Qt::red), "Horizontal translation has wrong distance")
        || !require(tdlas::cameraPhotograph(image, QPoint(0, -100), 0).pixelColor(10, 10) == QColor(Qt::black),
                    "Vertical translation did not move image off canvas")) return false;
    const QImage arbitrary = tdlas::cameraPhotograph(image, {}, 45);
    if (!require(arbitrary.width() > image.width() && arbitrary.height() > image.height(),
                 "Arbitrary rotation must preserve the rotated bounds")) return false;
    QString error;
    for (const auto &extension : {QStringLiteral("png"), QStringLiteral("jpg"), QStringLiteral("bmp")}) {
        const QString file = output.filePath(QStringLiteral("拍照.") + extension);
        if (!require(tdlas::saveCameraPhotograph(rotated, file, &error), qPrintable(error))
            || !require(QImage(file).size() == rotated.size(), "Saved photograph dimensions changed")) return false;
    }
    const QString preserved = output.filePath(QStringLiteral("existing.png"));
    if (!tdlas::saveCameraPhotograph(image, preserved, &error)) return false;
    if (!require(!tdlas::saveCameraPhotograph({}, preserved, &error), "Empty capture should fail")
        || !require(QImage(preserved) == image, "Failed save damaged the existing photograph")
        || !require(!tdlas::saveCameraPhotograph(image, output.filePath(QStringLiteral("missing/photo.png")), &error),
                    "Invalid destination should fail")
        || !require(!tdlas::saveCameraPhotograph(image, output.filePath(QStringLiteral("photo.txt")), &error),
                    "Unsupported image extension should fail")) return false;
    return true;
}

bool hardwareSmoke(const QString &directory)
{
    QDir().mkpath(directory);
    CameraWindow window;
    window.show();
    auto *devices = window.findChild<QComboBox *>(QStringLiteral("cameraDeviceCombo"));
    int selected = -1;
    for (int i = 0; i < devices->count(); ++i)
        if (devices->itemText(i).contains(QStringLiteral("4K ZOOM"), Qt::CaseInsensitive)) selected = i;
    if (!require(selected >= 0, "Requested external 4K ZOOM camera not found")) return false;
    devices->setCurrentIndex(selected);
    tdlas::UvcControls controls;
    QString error;
    if (!require(controls.open(devices->currentData().toByteArray(), devices->currentText(), &error),
                 qPrintable(error))) return false;
    QJsonArray ranges;
    const std::array<tdlas::CameraControl, 9> properties{{tdlas::CameraControl::Focus,
        tdlas::CameraControl::Zoom, tdlas::CameraControl::Brightness, tdlas::CameraControl::Contrast,
        tdlas::CameraControl::Hue, tdlas::CameraControl::Sharpness, tdlas::CameraControl::Saturation,
        tdlas::CameraControl::Gamma, tdlas::CameraControl::WhiteBalance}};
    const QStringList names{QStringLiteral("focus"), QStringLiteral("zoom"), QStringLiteral("brightness"),
        QStringLiteral("contrast"), QStringLiteral("hue"), QStringLiteral("sharpness"),
        QStringLiteral("saturation"), QStringLiteral("gamma"), QStringLiteral("whiteBalance")};
    for (size_t i = 0; i < properties.size(); ++i) {
        const auto range = controls.range(properties[i]);
        ranges.append(QJsonObject{{QStringLiteral("control"), names.at(i)},
            {QStringLiteral("supported"), range.supported}, {QStringLiteral("minimum"), range.minimum},
            {QStringLiteral("maximum"), range.maximum}, {QStringLiteral("step"), range.step},
            {QStringLiteral("current"), range.value}, {QStringLiteral("autoSupported"), range.autoSupported},
            {QStringLiteral("automatic"), range.automatic}});
    }
    controls.close();
    QFile report(QDir(directory).filePath(QStringLiteral("camera_capabilities.json")));
    if (!report.open(QIODevice::WriteOnly)) return false;
    report.write(QJsonDocument(QJsonObject{{QStringLiteral("device"), devices->currentText()},
        {QStringLiteral("deviceId"), QString::fromUtf8(devices->currentData().toByteArray())},
        {QStringLiteral("controls"), ranges}}).toJson());
    report.close();
    auto *formats = window.findChild<QComboBox *>(QStringLiteral("cameraFormatCombo"));
    qInfo() << "Preview format:" << formats->currentText();
    QFile formatsReport(QDir(directory).filePath(QStringLiteral("camera_formats.txt")));
    if (formatsReport.open(QIODevice::WriteOnly)) {
        for (int i = 0; i < formats->count(); ++i) {
            formatsReport.write(formats->itemText(i).toUtf8() + '\n');
        }
    }
    window.startPreview();
    auto *capture = window.findChild<QPushButton *>(QStringLiteral("cameraCaptureButton"));
    auto *save = window.findChild<QPushButton *>(QStringLiteral("cameraSaveButton"));
    auto *status = window.findChild<QLabel *>(QStringLiteral("cameraStatus"));
    if (!waitUntil([&]() { return capture->isEnabled(); })) {
        qCritical() << status->text();
        return false;
    }
    qInfo() << status->text();
    auto *recordStart = window.findChild<QPushButton *>(QStringLiteral("cameraRecordStart"));
    auto *recordPause = window.findChild<QPushButton *>(QStringLiteral("cameraRecordPause"));
    auto *recordStop = window.findChild<QPushButton *>(QStringLiteral("cameraRecordStop"));
    auto *recorder = window.findChild<tdlas::CameraRecorder *>();
    const QString videoDirectory = QDir(directory).absolutePath();
    if (!require(selectSavePath(window.findChild<QPushButton *>(QStringLiteral("cameraVideoDirectoryButton")), videoDirectory),
                 "Video directory selection failed")) return false;
    QString recordingError;
    int savedVideos = 0;
    QObject::connect(recorder, &tdlas::CameraRecorder::finished, &window,
        [&](const QString &path, const QString &message) {
            recordingError = message;
            ++savedVideos;
            std::fprintf(stderr, "Camera recording finished: %s error: %s\n", path.toUtf8().constData(), message.toUtf8().constData());
        });
    QStringList controlNames;
    for (const QString &name : names) controlNames << name + QStringLiteral("Value") << name + QStringLiteral("Slider");
    controlNames << QStringLiteral("focusAuto") << QStringLiteral("whiteBalanceAuto")
        << QStringLiteral("cameraHorizontal") << QStringLiteral("cameraVertical") << QStringLiteral("cameraRotation")
        << QStringLiteral("cameraResetImage") << QStringLiteral("cameraCaptureButton");
    QList<bool> enabledBefore;
    for (const QString &name : controlNames) enabledBefore << window.findChild<QWidget *>(name)->isEnabled();
    auto controlsUnchanged = [&]() {
        for (int i = 0; i < controlNames.size(); ++i)
            if (window.findChild<QWidget *>(controlNames.at(i))->isEnabled() != enabledBefore.at(i)) return false;
        return true;
    };
    recordStart->click();
    const QString firstVideo = recorder->filePath();
    if (!require(recorder->state() == tdlas::CameraRecorder::State::Recording && recordPause->isEnabled() && recordStop->isEnabled(),
                 "Start recording did not enter the recording state")
        || !require(controlsUnchanged(), "Recording disabled camera/image controls")) return false;
    processFor(10000);
    // A real change through the UI, then restore the original hardware value.
    auto *brightness = window.findChild<QSpinBox *>(QStringLiteral("brightnessValue"));
    if (brightness->isEnabled() && brightness->maximum() > brightness->minimum()) {
        const int original = brightness->value();
        const int changed = original < brightness->maximum()
            ? original + brightness->singleStep() : original - brightness->singleStep();
        brightness->setValue(changed);
        const bool applied = brightness->value() == changed;
        brightness->setValue(original);
        if (!require(applied && brightness->value() == original, "UVC brightness write/readback failed")) return false;
    }
    recordPause->click();
    const qint64 pausedDuration = recorder->duration();
    processFor(1500);
    if (!require(recorder->state() == tdlas::CameraRecorder::State::Paused && recorder->duration() == pausedDuration,
                 "Paused camera recording continued its clock")
        || !require(controlsUnchanged(), "Pausing disabled camera/image controls")) return false;
    recordPause->click();
    if (!require(recorder->state() == tdlas::CameraRecorder::State::Recording && recorder->filePath() == firstVideo,
                 "Resume must continue the same video")) return false;
    window.grab().save(QDir(directory).filePath(QStringLiteral("camera_window.png")));
    window.findChild<QPushButton *>(QStringLiteral("cameraRotateRight"))->click();
    window.findChild<QPushButton *>(QStringLiteral("cameraMoveRight"))->click();
    capture->click();
    // Later preview edits must not change the already captured photograph.
    window.findChild<QPushButton *>(QStringLiteral("cameraResetTransform"))->click();
    const QString photograph = QDir(directory).filePath(QStringLiteral("camera_smoke.png"));
    if (!require(selectSavePath(save, photograph), "Photo save dialog failed")
        || !require(QImage(photograph).height() > QImage(photograph).width(), "Captured rotation was not preserved on save")) return false;
    window.findChild<QSpinBox *>(QStringLiteral("cameraRotation"))->setValue(90);
    processFor(20000);
    recordStop->click();
    if (!require(waitUntil([&]() { return recorder->state() == tdlas::CameraRecorder::State::Idle; }), "Stop did not finish video")
        || !require(savedVideos == 1 && recordingError.isEmpty(), qPrintable(recordingError))
        || !require(playableVideo(firstVideo, 28000), "Camera recording did not play")) return false;
    recordStart->click();
    const QString secondVideo = recorder->filePath();
    processFor(1500);
    recordPause->click();
    window.close();
    if (!require(!capture->isEnabled(), "Closing camera window must stop capture")
        || !require(savedVideos == 2 && recordingError.isEmpty() && recorder->state() == tdlas::CameraRecorder::State::Idle,
                    "Closing a paused recording did not finish saving")
        || !require(playableVideo(secondVideo, 800), "Closing camera window lost the video")) return false;
    window.show();
    window.startPreview();
    if (!require(waitUntil([&]() { return capture->isEnabled(); }), "Camera did not reopen after close")) return false;
    window.reject();
    if (!require(!capture->isEnabled() && !window.isVisible(), "Escape/reject did not release the camera")) return false;
    MainWindow main;
    main.show();
    QApplication::processEvents();
    main.grab().save(QDir(directory).filePath(QStringLiteral("main_window.png")));
    main.findChild<QPushButton *>(QStringLiteral("cameraButton"))->click();
    auto *childCamera = main.findChild<CameraWindow *>();
    if (!require(childCamera && childCamera->isVisible(), "Main camera entry did not open a camera window")) return false;
    auto *childCapture = childCamera->findChild<QPushButton *>(QStringLiteral("cameraCaptureButton"));
    if (!require(waitUntil([&]() { return childCapture->isEnabled(); }), "Camera entry did not start preview")) return false;
    main.close();
    if (!require(!childCapture->isEnabled(), "Closing main window did not release its camera")) return false;
    qInfo() << "Hardware preview, live controls during recording, pause/resume, photo/video save, close-save and reopen passed";
    return true;
}
} // namespace

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    app.setAttribute(Qt::AA_DontUseNativeDialogs);
    app.setOrganizationName(QStringLiteral("TDLAS-tests"));
    app.setApplicationName(QStringLiteral("camera-test"));
    QTemporaryDir settings;
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settings.path());
    if (app.arguments().contains(QStringLiteral("--hardware"))) {
        if (argc < 3) return 1;
        return hardwareSmoke(QString::fromLocal8Bit(argv[2])) ? 0 : 1;
    }
    if (!imageTests()) return 1;
    CameraWindow window;
    if (!require(!window.findChild<QPushButton *>(QStringLiteral("cameraCaptureButton"))->isEnabled(),
                 "A newly created window must not capture without a live frame")
        || !require(!window.findChild<QPushButton *>(QStringLiteral("cameraSaveButton"))->isEnabled(),
                    "Saving must require a captured photograph")) return 1;
    for (const auto *name : {"cameraRecordStart", "cameraRecordPause", "cameraRecordStop"})
        if (!require(!window.findChild<QPushButton *>(QString::fromLatin1(name))->isEnabled(),
                     "Recording controls must be disabled without a live frame")) return 1;
    if (!directoryTests(window)) return 1;
    window.findChild<QPushButton *>(QStringLiteral("cameraMoveLeft"))->click();
    if (!require(window.findChild<QSpinBox *>(QStringLiteral("cameraHorizontal"))->value() == -2,
                 "Left translation control failed")) return 1;
    window.findChild<QPushButton *>(QStringLiteral("cameraRotateRight"))->click();
    if (!require(window.findChild<QSpinBox *>(QStringLiteral("cameraRotation"))->value() == 90,
                 "Clockwise rotation control failed")) return 1;
    window.findChild<QPushButton *>(QStringLiteral("cameraResetTransform"))->click();
    if (!require(window.findChild<QSpinBox *>(QStringLiteral("cameraHorizontal"))->value() == 0
        && window.findChild<QSpinBox *>(QStringLiteral("cameraRotation"))->value() == 0,
                 "Transform reset control failed")) return 1;
    MainWindow main;
    if (!require(main.windowTitle().contains(QStringLiteral("v1.6")), "Main window version is not 1.6")
        || !require(main.findChild<QPushButton *>(QStringLiteral("cameraButton")) != nullptr,
                    "Camera entry button is missing")) return 1;
    qInfo() << "Camera image, save failures and UI tests passed without starting hardware";
    return 0;
}
