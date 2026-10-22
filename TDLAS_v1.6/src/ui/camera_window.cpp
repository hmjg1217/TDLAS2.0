#include "ui/camera_window.h"
#include "camera/camera_image.h"

#include <QCamera>
#include <QCheckBox>
#include <QCloseEvent>
#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMediaCaptureSession>
#include <QMediaDevices>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QPointer>
#include <QScrollArea>
#include <QSettings>
#include <QSignalBlocker>
#include <QSlider>
#include <QStorageInfo>
#include <QSpinBox>
#include <QStandardPaths>
#include <QTimer>
#include <QVBoxLayout>
#include <QVideoFrameFormat>
#include <QVideoSink>
#include <QComboBox>
#include <algorithm>

class CameraImageView final : public QWidget {
public:
    explicit CameraImageView(QWidget *parent) : QWidget(parent)
    {
        setObjectName(QStringLiteral("cameraPreview"));
        setMinimumSize(300, 220);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    }
    QImage image;
    QPoint offset;
    int rotation = 0;
protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.fillRect(rect(), QColor(QStringLiteral("#141b24")));
        if (image.isNull()) {
            painter.setPen(QColor(QStringLiteral("#c5cfda")));
            painter.drawText(rect(), Qt::AlignCenter, QStringLiteral("等待摄像画面\n请选择摄像头并开启预览"));
        } else {
            tdlas::paintCameraImage(painter, image, QRectF(rect()), offset, rotation);
        }
    }
};

CameraWindow::CameraWindow(QWidget *parent) : QDialog(parent)
{
    setObjectName(QStringLiteral("cameraWindow"));
    setWindowTitle(QStringLiteral("摄像模组 · TDLAS v1.6"));
    setWindowFlag(Qt::WindowMinMaxButtonsHint);
    resize(1180, 800);
    setMinimumSize(880, 600);
    m_rows = {{
        {tdlas::CameraControl::Focus}, {tdlas::CameraControl::Zoom},
        {tdlas::CameraControl::Brightness}, {tdlas::CameraControl::Contrast},
        {tdlas::CameraControl::Hue}, {tdlas::CameraControl::Sharpness},
        {tdlas::CameraControl::Saturation}, {tdlas::CameraControl::Gamma},
        {tdlas::CameraControl::WhiteBalance}
    }};
    m_mediaDevices = new QMediaDevices(this);
    m_session = new QMediaCaptureSession(this);
    m_sink = new QVideoSink(this);
    m_session->setVideoSink(m_sink);
    m_frameTimer = new QTimer(this);
    m_frameTimer->setInterval(50);
    m_startTimeout = new QTimer(this);
    m_startTimeout->setSingleShot(true);
    m_startTimeout->setInterval(10000);
    m_recorder = new tdlas::CameraRecorder(this);
    m_recorder->setObjectName(QStringLiteral("cameraRecorder"));
    buildUi();
    connect(m_recorder, &tdlas::CameraRecorder::stateChanged, this, &CameraWindow::updateRecordingUi);
    connect(m_recorder, &tdlas::CameraRecorder::finished, this,
        [this](const QString &path, const QString &error) {
            m_recordMessage = error.isEmpty() ? QStringLiteral("视频已保存：%1").arg(QDir::toNativeSeparators(path))
                : QStringLiteral("录像保存异常：%1。请检查文件：%2").arg(error, QDir::toNativeSeparators(path));
            updateRecordingUi();
            m_recordStatus->setStyleSheet(error.isEmpty() ? QStringLiteral("color: #225a43;") : QStringLiteral("color: #b42318;"));
        });
    auto *recordTimer = new QTimer(this);
    recordTimer->setInterval(1000);
    connect(recordTimer, &QTimer::timeout, this, [this]() {
        updateRecordingUi();
        if (m_recorder->state() == tdlas::CameraRecorder::State::Recording
            || m_recorder->state() == tdlas::CameraRecorder::State::Paused) {
            QStorageInfo storage(m_videoDirectory);
            if (storage.isValid() && storage.bytesAvailable() >= 0 && storage.bytesAvailable() < 64 * 1024 * 1024) {
                m_recorder->stop();
                setStatus(QStringLiteral("录像磁盘剩余空间不足 64 MB，已停止录制并保存现有视频。"), true);
            }
        }
    });
    recordTimer->start();
    connect(m_frameTimer, &QTimer::timeout, this, &CameraWindow::pollFrame);
    connect(m_startTimeout, &QTimer::timeout, this, [this]() {
        if (m_source.isNull()) {
            stopPreview();
            setStatus(QStringLiteral("未收到画面。请检查摄像头占用、Windows 摄像头隐私权限，或尝试较低分辨率。"), true);
        }
    });
    connect(m_mediaDevices, &QMediaDevices::videoInputsChanged, this, &CameraWindow::refreshDevices);
    QSettings settings;
    m_directory = settings.value(QStringLiteral("camera/saveDirectory"),
                                 QStandardPaths::writableLocation(QStandardPaths::PicturesLocation)).toString();
    m_saveDirectory->setText(QDir::toNativeSeparators(m_directory));
    m_saveDirectory->setToolTip(m_saveDirectory->text());
    m_videoDirectory = settings.value(QStringLiteral("camera/videoDirectory"),
        QStandardPaths::writableLocation(QStandardPaths::MoviesLocation)).toString();
    if (m_videoDirectory.isEmpty()) m_videoDirectory = QDir::homePath();
    m_videoDirectoryLabel->setText(QDir::toNativeSeparators(m_videoDirectory));
    m_videoDirectoryLabel->setToolTip(m_videoDirectoryLabel->text());
    refreshDevices();
    updateRecordingUi();
}

