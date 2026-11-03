#include "AdminTemplateWindow.h"

#include "CameraPreviewWidget.h"
#include "ChartWidget.h"

#include <QAbstractItemView>
#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFileInfo>
#include <QFormLayout>
#include <QFrame>
#include <QGridLayout>
#include <QHeaderView>
#include <QKeyEvent>
#include <QLabel>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QSlider>
#include <QSpinBox>
#include <QStackedWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

namespace adminui {

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
    layout->addWidget(makeMetricRow(QStringLiteral("⌁  峰值吸光度"), QStringLiteral("0.0978  · 模拟"), body));
    layout->addWidget(makeMetricRow(QStringLiteral("☼  光强百分比"), QStringLiteral("78.65 %  · 模拟"), body));
    layout->addWidget(makeMetricRow(QStringLiteral("◇  气体浓度"), QStringLiteral("—  未校准"), body));
    return body;
}

QWidget *makeChartBody(ChartWidget::Mode mode, const QString &xLabel, const QString &yLabel, QWidget *parent)
{
    auto *chart = new ChartWidget(mode, parent);
    chart->setAxisLabels(xLabel, yLabel);
    return chart;
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

} // namespace

AdminTemplateWindow::AdminTemplateWindow(Screen initialScreen, QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("adminTemplateRoot"));
    setWindowTitle(QStringLiteral("TDLAS 测氢系统 · 管理员 Codex UI 模板"));
    setMinimumSize(1080, 680);
    resize(1504, 847);
    setFocusPolicy(Qt::StrongFocus);
    buildShell();
    buildParameterDrawer();
    buildMovementPanel();
    setScreen(initialScreen);
}

void AdminTemplateWindow::buildShell()
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
    pages_->addWidget(buildDebugPage());
    pages_->addWidget(buildPeakPage());
    cameraPage_ = buildCameraPage();
    pages_->addWidget(cameraPage_);
    contentLayout->addWidget(pages_, 1);

    bodyLayout->addWidget(content, 1);
    rootLayout->addWidget(body, 1);
}

QWidget *AdminTemplateWindow::buildTopBar()
{
    auto *bar = new QFrame(this);
    bar->setObjectName(QStringLiteral("topBar"));
    bar->setFixedHeight(64);
    auto *layout = new QHBoxLayout(bar);
    layout->setContentsMargins(22, 0, 14, 0);
    layout->setSpacing(14);

    auto *brand = makeLabel(QStringLiteral("◉  道纪量子   │   TDLAS 测氢系统"), QStringLiteral("brand"), bar);
    layout->addWidget(brand);

    layout->addSpacing(18);
    layout->addWidget(makeLabel(QStringLiteral("▣  设备状态：● 未连接"), QStringLiteral("statusPill"), bar));
    layout->addWidget(makeLabel(QStringLiteral("▶  采集状态：● 就绪"), QStringLiteral("statusPill"), bar));
    layout->addWidget(makeLabel(QStringLiteral("模拟数据"), QStringLiteral("modePill"), bar));
    layout->addStretch(1);

    templateNotice_ = makeLabel(QStringLiteral("UI 模板 · 无设备连接"), QStringLiteral("templateNotice"), bar);
    layout->addWidget(templateNotice_);
    layout->addWidget(makeButton(QStringLiteral("☼ 主题"), QStringLiteral("topAction"), false, bar));
    layout->addWidget(makeButton(QStringLiteral("⚙ 设置"), QStringLiteral("topAction"), false, bar));
    return bar;
}

