#include "camera/camera_session.h"

#include <QtTest>
#include <QDir>
#include <QFileInfo>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTransform>

using namespace tdlas;

class CameraTest final : public QObject {
    Q_OBJECT
private slots:
    void syntheticPreviewTransformAndStill() {
        QTemporaryDir root; QVERIFY(root.isValid());
        CameraSettings settings;
        settings.resolution = QSize(2, 1); settings.frameRate = 10.0;
        settings.mirrorPreview = true; settings.rotationDegrees = 90;
        settings.outputDirectory = QDir(root.path()).filePath("media");
        CameraSession session(root.path());
        QSignalSpy frames(&session, &CameraSession::frameReady);
        QSignalSpy saved(&session, &CameraSession::mediaSaved);
        QVERIFY(session.startSynthetic(settings));
        QCOMPARE(session.state(), CameraState::Previewing);
        QImage source(2, 1, QImage::Format_ARGB32);
        source.setPixelColor(0, 0, Qt::red); source.setPixelColor(1, 0, Qt::blue);
        QVERIFY(session.submitSyntheticFrame(source, 0));
        QCOMPARE(frames.count(), 1);
        const QImage expected = source.mirrored(true, false).transformed(QTransform().rotate(90));
        QCOMPARE(session.latestFrame(), expected);
        QVERIFY(session.captureStill()); QCOMPARE(saved.count(), 1);
        QVERIFY(QFileInfo::exists(session.lastStillPath()));
        QCOMPARE(QImage(session.lastStillPath()), expected);
        session.stop(); QCOMPARE(session.state(), CameraState::Disconnected);
    }

    void digitalPanZoomAndImageTuning() {
        QTemporaryDir root; QVERIFY(root.isValid());
        CameraSettings settings; settings.resolution = QSize(4, 2); settings.outputDirectory = root.path();
        settings.digitalZoom = 2.0; settings.digitalPanX = 1.0;
        CameraSession session(root.path()); QVERIFY(session.startSynthetic(settings));
        QImage source(4, 2, QImage::Format_ARGB32); source.fill(Qt::red);
        for (int y = 0; y < source.height(); ++y) for (int x = 2; x < source.width(); ++x) source.setPixelColor(x, y, Qt::blue);
        QVERIFY(session.submitSyntheticFrame(source));
        for (int y = 0; y < session.latestFrame().height(); ++y)
            for (int x = 0; x < session.latestFrame().width(); ++x) QCOMPARE(session.latestFrame().pixelColor(x, y), QColor(Qt::blue));
        settings.brightness = 100; session.updateSettings(settings); QVERIFY(session.submitSyntheticFrame(source));
        for (int y = 0; y < session.latestFrame().height(); ++y)
            for (int x = 0; x < session.latestFrame().width(); ++x) QCOMPARE(session.latestFrame().pixelColor(x, y), QColor(Qt::white));
    }

    void syntheticRecordingDurationAndContainer() {
        QTemporaryDir root; QVERIFY(root.isValid());
        CameraSettings settings;
        settings.resolution = QSize(320, 240); settings.frameRate = 10.0;
        settings.outputDirectory = QDir(root.path()).filePath("media");
        CameraSession session(root.path());
        QSignalSpy saved(&session, &CameraSession::mediaSaved);
        QVERIFY(session.startSynthetic(settings));
        QVERIFY(session.startRecording());
        QTRY_COMPARE_WITH_TIMEOUT(session.state(), CameraState::Recording, 5000);
        for (int index = 0; index < 20; ++index) {
            QImage frame(settings.resolution, QImage::Format_ARGB32);
            frame.fill(QColor::fromHsv((index * 15) % 360, 220, 220));
            bool accepted = false;
            for (int retry = 0; retry < 50 && !accepted; ++retry) {
                accepted = session.submitSyntheticFrame(frame, index * 100000LL);
                if (!accepted) QTest::qWait(10);
            }
            QVERIFY2(accepted, "Qt recorder did not accept a synthetic frame in time");
            QTest::qWait(5);
        }
        session.stopRecording();
        QTRY_COMPARE_WITH_TIMEOUT(session.state(), CameraState::Previewing, 10000);
        QTRY_VERIFY_WITH_TIMEOUT(QFileInfo::exists(session.lastRecordingPath()), 10000);
        QVERIFY(QFileInfo(session.lastRecordingPath()).size() > 0);
        QCOMPARE(QFileInfo(session.lastRecordingPath()).suffix().toLower(), QString("mp4"));
        QCOMPARE(session.recordingContainer(), QString("MPEG-4"));
        QVERIFY(session.recordingDurationMs() >= 1000);
        QVERIFY(saved.count() >= 1);
        // Leaving a preview while recording must finalize the container before
        // the session reports that the device has been released.
        session.stop(); QCOMPARE(session.state(), CameraState::Disconnected);
        CameraSession directStopSession(root.path());
        QVERIFY(directStopSession.startSynthetic(settings));
        QVERIFY(directStopSession.startRecording());
        QTRY_COMPARE_WITH_TIMEOUT(directStopSession.state(), CameraState::Recording, 5000);
        QImage finalFrame(settings.resolution, QImage::Format_ARGB32); finalFrame.fill(Qt::darkBlue);
        for (int index = 0; index < 20; ++index) {
            bool accepted = false;
            for (int retry = 0; retry < 50 && !accepted; ++retry) {
                accepted = directStopSession.submitSyntheticFrame(finalFrame);
                if (!accepted) QTest::qWait(10);
            }
            QVERIFY(accepted);
            QTest::qWait(5);
        }
        directStopSession.stop();
        QVERIFY(directStopSession.state() == CameraState::Stopping || directStopSession.state() == CameraState::Disconnected);
        QTRY_COMPARE_WITH_TIMEOUT(directStopSession.state(), CameraState::Disconnected, 10000);
        QVERIFY(QFileInfo(directStopSession.lastRecordingPath()).size() > 0);
    }
};

QTEST_GUILESS_MAIN(CameraTest)
#include "camera_test.moc"