CameraWindow::~CameraWindow()
{
    stopPreview();
}

void CameraWindow::buildUi()
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(10);
    auto *deviceLine = new QHBoxLayout;
    deviceLine->addWidget(new QLabel(QStringLiteral("摄像头"), this));
    m_deviceCombo = new QComboBox(this);
    m_deviceCombo->setObjectName(QStringLiteral("cameraDeviceCombo"));
    m_deviceCombo->setMinimumWidth(190);
    deviceLine->addWidget(m_deviceCombo, 1);
    auto *refresh = new QPushButton(QStringLiteral("刷新"), this);
    refresh->setObjectName(QStringLiteral("cameraRefreshButton"));
    deviceLine->addWidget(refresh);
    m_formatCombo = new QComboBox(this);
    m_formatCombo->setObjectName(QStringLiteral("cameraFormatCombo"));
    m_formatCombo->setMinimumWidth(220);
    deviceLine->addWidget(m_formatCombo);
    m_startButton = new QPushButton(QStringLiteral("开启预览"), this);
    m_startButton->setObjectName(QStringLiteral("cameraStartButton"));
    m_stopButton = new QPushButton(QStringLiteral("停止预览"), this);
    m_stopButton->setObjectName(QStringLiteral("cameraStopButton"));
    m_stopButton->setEnabled(false);
    deviceLine->addWidget(m_startButton);
    deviceLine->addWidget(m_stopButton);
    layout->addLayout(deviceLine);

    auto *body = new QHBoxLayout;
    auto *left = new QVBoxLayout;
    m_view = new CameraImageView(this);
    left->addWidget(m_view, 1);
    auto *photoLine = new QHBoxLayout;
    m_photoPreview = new QLabel(QStringLiteral("尚未拍摄"), this);
    m_photoPreview->setObjectName(QStringLiteral("cameraPhotoPreview"));
    m_photoPreview->setFixedSize(160, 90);
    m_photoPreview->setAlignment(Qt::AlignCenter);
    m_photoPreview->setStyleSheet(QStringLiteral("background: #141b24; color: #c5cfda;"));
    photoLine->addWidget(m_photoPreview);
    m_photoInfo = new QLabel(QStringLiteral("拍摄后可保存照片。\n照片包含当前位移与旋转效果。"), this);
    m_photoInfo->setObjectName(QStringLiteral("cameraPhotoInfo"));
    m_photoInfo->setWordWrap(true);
    photoLine->addWidget(m_photoInfo, 1);
    left->addLayout(photoLine);
    body->addLayout(left, 1);

    auto *scroll = new QScrollArea(this);
    scroll->setObjectName(QStringLiteral("cameraControlScroll"));
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setFixedWidth(330);
    auto *controls = new QWidget(scroll);
    auto *controlLayout = new QVBoxLayout(controls);
    controlLayout->setContentsMargins(8, 0, 8, 8);

    auto *lensGroup = new QGroupBox(QStringLiteral("镜头调焦 / 变焦"), controls);
    auto *lensLayout = new QVBoxLayout(lensGroup);
    auto *imageGroup = new QGroupBox(QStringLiteral("图像调节"), controls);
    auto *imageLayout = new QVBoxLayout(imageGroup);
    const QStringList names{QStringLiteral("对焦"), QStringLiteral("镜头变焦"),
        QStringLiteral("亮度"), QStringLiteral("对比度"), QStringLiteral("色调"),
        QStringLiteral("清晰度"), QStringLiteral("饱和度"), QStringLiteral("伽马"),
        QStringLiteral("白平衡")};
    const QStringList ids{QStringLiteral("focus"), QStringLiteral("zoom"),
        QStringLiteral("brightness"), QStringLiteral("contrast"), QStringLiteral("hue"),
        QStringLiteral("sharpness"), QStringLiteral("saturation"), QStringLiteral("gamma"),
        QStringLiteral("whiteBalance")};
    for (size_t i = 0; i < m_rows.size(); ++i) {
        auto &row = m_rows[i];
        auto *rowLayout = new QVBoxLayout;
        rowLayout->setSpacing(2);
        auto *title = new QHBoxLayout;
        title->addWidget(new QLabel(names.at(i), controls));
        title->addStretch();
        row.automatic = new QCheckBox(QStringLiteral("自动"), controls);
        row.automatic->setObjectName(ids.at(i) + QStringLiteral("Auto"));
        row.automatic->setVisible(i == 0 || i == 8);
        title->addWidget(row.automatic);
        rowLayout->addLayout(title);
        auto *valueLine = new QHBoxLayout;
        row.slider = new QSlider(Qt::Horizontal, controls);
        row.slider->setObjectName(ids.at(i) + QStringLiteral("Slider"));
        row.slider->setTracking(false);
        row.value = new QSpinBox(controls);
        row.value->setObjectName(ids.at(i) + QStringLiteral("Value"));
        row.value->setMinimumWidth(80);
        row.value->setKeyboardTracking(false);
        valueLine->addWidget(row.slider, 1);
        valueLine->addWidget(row.value);
        rowLayout->addLayout(valueLine);
        (i < 2 ? lensLayout : imageLayout)->addLayout(rowLayout);
        ControlRow *target = &row;
        connect(row.slider, &QSlider::valueChanged, row.value, &QSpinBox::setValue);
        connect(row.value, qOverload<int>(&QSpinBox::valueChanged), this, [this, target](int value) {
            const QSignalBlocker blocker(target->slider);
            target->slider->setValue(value);
            applyControl(*target);
        });
        connect(row.automatic, &QCheckBox::toggled, this, [this, target]() { applyControl(*target); });
    }
    auto *lensNote = new QLabel(QStringLiteral("手动调焦前请取消“自动”。变焦数值为设备刻度。"), lensGroup);
    lensNote->setWordWrap(true);
    lensLayout->addWidget(lensNote);
    controlLayout->addWidget(lensGroup);

    auto *transform = new QGroupBox(QStringLiteral("画面位移 / 旋转"), controls);
    auto *transformLayout = new QVBoxLayout(transform);
    auto *arrows = new QGridLayout;
    auto addArrow = [&](const QString &label, const QString &id, int r, int c, int dx, int dy) {
        auto *button = new QPushButton(label, transform);
        button->setObjectName(id);
        button->setAutoRepeat(true);
        arrows->addWidget(button, r, c);
        connect(button, &QPushButton::clicked, this, [this, dx, dy]() {
            m_horizontal->setValue(m_horizontal->value() + dx);
            m_vertical->setValue(m_vertical->value() + dy);
        });
    };
    addArrow(QStringLiteral("↑ 上"), QStringLiteral("cameraMoveUp"), 0, 1, 0, -2);
    addArrow(QStringLiteral("← 左"), QStringLiteral("cameraMoveLeft"), 1, 0, -2, 0);
    addArrow(QStringLiteral("右 →"), QStringLiteral("cameraMoveRight"), 1, 2, 2, 0);
    addArrow(QStringLiteral("↓ 下"), QStringLiteral("cameraMoveDown"), 2, 1, 0, 2);
    transformLayout->addLayout(arrows);
    auto *positionForm = new QFormLayout;
    auto addSpin = [&](const QString &label, const QString &id, int minimum, int maximum) {
        auto *spin = new QSpinBox(transform);
        spin->setObjectName(id);
        spin->setRange(minimum, maximum);
        spin->setSuffix(id == QStringLiteral("cameraRotation") ? QStringLiteral(" °") : QStringLiteral(" %"));
        positionForm->addRow(label, spin);
        connect(spin, qOverload<int>(&QSpinBox::valueChanged), this, &CameraWindow::updateTransform);
        return spin;
    };
    m_horizontal = addSpin(QStringLiteral("水平位移"), QStringLiteral("cameraHorizontal"), -100, 100);
    m_vertical = addSpin(QStringLiteral("垂直位移"), QStringLiteral("cameraVertical"), -100, 100);
    m_rotation = addSpin(QStringLiteral("顺时针旋转"), QStringLiteral("cameraRotation"), -180, 180);
    transformLayout->addLayout(positionForm);
    auto *rotateLine = new QHBoxLayout;
    auto *rotateLeft = new QPushButton(QStringLiteral("左转 90°"), transform);
    auto *rotateRight = new QPushButton(QStringLiteral("右转 90°"), transform);
    auto *reset = new QPushButton(QStringLiteral("复位"), transform);
    rotateLeft->setObjectName(QStringLiteral("cameraRotateLeft"));
    rotateRight->setObjectName(QStringLiteral("cameraRotateRight"));
    reset->setObjectName(QStringLiteral("cameraResetTransform"));
    rotateLine->addWidget(rotateLeft);
    rotateLine->addWidget(rotateRight);
    rotateLine->addWidget(reset);
    transformLayout->addLayout(rotateLine);
    connect(rotateLeft, &QPushButton::clicked, this, [this]() {
        m_rotation->setValue((m_rotation->value() - 90 + 540) % 360 - 180);
    });
    connect(rotateRight, &QPushButton::clicked, this, [this]() {
        m_rotation->setValue((m_rotation->value() + 90 + 540) % 360 - 180);
    });
    connect(reset, &QPushButton::clicked, this, [this]() {
        m_horizontal->setValue(0); m_vertical->setValue(0); m_rotation->setValue(0);
    });
    auto *transformNote = new QLabel(QStringLiteral("移动和旋转作用于图像；移出画布的部分裁切，空白处为黑色。"), transform);
    transformNote->setWordWrap(true);
    transformLayout->addWidget(transformNote);
    controlLayout->addWidget(transform);
    controlLayout->addWidget(imageGroup);
    auto *defaults = new QPushButton(QStringLiteral("恢复图像参数默认值"), controls);
    m_defaultsButton = defaults;
    defaults->setObjectName(QStringLiteral("cameraResetImage"));
    controlLayout->addWidget(defaults);
    connect(defaults, &QPushButton::clicked, this, [this]() {
        QString error;
        // Do not move the lens when restoring image parameters.
        for (size_t i = 2; i < m_rows.size(); ++i) {
            auto &row = m_rows[i];
            if (!row.range.supported) continue;
            const bool automatic = row.control == tdlas::CameraControl::WhiteBalance && row.range.autoSupported;
            if (!m_controls.set(row.control, row.range.defaultValue, automatic, &error)) break;
        }
        updateControls();
        setStatus(error.isEmpty() ? QStringLiteral("已恢复图像参数默认值") : error, !error.isEmpty());
    });
    m_controlStatus = new QLabel(controls);
    m_controlStatus->setWordWrap(true);
    controlLayout->addWidget(m_controlStatus);
    controlLayout->addStretch();
    scroll->setWidget(controls);
    body->addWidget(scroll);
    layout->addLayout(body, 1);

    auto *buttons = new QHBoxLayout;
    m_captureButton = new QPushButton(QStringLiteral("拍摄"), this);
    m_captureButton->setObjectName(QStringLiteral("cameraCaptureButton"));
    m_saveButton = new QPushButton(QStringLiteral("保存图片"), this);
    m_saveButton->setObjectName(QStringLiteral("cameraSaveButton"));
    m_captureButton->setEnabled(false);
    m_saveButton->setEnabled(false);
    auto *directory = new QPushButton(QStringLiteral("图片保存目录"), this);
    directory->setObjectName(QStringLiteral("cameraDirectoryButton"));
    buttons->addWidget(m_captureButton);
    buttons->addWidget(m_saveButton);
    buttons->addWidget(directory);
    m_saveDirectory = new QLabel(this);
    m_saveDirectory->setObjectName(QStringLiteral("cameraPhotoDirectory"));
    m_saveDirectory->setWordWrap(true);
    m_saveDirectory->setMinimumWidth(0);
    buttons->addWidget(m_saveDirectory, 1);
    layout->addLayout(buttons);

    auto *recordButtons = new QHBoxLayout;
    m_recordStart = new QPushButton(QStringLiteral("开始录制"), this);
    m_recordStart->setObjectName(QStringLiteral("cameraRecordStart"));
    m_recordPause = new QPushButton(QStringLiteral("暂停录制"), this);
    m_recordPause->setObjectName(QStringLiteral("cameraRecordPause"));
    m_recordStop = new QPushButton(QStringLiteral("停止录制"), this);
    m_recordStop->setObjectName(QStringLiteral("cameraRecordStop"));
    m_videoDirectoryButton = new QPushButton(QStringLiteral("视频保存目录"), this);
    m_videoDirectoryButton->setObjectName(QStringLiteral("cameraVideoDirectoryButton"));
    m_videoDirectoryLabel = new QLabel(this);
    m_videoDirectoryLabel->setObjectName(QStringLiteral("cameraVideoDirectory"));
    m_videoDirectoryLabel->setWordWrap(true);
    m_videoDirectoryLabel->setMinimumWidth(0);
    recordButtons->addWidget(m_recordStart);
    recordButtons->addWidget(m_recordPause);
    recordButtons->addWidget(m_recordStop);
    recordButtons->addWidget(m_videoDirectoryButton);
    recordButtons->addWidget(m_videoDirectoryLabel, 1);
    layout->addLayout(recordButtons);
    m_recordStatus = new QLabel(this);
    m_recordStatus->setObjectName(QStringLiteral("cameraRecordStatus"));
    m_recordStatus->setWordWrap(true);
    layout->addWidget(m_recordStatus);
    m_status = new QLabel(QStringLiteral("请选择摄像头"), this);
    m_status->setObjectName(QStringLiteral("cameraStatus"));
    m_status->setWordWrap(true);
    layout->addWidget(m_status);

    connect(refresh, &QPushButton::clicked, this, &CameraWindow::refreshDevices);
    connect(m_deviceCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, [this]() {
        if (m_refreshing) return;
        stopPreview();
        updateFormats();
        setStatus(QStringLiteral("摄像头已切换，请开启预览"));
    });
    connect(m_startButton, &QPushButton::clicked, this, &CameraWindow::startPreview);
    connect(m_stopButton, &QPushButton::clicked, this, [this]() {
        stopPreview(); setStatus(QStringLiteral("预览已停止，摄像头已释放"));
    });
    connect(m_captureButton, &QPushButton::clicked, this, &CameraWindow::capturePhoto);
    connect(m_saveButton, &QPushButton::clicked, this, &CameraWindow::savePhoto);
    connect(m_recordStart, &QPushButton::clicked, this, &CameraWindow::startRecording);
    connect(m_recordPause, &QPushButton::clicked, this, [this]() {
        if (m_recorder->state() == tdlas::CameraRecorder::State::Paused) m_recorder->resume();
        else m_recorder->pause();
    });
    connect(m_recordStop, &QPushButton::clicked, m_recorder, &tdlas::CameraRecorder::stop);
    connect(m_videoDirectoryButton, &QPushButton::clicked, this, [this]() {
        const QString path = QFileDialog::getExistingDirectory(this, QStringLiteral("选择视频保存目录"), m_videoDirectory);
        if (path.isEmpty()) return;
        m_videoDirectory = path;
        m_videoDirectoryLabel->setText(QDir::toNativeSeparators(path));
        m_videoDirectoryLabel->setToolTip(m_videoDirectoryLabel->text());
        QSettings().setValue(QStringLiteral("camera/videoDirectory"), path);
    });
    connect(directory, &QPushButton::clicked, this, [this]() {
        const QString path = QFileDialog::getExistingDirectory(this, QStringLiteral("选择图片保存目录"), m_directory);
        if (path.isEmpty()) return;
        m_directory = path;
        m_saveDirectory->setText(QDir::toNativeSeparators(path));
        m_saveDirectory->setToolTip(m_saveDirectory->text());
        QSettings().setValue(QStringLiteral("camera/saveDirectory"), path);
    });
    updateControls();
    // Enter in a numeric input must never trigger preview, capture or saving.
    for (auto *button : findChildren<QPushButton *>()) button->setAutoDefault(false);
}

