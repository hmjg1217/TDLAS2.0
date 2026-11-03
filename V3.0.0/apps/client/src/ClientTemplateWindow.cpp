#include "ClientTemplateWindow.h"

#include "CameraPreviewWidget.h"
#include "ChartWidget.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFileInfo>
#include <QFormLayout>
#include <QFrame>
#include <QGridLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QSlider>
#include <QSpinBox>
#include <QStackedWidget>
#include <QVBoxLayout>

namespace clientui {

namespace {

QLabel *makeLabel(const QString &text, const QString &role = QString(), QWidget *parent = nullptr)
{
    auto *label = new QLabel(text, parent);
    if (!role.isEmpty()) {
        label->setProperty("uiRole", role);
    }
    return label;
}

QPushButton *makeButton(const QString &text, const QString &role, bool checkable = false, QWidget *parent = nullptr)
{
    auto *button = new QPushButton(text, parent);
    button->setProperty("uiRole", role);
    button->setCheckable(checkable);
    button->setCursor(Qt::PointingHandCursor);
    return button;
}

QFrame *makePanel(const QString &title, QWidget *body, QWidget *parent = nullptr)
{
    auto *panel = new QFrame(parent);
    panel->setProperty("uiRole", QStringLiteral("panel"));
    auto *layout = new QVBoxLayout(panel);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    auto *titleLabel = makeLabel(title, QStringLiteral("panelTitle"), panel);
    titleLabel->setMinimumHeight(42);
    titleLabel->setContentsMargins(16, 0, 12, 0);
    layout->addWidget(titleLabel);
    layout->addWidget(body, 1);
    return panel;
}

QFrame *makeMetricRow(const QString &name, const QString &value, QWidget *parent)
{
    auto *row = new QFrame(parent);
    row->setProperty("uiRole", QStringLiteral("metricRow"));
    auto *layout = new QHBoxLayout(row);
    layout->setContentsMargins(18, 7, 18, 7);
    layout->setSpacing(10);
    auto *nameLabel = makeLabel(name, QStringLiteral("metricName"), row);
    auto *valueLabel = makeLabel(value, QStringLiteral("metricValue"), row);
    valueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    layout->addWidget(nameLabel, 1);
    layout->addWidget(valueLabel, 1);
    return row;
}

QWidget *makeResultsBody(QWidget *parent)
{
    auto *body = new QWidget(parent);
    auto *layout = new QVBoxLayout(body);
    layout->setContentsMargins(14, 12, 14, 14);
    layout->setSpacing(10);
    layout->addWidget(makeMetricRow(QStringLiteral("峰值吸光度"), QStringLiteral("0.0978  · 模拟"), body));
    layout->addWidget(makeMetricRow(QStringLiteral("☼  光强百分比"), QStringLiteral("78.65 %  · 模拟"), body));
    layout->addWidget(makeMetricRow(QStringLiteral("气体浓度"), QStringLiteral("—  未校准"), body));
    return body;
}

QComboBox *combo(const QStringList &items, QWidget *parent)
{
    auto *control = new QComboBox(parent);
    control->addItems(items);
    return control;
}

QDoubleSpinBox *decimalSpin(double value, double minimum, double maximum, int decimals, QWidget *parent)
{
    auto *control = new QDoubleSpinBox(parent);
    control->setRange(minimum, maximum);
    control->setDecimals(decimals);
    control->setValue(value);
    return control;
}

QWidget *makeSliderRow(const QString &labelText, QWidget *parent)
{
    auto *row = new QWidget(parent);
    auto *layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    auto *label = makeLabel(labelText, QStringLiteral("fieldLabel"), row);
    auto *slider = new QSlider(Qt::Horizontal, row);
    slider->setRange(0, 100);
    slider->setValue(50);
    auto *value = makeLabel(QStringLiteral("50"), QStringLiteral("sliderValue"), row);
    value->setMinimumWidth(34);
    value->setAlignment(Qt::AlignCenter);
    layout->addWidget(label);
    layout->addWidget(slider, 1);
    layout->addWidget(value);
    QObject::connect(slider, &QSlider::valueChanged, value, [value](int number) { value->setText(QString::number(number)); });
    return row;
}

} // namespace

ClientTemplateWindow::ClientTemplateWindow(Screen initialScreen, QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("clientTemplateRoot"));
    setWindowTitle(QStringLiteral("TDLAS 测氢系统 · 客户 Codex UI 模板"));
    setMinimumSize(1080, 680);
    resize(1504, 847);
    setFocusPolicy(Qt::StrongFocus);
    buildShell();
    buildParameterDrawer();
    buildMovementPanel();
    setScreen(initialScreen);
}

