#include "camera/camera_recorder.h"

#include <QApplication>
#include <QDebug>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QMediaPlayer>
#include <QTemporaryDir>
#include <QThread>
#include <QVideoFrame>
#include <QVideoSink>
#include <functional>
#include <cstdio>

namespace {
bool check(bool condition, const char *message)
{
    if (!condition) qCritical() << message;
    return condition;
}
bool waitFor(const std::function<bool()> &predicate, int timeout = 10000)
{
    QElapsedTimer timer;
    timer.start();
    while (!predicate() && timer.elapsed() < timeout) {
        QApplication::processEvents();
        QThread::msleep(5);
    }
    return predicate();
}
void feed(tdlas::CameraRecorder &recorder, const QImage &image, int milliseconds, int rotation = 0)
{
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < milliseconds) {
        recorder.append(image, {}, rotation);
        QElapsedTimer step;
        step.start();
        while (step.elapsed() < 50) {
            QApplication::processEvents();
            QThread::msleep(2);
        }
    }
}
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    qInstallMessageHandler([](QtMsgType, const QMessageLogContext &, const QString &message) {
        const QByteArray text = message.toUtf8();
        std::fprintf(stderr, "%s\n", text.constData());
    });
    QTemporaryDir output;
    tdlas::CameraRecorder recorder;
    QString error;
    QString recordingError;
    int saved = 0;
    QObject::connect(&recorder, &tdlas::CameraRecorder::finished, &app,
        [&](const QString &, const QString &message) { recordingError = message; ++saved; });
    const QString path = output.filePath(QStringLiteral("录像.mp4"));
    QImage image(1920, 1080, QImage::Format_RGB32);
    image.fill(Qt::red);
    for (int y = 0; y < image.height(); ++y)
        for (int x = image.width() / 2; x < image.width(); ++x) image.setPixelColor(x, y, Qt::blue);
    if (!check(!recorder.start(output.filePath(QStringLiteral("missing/video.mp4")), image.size(), 20, &error),
               "Invalid video directory must fail")
        || !check(recorder.start(path, image.size(), 20, &error), qPrintable(error))) return 1;
    feed(recorder, image, 1000);
    recorder.pause();
    const qint64 pausedDuration = recorder.duration();
    QImage green(image.size(), QImage::Format_RGB32);
    green.fill(Qt::green);
    feed(recorder, green, 1000);
    if (!check(recorder.duration() == pausedDuration, "Pause must not increase recorded duration")) return 1;
    recorder.resume();
    feed(recorder, image, 1000, 90);
    recorder.stop();
    if (!check(waitFor([&]() { return recorder.state() == tdlas::CameraRecorder::State::Idle; }), "Video did not finish")
        || !check(saved == 1 && recordingError.isEmpty(), qPrintable(recordingError))
        || !check(QFileInfo(path).size() > 1000, "Video was not saved on stop")) return 1;
    if (!check(!recorder.start(path, image.size(), 20, &error), "Existing recordings must not be overwritten")) return 1;

    QMediaPlayer player;
    QVideoSink sink;
    player.setVideoSink(&sink);
    bool originalSeen = false;
    bool rotatedSeen = false;
    bool greenSeen = false;
    QObject::connect(&sink, &QVideoSink::videoFrameChanged, &app, [&](const QVideoFrame &frame) {
        const QImage decoded = frame.toImage();
        if (decoded.isNull()) return;
        const QColor top = decoded.pixelColor(960, 270);
        const QColor bottom = decoded.pixelColor(960, 810);
        const QColor left = decoded.pixelColor(480, 540);
        const QColor right = decoded.pixelColor(1440, 540);
        originalSeen = originalSeen || (left.red() > 180 && right.blue() > 180);
        rotatedSeen = rotatedSeen || (top.red() > 180 && bottom.blue() > 180);
        greenSeen = greenSeen || (top.green() > 180 && top.red() < 70 && top.blue() < 70);
    });
    player.setSource(QUrl::fromLocalFile(path));
    player.play();
    if (!check(waitFor([&]() { return player.mediaStatus() == QMediaPlayer::EndOfMedia
        || player.error() != QMediaPlayer::NoError; }), "Saved MP4 did not play to the end")
        || !check(player.error() == QMediaPlayer::NoError, qPrintable(player.errorString()))
        || !check(player.duration() >= 1500 && player.duration() < 2800, "Pause leaked into the saved video timeline")
        || !check(originalSeen && rotatedSeen && !greenSeen, "Live transforms or pause were not preserved in the video")) return 1;
    player.stop();
    player.setSource({});
    const QString second = output.filePath(QStringLiteral("second.mp4"));
    if (!check(recorder.start(second, image.size(), 20, &error), qPrintable(error))) return 1;
    feed(recorder, image, 500);
    recorder.pause();
    if (!check(recorder.stopAndWait(), "Stopping a paused recording did not finish")
        || !check(saved == 2 && QFileInfo(second).size() > 1000, "Second recording was not saved")) return 1;
    qInfo() << "Recording, pause/resume timeline, live rotation, MP4 playback and save passed";
    return 0;
}