void CameraWindow::refreshDevices()
{
    if (m_refreshing) return;
    const QByteArray previous = m_deviceCombo->currentData().toByteArray();
    const bool wasRunning = m_camera != nullptr;
    stopPreview();
    m_refreshing = true;
    m_devices = QMediaDevices::videoInputs();
    m_deviceCombo->clear();
    int preferred = -1;
    const QByteArray saved = QSettings().value(QStringLiteral("camera/deviceId")).toByteArray();
    for (const auto &device : m_devices) {
        const int index = m_deviceCombo->count();
        m_deviceCombo->addItem(device.description(), device.id());
        if (device.id() == (previous.isEmpty() ? saved : previous)) preferred = index;
    }
    const bool previousPresent = m_deviceCombo->findData(previous) >= 0;
    if (preferred < 0) {
        for (int i = 0; i < m_devices.size(); ++i) {
            const auto description = m_devices.at(i).description();
            if (description.contains(QStringLiteral("ZOOM"), Qt::CaseInsensitive)
                || description.contains(QStringLiteral("IMX415"), Qt::CaseInsensitive)) {
                preferred = i;
                break;
            }
        }
    }
    if (preferred >= 0) m_deviceCombo->setCurrentIndex(preferred);
    m_refreshing = false;
    updateFormats();
    if (m_devices.isEmpty()) setStatus(QStringLiteral("未检测到摄像头，请连接 USB 摄像模组后刷新。"), true);
    else if (wasRunning && !previousPresent)
        setStatus(QStringLiteral("原摄像头已断开。请重新选择设备并开启预览。"), true);
    else if (wasRunning) startPreview();
    else setStatus(QStringLiteral("已发现 %1 个摄像头，点击开启预览。调节项按设备实际能力启用。").arg(m_devices.size()));
}