QWidget *AdminTemplateWindow::buildSideRail()
{
    auto *rail = new QFrame(this);
    rail->setObjectName(QStringLiteral("sideRail"));
    rail->setFixedWidth(88);
    auto *layout = new QVBoxLayout(rail);
    layout->setContentsMargins(4, 14, 4, 14);
    layout->setSpacing(6);

    auto *home = makeButton(QStringLiteral("⌂\n主页面"), QStringLiteral("railAction"), true, rail);
    auto *parameters = makeButton(QStringLiteral("≡\n参数区"), QStringLiteral("railAction"), true, rail);
    auto *start = makeButton(QStringLiteral("▶\n开始采集"), QStringLiteral("railAction"), false, rail);
    auto *stop = makeButton(QStringLiteral("■\n停止采集"), QStringLiteral("railAction"), false, rail);
    auto *reset = makeButton(QStringLiteral("↻\n复位"), QStringLiteral("railAction"), false, rail);
    auto *save = makeButton(QStringLiteral("▣\n数据保存"), QStringLiteral("railAction"), false, rail);
    home->setChecked(true);

    for (auto *button : {home, parameters, start, stop, reset, save}) {
        button->setMinimumHeight(86);
        layout->addWidget(button);
    }
    layout->addStretch(1);

    connect(home, &QPushButton::clicked, this, [this, home, parameters]() {
        home->setChecked(true);
        parameters->setChecked(false);
        setScreen(Screen::Main);
    });
    connect(parameters, &QPushButton::clicked, this, [this, home, parameters](bool checked) {
        home->setChecked(!checked);
        parameters->setChecked(checked);
        setScreen(checked ? Screen::Parameters : Screen::Main);
    });
    for (auto *button : {start, stop, reset, save}) {
        connect(button, &QPushButton::clicked, this, [this, button]() {
            emit actionRequested(button->text());
            showTemplateNotice(QStringLiteral("%1：仅发出 UI 意图").arg(button->text().remove('\n')));
        });
    }
    return rail;
}

QWidget *AdminTemplateWindow::buildTabBar()
{
    auto *bar = new QFrame(this);
    bar->setObjectName(QStringLiteral("tabBar"));
    bar->setFixedHeight(64);
    auto *layout = new QHBoxLayout(bar);
    layout->setContentsMargins(16, 10, 16, 8);
    layout->setSpacing(12);

    tabGroup_ = new QButtonGroup(this);
    tabGroup_->setExclusive(true);
    const QStringList labels{
        QStringLiteral("⌂  主页面"),
        QStringLiteral("⚒  调试界面"),
        QStringLiteral("⌁  峰值数据"),
        QStringLiteral("▣  摄像界面")};
    for (int i = 0; i < static_cast<int>(tabButtons_.size()); ++i) {
        tabButtons_[i] = makeButton(labels.at(i), QStringLiteral("topTab"), true, bar);
        tabButtons_[i]->setMinimumWidth(170);
        tabGroup_->addButton(tabButtons_[i], i);
        layout->addWidget(tabButtons_[i]);
    }
    tabButtons_[0]->setChecked(true);
    layout->addStretch(1);

    const auto selectTab = [this](int id) {
        const std::array<Screen, 4> screens{Screen::Main, Screen::Debug, Screen::PeakData, Screen::Camera};
        setScreen(screens.at(static_cast<std::size_t>(id)));
    };
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    connect(tabGroup_, &QButtonGroup::idClicked, this, selectTab);
#else
    connect(tabGroup_, QOverload<int>::of(&QButtonGroup::buttonClicked), this, selectTab);
#endif
    return bar;
}

QWidget *AdminTemplateWindow::buildMainPage()
{
    auto *page = new QWidget(this);
    page->setObjectName(QStringLiteral("mainPage"));
    auto *grid = new QGridLayout(page);
    grid->setContentsMargins(16, 10, 16, 16);
    grid->setHorizontalSpacing(12);
    grid->setVerticalSpacing(12);

    auto *preview = new CameraPreviewWidget(page);
    grid->addWidget(makePanel(QStringLiteral("▣  摄像窗口"), preview, page), 0, 0);
    grid->addWidget(makePanel(
        QStringLiteral("⌁  FV 曲线（吸收光谱）"),
        makeChartBody(ChartWidget::Mode::FvCurve, QStringLiteral("扫描点数（a.u.）"), QStringLiteral("吸光度"), page),
        page), 0, 1);
    grid->addWidget(makePanel(
        QStringLiteral("⌁  FV 峰值数据记录（实时趋势）"),
        makeChartBody(ChartWidget::Mode::PeakTrend, QStringLiteral("时间（s）"), QStringLiteral("峰值吸光度"), page),
        page), 1, 0);
    grid->addWidget(makePanel(QStringLiteral("◴  实时结果"), makeResultsBody(page), page), 1, 1);
    grid->setColumnStretch(0, 13);
    grid->setColumnStretch(1, 11);
    grid->setRowStretch(0, 1);
    grid->setRowStretch(1, 1);
    return page;
}

