#pragma once

#include <QElapsedTimer>
#include <QImage>
#include <QObject>
#include <QPoint>

class QMediaCaptureSession;
class QMediaRecorder;
class QVideoFrameInput;

namespace tdlas {

// Streams bounded video frames to Qt's encoder. Pauses omit frames and elapsed
// time together, so resuming continues the same file without a silent gap.
class CameraRecorder final : public QObject {
    Q_OBJECT
public:
    enum class State { Idle, Recording, Paused, Finalizing };
    Q_ENUM(State)
    explicit CameraRecorder(QObject *parent = nullptr);
    ~CameraRecorder() override;
    bool start(const QString &path, QSize size, int frameRate, QString *error);
    void append(const QImage &image, const QPoint &offsetPercent, int rotationDegrees);
    void pause();
    void resume();
    void stop();
    bool stopAndWait();
    State state() const { return m_state; }
    qint64 duration() const;
    QString filePath() const { return m_path; }

signals:
    void stateChanged();
    void finished(const QString &path, const QString &error);

private:
    void finalize();
    QMediaCaptureSession *m_session = nullptr;
    QMediaRecorder *m_recorder = nullptr;
    QVideoFrameInput *m_input = nullptr;
    State m_state = State::Idle;
    QSize m_size;
    int m_frameRate = 20;
    qint64 m_activeUs = 0;
    qint64 m_lastTimestamp = -1;
    QElapsedTimer m_clock;
    QString m_path;
    QString m_error;
};

} // namespace tdlas