void CameraWindow::updateFormats()
{
    m_formatCombo->clear();
    m_formats.clear();
    const int index = m_deviceCombo->currentIndex();
    if (index >= 0 && index < m_devices.size()) m_formats = m_devices.at(index).videoFormats();
    std::sort(m_formats.begin(), m_formats.end(), [](const auto &a, const auto &b) {
        if (a.resolution().width() != b.resolution().width()) return a.resolution().width() > b.resolution().width();
        if (a.resolution().height() != b.resolution().height()) return a.resolution().height() > b.resolution().height();
        return a.maxFrameRate() > b.maxFrameRate();
    });
    int preferred = -1;
    for (int i = 0; i < m_formats.size(); ++i) {
        const auto &format = m_formats.at(i);
        const auto size = format.resolution();
        const QString pixel = QVideoFrameFormat::pixelFormatToString(format.pixelFormat());
        m_formatCombo->addItem(QStringLiteral("%1×%2 · %3 fps · %4")
            .arg(size.width()).arg(size.height()).arg(format.maxFrameRate(), 0, 'g', 3).arg(pixel), i);
        // Start at 1080p for USB2 bandwidth and a responsive preview; 4K stays selectable.
        if (size == QSize(1920, 1080)) {
            const bool jpeg = format.pixelFormat() == QVideoFrameFormat::Format_Jpeg;
            const bool previousJpeg = preferred >= 0
                && m_formats.at(preferred).pixelFormat() == QVideoFrameFormat::Format_Jpeg;
            if (preferred < 0 || (jpeg && !previousJpeg)
                || (jpeg == previousJpeg && format.maxFrameRate() > m_formats.at(preferred).maxFrameRate()))
                preferred = i;
        }
    }
    if (preferred >= 0) m_formatCombo->setCurrentIndex(preferred);
    m_startButton->setEnabled(index >= 0);
    m_formatCombo->setEnabled(index >= 0);
}