QWidget *AdminTemplateWindow::buildDebugPage()
{
    auto *page = new QWidget(this);
    page->setObjectName(QStringLiteral("debugPage"));
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(16, 10, 16, 16);
    layout->setSpacing(12);

    auto *gridHost = new QWidget(page);
    auto *grid = new QGridLayout(gridHost);
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setSpacing(12);
    grid->addWidget(makePanel(QStringLiteral("⌁  原始信号（AI）"), makeChartBody(ChartWidget::Mode::RawSignal, QStringLiteral("时间（s）"), QStringLiteral("电压（V）"), page), page), 0, 0);
    grid->addWidget(makePanel(QStringLiteral("⌁  拟合曲线（透射光谱）"), makeChartBody(ChartWidget::Mode::FitCurve, QStringLiteral("扫描点数（a.u.）"), QStringLiteral("归一化强度"), page), page), 0, 1);
    grid->addWidget(makePanel(QStringLiteral("⌁  FV 曲线（吸收光谱）"), makeChartBody(ChartWidget::Mode::FvCurve, QStringLiteral("扫描点数（a.u.）"), QStringLiteral("吸光度"), page), page), 1, 0);
    grid->addWidget(makePanel(QStringLiteral("⌁  实时峰值历史（FV 峰值）"), makeChartBody(ChartWidget::Mode::PeakTrend, QStringLiteral("时间（s）"), QStringLiteral("峰值吸光度"), page), page), 1, 1);
    grid->setRowStretch(0, 1);
    grid->setRowStretch(1, 1);
    layout->addWidget(gridHost, 1);

    auto *summary = new QFrame(page);
    summary->setProperty("uiRole", QStringLiteral("resultStrip"));
    auto *summaryLayout = new QHBoxLayout(summary);
    summaryLayout->setContentsMargins(24, 6, 24, 6);
    summaryLayout->addWidget(makeLabel(QStringLiteral("⌁ 实时峰值：  0.0978 · 模拟"), QStringLiteral("stripMetric"), summary));
    summaryLayout->addWidget(makeLabel(QStringLiteral("☼ 光强百分比：  78.65 % · 模拟"), QStringLiteral("stripMetric"), summary));
    summaryLayout->addWidget(makeLabel(QStringLiteral("◇ 气体浓度：  — 未校准"), QStringLiteral("stripMetric"), summary));
    layout->addWidget(summary);
    return page;
}