void ClientTemplateWindow::buildShell()
{
    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);
    rootLayout->addWidget(buildTopBar());

    auto *body = new QWidget(this);
    body->setObjectName(QStringLiteral("workspaceBody"));
    auto *bodyLayout = new QHBoxLayout(body);
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->setSpacing(0);
    bodyLayout->addWidget(buildSideRail());

    auto *content = new QWidget(body);
    auto *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(0);
    contentLayout->addWidget(buildTabBar());

    pages_ = new QStackedWidget(content);
    pages_->setObjectName(QStringLiteral("pageStack"));
    pages_->addWidget(buildMainPage());
    cameraPage_ = buildCameraPage();
    pages_->addWidget(cameraPage_);
    contentLayout->addWidget(pages_, 1);

    bodyLayout->addWidget(content, 1);
    rootLayout->addWidget(body, 1);
}

QWidget *ClientTemplateWindow::buildTopBar()
{
    auto *bar = new QFrame(this);
    bar->setObjectName(QStringLiteral("topBar"));
    bar->setFixedHeight(64);
    auto *layout = new QHBoxLayout(bar);
    layout->setContentsMargins(22, 0, 14, 0);
    layout->setSpacing(12);

    layout->addWidget(makeLabel(QStringLiteral("◉  道纪量子   │   TDLAS 测氢系统"), QStringLiteral("brand"), bar));
    layout->addSpacing(12);
    layout->addWidget(makeLabel(QStringLiteral("设备状态：● 未连接"), QStringLiteral("statusPill"), bar));
    layout->addWidget(makeLabel(QStringLiteral("▶  采集状态：● 就绪"), QStringLiteral("statusPill"), bar));
    layout->addWidget(makeLabel(QStringLiteral("模拟数据"), QStringLiteral("modePill"), bar));
    layout->addStretch(1);

    templateNotice_ = makeLabel(QStringLiteral("UI 模板 · 无设备连接"), QStringLiteral("templateNotice"), bar);
    layout->addWidget(templateNotice_);
    auto *theme = makeButton(QStringLiteral("☼ 主题"), QStringLiteral("topAction"), false, bar);
    auto *settings = makeButton(QStringLiteral("⚙ 设置"), QStringLiteral("topAction"), false, bar);
    layout->addWidget(theme);
    layout->addWidget(settings);

    connect(theme, &QPushButton::clicked, this, [this]() { showTemplateNotice(QStringLiteral("主题按钮仅发出 UI 意图")); });
    connect(settings, &QPushButton::clicked, this, [this]() { showTemplateNotice(QStringLiteral("设置契约尚未接入")); });
    return bar;
}