void CameraWindow::startPreview()
{
    if (m_camera || m_stoppingPreview) return;
    const int index = m_deviceCombo->currentIndex();
    if (index < 0 || index >= m_devices.size()) {
        setStatus(QStringLiteral("没有可用摄像头，请连接设备并刷新。"), true);
        return;
    }
    const auto device = m_devices.at(index);
    m_source = {};
    m_lastFrame = {};
    m_sink->setVideoFrame(QVideoFrame());
    m_camera = new QCamera(device, this);
    const int format = m_formatCombo->currentIndex();
    if (format >= 0 && format < m_formats.size()) m_camera->setCameraFormat(m_formats.at(format));
    m_session->setCamera(m_camera);
    QString error;
    const bool controlsAvailable = m_controls.open(device.id(), device.description(), &error);
    m_controlStatus->setText(controlsAvailable
        ? QStringLiteral("灰色项表示设备未提供相应控制，或当前处于自动模式。") : error);
    updateControls();
    const QPointer<QCamera> openedCamera(m_camera);
    connect(m_camera, &QCamera::errorOccurred, this, [this, openedCamera](QCamera::Error, const QString &message) {
        // Release resources outside the backend's error signal stack.
        QTimer::singleShot(0, this, [this, openedCamera, message]() {
            if (!openedCamera || openedCamera != m_camera) return;
            stopPreview();
            setStatus(QStringLiteral("摄像头错误：%1。请检查设备占用和摄像头隐私权限。").arg(message), true);
        });
    });
    connect(m_camera, &QCamera::activeChanged, this, [this, openedCamera](bool active) {
        if (!active && m_camera) {
            m_captureButton->setEnabled(false);
            // Backends may report a disconnect without an error signal.
            QTimer::singleShot(0, this, [this, openedCamera]() {
                if (openedCamera && openedCamera == m_camera && !m_camera->isActive()) {
                    stopPreview();
                    setStatus(QStringLiteral("摄像头预览已中断，请检查连接后重新开启。"), true);
                }
            });
        }
    });
    m_startButton->setEnabled(false);
    m_stopButton->setEnabled(true);
    m_formatCombo->setEnabled(false);
    QSettings().setValue(QStringLiteral("camera/deviceId"), device.id());
    setStatus(QStringLiteral("正在打开 %1…").arg(device.description()));
    m_frameTimer->start();
    m_startTimeout->start();
    m_frameAge.start();
    m_camera->start();
}

