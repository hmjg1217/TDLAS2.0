#pragma once

#include "camera/uvc_controls.h"
#include "camera/camera_recorder.h"

#include <QCameraDevice>
#include <QDialog>
#include <QElapsedTimer>
#include <QImage>
#include <QVideoFrame>
#include <array>

class QCamera;
class QCheckBox;
class QComboBox;
class QLabel;
class QMediaCaptureSession;
class QMediaDevices;
class QPushButton;
class QSlider;
class QSpinBox;
class QTimer;
class QVideoSink;
class CameraImageView;

class CameraWindow final : public QDialog {
    Q_OBJECT
public:
    explicit CameraWindow(QWidget *parent = nullptr);
    ~CameraWindow() override;
    void startPreview();
    void stopPreview();
    void reject() override;

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    struct ControlRow {
        tdlas::CameraControl control;
        QSlider *slider = nullptr;
        QSpinBox *value = nullptr;
        QCheckBox *automatic = nullptr;
        tdlas::CameraControlRange range;
    };
    void buildUi();
    void refreshDevices();
    void updateFormats();
    void updateControls();
    void syncControl(ControlRow &row);
    void applyControl(ControlRow &row);
    void pollFrame();
    void capturePhoto();
    void savePhoto();
    void startRecording();
    void updateRecordingUi();
    void updateTransform();
    void setStatus(const QString &message, bool error = false);

    QMediaDevices *m_mediaDevices = nullptr;
    QMediaCaptureSession *m_session = nullptr;
    QVideoSink *m_sink = nullptr;
    QCamera *m_camera = nullptr;
    QTimer *m_frameTimer = nullptr;
    QTimer *m_startTimeout = nullptr;
    tdlas::UvcControls m_controls;
    tdlas::CameraRecorder *m_recorder = nullptr;
    QList<QCameraDevice> m_devices;
    QList<QCameraFormat> m_formats;
    QComboBox *m_deviceCombo = nullptr;
    QComboBox *m_formatCombo = nullptr;
    QPushButton *m_startButton = nullptr;
    QPushButton *m_stopButton = nullptr;
    QPushButton *m_captureButton = nullptr;
    QPushButton *m_saveButton = nullptr;
    QPushButton *m_defaultsButton = nullptr;
    QPushButton *m_recordStart = nullptr;
    QPushButton *m_recordPause = nullptr;
    QPushButton *m_recordStop = nullptr;
    QPushButton *m_videoDirectoryButton = nullptr;
    QLabel *m_status = nullptr;
    QLabel *m_controlStatus = nullptr;
    QLabel *m_photoPreview = nullptr;
    QLabel *m_photoInfo = nullptr;
    QLabel *m_saveDirectory = nullptr;
    QLabel *m_videoDirectoryLabel = nullptr;
    QLabel *m_recordStatus = nullptr;
    QSpinBox *m_horizontal = nullptr;
    QSpinBox *m_vertical = nullptr;
    QSpinBox *m_rotation = nullptr;
    CameraImageView *m_view = nullptr;
    std::array<ControlRow, 9> m_rows;
    QVideoFrame m_lastFrame;
    QElapsedTimer m_frameAge;
    QImage m_source;
    QImage m_photograph;
    QString m_directory;
    QString m_videoDirectory;
    QString m_recordMessage;
    bool m_refreshing = false;
    bool m_stoppingPreview = false;
};