QWidget *ClientTemplateWindow::buildSideRail()
{
    auto *rail = new QFrame(this);
    rail->setObjectName(QStringLiteral("sideRail"));
    rail->setFixedWidth(88);
    auto *layout = new QVBoxLayout(rail);
    layout->setContentsMargins(4, 14, 4, 14);
    layout->setSpacing(6);

    homeRail_ = makeButton(QStringLiteral("●\n主页面"), QStringLiteral("railAction"), true, rail);
    parameterRail_ = makeButton(QStringLiteral("≡\n参数区"), QStringLiteral("railAction"), true, rail);
    auto *start = makeButton(QStringLiteral("▶\n开始采集"), QStringLiteral("railAction"), false, rail);
    auto *stop = makeButton(QStringLiteral("■\n停止采集"), QStringLiteral("railAction"), false, rail);
    auto *reset = makeButton(QStringLiteral("↻\n复位"), QStringLiteral("railAction"), false, rail);
    auto *save = makeButton(QStringLiteral("保存\n数据保存"), QStringLiteral("railAction"), false, rail);

    for (auto *button : {homeRail_, parameterRail_, start, stop, reset, save}) {
        button->setMinimumHeight(86);
        layout->addWidget(button);
    }
    layout->addStretch(1);

    connect(homeRail_, &QPushButton::clicked, this, [this]() { setScreen(Screen::Main); });
    connect(parameterRail_, &QPushButton::clicked, this, [this](bool checked) {
        setScreen(checked ? Screen::Parameters : Screen::Main);
    });
    connect(start, &QPushButton::clicked, this, [this]() { showTemplateNotice(QStringLiteral("开始采集：仅 UI 意图，未连接设备")); });
    connect(stop, &QPushButton::clicked, this, [this]() { showTemplateNotice(QStringLiteral("停止采集：仅 UI 意图")); });
    connect(reset, &QPushButton::clicked, this, [this]() { showTemplateNotice(QStringLiteral("复位：未清除文件，等待业务确认")); });
    connect(save, &QPushButton::clicked, this, [this]() { showTemplateNotice(QStringLiteral("数据保存：未写入磁盘")); });
    return rail;
}

QWidget *ClientTemplateWindow::buildTabBar()
{
    auto *bar = new QFrame(this);
    bar->setObjectName(QStringLiteral("tabBar"));
    bar->setFixedHeight(64);
    auto *layout = new QHBoxLayout(bar);
    layout->setContentsMargins(16, 10, 16, 8);
    layout->setSpacing(10);

    tabGroup_ = new QButtonGroup(bar);
    tabGroup_->setExclusive(true);
    const QStringList names{QStringLiteral("主页面"), QStringLiteral("摄像界面")};
    for (int index = 0; index < names.size(); ++index) {
        auto *button = makeButton(names.at(index), QStringLiteral("topTab"), true, bar);
        button->setMinimumWidth(166);
        tabButtons_[static_cast<std::size_t>(index)] = button;
        tabGroup_->addButton(button, index);
        layout->addWidget(button);
    }
    layout->addStretch(1);

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    connect(tabGroup_, &QButtonGroup::idClicked, this, [this](int id) {
        setScreen(id == 0 ? Screen::Main : Screen::Camera);
    });
#else
    connect(tabGroup_, QOverload<int>::of(&QButtonGroup::buttonClicked), this, [this](int id) {
        setScreen(id == 0 ? Screen::Main : Screen::Camera);
    });
#endif
    return bar;
}

QWidget *ClientTemplateWindow::buildMainPage()
{
    auto *page = new QWidget(this);
    page->setObjectName(QStringLiteral("mainPage"));
    auto *grid = new QGridLayout(page);
    grid->setContentsMargins(16, 12, 16, 16);
    grid->setHorizontalSpacing(12);
    grid->setVerticalSpacing(12);

    auto *preview = new CameraPreviewWidget(page);
    auto *fv = new ChartWidget(ChartWidget::Mode::FvCurve, page);
    fv->setAxisLabels(QStringLiteral("扫描点数（a.u.）"), QStringLiteral("吸光度"));
    auto *peak = new ChartWidget(ChartWidget::Mode::PeakTrend, page);
    peak->setAxisLabels(QStringLiteral("时间（s）"), QStringLiteral("峰值吸光度"));

    grid->addWidget(makePanel(QStringLiteral("摄像窗口"), preview, page), 0, 0);
    grid->addWidget(makePanel(QStringLiteral("FV 曲线（吸收光谱）"), fv, page), 0, 1);
    grid->addWidget(makePanel(QStringLiteral("FV 峰值数据记录（实时趋势）"), peak, page), 1, 0);
    grid->addWidget(makePanel(QStringLiteral("实时结果"), makeResultsBody(page), page), 1, 1);
    grid->setColumnStretch(0, 5);
    grid->setColumnStretch(1, 6);
    grid->setRowStretch(0, 6);
    grid->setRowStretch(1, 5);
    return page;
}