void CameraWindow::stopPreview()
{
    if (m_stoppingPreview) return;
    m_stoppingPreview = true;
    m_frameTimer->stop();
    m_startTimeout->stop();
    // Finish the file before destroying a camera window or exiting the app.
    if (!m_recorder->stopAndWait()) {
        m_recordMessage = QStringLiteral("录像未正常完成，请检查：%1").arg(QDir::toNativeSeparators(m_recorder->filePath()));
    }
    if (m_camera) {
        disconnect(m_camera, nullptr, this, nullptr);
        m_camera->stop();
        m_session->setCamera(nullptr);
        delete m_camera;
        m_camera = nullptr;
    }
    m_controls.close();
    m_source = {};
    m_lastFrame = {};
    m_sink->setVideoFrame(QVideoFrame());
    m_view->image = {};
    m_view->update();
    m_captureButton->setEnabled(false);
    m_startButton->setEnabled(m_deviceCombo->currentIndex() >= 0);
    m_stopButton->setEnabled(false);
    m_formatCombo->setEnabled(m_deviceCombo->currentIndex() >= 0);
    updateControls();
    m_stoppingPreview = false;
    updateRecordingUi();
}

void CameraWindow::syncControl(ControlRow &row)
{
    row.range = m_controls.range(row.control);
    const auto &range = row.range;
    const QSignalBlocker sliderBlocker(row.slider);
    const QSignalBlocker valueBlocker(row.value);
    const QSignalBlocker autoBlocker(row.automatic);
    row.slider->setRange(range.minimum, range.maximum);
    row.slider->setSingleStep(range.step);
    row.slider->setPageStep(range.step * 5);
    row.slider->setValue(range.value);
    row.value->setRange(range.minimum, range.maximum);
    row.value->setSingleStep(range.step);
    row.value->setValue(range.value);
    row.automatic->setChecked(range.automatic);
    row.automatic->setEnabled(range.supported && range.autoSupported && range.manualSupported);
    row.automatic->setVisible(range.autoSupported || row.control == tdlas::CameraControl::Focus
                              || row.control == tdlas::CameraControl::WhiteBalance);
    const bool manual = range.supported && range.manualSupported && !range.automatic;
    row.slider->setEnabled(manual);
    row.value->setEnabled(manual);
    const QString tooltip = range.supported
        ? QStringLiteral("设备刻度 %1～%2，步长 %3").arg(range.minimum).arg(range.maximum).arg(range.step)
        : QStringLiteral("摄像头未开启，或设备未提供此标准 UVC 控制");
    row.value->setToolTip(tooltip);
    row.slider->setToolTip(tooltip);
}