QWidget *AdminTemplateWindow::buildPeakPage()
{
    auto *page = new QWidget(this);
    page->setObjectName(QStringLiteral("peakPage"));
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(12, 8, 12, 12);
    layout->setSpacing(10);

    auto *filters = new QFrame(page);
    filters->setProperty("uiRole", QStringLiteral("filterBar"));
    auto *filterLayout = new QHBoxLayout(filters);
    filterLayout->setContentsMargins(16, 9, 16, 9);
    filterLayout->setSpacing(8);
    filterLayout->addWidget(makeLabel(QStringLiteral("时间范围："), QStringLiteral("fieldLabel"), filters));
    for (const QString &text : {QStringLiteral("本次 run"), QStringLiteral("最近 1 分钟"), QStringLiteral("10 分钟"), QStringLiteral("1 小时"), QStringLiteral("自定义")}) {
        filterLayout->addWidget(makeButton(text, text == QStringLiteral("本次 run") ? QStringLiteral("primary") : QStringLiteral("secondary"), false, filters));
    }
    filterLayout->addSpacing(10);
    filterLayout->addWidget(makeLabel(QStringLiteral("选择 run："), QStringLiteral("fieldLabel"), filters));
    filterLayout->addWidget(combo({QStringLiteral("run_20250117_001 · 模拟")}, filters));
    filterLayout->addWidget(new QCheckBox(QStringLiteral("自动跟随"), filters));
    filterLayout->addStretch(1);
    filterLayout->addWidget(makeButton(QStringLiteral("↶ 返回实时"), QStringLiteral("secondary"), false, filters));
    filterLayout->addWidget(makeButton(QStringLiteral("⌕ 缩放复位"), QStringLiteral("secondary"), false, filters));
    filterLayout->addWidget(makeButton(QStringLiteral("▣ 导出 CSV"), QStringLiteral("secondary"), false, filters));
    layout->addWidget(filters);

    auto *trendBody = new QWidget(page);
    auto *trendLayout = new QVBoxLayout(trendBody);
    trendLayout->setContentsMargins(10, 8, 10, 8);
    trendLayout->setSpacing(5);
    auto *stats = makeLabel(QStringLiteral("历史条数 592（模拟）  │  有效 571  │  无效 21  │  当前 runId run_20250117_001  │  当前峰值 0.0978  │  质量状态：模拟"), QStringLiteral("statsLine"), trendBody);
    trendLayout->addWidget(stats);
    trendLayout->addWidget(makeChartBody(ChartWidget::Mode::PeakTrend, QStringLiteral("时间（s）"), QStringLiteral("峰值吸光度（a.u.）"), trendBody), 1);
    layout->addWidget(makePanel(QStringLiteral("⌁  峰值趋势（PeakRecord）"), trendBody, page), 5);

    auto *table = new QTableWidget(10, 8, page);
    table->setObjectName(QStringLiteral("peakRecordTable"));
    table->setHorizontalHeaderLabels({QStringLiteral("序号"), QStringLiteral("采集时间"), QStringLiteral("elapsed (s)"), QStringLiteral("峰值 (a.u.)"), QStringLiteral("峰值宽 peakX"), QStringLiteral("单位"), QStringLiteral("有效性 / 原因"), QStringLiteral("runId")});
    const QStringList times{
        QStringLiteral("2025-01-17 14:23:10.103"), QStringLiteral("2025-01-17 14:23:11.104"), QStringLiteral("2025-01-17 14:23:12.102"),
        QStringLiteral("2025-01-17 14:23:13.103"), QStringLiteral("2025-01-17 14:23:14.105"), QStringLiteral("2025-01-17 14:23:15.103"),
        QStringLiteral("2025-01-17 14:25:01.226"), QStringLiteral("2025-01-17 14:25:02.228"), QStringLiteral("2025-01-17 14:25:03.229"), QStringLiteral("2025-01-17 14:25:04.231")};
    for (int row = 0; row < table->rowCount(); ++row) {
        const bool invalid = row == 7 || row == 8;
        const QStringList values{
            QString::number(row + 1), times.at(row), QString::number(row < 7 ? row : 104 + row, 'f', 1),
            invalid && row == 8 ? QStringLiteral("—") : QString::number(0.0976 + (row % 4) * 0.0001, 'f', 4),
            invalid && row == 8 ? QStringLiteral("null") : QString::number(52341 + row * 7), QStringLiteral("a.u."),
            invalid ? QStringLiteral("● 模拟无效") : QStringLiteral("● 模拟有效"), QStringLiteral("run_20250117_001")};
        for (int column = 0; column < values.size(); ++column) {
            auto *item = new QTableWidgetItem(values.at(column));
            item->setTextAlignment(Qt::AlignCenter);
            if (invalid) {
                item->setBackground(QColor(QStringLiteral("#fff0f0")));
                item->setForeground(QColor(QStringLiteral("#a82d3d")));
            }
            table->setItem(row, column, item);
        }
    }
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setAlternatingRowColors(true);
    table->verticalHeader()->setVisible(false);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->setMinimumHeight(210);
    layout->addWidget(makePanel(QStringLiteral("▦  峰值记录明细 · 全部为稳定模拟数据"), table, page), 4);
    return page;
}