QWidget *ClientTemplateWindow::buildCameraPage()
{
    auto *page = new QWidget(this);
    page->setObjectName(QStringLiteral("cameraPage"));
    auto *grid = new QGridLayout(page);
    grid->setContentsMargins(16, 12, 16, 16);
    grid->setHorizontalSpacing(12);
    grid->setVerticalSpacing(12);

    auto *previewColumn = new QWidget(page);
    auto *previewLayout = new QVBoxLayout(previewColumn);
    previewLayout->setContentsMargins(0, 0, 0, 0);
    previewLayout->setSpacing(12);
    previewLayout->addWidget(makePanel(QStringLiteral("相机预览"), new CameraPreviewWidget(previewColumn), previewColumn), 1);

    auto *captureBar = new QFrame(previewColumn);
    captureBar->setProperty("uiRole", QStringLiteral("captureBar"));
    captureBar->setMinimumHeight(94);
    auto *captureLayout = new QHBoxLayout(captureBar);
    captureLayout->setContentsMargins(24, 14, 24, 14);
    captureLayout->setSpacing(12);
    auto *photo = makeButton(QStringLiteral("拍照"), QStringLiteral("secondary"), false, captureBar);
    auto *record = makeButton(QStringLiteral("●  开始录像"), QStringLiteral("primary"), false, captureBar);
    auto *stop = makeButton(QStringLiteral("■  停止录像"), QStringLiteral("secondary"), false, captureBar);
    stop->setEnabled(false);
    captureLayout->addWidget(photo);
    captureLayout->addWidget(record);
    captureLayout->addWidget(stop);
    captureLayout->addStretch(1);
    captureLayout->addWidget(makeLabel(QStringLiteral("●  录像时长：00:00:00  · 模拟"), QStringLiteral("recordingStatus"), captureBar));
    previewLayout->addWidget(captureBar);

    auto *right = new QWidget(page);
    auto *rightLayout = new QVBoxLayout(right);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(12);

    auto *deviceBody = new QWidget(right);
    auto *deviceForm = new QFormLayout(deviceBody);
    deviceForm->setContentsMargins(16, 12, 16, 12);
    deviceForm->setHorizontalSpacing(12);
    deviceForm->setVerticalSpacing(8);
    auto *cameraSelect = combo({QStringLiteral("未发现设备")}, deviceBody);
    auto *format = combo({QStringLiteral("1920 × 1080 (16:9)")}, deviceBody);
    auto *fps = combo({QStringLiteral("30 FPS")}, deviceBody);
    auto *motion = makeButton(QStringLiteral("↔  位移/缩放设置"), QStringLiteral("secondary"), false, deviceBody);
    deviceForm->addRow(QStringLiteral("相机选择"), cameraSelect);
    deviceForm->addRow(QString(), motion);
    deviceForm->addRow(QStringLiteral("分辨率"), format);
    deviceForm->addRow(QStringLiteral("帧率"), fps);
    rightLayout->addWidget(makePanel(QStringLiteral("相机控制"), deviceBody, right));

    auto *parameterBody = new QWidget(right);
    auto *parameterLayout = new QVBoxLayout(parameterBody);
    parameterLayout->setContentsMargins(16, 10, 16, 12);
    parameterLayout->setSpacing(8);
    auto *exposureRow = new QWidget(parameterBody);
    auto *exposureLayout = new QHBoxLayout(exposureRow);
    exposureLayout->setContentsMargins(0, 0, 0, 0);
    exposureLayout->addWidget(makeLabel(QStringLiteral("曝光时间"), QStringLiteral("fieldLabel"), exposureRow));
    exposureLayout->addWidget(decimalSpin(10.0, 0.1, 1000.0, 1, exposureRow));
    exposureLayout->addWidget(makeLabel(QStringLiteral("ms"), QString(), exposureRow));
    exposureLayout->addStretch(1);
    exposureLayout->addWidget(new QCheckBox(QStringLiteral("自动曝光"), exposureRow));
    parameterLayout->addWidget(exposureRow);
    parameterLayout->addWidget(makeSliderRow(QStringLiteral("亮度"), parameterBody));
    parameterLayout->addWidget(makeSliderRow(QStringLiteral("对比度"), parameterBody));
    parameterLayout->addWidget(makeSliderRow(QStringLiteral("饱和度"), parameterBody));
    rightLayout->addWidget(makePanel(QStringLiteral("⚙  相机参数"), parameterBody, right));

    auto *mediaBody = new QWidget(right);
    auto *mediaLayout = new QGridLayout(mediaBody);
    mediaLayout->setContentsMargins(14, 10, 14, 12);
    mediaLayout->setSpacing(8);
    for (int index = 0; index < 4; ++index) {
        const QString name = index == 3 ? QStringLiteral("视频\nVID_0001.mp4") : QStringLiteral("图片\nIMG_000%1.png").arg(index + 1);
        auto *thumb = makeButton(name, QStringLiteral("thumbnail"), false, mediaBody);
        mediaLayout->addWidget(thumb, 0, index);
    }
    rightLayout->addWidget(makePanel(QStringLiteral("图像/视频管理       打开目录"), mediaBody, right), 1);

    grid->addWidget(previewColumn, 0, 0);
    grid->addWidget(right, 0, 1);
    grid->setColumnStretch(0, 5);
    grid->setColumnStretch(1, 3);

    connect(photo, &QPushButton::clicked, this, [this]() { showTemplateNotice(QStringLiteral("拍照：未申请摄像头，未写入文件")); });
    connect(record, &QPushButton::clicked, this, [this]() { showTemplateNotice(QStringLiteral("录像：未申请摄像头，未创建媒体文件")); });
    connect(motion, &QPushButton::clicked, this, [this]() { setScreen(Screen::CameraMotion); });
    return page;
}