void CameraWindow::updateControls()
{
    for (auto &row : m_rows) syncControl(row);
    bool haveImageControls = false;
    for (size_t i = 2; i < m_rows.size(); ++i)
        haveImageControls = haveImageControls || m_rows[i].range.supported;
    m_defaultsButton->setEnabled(haveImageControls);
}

void CameraWindow::applyControl(ControlRow &row)
{
    QString error;
    if (!m_controls.set(row.control, row.value->value(), row.automatic->isChecked(), &error)) {
        setStatus(error, true);
    }
    // Read back the value: some UVC firmwares quantize or reject values silently.
    syncControl(row);
}

void CameraWindow::pollFrame()
{
    if (!m_camera || !m_camera->isActive()) return;
    if (m_frameAge.isValid() && m_frameAge.elapsed() > 10000) {
        stopPreview();
        setStatus(QStringLiteral("摄像头超过 10 秒未返回新画面，请检查连接后重新开启预览。"), true);
        return;
    }
    const QVideoFrame frame = m_sink->videoFrame();
    if (!frame.isValid() || frame == m_lastFrame) return;
    const QImage image = frame.toImage();
    if (image.isNull()) return;
    const bool first = m_source.isNull();
    m_source = image;
    m_lastFrame = frame;
    m_frameAge.restart();
    m_view->image = image;
    m_view->update();
    m_captureButton->setEnabled(true);
    m_recorder->append(image, m_view->offset, m_view->rotation);
    updateRecordingUi();
    if (first) {
        m_startTimeout->stop();
        setStatus(QStringLiteral("实时预览：%1 · %2×%3。拍摄保存的是当前视频帧。")
            .arg(m_camera->cameraDevice().description()).arg(image.width()).arg(image.height()));
    }
}

void CameraWindow::updateTransform()
{
    m_view->offset = QPoint(m_horizontal->value(), m_vertical->value());
    m_view->rotation = m_rotation->value();
    m_view->update();
}