QWidget *AdminTemplateWindow::buildCameraPage()
{
    auto *page = new QWidget(this);
    page->setObjectName(QStringLiteral("cameraPage"));
    auto *layout = new QHBoxLayout(page);
    layout->setContentsMargins(16, 10, 16, 16);
    layout->setSpacing(12);

    auto *left = new QWidget(page);
    auto *leftLayout = new QVBoxLayout(left);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(12);
    auto *preview = new CameraPreviewWidget(left);
    leftLayout->addWidget(makePanel(QStringLiteral("▣  相机预览"), preview, left), 1);
    auto *captureBar = new QFrame(left);
    captureBar->setProperty("uiRole", QStringLiteral("captureBar"));
    auto *captureLayout = new QHBoxLayout(captureBar);
    captureLayout->setContentsMargins(22, 10, 22, 10);
    captureLayout->addWidget(makeButton(QStringLiteral("▣ 拍照"), QStringLiteral("secondary"), false, captureBar));
    captureLayout->addWidget(makeButton(QStringLiteral("▶ 开始录像"), QStringLiteral("secondary"), false, captureBar));
    auto *stop = makeButton(QStringLiteral("■ 停止录像"), QStringLiteral("secondary"), false, captureBar);
    stop->setEnabled(false);
    captureLayout->addWidget(stop);
    captureLayout->addStretch(1);
    captureLayout->addWidget(makeLabel(QStringLiteral("● 录像时长：00:00:00"), QStringLiteral("fieldLabel"), captureBar));
    leftLayout->addWidget(captureBar);
    layout->addWidget(left, 5);

    auto *right = new QWidget(page);
    auto *rightLayout = new QVBoxLayout(right);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(12);

    auto *deviceBody = new QWidget(right);
    auto *deviceGrid = new QGridLayout(deviceBody);
    deviceGrid->setContentsMargins(14, 10, 14, 12);
    deviceGrid->setHorizontalSpacing(8);
    deviceGrid->setVerticalSpacing(8);
    deviceGrid->addWidget(makeLabel(QStringLiteral("相机选择"), QStringLiteral("fieldLabel"), deviceBody), 0, 0);
    deviceGrid->addWidget(combo({QStringLiteral("USB Camera (Dev0) · 未连接")}, deviceBody), 0, 1);
    auto *motion = makeButton(QStringLiteral("⇄ 位移/缩放设置"), QStringLiteral("secondary"), false, deviceBody);
    deviceGrid->addWidget(motion, 0, 2);
    deviceGrid->addWidget(makeLabel(QStringLiteral("分辨率"), QStringLiteral("fieldLabel"), deviceBody), 1, 0);
    deviceGrid->addWidget(combo({QStringLiteral("1920 × 1080 (16:9)")}, deviceBody), 1, 1, 1, 2);
    deviceGrid->addWidget(makeLabel(QStringLiteral("帧率"), QStringLiteral("fieldLabel"), deviceBody), 2, 0);
    deviceGrid->addWidget(combo({QStringLiteral("30 FPS")}, deviceBody), 2, 1, 1, 2);
    rightLayout->addWidget(makePanel(QStringLiteral("☷  相机控制"), deviceBody, right));

    auto *paramBody = new QWidget(right);
    auto *paramGrid = new QGridLayout(paramBody);
    paramGrid->setContentsMargins(14, 10, 14, 12);
    paramGrid->setHorizontalSpacing(8);
    paramGrid->setVerticalSpacing(8);
    paramGrid->addWidget(makeLabel(QStringLiteral("曝光时间"), QStringLiteral("fieldLabel"), paramBody), 0, 0);
    paramGrid->addWidget(decimalSpin(10.0, 0.1, 1000.0, 2, paramBody), 0, 1);
    paramGrid->addWidget(makeLabel(QStringLiteral("ms"), QStringLiteral("fieldLabel"), paramBody), 0, 2);
    paramGrid->addWidget(new QCheckBox(QStringLiteral("自动曝光"), paramBody), 0, 3);
    paramGrid->addWidget(makeLabel(QStringLiteral("增益"), QStringLiteral("fieldLabel"), paramBody), 1, 0);
    paramGrid->addWidget(decimalSpin(6.0, 0.0, 24.0, 1, paramBody), 1, 1);
    paramGrid->addWidget(makeLabel(QStringLiteral("dB"), QStringLiteral("fieldLabel"), paramBody), 1, 2);
    paramGrid->addWidget(combo({QStringLiteral("白平衡：自动")}, paramBody), 1, 3);
    const QStringList sliderNames{QStringLiteral("亮度"), QStringLiteral("对比度"), QStringLiteral("饱和度")};
    for (int row = 0; row < sliderNames.size(); ++row) {
        paramGrid->addWidget(makeLabel(sliderNames.at(row), QStringLiteral("fieldLabel"), paramBody), row + 2, 0);
        auto *slider = new QSlider(Qt::Horizontal, paramBody);
        slider->setValue(50);
        paramGrid->addWidget(slider, row + 2, 1, 1, 2);
        auto *value = new QSpinBox(paramBody);
        value->setRange(0, 100);
        value->setValue(50);
        paramGrid->addWidget(value, row + 2, 3);
    }
    rightLayout->addWidget(makePanel(QStringLiteral("⚙  相机参数"), paramBody, right));

    auto *mediaBody = new QWidget(right);
    auto *mediaLayout = new QVBoxLayout(mediaBody);
    mediaLayout->setContentsMargins(14, 10, 14, 12);
    auto *open = makeButton(QStringLiteral("▭ 打开目录"), QStringLiteral("link"), false, mediaBody);
    mediaLayout->addWidget(open, 0, Qt::AlignRight);
    auto *thumbs = new QHBoxLayout();
    for (const QString &name : {QStringLiteral("IMG_0001.png"), QStringLiteral("IMG_0002.png"), QStringLiteral("IMG_0003.png"), QStringLiteral("VID_0001.mp4")}) {
        auto *thumb = makeButton(QStringLiteral("▧\n%1").arg(name), QStringLiteral("thumbnail"), false, mediaBody);
        thumbs->addWidget(thumb);
    }
    mediaLayout->addLayout(thumbs);
    rightLayout->addWidget(makePanel(QStringLiteral("▣  图像/视频管理 · 模拟条目"), mediaBody, right), 1);
    layout->addWidget(right, 3);

    connect(motion, &QPushButton::clicked, this, [this]() { setScreen(Screen::CameraMotion); });
    for (auto *button : page->findChildren<QPushButton *>()) {
        if (button == motion) {
            continue;
        }
        connect(button, &QPushButton::clicked, this, [this, button]() {
            emit actionRequested(button->text());
            showTemplateNotice(QStringLiteral("摄像功能未接入：%1").arg(button->text().remove('\n')));
        });
    }
    return page;
}