void ClientTemplateWindow::buildParameterDrawer()
{
    parameterDrawer_ = new QFrame(this);
    parameterDrawer_->setObjectName(QStringLiteral("parameterDrawer"));
    auto *layout = new QVBoxLayout(parameterDrawer_);
    layout->setContentsMargins(18, 16, 18, 18);
    layout->setSpacing(12);

    auto *header = new QHBoxLayout;
    header->addWidget(makeLabel(QStringLiteral("采集参数 · UI 草案"), QStringLiteral("drawerTitle"), parameterDrawer_));
    header->addStretch(1);
    auto *close = makeButton(QStringLiteral("×"), QStringLiteral("iconAction"), false, parameterDrawer_);
    header->addWidget(close);
    layout->addLayout(header);
    layout->addWidget(makeLabel(QStringLiteral("未连接设备；字段值仅用于布局，不会创建硬件任务。"), QStringLiteral("drawerNote"), parameterDrawer_));

    auto *scroll = new QScrollArea(parameterDrawer_);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto *formHost = new QWidget(scroll);
    auto *form = new QFormLayout(formHost);
    form->setContentsMargins(0, 4, 8, 4);
    form->setHorizontalSpacing(12);
    form->setVerticalSpacing(10);
    form->addRow(QStringLiteral("采集卡"), combo({QStringLiteral("未发现设备")}, formHost));
    form->addRow(QStringLiteral("输出通道"), combo({QStringLiteral("不使用输出通道")}, formHost));
    form->addRow(QStringLiteral("输出波形"), combo({QStringLiteral("正弦波"), QStringLiteral("方波")}, formHost));
    form->addRow(QStringLiteral("波形频率"), decimalSpin(10.0, 0.0, 100000.0, 2, formHost));
    form->addRow(QStringLiteral("波形幅值"), decimalSpin(1.0, -10.0, 10.0, 3, formHost));
    form->addRow(QStringLiteral("波形偏移量"), decimalSpin(0.0, -10.0, 10.0, 3, formHost));
    form->addRow(QStringLiteral("采集通道"), combo({QStringLiteral("待设备枚举")}, formHost));
    form->addRow(QStringLiteral("采样率"), decimalSpin(100000.0, 1.0, 10000000.0, 0, formHost));
    auto *points = new QSpinBox(formHost);
    points->setRange(1, 1000000);
    points->setValue(100000);
    form->addRow(QStringLiteral("显示窗口读取点数"), points);
    form->addRow(QStringLiteral("量程上限"), decimalSpin(10.0, -1000.0, 1000.0, 2, formHost));
    form->addRow(QStringLiteral("量程下限"), decimalSpin(-10.0, -1000.0, 1000.0, 2, formHost));
    scroll->setWidget(formHost);
    layout->addWidget(scroll, 1);

    auto *actions = new QHBoxLayout;
    auto *cancel = makeButton(QStringLiteral("取消"), QStringLiteral("secondary"), false, parameterDrawer_);
    auto *save = makeButton(QStringLiteral("保存参数"), QStringLiteral("primary"), false, parameterDrawer_);
    actions->addWidget(cancel);
    actions->addWidget(save);
    layout->addLayout(actions);

    connect(close, &QPushButton::clicked, this, [this]() { setScreen(Screen::Main); });
    connect(cancel, &QPushButton::clicked, this, [this]() { setScreen(Screen::Main); });
    connect(save, &QPushButton::clicked, this, [this]() { showTemplateNotice(QStringLiteral("参数未写入配置；等待正式业务接口")); });
    parameterDrawer_->hide();
}

