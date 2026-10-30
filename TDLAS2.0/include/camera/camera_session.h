#pragma once

#include <QByteArray>
#include <QImage>
#include <QList>
#include <QObject>
#include <QSize>
#include <QString>
#include <memory>

class QCamera;
class QMediaCaptureSession;
class QMediaRecorder;
class QVideoFrame;
class QVideoFrameInput;
class QVideoSink;

namespace tdlas {

struct CameraFormatCapability {
    QSize resolution;
    double minimumFrameRate = 0.0;
    double maximumFrameRate = 0.0;
};

struct CameraDeviceCapability {
    QByteArray id;
    QString description;
    bool isDefault = false;
    QList<CameraFormatCapability> formats;
};

struct CameraSettings {
    QByteArray deviceId;
    QSize resolution = QSize(1280, 720);
    double frameRate = 30.0;
    bool mirrorPreview = false;
    int rotationDegrees = 0;
    double digitalPanX = 0.0;
    double digitalPanY = 0.0;
    double digitalZoom = 1.0;
    int brightness = 0;
    int contrast = 100;
    int saturation = 100;
    int hue = 0;
    int sharpness = 0;
    double gamma = 1.0;
    bool autoFocus = true;
    double focusDistance = 0.5;
    double hardwareZoom = 1.0;
    bool autoExposure = true;
    double exposureCompensation = 0.0;
    double manualExposureSeconds = 0.01;
    int manualIso = 100;
    bool autoWhiteBalance = true;
    int colorTemperature = 4500;
    QString outputDirectory;
};

struct CameraControlCapabilities {
    bool hardwareZoom = false;
    double minimumHardwareZoom = 1.0;
    double maximumHardwareZoom = 1.0;
    bool autoFocus = false;
    bool manualFocus = false;
    bool autoExposure = false;
    bool manualExposure = false;
    bool exposureCompensation = false;
    bool isoSensitivity = false;
    int minimumIso = -1;
    int maximumIso = -1;
    double minimumExposureSeconds = -1.0;
    double maximumExposureSeconds = -1.0;
    bool autoWhiteBalance = false;
    bool manualWhiteBalance = false;
};

enum class CameraState {
    Disconnected,
    Starting,
    Previewing,
    Recording,
    Stopping,
    Error
};

// Owns all Qt Multimedia objects for one camera session. The production path
// uses a QCamera. startSynthetic() is an explicit hardware-free path used by
// tests to exercise the same preview, still-image and recorder pipeline.
class CameraSession final : public QObject {
    Q_OBJECT
public:
    explicit CameraSession(QString dataRoot, QObject* parent = nullptr);
    ~CameraSession() override;

    static QList<CameraDeviceCapability> discoverDevices();

    bool start(const CameraSettings& settings);
    bool startSynthetic(const CameraSettings& settings);
    void updateSettings(const CameraSettings& settings);
    void stop();
    bool captureStill();
    bool startRecording();
    void stopRecording();
    bool submitSyntheticFrame(const QImage& frame, qint64 timestampMicroseconds = -1);

    CameraState state() const { return state_; }
    bool active() const;
    bool synthetic() const { return synthetic_; }
    CameraSettings settings() const { return settings_; }
    CameraControlCapabilities controlCapabilities() const;
    QImage latestFrame() const { return latestFrame_; }
    QString errorString() const { return errorString_; }
    QString lastStillPath() const { return lastStillPath_; }
    QString lastRecordingPath() const { return lastRecordingPath_; }
    qint64 recordingDurationMs() const { return recordingDurationMs_; }
    QString recordingContainer() const { return QStringLiteral("MPEG-4"); }

signals:
    void stateChanged(tdlas::CameraState state);
    void frameReady(const QImage& image);
    void recordingDurationChanged(qint64 milliseconds);
    void controlCapabilitiesChanged();
    void mediaSaved(const QString& path);
    void errorOccurred(const QString& message);

private:
    bool prepareSession(const CameraSettings& settings, bool synthetic);
    void acceptFrame(const QImage& image);
    QImage transformed(const QImage& image) const;
    QImage adjusted(const QImage& image) const;
    void applyHardwareSettings();
    QString nextOutputPath(const QString& prefix, const QString& suffix) const;
    void setState(CameraState state);
    void fail(const QString& message);
    void releaseObjects();
    void finalizeRecording(int remainingChecks);

    QString dataRoot_;
    CameraSettings settings_;
    CameraState state_ = CameraState::Disconnected;
    bool synthetic_ = false;
    QString errorString_;
    QImage latestFrame_;
    QImage latestRawFrame_;
    QString lastStillPath_;
    QString lastRecordingPath_;
    qint64 recordingDurationMs_ = 0;
    qint64 syntheticTimestampUs_ = 0;
    bool releasePending_ = false;
    bool finalizationPending_ = false;

    std::unique_ptr<QMediaCaptureSession> captureSession_;
    std::unique_ptr<QVideoSink> videoSink_;
    std::unique_ptr<QMediaRecorder> recorder_;
    std::unique_ptr<QCamera> camera_;
    std::unique_ptr<QVideoFrameInput> videoInput_;
};

}