void AdminTemplateWindow::buildParameterDrawer()
{
    parameterDrawer_ = new QFrame(this);
    parameterDrawer_->setObjectName(QStringLiteral("parameterDrawer"));
    parameterDrawer_->setVisible(false);
    auto *layout = new QVBoxLayout(parameterDrawer_);
    layout->setContentsMargins(18, 14, 18, 18);
    layout->setSpacing(10);

    auto *header = new QHBoxLayout();
    header->addWidget(makeLabel(QStringLiteral("采集参数"), QStringLiteral("drawerTitle"), parameterDrawer_));
    header->addStretch(1);
    auto *close = makeButton(QStringLiteral("×"), QStringLiteral("iconAction"), false, parameterDrawer_);
    close->setFixedSize(38, 38);
    header->addWidget(close);
    layout->addLayout(header);

    auto *formHost = new QWidget(parameterDrawer_);
    auto *form = new QFormLayout(formHost);
    form->setContentsMargins(0, 0, 0, 0);
    form->setHorizontalSpacing(12);
    form->setVerticalSpacing(12);
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    form->addRow(QStringLiteral("1.采集卡（设备识别）"), combo({QStringLiteral("NI USB-6361 (Dev1) · 未连接")}, formHost));
    form->addRow(QStringLiteral("2.输出通道"), combo({QStringLiteral("AO0")}, formHost));
    form->addRow(QStringLiteral("3.输出波形"), combo({QStringLiteral("正弦波")}, formHost));
    form->addRow(QStringLiteral("4.波形频率"), decimalSpin(10.000, 0.001, 100000.0, 3, formHost));
    form->addRow(QStringLiteral("5.波形幅值"), decimalSpin(2.000, 0.0, 10.0, 3, formHost));
    form->addRow(QStringLiteral("6.波形偏移量"), decimalSpin(0.000, -10.0, 10.0, 3, formHost));
    form->addRow(QStringLiteral("7.采集通道"), combo({QStringLiteral("AI0")}, formHost));
    auto *sampleRate = new QSpinBox(formHost);
    sampleRate->setRange(1, 2000000);
    sampleRate->setValue(200000);
    form->addRow(QStringLiteral("8.采样率（S/s）"), sampleRate);
    auto *windowPoints = new QSpinBox(formHost);
    windowPoints->setRange(100, 1000000);
    windowPoints->setValue(100000);
    form->addRow(QStringLiteral("9.显示窗口读取点数"), windowPoints);
    form->addRow(QStringLiteral("10.量程上限（V）"), decimalSpin(10.000, -10.0, 10.0, 3, formHost));
    form->addRow(QStringLiteral("11.量程下限（V）"), decimalSpin(-10.000, -10.0, 10.0, 3, formHost));
    layout->addWidget(formHost, 1);

    auto *actions = new QHBoxLayout();
    auto *save = makeButton(QStringLiteral("▣ 保存参数"), QStringLiteral("primary"), false, parameterDrawer_);
    auto *restore = makeButton(QStringLiteral("↻ 恢复默认"), QStringLiteral("secondary"), false, parameterDrawer_);
    actions->addWidget(save);
    actions->addWidget(restore);
    layout->addLayout(actions);

    connect(close, &QPushButton::clicked, this, [this]() { setScreen(Screen::Main); });
    connect(save, &QPushButton::clicked, this, [this]() { showTemplateNotice(QStringLiteral("参数仅保留在模板 UI，未写入设备")); });
    connect(restore, &QPushButton::clicked, this, [this]() { showTemplateNotice(QStringLiteral("未配置正式默认参数契约")); });
}