void ClientTemplateWindow::buildMovementPanel()
{
    movementPanel_ = new QFrame(cameraPage_);
    movementPanel_->setObjectName(QStringLiteral("movementPanel"));
    auto *layout = new QVBoxLayout(movementPanel_);
    layout->setContentsMargins(18, 14, 18, 16);
    layout->setSpacing(10);

    auto *header = new QHBoxLayout;
    header->addWidget(makeLabel(QStringLiteral("↔  位移/缩放控制"), QStringLiteral("drawerTitle"), movementPanel_));
    header->addStretch(1);
    auto *close = makeButton(QStringLiteral("×"), QStringLiteral("iconAction"), false, movementPanel_);
    header->addWidget(close);
    layout->addLayout(header);

    auto *columns = new QHBoxLayout;
    auto *direction = new QWidget(movementPanel_);
    auto *directionLayout = new QGridLayout(direction);
    directionLayout->addWidget(makeLabel(QStringLiteral("图像位移"), QStringLiteral("fieldLabel"), direction), 0, 0, 1, 3);
    auto *up = makeButton(QStringLiteral("▲"), QStringLiteral("movement"), false, direction);
    auto *left = makeButton(QStringLiteral("◀"), QStringLiteral("movement"), false, direction);
    auto *right = makeButton(QStringLiteral("▶"), QStringLiteral("movement"), false, direction);
    auto *down = makeButton(QStringLiteral("▼"), QStringLiteral("movement"), false, direction);
    directionLayout->addWidget(up, 1, 1);
    directionLayout->addWidget(left, 2, 0);
    directionLayout->addWidget(right, 2, 2);
    directionLayout->addWidget(down, 3, 1);
    columns->addWidget(direction, 2);

    auto *divider = new QFrame(movementPanel_);
    divider->setFrameShape(QFrame::VLine);
    columns->addWidget(divider);

    auto *zoom = new QWidget(movementPanel_);
    auto *zoomLayout = new QVBoxLayout(zoom);
    zoomLayout->addWidget(makeLabel(QStringLiteral("缩放控制"), QStringLiteral("fieldLabel"), zoom));
    auto *zoomIn = makeButton(QStringLiteral("⊕\n放大"), QStringLiteral("movement"), false, zoom);
    auto *zoomOut = makeButton(QStringLiteral("⊖\n缩小"), QStringLiteral("movement"), false, zoom);
    zoomLayout->addWidget(zoomIn);
    zoomLayout->addWidget(zoomOut);
    columns->addWidget(zoom, 1);
    layout->addLayout(columns, 1);

    connect(close, &QPushButton::clicked, this, [this]() { setScreen(Screen::Camera); });
    for (auto *button : {up, left, right, down, zoomIn, zoomOut}) {
        connect(button, &QPushButton::clicked, this, [this]() { showTemplateNotice(QStringLiteral("仅改变模拟画布视图，不控制实体镜头")); });
    }
    movementPanel_->hide();
}

