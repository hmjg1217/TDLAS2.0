#include "camera/camera_recorder.h"
#include "camera/camera_image.h"

#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QMediaCaptureSession>
#include <QMediaFormat>
#include <QMediaRecorder>
#include <QPainter>
#include <QStorageInfo>
#include <QTimer>
#include <QUrl>
#include <QVideoFrame>
#include <QVideoFrameFormat>
#include <QVideoFrameInput>
#include <QVideoSink>
#include <algorithm>

namespace tdlas {

CameraRecorder::CameraRecorder(QObject *parent) : QObject(parent) {}

CameraRecorder::~CameraRecorder()
{
    stopAndWait();
}

bool CameraRecorder::start(const QString &path, QSize size, int frameRate, QString *error)
{
    const auto reject = [&](const QString &message) {
        if (error) *error = message;
        return false;
    };
    if (m_state != State::Idle) return reject(QStringLiteral("请先停止当前录像并等待保存完成。"));
    if (size.width() < 2 || size.height() < 2 || frameRate < 1)
        return reject(QStringLiteral("没有有效的视频画面。"));
    if (QFileInfo(path).suffix().compare(QStringLiteral("mp4"), Qt::CaseInsensitive) != 0)
        return reject(QStringLiteral("录像文件需要使用 MP4 格式。"));
    QMediaFormat format(QMediaFormat::MPEG4);
    format.setVideoCodec(QMediaFormat::VideoCodec::H264);
    if (!format.isSupported(QMediaFormat::Encode))
        return reject(QStringLiteral("当前多媒体后端不支持 MP4/H.264 编码，请使用随程序发布的 FFmpeg 组件。"));
    QStorageInfo storage(QFileInfo(path).absolutePath());
    if (storage.isValid() && storage.bytesAvailable() >= 0 && storage.bytesAvailable() < 128 * 1024 * 1024)
        return reject(QStringLiteral("录像目录剩余空间不足 128 MB，请选择其他磁盘。"));
    // Reserve a new file exclusively; never overwrite an earlier recording.
    QFile target(path);
    if (!target.open(QIODevice::WriteOnly | QIODevice::NewOnly))
        return reject(QStringLiteral("无法创建录像文件：%1").arg(target.errorString()));
    target.close();

    delete m_session;
    delete m_input;
    delete m_recorder;
    m_path = QFileInfo(path).absoluteFilePath();
    m_error.clear();
    m_size = QSize(size.width() & ~1, size.height() & ~1);
    m_frameRate = frameRate;
    m_activeUs = 0;
    m_lastTimestamp = -1;
    m_session = new QMediaCaptureSession(this);
    m_recorder = new QMediaRecorder(this);
    // Let the frame input negotiate from the first actual image. This also
    // avoids rejecting QImage-backed frames delivered from UVC cameras.
    m_input = new QVideoFrameInput(this);
    m_session->setRecorder(m_recorder);
    m_session->setVideoFrameInput(m_input);
    // Qt's FFmpeg backend needs an output sink to activate this frame-input
    // session. The sink retains only the latest frame; it is not a frame queue.
    m_session->setVideoSink(new QVideoSink(m_session));
    m_recorder->setMediaFormat(format);
    m_recorder->setVideoResolution(m_size);
    m_recorder->setVideoFrameRate(frameRate);
    m_recorder->setEncodingMode(QMediaRecorder::AverageBitRateEncoding);
    const qint64 pixels = qint64(m_size.width()) * m_size.height();
    m_recorder->setVideoBitRate(int(std::clamp(pixels * 3, qint64(1000000), qint64(24000000))));
    m_recorder->setOutputLocation(QUrl::fromLocalFile(m_path));
    connect(m_recorder, &QMediaRecorder::errorOccurred, this,
        [this](QMediaRecorder::Error, const QString &message) {
            m_error = message;
            if (m_state != State::Idle) {
                stop();
                if (m_recorder->recorderState() == QMediaRecorder::StoppedState) finalize();
            }
        });
    connect(m_recorder, &QMediaRecorder::recorderStateChanged, this,
        [this](QMediaRecorder::RecorderState state) {
            if (state == QMediaRecorder::StoppedState && m_state != State::Idle) finalize();
        });
    m_state = State::Recording;
    m_clock.start();
    emit stateChanged();
    m_recorder->record();
    if (m_state == State::Idle) return reject(m_error);
    return true;
}

qint64 CameraRecorder::duration() const
{
    return (m_activeUs + (m_state == State::Recording ? m_clock.nsecsElapsed() / 1000 : 0)) / 1000;
}

void CameraRecorder::append(const QImage &image, const QPoint &offsetPercent, int rotationDegrees)
{
    if (m_state != State::Recording || image.isNull()) return;
    // No unbounded image queue: when the encoder is full, keep only the camera's
    // latest frame and try on the next live frame. Timestamps preserve real time.
    QImage canvas(m_size, QImage::Format_RGB32);
    canvas.fill(Qt::black);
    {
        QPainter painter(&canvas);
        paintCameraImage(painter, image, QRectF(canvas.rect()), offsetPercent, rotationDegrees);
    }
    QVideoFrame frame(canvas);
    const qint64 timestamp = m_lastTimestamp < 0 ? 0
        : std::max(m_lastTimestamp + 1, m_activeUs + m_clock.nsecsElapsed() / 1000);
    frame.setStartTime(timestamp);
    frame.setEndTime(timestamp + 1000000 / m_frameRate);
    if (m_input->sendVideoFrame(frame)) m_lastTimestamp = timestamp;
}

void CameraRecorder::pause()
{
    if (m_state != State::Recording) return;
    m_activeUs += m_clock.nsecsElapsed() / 1000;
    m_state = State::Paused;
    emit stateChanged();
}

void CameraRecorder::resume()
{
    if (m_state != State::Paused) return;
    m_clock.restart();
    m_state = State::Recording;
    emit stateChanged();
}

void CameraRecorder::stop()
{
    if (m_state == State::Idle || m_state == State::Finalizing) return;
    if (m_state == State::Recording) m_activeUs += m_clock.nsecsElapsed() / 1000;
    m_state = State::Finalizing;
    emit stateChanged();
    m_recorder->stop();
}

bool CameraRecorder::stopAndWait()
{
    stop();
    if (m_state != State::Idle) {
        QEventLoop loop;
        QTimer timeout;
        timeout.setSingleShot(true);
        connect(this, &CameraRecorder::finished, &loop, &QEventLoop::quit);
        connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
        timeout.start(30000);
        loop.exec(QEventLoop::ExcludeUserInputEvents);
        if (m_state != State::Idle)
            m_error = QStringLiteral("视频保存超时，请检查磁盘和录像文件。");
    }
    return m_state == State::Idle && m_error.isEmpty();
}

void CameraRecorder::finalize()
{
    if (m_state == State::Idle) return;
    if (m_error.isEmpty() && m_recorder->error() != QMediaRecorder::NoError)
        m_error = m_recorder->errorString();
    if (m_lastTimestamp < 0 && m_error.isEmpty()) m_error = QStringLiteral("未录到有效视频帧。请等待画面后重试。");
    if ((!QFileInfo::exists(m_path) || QFileInfo(m_path).size() == 0) && m_error.isEmpty())
        m_error = QStringLiteral("录像文件未成功写入，请检查保存目录和磁盘空间。");
    m_state = State::Idle;
    emit stateChanged();
    emit finished(m_path, m_error);
}

} // namespace tdlas