void AdminTemplateWindow::buildMovementPanel()
{
    movementPanel_ = new QFrame(cameraPage_);
    movementPanel_->setObjectName(QStringLiteral("movementPanel"));
    movementPanel_->setVisible(false);
    auto *layout = new QVBoxLayout(movementPanel_);
    layout->setContentsMargins(16, 12, 16, 16);
    layout->setSpacing(10);

    auto *header = new QHBoxLayout();
    header->addWidget(makeLabel(QStringLiteral("⇄  位移/缩放控制"), QStringLiteral("drawerTitle"), movementPanel_));
    header->addStretch(1);
    auto *close = makeButton(QStringLiteral("×"), QStringLiteral("iconAction"), false, movementPanel_);
    close->setFixedSize(34, 34);
    header->addWidget(close);
    layout->addLayout(header);

    auto *content = new QHBoxLayout();
    auto *moveBox = new QFrame(movementPanel_);
    auto *moveGrid = new QGridLayout(moveBox);
    moveGrid->setContentsMargins(8, 8, 8, 8);
    moveGrid->addWidget(makeLabel(QStringLiteral("图像位移"), QStringLiteral("fieldLabel"), moveBox), 0, 0, 1, 3, Qt::AlignCenter);
    auto *up = makeButton(QStringLiteral("▲"), QStringLiteral("movement"), false, moveBox);
    auto *left = makeButton(QStringLiteral("◀"), QStringLiteral("movement"), false, moveBox);
    auto *right = makeButton(QStringLiteral("▶"), QStringLiteral("movement"), false, moveBox);
    auto *down = makeButton(QStringLiteral("▼"), QStringLiteral("movement"), false, moveBox);
    moveGrid->addWidget(up, 1, 1);
    moveGrid->addWidget(left, 2, 0);
    moveGrid->addWidget(right, 2, 2);
    moveGrid->addWidget(down, 3, 1);
    content->addWidget(moveBox, 3);

    auto *zoomBox = new QFrame(movementPanel_);
    auto *zoomLayout = new QVBoxLayout(zoomBox);
    zoomLayout->setContentsMargins(8, 8, 8, 8);
    zoomLayout->addWidget(makeLabel(QStringLiteral("缩放控制"), QStringLiteral("fieldLabel"), zoomBox), 0, Qt::AlignCenter);
    auto *zoomIn = makeButton(QStringLiteral("⊕\n放大"), QStringLiteral("movement"), false, zoomBox);
    auto *zoomOut = makeButton(QStringLiteral("⊖\n缩小"), QStringLiteral("movement"), false, zoomBox);
    zoomLayout->addWidget(zoomIn);
    zoomLayout->addWidget(zoomOut);
    content->addWidget(zoomBox, 2);
    layout->addLayout(content, 1);

    connect(close, &QPushButton::clicked, this, [this]() { setScreen(Screen::Camera); });
    for (auto *button : {up, left, right, down, zoomIn, zoomOut}) {
        connect(button, &QPushButton::clicked, this, [this, button]() {
            emit actionRequested(button->text());
            showTemplateNotice(QStringLiteral("仅改变模拟画面意图：%1").arg(button->text().remove('\n')));
        });
    }
}

