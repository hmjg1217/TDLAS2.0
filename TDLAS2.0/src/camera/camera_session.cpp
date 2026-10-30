#include "camera/camera_session.h"

#include <QCamera>
#include <QCameraDevice>
#include <QCameraFormat>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QMediaCaptureSession>
#include <QMediaDevices>
#include <QMediaFormat>
#include <QMediaRecorder>
#include <QTransform>
#include <QTimer>
#include <QUrl>
#include <QVideoFrame>
#include <QVideoFrameInput>
#include <QVideoSink>
#include <algorithm>
#include <cmath>

namespace tdlas {
namespace {
int normalizedRotation(int degrees) {
    int value = degrees % 360;
    if (value < 0) value += 360;
    return value == 90 || value == 180 || value == 270 ? value : 0;
}
}

CameraSession::CameraSession(QString dataRoot, QObject* parent)
    : QObject(parent), dataRoot_(std::move(dataRoot)) {}

CameraSession::~CameraSession() { stop(); }

QList<CameraDeviceCapability> CameraSession::discoverDevices() {
    QList<CameraDeviceCapability> result;
    for (const auto& device : QMediaDevices::videoInputs()) {
        CameraDeviceCapability capability;
        capability.id = device.id();
        capability.description = device.description();
        capability.isDefault = device.isDefault();
        for (const auto& format : device.videoFormats()) {
            const CameraFormatCapability entry{format.resolution(), format.minFrameRate(), format.maxFrameRate()};
            const bool duplicate = std::any_of(capability.formats.cbegin(), capability.formats.cend(), [&](const auto& existing) {
                return existing.resolution == entry.resolution
                    && std::abs(existing.minimumFrameRate - entry.minimumFrameRate) < 0.01
                    && std::abs(existing.maximumFrameRate - entry.maximumFrameRate) < 0.01;
            });
            if (!duplicate) capability.formats.append(entry);
        }
        std::sort(capability.formats.begin(), capability.formats.end(), [](const auto& a, const auto& b) {
            if (a.resolution.width() * a.resolution.height() != b.resolution.width() * b.resolution.height())
                return a.resolution.width() * a.resolution.height() > b.resolution.width() * b.resolution.height();
            return a.maximumFrameRate > b.maximumFrameRate;
        });
        result.append(capability);
    }
    return result;
}

bool CameraSession::prepareSession(const CameraSettings& settings, bool synthetic) {
    stop();
    errorString_.clear();
    latestFrame_ = {};
    recordingDurationMs_ = 0;
    syntheticTimestampUs_ = 0;
    settings_ = settings;
    settings_.rotationDegrees = normalizedRotation(settings.rotationDegrees);
    settings_.digitalPanX = std::clamp(settings_.digitalPanX, -1.0, 1.0);
    settings_.digitalPanY = std::clamp(settings_.digitalPanY, -1.0, 1.0);
    settings_.digitalZoom = std::clamp(settings_.digitalZoom, 1.0, 4.0);
    settings_.brightness = std::clamp(settings_.brightness, -100, 100);
    settings_.contrast = std::clamp(settings_.contrast, 0, 200);
    settings_.saturation = std::clamp(settings_.saturation, 0, 200);
    settings_.hue = std::clamp(settings_.hue, -180, 180);
    settings_.sharpness = std::clamp(settings_.sharpness, 0, 100);
    settings_.gamma = std::clamp(settings_.gamma, 0.1, 3.0);
    if (!settings_.resolution.isValid()) settings_.resolution = QSize(1280, 720);
    if (!(settings_.frameRate > 0.0) || !std::isfinite(settings_.frameRate)) settings_.frameRate = 30.0;
    if (settings_.outputDirectory.isEmpty()) settings_.outputDirectory = QDir(dataRoot_).filePath(QStringLiteral("camera"));
    if (!QDir().mkpath(settings_.outputDirectory)) {
        fail(QStringLiteral("无法创建摄像保存目录：%1").arg(settings_.outputDirectory));
        return false;
    }

    synthetic_ = synthetic;
    setState(CameraState::Starting);
    captureSession_ = std::make_unique<QMediaCaptureSession>();
    videoSink_ = std::make_unique<QVideoSink>();
    recorder_ = std::make_unique<QMediaRecorder>();
    captureSession_->setVideoSink(videoSink_.get());
    captureSession_->setRecorder(recorder_.get());

    connect(videoSink_.get(), &QVideoSink::videoFrameChanged, this, [this](const QVideoFrame& frame) {
        const auto image = frame.toImage();
        if (!image.isNull()) acceptFrame(image);
    });
    connect(recorder_.get(), &QMediaRecorder::durationChanged, this, [this](qint64 duration) {
        recordingDurationMs_ = duration;
        emit recordingDurationChanged(duration);
    });
    connect(recorder_.get(), &QMediaRecorder::recorderStateChanged, this, [this](QMediaRecorder::RecorderState state) {
        if (state == QMediaRecorder::RecordingState) setState(CameraState::Recording);
        else if (state == QMediaRecorder::StoppedState && (state_ == CameraState::Recording || state_ == CameraState::Stopping)) {
            setState(CameraState::Stopping);
            if (!finalizationPending_) { finalizationPending_ = true; finalizeRecording(100); }
        }
    });
    connect(recorder_.get(), &QMediaRecorder::errorOccurred, this, [this](QMediaRecorder::Error, const QString& message) {
        fail(QStringLiteral("录像失败：%1").arg(message));
    });
    return true;
}

bool CameraSession::start(const CameraSettings& settings) {
    if (settings.deviceId.isEmpty()) {
        fail(QStringLiteral("请先选择摄像设备。"));
        return false;
    }
    QCameraDevice selected;
    for (const auto& device : QMediaDevices::videoInputs()) if (device.id() == settings.deviceId) { selected = device; break; }
    if (selected.isNull()) {
        fail(QStringLiteral("摄像设备已离线或不可用。"));
        return false;
    }
    if (!prepareSession(settings, false)) return false;
    camera_ = std::make_unique<QCamera>(selected);
    QCameraFormat chosen;
    double score = -1.0;
    for (const auto& format : selected.videoFormats()) {
        const double resolutionPenalty = std::abs(format.resolution().width() - settings_.resolution.width())
            + std::abs(format.resolution().height() - settings_.resolution.height());
        const double fpsPenalty = settings_.frameRate < format.minFrameRate() ? format.minFrameRate() - settings_.frameRate
            : settings_.frameRate > format.maxFrameRate() ? settings_.frameRate - format.maxFrameRate() : 0.0;
        const double candidate = -(resolutionPenalty + fpsPenalty * 100.0);
        if (chosen.isNull() || candidate > score) { chosen = format; score = candidate; }
    }
    if (!chosen.isNull()) camera_->setCameraFormat(chosen);
    connect(camera_.get(), &QCamera::activeChanged, this, [this](bool active) {
        if (active && state_ == CameraState::Starting) {
            applyHardwareSettings();
            emit controlCapabilitiesChanged();
            setState(CameraState::Previewing);
        }
    });
    connect(camera_.get(), &QCamera::errorOccurred, this, [this](QCamera::Error, const QString& message) {
        fail(QStringLiteral("摄像设备错误：%1").arg(message));
    });
    captureSession_->setCamera(camera_.get());
    applyHardwareSettings();
    camera_->start();
    return true;
}

bool CameraSession::startSynthetic(const CameraSettings& settings) {
    if (!prepareSession(settings, true)) return false;
    videoInput_ = std::make_unique<QVideoFrameInput>();
    captureSession_->setVideoFrameInput(videoInput_.get());
    setState(CameraState::Previewing);
    return true;
}

void CameraSession::updateSettings(const CameraSettings& settings) {
    settings_ = settings;
    settings_.rotationDegrees = normalizedRotation(settings.rotationDegrees);
    settings_.digitalPanX = std::clamp(settings_.digitalPanX, -1.0, 1.0);
    settings_.digitalPanY = std::clamp(settings_.digitalPanY, -1.0, 1.0);
    settings_.digitalZoom = std::clamp(settings_.digitalZoom, 1.0, 4.0);
    settings_.brightness = std::clamp(settings_.brightness, -100, 100);
    settings_.contrast = std::clamp(settings_.contrast, 0, 200);
    settings_.saturation = std::clamp(settings_.saturation, 0, 200);
    settings_.hue = std::clamp(settings_.hue, -180, 180);
    settings_.sharpness = std::clamp(settings_.sharpness, 0, 100);
    settings_.gamma = std::clamp(settings_.gamma, 0.1, 3.0);
    applyHardwareSettings();
    if (!latestRawFrame_.isNull()) acceptFrame(latestRawFrame_);
}

CameraControlCapabilities CameraSession::controlCapabilities() const {
    CameraControlCapabilities result;
    if (!camera_) return result;
    result.minimumHardwareZoom = camera_->minimumZoomFactor();
    result.maximumHardwareZoom = camera_->maximumZoomFactor();
    result.hardwareZoom = result.maximumHardwareZoom > result.minimumHardwareZoom + 0.001;
    result.autoFocus = camera_->isFocusModeSupported(QCamera::FocusModeAuto);
    result.manualFocus = camera_->isFocusModeSupported(QCamera::FocusModeManual)
        && camera_->supportedFeatures().testFlag(QCamera::Feature::FocusDistance);
    result.autoExposure = camera_->isExposureModeSupported(QCamera::ExposureAuto);
    result.manualExposure = camera_->isExposureModeSupported(QCamera::ExposureManual)
        && camera_->supportedFeatures().testFlag(QCamera::Feature::ManualExposureTime);
    result.exposureCompensation = camera_->supportedFeatures().testFlag(QCamera::Feature::ExposureCompensation);
    result.isoSensitivity = camera_->supportedFeatures().testFlag(QCamera::Feature::IsoSensitivity);
    result.minimumIso = camera_->minimumIsoSensitivity(); result.maximumIso = camera_->maximumIsoSensitivity();
    result.minimumExposureSeconds = camera_->minimumExposureTime(); result.maximumExposureSeconds = camera_->maximumExposureTime();
    result.autoWhiteBalance = camera_->isWhiteBalanceModeSupported(QCamera::WhiteBalanceAuto);
    result.manualWhiteBalance = camera_->isWhiteBalanceModeSupported(QCamera::WhiteBalanceManual)
        && camera_->supportedFeatures().testFlag(QCamera::Feature::ColorTemperature);
    return result;
}

void CameraSession::stop() {
    if (camera_) camera_->stop();
    if (finalizationPending_) {
        releasePending_ = true;
        setState(CameraState::Stopping);
        return;
    }
    if (recorder_ && recorder_->recorderState() != QMediaRecorder::StoppedState) {
        releasePending_ = true;
        setState(CameraState::Stopping);
        recorder_->stop();
        return;
    }
    releaseObjects();
    latestFrame_ = {};
    latestRawFrame_ = {};
    synthetic_ = false;
    errorString_.clear();
    setState(CameraState::Disconnected);
}

void CameraSession::releaseObjects() {
    if (captureSession_) {
        captureSession_->setCamera(nullptr);
        captureSession_->setVideoFrameInput(nullptr);
        captureSession_->setVideoSink(nullptr);
        captureSession_->setRecorder(nullptr);
    }
    videoInput_.reset();
    camera_.reset();
    recorder_.reset();
    videoSink_.reset();
    captureSession_.reset();
    emit controlCapabilitiesChanged();
}

bool CameraSession::captureStill() {
    if (!active() || latestRawFrame_.isNull()) {
        fail(QStringLiteral("当前没有可保存的预览帧。"));
        return false;
    }
    const QString path = nextOutputPath(QStringLiteral("photo"), QStringLiteral("png"));
    const QImage still = adjusted(transformed(latestRawFrame_));
    if (!still.save(path, "PNG")) {
        fail(QStringLiteral("图片保存失败：%1").arg(path));
        return false;
    }
    lastStillPath_ = path;
    emit mediaSaved(path);
    return true;
}

bool CameraSession::startRecording() {
    if (state_ != CameraState::Previewing || !recorder_) return false;
    lastRecordingPath_ = nextOutputPath(QStringLiteral("video"), QStringLiteral("mp4"));
    QMediaFormat format(QMediaFormat::MPEG4);
    const auto codecs = format.supportedVideoCodecs(QMediaFormat::Encode);
    if (codecs.contains(QMediaFormat::VideoCodec::H264)) format.setVideoCodec(QMediaFormat::VideoCodec::H264);
    else if (codecs.contains(QMediaFormat::VideoCodec::MPEG4)) format.setVideoCodec(QMediaFormat::VideoCodec::MPEG4);
    recorder_->setMediaFormat(format);
    recorder_->setVideoResolution(settings_.resolution);
    recorder_->setVideoFrameRate(settings_.frameRate);
    recorder_->setQuality(QMediaRecorder::HighQuality);
    recorder_->setOutputLocation(QUrl::fromLocalFile(lastRecordingPath_));
    recordingDurationMs_ = 0;
    recorder_->record();
    return true;
}

void CameraSession::stopRecording() {
    if (recorder_ && recorder_->recorderState() != QMediaRecorder::StoppedState) {
        setState(CameraState::Stopping);
        recorder_->stop();
    }
}

void CameraSession::finalizeRecording(int remainingChecks) {
    const QFileInfo output(lastRecordingPath_);
    if (lastRecordingPath_.isEmpty() || output.size() > 0 || remainingChecks <= 0) {
        finalizationPending_ = false;
        if (output.size() > 0) emit mediaSaved(lastRecordingPath_);
        if (releasePending_) {
            releaseObjects(); latestFrame_ = {}; latestRawFrame_ = {}; synthetic_ = false; errorString_.clear(); releasePending_ = false;
            setState(CameraState::Disconnected);
        } else setState(CameraState::Previewing);
        return;
    }
    QTimer::singleShot(20, this, [this, remainingChecks] { finalizeRecording(remainingChecks - 1); });
}

bool CameraSession::submitSyntheticFrame(const QImage& frame, qint64 timestampMicroseconds) {
    if (!synthetic_ || !active() || frame.isNull()) return false;
    acceptFrame(frame);
    if (!videoInput_ || !recorder_ || recorder_->recorderState() != QMediaRecorder::RecordingState) return true;
    QVideoFrame video(transformed(frame).convertedTo(QImage::Format_ARGB32));
    if (timestampMicroseconds < 0) {
        timestampMicroseconds = syntheticTimestampUs_;
        syntheticTimestampUs_ += qRound64(1000000.0 / settings_.frameRate);
    } else syntheticTimestampUs_ = std::max(syntheticTimestampUs_, timestampMicroseconds + qRound64(1000000.0 / settings_.frameRate));
    video.setStartTime(timestampMicroseconds);
    video.setEndTime(timestampMicroseconds + qRound64(1000000.0 / settings_.frameRate));
    return videoInput_->sendVideoFrame(video);
}

bool CameraSession::active() const {
    return state_ == CameraState::Starting || state_ == CameraState::Previewing || state_ == CameraState::Recording || state_ == CameraState::Stopping;
}

void CameraSession::acceptFrame(const QImage& image) {
    latestRawFrame_ = image;
    QImage preview = image;
    if (preview.width() > 1600 || preview.height() > 1200)
        preview = preview.scaled(QSize(1600, 1200), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    latestFrame_ = adjusted(transformed(preview));
    emit frameReady(latestFrame_);
}

QImage CameraSession::transformed(const QImage& image) const {
    QImage result = image;
    if (settings_.digitalZoom > 1.0001 && image.width() > 1 && image.height() > 1) {
        const int cropWidth = std::max(1, qRound(image.width() / settings_.digitalZoom));
        const int cropHeight = std::max(1, qRound(image.height() / settings_.digitalZoom));
        const int maximumX = image.width() - cropWidth;
        const int maximumY = image.height() - cropHeight;
        const int x = std::clamp(qRound(maximumX * (settings_.digitalPanX + 1.0) / 2.0), 0, maximumX);
        const int y = std::clamp(qRound(maximumY * (settings_.digitalPanY + 1.0) / 2.0), 0, maximumY);
        result = image.copy(x, y, cropWidth, cropHeight).scaled(image.size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }
    if (settings_.mirrorPreview) result = result.mirrored(true, false);
    if (settings_.rotationDegrees != 0) result = result.transformed(QTransform().rotate(settings_.rotationDegrees));
    return result;
}

QImage CameraSession::adjusted(const QImage& image) const {
    if (image.isNull()) return image;
    if (settings_.brightness == 0 && settings_.contrast == 100 && settings_.saturation == 100
        && settings_.hue == 0 && settings_.sharpness == 0 && std::abs(settings_.gamma - 1.0) < 0.0001) return image;
    QImage result = image.convertToFormat(QImage::Format_ARGB32);
    const double brightness = settings_.brightness * 2.55;
    const double contrast = settings_.contrast / 100.0;
    const double saturation = settings_.saturation / 100.0;
    const double radians = settings_.hue * 3.14159265358979323846 / 180.0;
    const double cosine = std::cos(radians), sine = std::sin(radians);
    int gammaTable[256];
    for (int value = 0; value < 256; ++value)
        gammaTable[value] = std::clamp(qRound(std::pow(value / 255.0, 1.0 / settings_.gamma) * 255.0), 0, 255);
    for (int y = 0; y < result.height(); ++y) {
        auto* pixels = reinterpret_cast<QRgb*>(result.scanLine(y));
        for (int x = 0; x < result.width(); ++x) {
            const QRgb pixel = pixels[x];
            const double red = qRed(pixel), green = qGreen(pixel), blue = qBlue(pixel);
            const double luminance = 0.299 * red + 0.587 * green + 0.114 * blue;
            const double u = (blue - luminance) * saturation;
            const double v = (red - luminance) * saturation;
            const double rotatedU = cosine * u - sine * v;
            const double rotatedV = sine * u + cosine * v;
            double adjustedRed = luminance + rotatedV;
            double adjustedBlue = luminance + rotatedU;
            double adjustedGreen = (luminance - 0.299 * adjustedRed - 0.114 * adjustedBlue) / 0.587;
            const auto channel = [&](double value) {
                return gammaTable[std::clamp(qRound((value - 127.5) * contrast + 127.5 + brightness), 0, 255)];
            };
            pixels[x] = qRgba(channel(adjustedRed), channel(adjustedGreen), channel(adjustedBlue), qAlpha(pixel));
        }
    }
    if (settings_.sharpness > 0 && result.width() > 2 && result.height() > 2) {
        const QImage source = result;
        const double strength = settings_.sharpness / 100.0;
        for (int y = 1; y < result.height() - 1; ++y) {
            auto* output = reinterpret_cast<QRgb*>(result.scanLine(y));
            const auto* current = reinterpret_cast<const QRgb*>(source.constScanLine(y));
            const auto* above = reinterpret_cast<const QRgb*>(source.constScanLine(y - 1));
            const auto* below = reinterpret_cast<const QRgb*>(source.constScanLine(y + 1));
            for (int x = 1; x < result.width() - 1; ++x) {
                const auto sharpen = [&](int center, int left, int right, int up, int down) {
                    return std::clamp(qRound(center + strength * (4.0 * center - left - right - up - down)), 0, 255);
                };
                output[x] = qRgba(
                    sharpen(qRed(current[x]), qRed(current[x - 1]), qRed(current[x + 1]), qRed(above[x]), qRed(below[x])),
                    sharpen(qGreen(current[x]), qGreen(current[x - 1]), qGreen(current[x + 1]), qGreen(above[x]), qGreen(below[x])),
                    sharpen(qBlue(current[x]), qBlue(current[x - 1]), qBlue(current[x + 1]), qBlue(above[x]), qBlue(below[x])),
                    qAlpha(current[x]));
            }
        }
    }
    return result;
}

void CameraSession::applyHardwareSettings() {
    if (!camera_) return;
    const auto capabilities = controlCapabilities();
    if (capabilities.hardwareZoom)
        camera_->setZoomFactor(float(std::clamp(settings_.hardwareZoom, capabilities.minimumHardwareZoom, capabilities.maximumHardwareZoom)));
    if (settings_.autoFocus && capabilities.autoFocus) camera_->setFocusMode(QCamera::FocusModeAuto);
    else if (capabilities.manualFocus) {
        camera_->setFocusMode(QCamera::FocusModeManual);
        camera_->setFocusDistance(float(std::clamp(settings_.focusDistance, 0.0, 1.0)));
    }
    if (settings_.autoExposure && capabilities.autoExposure) camera_->setExposureMode(QCamera::ExposureAuto);
    else if (capabilities.manualExposure) {
        camera_->setExposureMode(QCamera::ExposureManual);
        camera_->setManualExposureTime(float(std::clamp(settings_.manualExposureSeconds,
            std::max(0.000001, capabilities.minimumExposureSeconds), std::max(0.000001, capabilities.maximumExposureSeconds))));
    }
    if (capabilities.exposureCompensation) camera_->setExposureCompensation(float(settings_.exposureCompensation));
    if (!settings_.autoExposure && capabilities.isoSensitivity && capabilities.minimumIso > 0 && capabilities.maximumIso >= capabilities.minimumIso)
        camera_->setManualIsoSensitivity(std::clamp(settings_.manualIso, capabilities.minimumIso, capabilities.maximumIso));
    else if (capabilities.isoSensitivity) camera_->setAutoIsoSensitivity();
    if (settings_.autoWhiteBalance && capabilities.autoWhiteBalance) camera_->setWhiteBalanceMode(QCamera::WhiteBalanceAuto);
    else if (capabilities.manualWhiteBalance) {
        camera_->setWhiteBalanceMode(QCamera::WhiteBalanceManual);
        camera_->setColorTemperature(std::clamp(settings_.colorTemperature, 2000, 10000));
    }
}

QString CameraSession::nextOutputPath(const QString& prefix, const QString& suffix) const {
    QDir directory(settings_.outputDirectory);
    const QString base = QStringLiteral("%1-%2").arg(prefix, QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss-zzz")));
    QString path = directory.filePath(base + QStringLiteral(".") + suffix);
    for (int counter = 1; QFileInfo::exists(path); ++counter)
        path = directory.filePath(base + QStringLiteral("-%1.").arg(counter) + suffix);
    return path;
}

void CameraSession::setState(CameraState state) {
    if (state_ == state) return;
    state_ = state;
    emit stateChanged(state_);
}

void CameraSession::fail(const QString& message) {
    errorString_ = message;
    setState(CameraState::Error);
    emit errorOccurred(message);
}

}