void ClientTemplateWindow::setScreen(Screen screen)
{
    screen_ = screen;
    const bool camera = screen == Screen::Camera || screen == Screen::CameraMotion;
    pages_->setCurrentIndex(camera ? 1 : 0);
    selectPageButton(camera ? 1 : 0);
    parameterDrawer_->setVisible(screen == Screen::Parameters);
    movementPanel_->setVisible(screen == Screen::CameraMotion);
    homeRail_->setChecked(screen == Screen::Main || screen == Screen::Parameters);
    parameterRail_->setChecked(screen == Screen::Parameters);
    updateOverlays();
    if (parameterDrawer_->isVisible()) {
        parameterDrawer_->raise();
    }
    if (movementPanel_->isVisible()) {
        movementPanel_->raise();
    }
}

bool ClientTemplateWindow::saveCapture(const QString &path)
{
    const QFileInfo info(path);
    if (!info.absolutePath().isEmpty() && !QDir().mkpath(info.absolutePath())) {
        return false;
    }
    return grab().save(path, "PNG");
}

void ClientTemplateWindow::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_F1:
        setScreen(Screen::Main);
        return;
    case Qt::Key_F2:
        setScreen(Screen::Parameters);
        return;
    case Qt::Key_F3:
        setScreen(Screen::Camera);
        return;
    case Qt::Key_F4:
        setScreen(Screen::CameraMotion);
        return;
    case Qt::Key_Escape:
        if (screen_ == Screen::Parameters) {
            setScreen(Screen::Main);
        } else if (screen_ == Screen::CameraMotion) {
            setScreen(Screen::Camera);
        } else {
            close();
        }
        return;
    default:
        break;
    }
    QWidget::keyPressEvent(event);
}

void ClientTemplateWindow::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateOverlays();
}

void ClientTemplateWindow::updateOverlays()
{
    if (parameterDrawer_) {
        parameterDrawer_->setGeometry(88, 64, qMin(420, qMax(360, width() / 3)), qMax(240, height() - 64));
    }
    if (movementPanel_ && cameraPage_) {
        const int panelWidth = qMin(410, qMax(350, cameraPage_->width() / 3));
        const int panelHeight = qMin(330, qMax(290, cameraPage_->height() / 2));
        movementPanel_->setGeometry(qMax(12, cameraPage_->width() - panelWidth - 24), 96, panelWidth, panelHeight);
    }
}

void ClientTemplateWindow::selectPageButton(int index)
{
    for (std::size_t item = 0; item < tabButtons_.size(); ++item) {
        tabButtons_[item]->setChecked(static_cast<int>(item) == index);
    }
}

void ClientTemplateWindow::showTemplateNotice(const QString &text)
{
    templateNotice_->setText(text);
}

} // namespace clientui