void AdminTemplateWindow::setScreen(Screen screen)
{
    screen_ = screen;
    int index = 0;
    if (screen == Screen::Debug) {
        index = 1;
    } else if (screen == Screen::PeakData) {
        index = 2;
    } else if (screen == Screen::Camera || screen == Screen::CameraMotion) {
        index = 3;
    }
    pages_->setCurrentIndex(index);
    selectPageButton(index);
    parameterDrawer_->setVisible(screen == Screen::Parameters);
    movementPanel_->setVisible(screen == Screen::CameraMotion);
    updateOverlays();
}

bool AdminTemplateWindow::saveCapture(const QString &path)
{
    const QFileInfo info(path);
    if (!QDir().mkpath(info.absolutePath())) {
        return false;
    }
    return grab().save(info.absoluteFilePath(), "PNG");
}

void AdminTemplateWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() >= Qt::Key_F1 && event->key() <= Qt::Key_F6) {
        const std::array<Screen, 6> screens{Screen::Main, Screen::Parameters, Screen::Debug, Screen::PeakData, Screen::Camera, Screen::CameraMotion};
        setScreen(screens.at(static_cast<std::size_t>(event->key() - Qt::Key_F1)));
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Escape) {
        if (screen_ == Screen::Parameters) {
            setScreen(Screen::Main);
        } else if (screen_ == Screen::CameraMotion) {
            setScreen(Screen::Camera);
        } else {
            close();
        }
        event->accept();
        return;
    }
    QWidget::keyPressEvent(event);
}

void AdminTemplateWindow::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateOverlays();
}

void AdminTemplateWindow::updateOverlays()
{
    if (parameterDrawer_) {
        parameterDrawer_->setGeometry(88, 64, qMin(410, width() - 140), qMax(300, height() - 64));
        if (parameterDrawer_->isVisible()) {
            parameterDrawer_->raise();
        }
    }
    if (movementPanel_ && cameraPage_) {
        const int panelWidth = qMin(420, cameraPage_->width() - 40);
        movementPanel_->setGeometry(qMax(18, cameraPage_->width() - panelWidth - 18), 72, panelWidth, qMin(340, cameraPage_->height() - 90));
        if (movementPanel_->isVisible()) {
            movementPanel_->raise();
        }
    }
}

void AdminTemplateWindow::selectPageButton(int index)
{
    if (index >= 0 && index < static_cast<int>(tabButtons_.size())) {
        tabButtons_[static_cast<std::size_t>(index)]->setChecked(true);
    }
}

void AdminTemplateWindow::showTemplateNotice(const QString &text)
{
    if (templateNotice_) {
        templateNotice_->setText(text);
    }
}

} // namespace adminui