void CameraWindow::capturePhoto()
{
    if (!m_camera || !m_camera->isActive() || m_source.isNull()) {
        setStatus(QStringLiteral("请等待实时画面后再拍摄。"), true);
        return;
    }
    m_photograph = tdlas::cameraPhotograph(m_source, m_view->offset, m_view->rotation);
    m_photoPreview->setPixmap(QPixmap::fromImage(m_photograph).scaled(
        m_photoPreview->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    m_photoInfo->setText(QStringLiteral("已拍摄 %1 · %2×%3\n点击“保存图片”选择文件名和格式。")
        .arg(QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss")))
        .arg(m_photograph.width()).arg(m_photograph.height()));
    m_saveButton->setEnabled(!m_photograph.isNull());
    setStatus(QStringLiteral("照片已拍摄，预览继续；照片在保存前保留在本窗口中。"));
}

void CameraWindow::savePhoto()
{
    if (m_photograph.isNull()) return;
    QString selectedFilter = QStringLiteral("PNG 图片 (*.png)");
    QString path = QFileDialog::getSaveFileName(this, QStringLiteral("保存摄像图片"),
        QDir(m_directory).filePath(QStringLiteral("TDLAS_%1.png")
            .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmmss_zzz")))),
        QStringLiteral("PNG 图片 (*.png);;JPEG 图片 (*.jpg *.jpeg);;BMP 图片 (*.bmp)"), &selectedFilter);
    if (path.isEmpty()) return;
    if (QFileInfo(path).suffix().isEmpty()) {
        path += selectedFilter.startsWith(QStringLiteral("JPEG")) ? QStringLiteral(".jpg")
            : selectedFilter.startsWith(QStringLiteral("BMP")) ? QStringLiteral(".bmp") : QStringLiteral(".png");
        // The native dialog cannot confirm replacement for an extension appended later.
        if (QFileInfo::exists(path) && QMessageBox::question(this, QStringLiteral("替换图片"),
            QStringLiteral("文件已存在，是否替换？\n%1").arg(QDir::toNativeSeparators(path))) != QMessageBox::Yes) return;
    }
    QString error;
    if (!tdlas::saveCameraPhotograph(m_photograph, path, &error)) {
        QMessageBox::warning(this, QStringLiteral("图片保存失败"), error);
        return;
    }
    m_directory = QFileInfo(path).absolutePath();
    QSettings().setValue(QStringLiteral("camera/saveDirectory"), m_directory);
    m_saveDirectory->setText(QDir::toNativeSeparators(m_directory));
    m_saveDirectory->setToolTip(m_saveDirectory->text());
    setStatus(QStringLiteral("图片已保存：%1").arg(QDir::toNativeSeparators(path)));
}

void CameraWindow::setStatus(const QString &message, bool error)
{
    m_status->setText(message);
    m_status->setStyleSheet(error ? QStringLiteral("color: #b42318;") : QStringLiteral("color: #225a43;"));
}

void CameraWindow::startRecording()
{
    if (m_recorder->state() == tdlas::CameraRecorder::State::Paused) {
        m_recorder->resume();
        return;
    }
    if (!m_camera || !m_camera->isActive() || m_source.isNull()) {
        setStatus(QStringLiteral("请等待摄像画面后再开始录制。"), true);
        return;
    }
    if (!QDir().mkpath(m_videoDirectory)) {
        setStatus(QStringLiteral("无法创建视频保存目录，请重新选择目录。"), true);
        return;
    }
    const QString path = QDir(m_videoDirectory).filePath(QStringLiteral("TDLAS_%1.mp4")
        .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmmss_zzz"))));
    QString error;
    m_recordMessage.clear();
    m_recordStatus->setStyleSheet(QStringLiteral("color: #225a43;"));
    if (!m_recorder->start(path, m_source.size(), 20, &error)) {
        m_recordMessage = error;
        m_recordStatus->setStyleSheet(QStringLiteral("color: #b42318;"));
    }
    updateRecordingUi();
}

void CameraWindow::updateRecordingUi()
{
    using State = tdlas::CameraRecorder::State;
    const auto state = m_recorder->state();
    const bool live = m_camera && !m_source.isNull() && !m_stoppingPreview;
    m_recordStart->setEnabled(live && (state == State::Idle || state == State::Paused));
    m_recordStart->setText(state == State::Paused ? QStringLiteral("继续录制") : QStringLiteral("开始录制"));
    m_recordPause->setEnabled(state == State::Recording || state == State::Paused);
    m_recordPause->setText(state == State::Paused ? QStringLiteral("继续录制") : QStringLiteral("暂停录制"));
    m_recordStop->setEnabled(state == State::Recording || state == State::Paused);
    m_videoDirectoryButton->setEnabled(state == State::Idle);
    const qint64 seconds = m_recorder->duration() / 1000;
    const QString elapsed = QStringLiteral("%1:%2:%3").arg(seconds / 3600, 2, 10, QLatin1Char('0'))
        .arg((seconds / 60) % 60, 2, 10, QLatin1Char('0')).arg(seconds % 60, 2, 10, QLatin1Char('0'));
    if (state == State::Recording || state == State::Paused) {
        m_recordStatus->setText(QStringLiteral("%1 · %2 · %3")
            .arg(state == State::Paused ? QStringLiteral("已暂停") : QStringLiteral("正在录制"), elapsed,
                 QDir::toNativeSeparators(m_recorder->filePath())));
    } else if (state == State::Finalizing) {
        m_recordStatus->setText(QStringLiteral("正在保存视频，请等待…"));
    } else {
        m_recordStatus->setText(m_recordMessage.isEmpty()
            ? QStringLiteral("MP4 录像 · 停止时自动保存 · 录制期间可调节镜头与图像") : m_recordMessage);
    }
}

void CameraWindow::closeEvent(QCloseEvent *event)
{
    stopPreview();
    event->accept();
}

void CameraWindow::reject()
{
    // Escape invokes reject() directly and bypasses closeEvent().
    stopPreview();
    QDialog::reject();
}
