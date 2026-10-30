#include "ui/main_window.h"
#include "ui/interactive_chart_view.h"
#include <QApplication>
#include <QAbstractSpinBox>
#include <QChartView>
#include <QCloseEvent>
#include <QCheckBox>
#include <QComboBox>
#include <QDateTime>
#include <QDir>
#include <QDoubleSpinBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QEventLoop>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QIcon>
#include <QIconEngine>
#include <QPainter>
#include <QPainterPath>
#include <QKeyEvent>
#include <QJsonDocument>
#include <QLabel>
#include <QLineSeries>
#include <QLegendMarker>
#include <QMessageBox>
#include <QPushButton>
#include <QPixmap>
#include <QResizeEvent>
#include <QScrollArea>
#include <QScrollBar>
#include <QSettings>
#include <QScopedValueRollback>
#include <QSlider>
#include <QSpinBox>
#include <QTabWidget>
#include <QTabBar>
#include <QStyle>
#include <QTableWidget>
#include <QTimeZone>
#include <QTimer>
#include <QThread>
#include <QValueAxis>
#include <QVBoxLayout>
#include <algorithm>
#include <cmath>

// Explicit reference ensures the resource object is linked from the static UI library.
static void initializeResources() { Q_INIT_RESOURCE(resources); }
namespace tdlas {
namespace {
// Logical-coordinate vector icons stay crisp at every device pixel ratio.
class WorkbenchIcon final : public QIconEngine {
public:
    explicit WorkbenchIcon(QString name) : name_(std::move(name)) {}
    QIconEngine* clone() const override { return new WorkbenchIcon(name_); }
    void paint(QPainter* p, const QRect& r, QIcon::Mode mode, QIcon::State) override {
        p->save(); p->setRenderHint(QPainter::Antialiasing);
        p->translate(r.center()); const double scale = std::min(r.width(), r.height()) / 24.0;
        p->scale(scale, scale); p->translate(-12, -12);
        const auto group = mode == QIcon::Disabled ? QPalette::Disabled : QPalette::Active;
        const QColor ink = qApp->palette().color(group, QPalette::ButtonText);
        p->setPen(QPen(ink, 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p->setBrush(Qt::NoBrush);
        if (name_ == "pulse") { p->drawPolyline(QPolygonF{{1,12},{6,12},{9,4},{12,21},{16,2},{19,12},{23,12}}); }
        else if (name_ == "user") { p->drawEllipse(QRectF(8,3,8,8)); p->drawArc(QRectF(4,13,16,12),0,180*16); p->drawLine(4,19,20,19); }
        else if (name_ == "play") { p->drawPolygon(QPolygonF{{7,4},{20,12},{7,20}}); }
        else if (name_ == "stop") { p->drawRect(QRectF(5,5,14,14)); }
        else if (name_ == "reset") { p->drawArc(QRectF(4,4,16,16),45*16,285*16); p->drawPolyline(QPolygonF{{14,3},{19,5},{18,10}}); }
        else if (name_ == "export") { p->drawPolyline(QPolygonF{{4,14},{4,21},{20,21},{20,14}}); p->drawLine(12,3,12,16); p->drawPolyline(QPolygonF{{7,8},{12,3},{17,8}}); }
        else if (name_ == "history") { p->drawRoundedRect(QRectF(5,2,14,20),2,2); for (int y : {7,12,17}) p->drawLine(8,y,16,y); }
        else if (name_ == "fit") { p->drawLine(4,20,4,5); p->drawLine(4,20,21,20); p->drawPolyline(QPolygonF{{6,17},{9,13},{12,15},{16,8},{20,6}}); }
        else if (name_ == "file") { p->drawRoundedRect(QRectF(5,3,14,18),1,1); p->drawLine(8,9,16,9); p->drawLine(8,13,16,13); p->drawLine(8,17,14,17); }
        else if (name_ == "bars") { p->drawLine(4,21,21,21); p->drawRect(QRectF(6,13,3,8)); p->drawRect(QRectF(11,8,3,13)); p->drawRect(QRectF(16,4,3,17)); }
        else if (name_ == "camera") { p->drawRoundedRect(QRectF(2,7,20,14),2,2); p->drawEllipse(QRectF(8,10,8,8)); p->drawPolyline(QPolygonF{{7,7},{9,3},{15,3},{17,7}}); }
        else if (name_ == "record") { p->setBrush(ink); p->drawEllipse(QRectF(5,5,14,14)); }
        else if (name_ == "save") { p->drawRoundedRect(QRectF(4,3,16,18),1.5,1.5); p->drawRect(QRectF(7,3,9,6)); p->drawRect(QRectF(7,13,10,8)); p->drawLine(14,4,14,8); }
        else if (name_ == "brightness") { p->drawEllipse(QRectF(7,7,10,10)); for(int a=0;a<8;++a){p->save();p->translate(12,12);p->rotate(a*45);p->drawLine(0,-8,0,-11);p->restore();} }
        else if (name_ == "contrast") { p->drawEllipse(QRectF(3,3,18,18)); p->setBrush(ink); p->drawPie(QRectF(3,3,18,18),90*16,180*16); }
        else if (name_ == "zoom") { p->drawEllipse(QRectF(3,3,14,14)); p->drawLine(16,16,22,22); p->drawLine(7,10,13,10); p->drawLine(10,7,10,13); }
        else if (name_ == "expand") { p->drawPolyline(QPolygonF{{3,9},{3,3},{9,3}}); p->drawPolyline(QPolygonF{{15,3},{21,3},{21,9}}); p->drawPolyline(QPolygonF{{21,15},{21,21},{15,21}}); p->drawPolyline(QPolygonF{{9,21},{3,21},{3,15}}); }
        else if (name_ == "refresh") { p->drawArc(QRectF(4,4,16,16),40*16,285*16); p->drawPolyline(QPolygonF{{14,3},{19,5},{18,10}}); }
        else if (name_ == "connect") { p->drawLine(12,2,12,11); p->drawArc(QRectF(4,5,16,16),-45*16,-270*16); }
        else if (name_ == "theme") { p->drawEllipse(QRectF(6,6,12,12)); for(int a=0;a<8;++a){p->save();p->translate(12,12);p->rotate(a*45);p->drawLine(0,-9,0,-11);p->restore();} }
        else { p->drawEllipse(QRectF(8,8,8,8)); for(int a=0;a<8;++a){p->save();p->translate(12,12);p->rotate(a*45);p->drawLine(0,-8,0,-10);p->restore();} }
        p->restore();
    }
    QPixmap pixmap(const QSize& size, QIcon::Mode mode, QIcon::State state) override {
        QPixmap result(size); result.fill(Qt::transparent); QPainter p(&result); paint(&p, QRect(QPoint(),size),mode,state); return result;
    }
private:
    QString name_;
};
QIcon workbenchIcon(const QString& name) { return QIcon(new WorkbenchIcon(name)); }
class SampleTabBar final : public QTabBar {
public:
    SampleTabBar() { setIconSize(QSize(24,24)); setExpanding(true); setDrawBase(false); setMouseTracking(true); }
    QJsonObject colors;
protected:
    QSize tabSizeHint(int index) const override { return QSize(std::max(fontMetrics().horizontalAdvance(tabText(index)) + 80, parentWidget() ? (parentWidget()->width()-4)/4 : 240),54); }
    QSize minimumTabSizeHint(int) const override { return QSize(180,54); }
    void keyPressEvent(QKeyEvent* e) override {
        if(e->key()==Qt::Key_Return || e->key()==Qt::Key_Space) { e->accept(); return; }
        QTabBar::keyPressEvent(e);
    }
    void paintEvent(QPaintEvent*) override {
        QPainter p(this); p.setRenderHint(QPainter::Antialiasing);
        const auto color = [this](const char* key){return QColor(colors[key].toString());};
        p.setPen(color("controlBorder")); p.drawLine(0,height()-1,width(),height()-1);
        for(int i=0;i<count();++i) {
            if(!isTabVisible(i)) continue;
            const QRectF r=tabRect(i).adjusted(1,2,-1,-2); const bool selected=i==currentIndex();
            const bool hovered=underMouse() && tabAt(mapFromGlobal(QCursor::pos()))==i;
            QPainterPath path; path.moveTo(r.left(),r.bottom()); path.lineTo(r.left(),r.top()+6);
            path.quadTo(r.left(),r.top(),r.left()+6,r.top()); path.lineTo(r.right()-20,r.top());
            path.quadTo(r.right()-15,r.top(),r.right()-13,r.top()+6); path.lineTo(r.right(),r.bottom()); path.closeSubpath();
            QLinearGradient fill(r.topLeft(),r.bottomLeft());
            fill.setColorAt(0,color(selected ? "primary" : "surface"));
            fill.setColorAt(1,color(selected ? "primaryHover" : hovered ? "selectedTabBackground" : "surfaceSubtle"));
            p.setBrush(fill); p.setPen(color("borderSubtle")); p.drawPath(path);
            const auto ink=color(!isTabEnabled(i) ? "disabledText" : selected ? "onPrimary" : "text");
            QPixmap icon=tabIcon(i).pixmap(QSize(24,24),devicePixelRatioF());
            { QPainter tint(&icon); tint.setCompositionMode(QPainter::CompositionMode_SourceIn); tint.fillRect(icon.rect(),ink); }
            const int textWidth=fontMetrics().horizontalAdvance(tabText(i));
            const int start=qRound(r.center().x()-(textWidth+38)/2.0);
            p.drawPixmap(QRect(start,qRound(r.center().y()-12),24,24),icon);
            p.setPen(ink); p.drawText(QRect(start+38,r.top(),textWidth+8,r.height()),Qt::AlignVCenter,tabText(i));
            if(hasFocus() && selected) { p.setBrush(Qt::NoBrush); p.setPen(QPen(ink,2,Qt::DashLine)); p.drawRoundedRect(r.adjusted(5,5,-20,-5),3,3); }
        }
    }
};
class SampleTabs final : public QTabWidget {
public:
    SampleTabs() { setTabBar(new SampleTabBar); }
protected:
    void resizeEvent(QResizeEvent* event) override { QTabWidget::resizeEvent(event); tabBar()->updateGeometry(); }
};
class CameraPreview final : public QLabel {
public:
    void setFrame(const QImage& frame) { frame_=frame; refresh(); }
protected:
    void resizeEvent(QResizeEvent* event) override {
        QLabel::resizeEvent(event);
        if (auto* overlay = findChild<QWidget*>(QStringLiteral("cameraPreviewOverlay"), Qt::FindDirectChildrenOnly)) {
            overlay->adjustSize();
            overlay->move(std::max(10, width() - overlay->width() - 10), 10);
            overlay->raise();
        }
        refresh();
    }
private:
    QImage frame_;
    void refresh() {
        if(frame_.isNull()) return;
        setPixmap(QPixmap::fromImage(frame_).scaled(contentsRect().size(),Qt::KeepAspectRatio,Qt::SmoothTransformation));
    }
};
QWidget* cardHeading(const QString& title, const QString& iconName, QHBoxLayout** row = nullptr) {
    auto* header = new QFrame; header->setProperty("cardHeader",true);
    auto* layout = new QHBoxLayout(header); layout->setContentsMargins(14,6,14,6); layout->setSpacing(10);
    auto* icon = new QLabel; icon->setProperty("workbenchIcon",iconName); icon->setFixedSize(26,26);
    auto* heading = new QLabel(title); heading->setProperty("heading",true); heading->setProperty("chartTitle", true);
    heading->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    heading->setMinimumWidth(heading->fontMetrics().horizontalAdvance(title) + 8);
    heading->setTextFormat(Qt::PlainText);
    layout->addWidget(icon); layout->addWidget(heading); layout->addStretch(1);
    if(row) *row=layout;
    return header;
}
class Workspace final : public QWidget {
public:
    QWidget *left = nullptr, *right = nullptr;
    std::function<void(bool)> compactChanged;
    explicit Workspace(QWidget* parent = nullptr) : QWidget(parent) { setMinimumWidth(1440); }
protected:
    void resizeEvent(QResizeEvent* e) override {
        QWidget::resizeEvent(e);
        // Give the parameter editor a little more room for the value/unit
        // split controls while keeping the debug canvas as a 2x2 grid.
        const int leftWidth = qRound(width() * 0.22);
        if (left) left->setGeometry(0, 0, leftWidth, height());
        if (right) right->setGeometry(leftWidth, 0, width() - leftWidth, height());
        // Dense controls are used only when the narrower workspace still has
        // enough height to show all three sidebar sections. Short windows keep
        // the regular control size and rely on the tested vertical scroll path.
        if (compactChanged) compactChanged(width() < 1800 && height() >= 680);
    }
};
QLabel* label(const QString& text, QWidget* parent = nullptr) {
    auto* result = new QLabel(text, parent); result->setWordWrap(true); return result;
}
QDoubleSpinBox* number(double value, double low, double high, const QString& suffix) {
    auto* box = new QDoubleSpinBox; box->setRange(low, high); box->setDecimals(3); box->setValue(value);
    box->setSuffix(suffix); box->setMinimumHeight(32); box->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed); return box;
}
QWidget* unitEditor(QAbstractSpinBox* editor, const QString& unit) {
    if (auto* doubleEditor = qobject_cast<QDoubleSpinBox*>(editor)) doubleEditor->setSuffix(QString());
    if (auto* integerEditor = qobject_cast<QSpinBox*>(editor)) integerEditor->setSuffix(QString());
    editor->setButtonSymbols(QAbstractSpinBox::NoButtons);
    editor->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    editor->setProperty("parameterEditor", true);
    editor->setMinimumHeight(34);
    editor->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    auto* wrapper = new QWidget;
    wrapper->setObjectName("parameterControl");
    wrapper->setProperty("parameterControl", true);
    wrapper->setFocusPolicy(Qt::StrongFocus);
    wrapper->setFocusProxy(editor);
    auto* layout = new QHBoxLayout(wrapper);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(editor, 1);

    auto* unitLabel = new QLabel(unit, wrapper);
    unitLabel->setProperty("unitSegment", true);
    unitLabel->setAlignment(Qt::AlignCenter);
    unitLabel->setMinimumWidth(unitLabel->fontMetrics().horizontalAdvance(unit) + 16);
    unitLabel->setFixedHeight(34);
    layout->addWidget(unitLabel);
    wrapper->setFixedHeight(36);
    return wrapper;
}
QScrollArea* makeScrollArea(QWidget* content) {
    auto* area = new QScrollArea; area->setWidgetResizable(true); area->setFrameShape(QFrame::NoFrame); area->setWidget(content); return area;
}
QGroupBox* section(const QString& title) {
    auto* box = new QGroupBox(title); box->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum); return box;
}
QWidget* makeMetricCard(const QString& name, const QString& value, const QString& key) {
    auto* box = new QFrame; box->setObjectName("metricCard");
    auto* layout = new QHBoxLayout(box); layout->setContentsMargins(12, 8, 12, 8);
    auto* caption = label(name); auto* reading = new QLabel(value); reading->setProperty("metricValue", true);
    reading->setObjectName(key);
    reading->setAlignment(Qt::AlignRight | Qt::AlignVCenter); layout->addWidget(caption, 1); layout->addWidget(reading);
    box->setMinimumHeight(56); return box;
}
QWidget* makeSidebarMetric(const QString& icon, const QString& name, const QString& unit, const QString& value, const QString& key) {
    auto* box = new QFrame; box->setObjectName("sidebarMetric");
    auto* layout = new QVBoxLayout(box); layout->setContentsMargins(6, 6, 6, 6); layout->setSpacing(2);
    auto* symbol = new QLabel; symbol->setProperty("metricIcon", true); symbol->setAlignment(Qt::AlignCenter); symbol->setFixedHeight(30);
    symbol->setPixmap(QIcon(icon).pixmap(QSize(30, 30)));
    auto* labels = new QWidget; labels->setProperty("metricLabels", true);
    auto* labelLayout = new QVBoxLayout(labels); labelLayout->setContentsMargins(0, 0, 0, 0); labelLayout->setSpacing(0);
    auto* caption = new QLabel(name); caption->setProperty("metricCaption", true); caption->setAlignment(Qt::AlignCenter | Qt::AlignVCenter);
    auto* units = new QLabel(unit); units->setProperty("metricUnit", true); units->setAlignment(Qt::AlignCenter | Qt::AlignVCenter);
    labelLayout->addWidget(caption); labelLayout->addWidget(units);
    auto* reading = new QLabel(value); reading->setProperty("metricValue", true); reading->setObjectName(key); reading->setAlignment(Qt::AlignCenter | Qt::AlignVCenter);
    layout->addWidget(symbol); layout->addWidget(labels); layout->addWidget(reading, 1);
    box->setMinimumHeight(112); return box;
}
QFrame* makeCameraPlaceholder(const QString& objectName, bool compact) {
    auto* card = new QFrame; card->setObjectName(objectName); card->setProperty("chartCard", true);
    auto* layout = new QVBoxLayout(card); layout->setContentsMargins(12, 8, 12, 10); layout->setSpacing(8);
    layout->addWidget(cardHeading(compact ? QStringLiteral("摄像窗口") : QStringLiteral("摄像图像"),"camera"));
    auto* preview = new CameraPreview; preview->setText(QStringLiteral("摄像头未连接\n请在摄像界面选择设备并连接")); preview->setWordWrap(true); preview->setObjectName(objectName + "Preview");
    preview->setProperty("cameraSurface", true); preview->setAlignment(Qt::AlignCenter); preview->setMinimumSize(compact ? QSize(360, 220) : QSize(600, 320));
    layout->addWidget(preview, 1);
    auto* state = new QLabel(QStringLiteral("摄像状态：未连接    ·    预览不会自动启动")); state->setProperty("cameraState", true); state->setWordWrap(true); layout->addWidget(state);
    state->setObjectName(objectName + "State");
    return card;
}
InteractiveChartView* chartView(QChart* chart) {
    if (!chart || !chart->scene()) return nullptr;
    for (auto* view : chart->scene()->views()) if (auto* interactive = qobject_cast<InteractiveChartView*>(view)) return interactive;
    return nullptr;
}
}
MainWindow::MainWindow(const QString& dataRoot, QWidget* parent, ApplicationRole role, const QString& accountName,
                       std::shared_ptr<IDaqBackend> daqBackend)
    : QMainWindow(parent), dataRoot_(dataRoot), role_(role), accountName_(accountName.isEmpty() ? QStringLiteral("本地用户") : accountName),
      cameraSession_(dataRoot), daqBackend_(daqBackend ? std::move(daqBackend) : makeNiDaqBackend()) {
    initializeResources();
    QFile file(":/design-tokens.json");
    if (!file.open(QIODevice::ReadOnly)) qFatal("Missing design tokens");
    tokens_ = QJsonDocument::fromJson(file.readAll()).object();
    setWindowTitle("TDLAS测氢系统"); resize(1600, 900);
    auto* shell = new QWidget; auto* shellLayout = new QVBoxLayout(shell);
    shellLayout->setContentsMargins(0, 0, 0, 0); shellLayout->setSpacing(0);
    auto* header = new QFrame; header->setObjectName("header");
    header->setFixedHeight(tokens_["layout"].toObject()["headerHeight"].toInt());
    auto* headerLayout = new QHBoxLayout(header); headerLayout->setContentsMargins(22, 0, 18, 0); headerLayout->setSpacing(14);
    auto* brandIcon = new QLabel; brandIcon->setObjectName("brandIcon"); brandIcon->setPixmap(QIcon(QStringLiteral(":/brand/laser-mark.svg")).pixmap(QSize(38, 38)));
    auto* title = new QLabel("TDLAS测氢系统"); title->setObjectName("brandTitle");
    auto* pulse = new QLabel; pulse->setObjectName("brandPulse"); pulse->setProperty("workbenchIcon","pulse"); pulse->setFixedSize(46,32);
    auto* slogan = new QLabel(QStringLiteral("激光光谱 · 精准测量 · 智能分析")); slogan->setObjectName("brandSlogan");
    clock_ = new QLabel; clock_->setObjectName("headerClock");
    status_ = new QLabel("未连接 · 等待选择数据源"); status_->setObjectName("acquisitionStatus");
    auto* settingsButton = new QPushButton(workbenchIcon("settings"),QStringLiteral("设置")); settingsButton->setObjectName("settingsButton"); settingsButton->setFlat(true);
    settingsButton->setToolTip(QStringLiteral("系统设置将在设备接入步骤完善")); settingsButton->setEnabled(false);
    settingsButton->setAccessibleDescription(settingsButton->toolTip());
    auto* user = new QLabel(accountName_); user->setObjectName("headerUser");
    theme_ = new QPushButton(workbenchIcon("theme"),"切换夜间"); theme_->setObjectName("themeButton"); theme_->setFlat(true);
    headerLayout->addWidget(brandIcon); headerLayout->addWidget(title); headerLayout->addWidget(pulse); headerLayout->addWidget(slogan);
    headerLayout->addStretch(); headerLayout->addWidget(clock_); headerLayout->addWidget(status_); headerLayout->addWidget(settingsButton); headerLayout->addWidget(user); headerLayout->addWidget(theme_);
    shellLayout->addWidget(header);
    auto* workspace = new Workspace; workspace->setObjectName("workspace");
    workspace->left = makeScrollArea(makeSidebar()); workspace->left->setParent(workspace); workspace->left->setObjectName("leftPane");
    workspace->right = new QWidget(workspace); workspace->right->setObjectName("rightPane");
    auto* rightLayout = new QVBoxLayout(workspace->right); rightLayout->setContentsMargins(10, 8, 10, 8);
    tabs_ = new SampleTabs; tabs_->setObjectName("mainTabs"); rightLayout->addWidget(tabs_); createPages();
    if (role_ == ApplicationRole::Customer) {
        tabs_->setCurrentIndex(0);
        for (int page = 1; page < tabs_->count(); ++page) tabs_->setTabVisible(page, false);
        tabs_->tabBar()->hide();
        setWindowTitle(QStringLiteral("TDLAS测氢系统 · 客户显示工作台"));
    }
    notice_ = label("旧版算法已迁入回放适配层但物理标定未确认；模拟数据仍只验证线程与原始曲线。");
    notice_->setObjectName("notice"); rightLayout->addWidget(notice_);
    workspace->compactChanged = [this](bool compact) {
        parameterForm_->setRowWrapPolicy(compact ? QFormLayout::WrapLongRows : QFormLayout::DontWrapRows);
        parameterForm_->setVerticalSpacing(compact ? 0 : 2);
        for (int index = 1; index <= 11; ++index)
            if (auto* caption = findChild<QWidget*>(QStringLiteral("parameterCaption%1").arg(index))) caption->setFixedWidth(compact ? 144 : 158);
        for (auto* editor : QList<QWidget*>{device_, output_, waveform_, frequency_, amplitude_, offset_, input_, sampleRate_, displaySamples_, maximum_, minimum_})
            editor->setFixedHeight(compact ? 32 : 34);
        for (auto* control : findChildren<QWidget*>(QStringLiteral("parameterControl")))
            control->setFixedHeight(compact ? 34 : 36);
        refreshDevices_->setFixedHeight(compact ? 28 : 32);
        for (auto* button : {start_, stop_, reset_}) button->setFixedHeight(compact ? 56 : 68);
        for (auto* metric : findChildren<QFrame*>(QStringLiteral("sidebarMetric"))) metric->setFixedHeight(compact ? 112 : 120);
    };
    auto* workspaceScroll = makeScrollArea(workspace); workspaceScroll->setObjectName("workspaceScroll");
    shellLayout->addWidget(workspaceScroll, 1); setCentralWidget(shell);
    // Reveal keyboard focus through every nested scrolling viewport, inside out.
    connect(qApp, &QApplication::focusChanged, this, [this](QWidget*, QWidget* focused) {
        if (!focused || !isAncestorOf(focused)) return;
        for (auto* parent = focused->parentWidget(); parent; parent = parent->parentWidget()) {
            if (auto* scroll = qobject_cast<QScrollArea*>(parent)) {
                const QRect bounds(focused->mapTo(scroll->widget(), QPoint()), focused->size());
                // ensureWidgetVisible can use only the editor's cursor rectangle.
                scroll->ensureVisible(bounds.right(), bounds.bottom(), 8, 8);
                scroll->ensureVisible(bounds.left(), bounds.top(), 8, 8);
            }
        }
    });
    const QList<QWidget*> focusOrder = {device_, output_, waveform_, frequency_, amplitude_, offset_, input_,
        sampleRate_, displaySamples_, maximum_, minimum_, start_, stop_, reset_, theme_, tabs_->tabBar()};
    for (qsizetype i = 1; i < focusOrder.size(); ++i) QWidget::setTabOrder(focusOrder[i - 1], focusOrder[i]);
    connect(theme_, &QPushButton::clicked, this, [this] { setNightTheme(!night_); });
    connect(&cameraSession_, &CameraSession::frameReady, this, &MainWindow::renderCameraFrame);
    connect(&cameraSession_, &CameraSession::stateChanged, this, [this](CameraState state) {
        updateCameraControls();
        if (state == CameraState::Disconnected) {
            renderCameraFrame({});
            if (closePending_ && !runtime_.active() && !exportWorker_ && !discoveryWorker_) { closePending_ = false; QTimer::singleShot(0, this, &QWidget::close); }
        }
    });
    connect(&cameraSession_, &CameraSession::recordingDurationChanged, this, [this](qint64) {
        updateCameraControls();
    });
    connect(&cameraSession_, &CameraSession::mediaSaved, this, [this](const QString& path) {
        if (cameraSaveState_) cameraSaveState_->setText(QStringLiteral("保存状态：%1").arg(QDir::toNativeSeparators(path)));
    });
    connect(&cameraSession_, &CameraSession::errorOccurred, this, [this](const QString& message) {
        if (cameraState_) cameraState_->setText(QStringLiteral("摄像故障 · %1").arg(message));
        notice_->setText(message);
    });
    connect(tabs_, &QTabWidget::currentChanged, this, [this](int index) {
        if (notice_) notice_->setVisible(index == 0 || index == 1);
        // Both the customer page and camera page contain a live preview.
        // Leaving both preview pages stops recording and releases the UVC device.
        if (index != 0 && index != 3 && cameraSession_.active()) cameraSession_.stop();
    });
    connect(&runtime_, &AcquisitionRuntime::stateChanged, this, [this] { updateControls(); });
    connect(&runtime_, &AcquisitionRuntime::faultOccurred, this, [this](const QString& error) { notice_->setText(error); });
    connect(&runtime_, &AcquisitionRuntime::stopped, this, [this] {
        updateControls(); refreshSnapshot();
        if (currentArchive_) {
            const auto error = currentArchive_->error();
            if (!error.isEmpty()) notice_->setText(QStringLiteral("原始数据归档失败：%1").arg(error));
            else if (runtime_.error().isEmpty()) notice_->setText(QStringLiteral("原始数据已归档：%1").arg(currentArchive_->runDirectory()));
        }
        if (closePending_ && !exportWorker_ && !discoveryWorker_) { closePending_ = false; QTimer::singleShot(0, this, &QWidget::close); }
    });
    connect(&displayTimer_, &QTimer::timeout, this, &MainWindow::refreshSnapshot);
    displayTimer_.start(1000 / tokens_["chart"].toObject()["maxRenderFps"].toInt(20));
    const auto updateClock = [this] {
        const auto now = QDateTime::currentDateTime();
        clock_->setText(now.toString("yyyy-MM-dd  HH:mm:ss"));
        if (cameraPreviewTime_) cameraPreviewTime_->setText(now.toString("yyyy-MM-dd  HH:mm:ss"));
    };
    connect(&clockTimer_, &QTimer::timeout, this, updateClock); clockTimer_.start(1000); updateClock();
    QSettings settings(dataRoot_ + "/settings.ini", QSettings::IniFormat);
    cameraSettings_.outputDirectory = settings.value("camera/outputDirectory", QDir(dataRoot_).filePath("camera")).toString();
    cameraSettings_.mirrorPreview = settings.value("camera/mirror", false).toBool();
    cameraSettings_.rotationDegrees = settings.value("camera/rotation", 0).toInt();
    cameraSettings_.frameRate = settings.value("camera/frameRate", 30.0).toDouble();
    cameraSettings_.digitalPanX = settings.value("camera/digitalPanX", 0.0).toDouble();
    cameraSettings_.digitalPanY = settings.value("camera/digitalPanY", 0.0).toDouble();
    cameraSettings_.digitalZoom = settings.value("camera/digitalZoom", 1.0).toDouble();
    cameraSettings_.brightness = settings.value("camera/brightness", 0).toInt();
    cameraSettings_.contrast = settings.value("camera/contrast", 100).toInt();
    cameraSettings_.saturation = settings.value("camera/saturation", 100).toInt();
    cameraSettings_.hue = settings.value("camera/hue", 0).toInt();
    cameraSettings_.sharpness = settings.value("camera/sharpness", 0).toInt();
    cameraSettings_.gamma = settings.value("camera/gamma", 1.0).toDouble();
    cameraSettings_.autoFocus = settings.value("camera/autoFocus", true).toBool();
    cameraSettings_.focusDistance = settings.value("camera/focusDistance", 0.5).toDouble();
    cameraSettings_.hardwareZoom = settings.value("camera/hardwareZoom", 1.0).toDouble();
    cameraSettings_.autoExposure = settings.value("camera/autoExposure", true).toBool();
    cameraSettings_.exposureCompensation = settings.value("camera/exposureCompensation", 0.0).toDouble();
    cameraSettings_.manualExposureSeconds = settings.value("camera/manualExposureSeconds", 0.01).toDouble();
    cameraSettings_.manualIso = settings.value("camera/manualIso", 100).toInt();
    cameraSettings_.autoWhiteBalance = settings.value("camera/autoWhiteBalance", true).toBool();
    cameraSettings_.colorTemperature = settings.value("camera/colorTemperature", 4500).toInt();
    setNightTheme(settings.value("ui/theme", "day").toString() == "night"); updateControls(); updateCameraControls();
}
MainWindow::~MainWindow() {
    // Normal user closure is asynchronous and waits in closeEvent. This is a
    // final fallback for direct stack/test destruction while discovery returns.
    if (discoveryWorker_) { discoveryWorker_->wait(); delete discoveryWorker_; discoveryWorker_ = nullptr; }
    cameraSession_.stop();
}
QWidget* MainWindow::makeSidebar() {
    auto* pane = new QWidget; auto* layout = new QVBoxLayout(pane);
    layout->setContentsMargins(10, 8, 10, 8); layout->setSpacing(8);
    auto* parameters = section("A   数据输入"); parameters_ = parameters;
    parameterForm_ = new QFormLayout(parameters); parameterForm_->setContentsMargins(10, 28, 10, 8);
    parameterForm_->setVerticalSpacing(2); parameterForm_->setHorizontalSpacing(8);
    parameterForm_->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    device_ = new QComboBox; device_->setObjectName("deviceSelector");
    device_->addItem("请选择数据源", ""); device_->addItem("模拟数据源（非测量）", "simulation");
    device_->setToolTip("模拟源始终可用；点击刷新后异步枚举 NI-DAQmx 设备。");
    output_ = new QComboBox; output_->addItem("无输出（AI-only）"); output_->setEnabled(false);
    waveform_ = new QComboBox; waveform_->addItem("未配置"); waveform_->setEnabled(false);
    frequency_ = number(10, 0.001, 100000, " Hz"); amplitude_ = number(1, 0, 10, " V 峰值"); offset_ = number(0, -10, 10, " V");
    frequency_->setEnabled(false); amplitude_->setEnabled(false); offset_->setEnabled(false);
    input_ = new QComboBox; input_->addItem("未配置"); input_->setEnabled(false);
    sampleRate_ = number(10000, 1, 1000000, " Sa/s"); sampleRate_->setObjectName("sampleRate"); sampleRate_->setDecimals(0);
    displaySamples_ = new QSpinBox; displaySamples_->setRange(10, 100000); displaySamples_->setValue(2000); displaySamples_->setSuffix(" 点");
    displaySamples_->setToolTip("只控制最近显示窗口，不改变读取块（1000 点）或算法窗口（100000 点）。");
    displaySamples_->setObjectName("displaySamples");
    maximum_ = number(10, -10, 10, " V"); maximum_->setObjectName("inputMaximum");
    minimum_ = number(-10, -10, 10, " V"); minimum_->setObjectName("inputMinimum");
    const QStringList names = {"采集卡（设备识别）", "输出通道", "输出波形", "波形频率", "波形幅值", "波形偏移量", "采集通道", "采样率", "显示窗口读取点数", "量程上限", "量程下限"};
    const QList<QWidget*> fields = {
        device_, output_, waveform_,
        unitEditor(frequency_, QStringLiteral("Hz")),
        unitEditor(amplitude_, QStringLiteral("V 峰值")),
        unitEditor(offset_, QStringLiteral("V")),
        input_,
        unitEditor(sampleRate_, QStringLiteral("Sa/s")),
        unitEditor(displaySamples_, QStringLiteral("点")),
        unitEditor(maximum_, QStringLiteral("V")),
        unitEditor(minimum_, QStringLiteral("V"))
    };
    for (int i = 0; i < names.size(); ++i) {
        auto* captionContainer = new QWidget; captionContainer->setObjectName(QStringLiteral("parameterCaption%1").arg(i + 1));
        captionContainer->setProperty("parameterCaption", true); captionContainer->setFixedWidth(154);
        auto* captionLayout = new QHBoxLayout(captionContainer); captionLayout->setContentsMargins(0, 0, 0, 0); captionLayout->setSpacing(3);
        auto* numberLabel = new QLabel(QString::number(i + 1) + QStringLiteral(".")); numberLabel->setProperty("parameterNumber", true); numberLabel->setFixedWidth(20);
        auto* caption = label(names[i]); caption->setProperty("parameterCaptionText", true); caption->setObjectName("parameterLabel" + QString::number(i + 1)); caption->setBuddy(fields[i]);
        captionLayout->addWidget(numberLabel); captionLayout->addWidget(caption, 1);
        fields[i]->setAccessibleName(names[i]); fields[i]->setFixedHeight(36);
        fields[i]->setMinimumWidth(70); parameterForm_->addRow(captionContainer, fields[i]);
    }
    for (auto* field : QList<QWidget*>{output_, waveform_, frequency_, amplitude_, offset_}) {
        field->setToolTip("当前数据源不提供硬件输出，输出参数暂不可用。");
        field->setAccessibleDescription(field->toolTip());
    }
    input_->setAccessibleDescription("模拟源使用固定电压通道；NI 模式在手动刷新设备后按实际能力列出输入通道。");
    parameterHint_ = label("请选择数据源。模拟源不连接硬件，输出参数不可用。");
    parameterHint_->setObjectName("parameterHint"); parameterHint_->setWordWrap(false); parameterHint_->setMinimumWidth(0);
    parameterHint_->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    refreshDevices_ = new QPushButton(niDaqmxCompiled() ? "刷新设备" : "检查支持");
    refreshDevices_->setObjectName("refreshDevicesButton");
    refreshDevices_->setAccessibleDescription("在后台线程中枚举设备和 AI/AO 能力；不创建采集或输出任务。");
    refreshDevices_->setMinimumWidth(76);
    auto* parameterFooter = new QWidget; auto* parameterFooterLayout = new QHBoxLayout(parameterFooter);
    parameterFooterLayout->setContentsMargins(0, 0, 0, 0); parameterFooterLayout->setSpacing(6);
    parameterFooterLayout->addWidget(parameterHint_, 1); parameterFooterLayout->addWidget(refreshDevices_);
    parameterForm_->addRow(parameterFooter);
    layout->addWidget(parameters);
    auto* controls = section("B   采集控制"); controls->setObjectName("controlSection");
    auto* buttons = new QHBoxLayout(controls); buttons->setContentsMargins(10, 28, 10, 10); buttons->setSpacing(8);
    start_ = new QPushButton(workbenchIcon("play"),"开始采集"); start_->setObjectName("startButton"); start_->setProperty("action", "start");
    stop_ = new QPushButton(workbenchIcon("stop"),"停止采集"); stop_->setObjectName("stopButton"); stop_->setProperty("action", "stop");
    reset_ = new QPushButton(workbenchIcon("reset"),"复位"); reset_->setObjectName("resetButton"); reset_->setProperty("action", "reset");
    for (auto* button : {start_, stop_, reset_}) { button->setFixedHeight(68); buttons->addWidget(button, 1); }
    layout->addWidget(controls);
    auto* results = section("C   数据监测"); results->setObjectName("resultSection");
    auto* resultLayout = new QHBoxLayout(results); resultLayout->setContentsMargins(8, 28, 8, 8); resultLayout->setSpacing(6);
    resultLayout->addWidget(makeSidebarMetric(QStringLiteral(":/metrics/absorbance.svg"), "峰值吸光度", "(a.u.)", "—", "peakAbsorbance"), 1);
    resultLayout->addWidget(makeSidebarMetric(QStringLiteral(":/metrics/intensity.svg"), "光强百分比", "(%)", "—", "intensityPercent"), 1);
    resultLayout->addWidget(makeSidebarMetric(QStringLiteral(":/metrics/concentration.svg"), "气体浓度", "(未校准)", "未校准", "concentration"), 1); layout->addWidget(results);
    diagnostics_ = label("尚无采集数据"); diagnostics_->setObjectName("runtimeDiagnostics"); diagnostics_->setVisible(false); layout->addWidget(diagnostics_); layout->addStretch();
    connect(device_, &QComboBox::currentIndexChanged, this, &MainWindow::applySelectedDevice);
    connect(output_, &QComboBox::currentIndexChanged, this, [this] { updateControls(); });
    connect(input_, &QComboBox::currentIndexChanged, this, [this] { updateControls(); });
    connect(waveform_, &QComboBox::currentIndexChanged, this, [this] { updateControls(); });
    for (auto* field : {frequency_, amplitude_, offset_, sampleRate_, maximum_, minimum_}) connect(field, &QDoubleSpinBox::valueChanged, this, [this] { updateControls(); });
    connect(refreshDevices_, &QPushButton::clicked, this, &MainWindow::refreshDaqDevices);
    connect(start_, &QPushButton::clicked, this, &MainWindow::startAcquisition);
    connect(stop_, &QPushButton::clicked, &runtime_, &AcquisitionRuntime::requestStop);
    connect(reset_, &QPushButton::clicked, this, &MainWindow::resetDisplay);
    return pane;
}
QWidget* MainWindow::makeChart(const QString& title, const QString& xTitle, const QString& yTitle, const QString& objectName, bool raw) {
    const bool debugCard = objectName == "rawCard" || objectName == "fitCard" || objectName == "fvCard" || objectName == "peakCard";
    const bool peakHistory = objectName == "historyCard";
    auto* card = new QFrame; card->setObjectName(objectName); card->setProperty("chartCard", true); card->setProperty("debugChartCard", debugCard);
    card->setProperty("peakHistoryCard", peakHistory); card->setMinimumSize(360, 240);
    const bool customer = objectName.startsWith("customer");
    auto* layout = new QVBoxLayout(card); layout->setContentsMargins(customer || peakHistory ? 1 : 10, customer || peakHistory ? 1 : 8, customer || peakHistory ? 1 : 10, 8); layout->setSpacing(debugCard ? 2 : 4);
    QHBoxLayout* toolbar = nullptr;
    const QString headingIcon = objectName == "fitCard" ? QStringLiteral("fit") :
        objectName == "fvCard" ? QStringLiteral("file") :
        objectName == "peakCard" || objectName.contains("History") ? QStringLiteral("bars") : QStringLiteral("pulse");
    layout->addWidget(cardHeading(title, headingIcon, &toolbar));
    const auto addTool = [&](const QString& caption, const QStringList& options, const QString& name, const QString& accessibleName) {
        auto* label = new QLabel(caption); label->setProperty("chartToolLabel", true);
        auto* combo = new QComboBox; combo->setObjectName(name); combo->addItems(options);
        combo->setProperty("chartTool", true); combo->setAccessibleName(accessibleName); combo->setMinimumWidth(78);
        toolbar->addWidget(label); toolbar->addWidget(combo);
        return combo;
    };
    if (debugCard) {
        if (objectName == "rawCard") {
            addTool(QStringLiteral("显示通道："), {QStringLiteral("通道1")}, QStringLiteral("rawChannelSelector"), QStringLiteral("原始波形显示通道"));
            addTool(QStringLiteral("时间范围："), {QStringLiteral("10 ms")}, QStringLiteral("rawRangeSelector"), QStringLiteral("原始波形时间范围"));
        } else if (objectName == "fitCard") {
            addTool(QStringLiteral("拟合模型："), {QStringLiteral("Voigt")}, QStringLiteral("fitModelSelector"), QStringLiteral("拟合模型"));
            addTool(QStringLiteral("显示范围："), {QStringLiteral("自动")}, QStringLiteral("fitRangeSelector"), QStringLiteral("拟合图显示范围"));
        } else {
            addTool(QStringLiteral("显示时长："), {QStringLiteral("1 小时")}, objectName + QStringLiteral("TimeRangeSelector"), title + QStringLiteral("显示时长"));
            addTool(QStringLiteral("时间粒度："), {QStringLiteral("1 秒")}, objectName + QStringLiteral("ResolutionSelector"), title + QStringLiteral("时间粒度"));
        }
    }
    if (objectName == "customerHistoryCard") {
        auto* caption = new QLabel("显示时长：");
        customerHistoryRange_ = new QComboBox; customerHistoryRange_->setObjectName("customerHistoryRange");
        customerHistoryRange_->addItem("全部记录",0); customerHistoryRange_->addItem("最近 1 分钟",60);
        customerHistoryRange_->addItem("最近 10 分钟",600); customerHistoryRange_->addItem("最近 1 小时",3600);
        customerHistoryRange_->setAccessibleName("FV 历史显示时长"); caption->setBuddy(customerHistoryRange_);
        customerHistoryRange_->setToolTip("按各会话经过时间显示最近范围；保留全部原始记录，导出不受影响。");
        toolbar->addWidget(caption); toolbar->addWidget(customerHistoryRange_);
    }
    QComboBox* peakRange = nullptr;
    QPushButton* savePeakImage = nullptr;
    if (peakHistory) {
        auto* displayMode = addTool(QStringLiteral("显示模式："), {QStringLiteral("FV峰值")}, QStringLiteral("peakDisplayMode"), QStringLiteral("峰值数据显示模式"));
        displayMode->setFixedWidth(104);
        peakRange = new QComboBox; peakRange->setObjectName("peakTimeRange"); peakRange->setProperty("chartTool", true);
        peakRange->addItem(QStringLiteral("近30分钟"), 1800); peakRange->addItem(QStringLiteral("近1小时"), 3600); peakRange->addItem(QStringLiteral("全部记录"), 0);
        peakRange->setAccessibleName(QStringLiteral("峰值数据时间范围")); peakRange->setFixedWidth(104);
        auto* rangeLabel = new QLabel(QStringLiteral("时间范围：")); rangeLabel->setProperty("chartToolLabel", true); rangeLabel->setBuddy(peakRange);
        toolbar->addWidget(rangeLabel); toolbar->addWidget(peakRange);
        auto* interval = addTool(QStringLiteral("数据间隔："), {QStringLiteral("全部数据")}, QStringLiteral("peakDataInterval"), QStringLiteral("峰值数据间隔"));
        interval->setFixedWidth(104);
        savePeakImage = new QPushButton(workbenchIcon("camera"), QStringLiteral("图像保存")); savePeakImage->setObjectName("savePeakImageButton");
        savePeakImage->setProperty("peakImageAction", true); savePeakImage->setAccessibleDescription(QStringLiteral("将当前峰值数据图保存为 PNG 图像。"));
        toolbar->addWidget(savePeakImage);
    }
    QPushButton* follow = nullptr;
    QPushButton* resetZoom = nullptr;
    if (!peakHistory) {
        follow = new QPushButton(debugCard ? QStringLiteral("自动") : QStringLiteral("自动跟随")); follow->setObjectName(objectName + "AutoFollow"); follow->setProperty("debugTool", debugCard); follow->setCheckable(true); follow->setChecked(true);
        follow->setAccessibleDescription("开启后新数据自动适配坐标范围；手动缩放或拖拽会关闭自动跟随。"); toolbar->addWidget(follow);
        resetZoom = new QPushButton(workbenchIcon("reset"), debugCard ? QString() : QStringLiteral("复位缩放")); resetZoom->setObjectName(objectName + "ResetZoom"); resetZoom->setProperty("debugTool", debugCard); resetZoom->setAccessibleName(title + "复位缩放并恢复自动跟随"); resetZoom->setToolTip(title + "：复位缩放并恢复自动跟随"); toolbar->addWidget(resetZoom);
        if (debugCard) resetZoom->setFixedSize(32, 32);
        if(customer) { follow->setMinimumHeight(40); resetZoom->setMinimumHeight(40); }
    }
    auto* state = label(raw ? "等待采集 · 仅模拟电压" : "等待采集 · 算法/物理定义待接入");
    state->setObjectName(objectName + "State"); state->setProperty("chartState",true); state->setMinimumHeight(debugCard ? 18 : 24); state->setContentsMargins(6,0,6,0); layout->addWidget(state);
    if (raw) rawStates_.append(state);
    auto* chart = new QChart; chart->setAnimationOptions(QChart::NoAnimation); chart->setBackgroundRoundness(0); chart->setMargins(QMargins(0, 0, 0, 0));
    chart->legend()->hide();
    auto* series = new QLineSeries; series->setName(raw ? "模拟电压" : title); chart->addSeries(series);
    auto* x = new QValueAxis; x->setTitleText(xTitle); x->setRange(0, 1); x->setLabelFormat("%.6g");
    auto* y = new QValueAxis; y->setTitleText(yTitle); y->setRange(-1, 1); y->setLabelFormat("%.6g");
    if (objectName == "rawCard") { x->setRange(0, 10); y->setRange(-2, 2); }
    else if (objectName == "fitCard") { x->setRange(-3, 3); y->setRange(0, 1); }
    else if (objectName == "fvCard") { x->setRange(-3.5, 3.5); y->setRange(-0.5, 2.5); }
    else if (objectName == "peakCard") { x->setRange(0, 600); y->setRange(0, 1.6); }
    if(customer) { x->setLabelFormat("%.3g"); y->setLabelFormat("%.3g"); }
    x->setTickCount(peakHistory ? 9 : objectName == "customerHistoryCard" || objectName == "peakCard" ? 7 : debugCard ? 7 : 5);
    y->setTickCount(peakHistory ? 9 : debugCard ? 6 : 5);
    chart->addAxis(x, Qt::AlignBottom); chart->addAxis(y, Qt::AlignLeft); series->attachAxis(x); series->attachAxis(y);
    if (raw) rawSeries_ = series;
    else { resultCharts_.insert(objectName, chart); resultStates_.insert(objectName, state); }
    auto* view = new InteractiveChartView(chart); view->setObjectName(objectName + "View"); view->setMinimumSize(0, 0);
    view->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored); layout->addWidget(view, 1); charts_.append(chart);
    if (follow && resetZoom) {
        connect(follow, &QPushButton::toggled, view, &InteractiveChartView::setAutoFollow);
        connect(view, &InteractiveChartView::autoFollowChanged, follow, &QPushButton::setChecked);
        connect(resetZoom, &QPushButton::clicked, this, [this, chart, view] {
            chart->zoomReset(); view->setAutoFollow(true);
            if(chart == resultCharts_.value("customerHistoryCard")) updateCustomerHistoryRange();
        });
    }
    if (peakRange) {
        const auto applyRange = [chart, view, peakRange] {
            if (!view->autoFollow()) return;
            double low = 0.0, high = 0.0; bool any = false;
            for (auto* item : chart->series()) if (auto* line = qobject_cast<QLineSeries*>(item))
                for (const auto& point : line->points()) { if (!any) { low = high = point.x(); any = true; } low = std::min(low, point.x()); high = std::max(high, point.x()); }
            const int seconds = peakRange->currentData().toInt();
            if (seconds > 0 && any) low = std::max(low, high - seconds);
            if (auto* axis = qobject_cast<QValueAxis*>(chart->axes(Qt::Horizontal).value(0))) axis->setRange(low, high > low ? high : low + 1.0);
        };
        connect(peakRange, &QComboBox::currentIndexChanged, this, [view, applyRange] { view->setAutoFollow(true); applyRange(); });
        connect(view, &InteractiveChartView::autoFollowChanged, this, [applyRange](bool enabled) { if (enabled) applyRange(); });
    }
    if (savePeakImage) connect(savePeakImage, &QPushButton::clicked, this, [this, view] {
        const QString suggested = QDir(dataRoot_).filePath(QStringLiteral("峰值数据图-%1.png").arg(QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss")));
        const QString path = QFileDialog::getSaveFileName(this, QStringLiteral("保存峰值数据图"), suggested, QStringLiteral("PNG 图像 (*.png)"));
        if (path.isEmpty()) return;
        if (!view->grab().save(path, "PNG")) QMessageBox::warning(this, QStringLiteral("图像保存失败"), QStringLiteral("无法写入所选文件，请检查目录权限。"));
    });
    if(objectName == "customerHistoryCard") {
        auto* footer = new QHBoxLayout; footer->setContentsMargins(14,0,14,0);
        auto* hint = new QLabel("显示参数：FV 最大有效峰值 · 每完整批次一条记录"); hint->setWordWrap(true); hint->setProperty("chartState",true);
        footer->addWidget(hint,1);
        customerExport_ = new QPushButton(workbenchIcon("export"),"导出数据"); customerExport_->setObjectName("customerExportDataButton");
        customerExport_->setMinimumHeight(40); customerExport_->setEnabled(false);
        customerExport_->setToolTip("导出全部完整批次 CSV + JSON，不受显示时长限制。");
        footer->addWidget(customerExport_); layout->addLayout(footer);
        connect(customerExport_,&QPushButton::clicked,this,&MainWindow::exportData);
        connect(customerHistoryRange_,&QComboBox::currentIndexChanged,this,[this,view] { view->setAutoFollow(true); updateCustomerHistoryRange(); });
        connect(view,&InteractiveChartView::autoFollowChanged,this,[this](bool enabled) { if(enabled) updateCustomerHistoryRange(); });
    }
    return card;
}
void MainWindow::updateCustomerHistoryRange() {
    auto* chart = resultCharts_.value("customerHistoryCard");
    if(!chart || !customerHistoryRange_ || !chartView(chart)->autoFollow()) return;
    double low=0, high=0; bool any=false;
    for(auto* series : chart->series()) if(auto* line=qobject_cast<QLineSeries*>(series))
        for(const auto& point : line->points()) { if(!any) { low=high=point.x(); any=true; } low=std::min(low,point.x()); high=std::max(high,point.x()); }
    const int seconds=customerHistoryRange_->currentData().toInt();
    if(seconds>0 && any) low=std::max(low,high-seconds);
    qobject_cast<QValueAxis*>(chart->axes(Qt::Horizontal).first())->setRange(low,high>low ? high : low+1);
}
void MainWindow::createPages() {
    auto* customer = new QWidget; customer->setObjectName("customerDisplayPage");
    auto* customerLayout = new QGridLayout(customer); customerLayout->setContentsMargins(0, 10, 0, 0); customerLayout->setSpacing(10);
    auto* customerCamera = makeCameraPlaceholder(QStringLiteral("customerCameraCard"), true);
    if(role_ == ApplicationRole::Customer)
        customerCamera->findChild<QLabel*>("customerCameraCardPreview")->setText("摄像头未连接\n等待摄像会话提供预览");
    if(role_ == ApplicationRole::Administrator) {
        auto* cameraLayout = qobject_cast<QVBoxLayout*>(customerCamera->layout());
        auto* cameraState = customerCamera->findChild<QLabel*>("customerCameraCardState");
        cameraLayout->removeWidget(cameraState);
        auto* row = new QHBoxLayout; row->addWidget(cameraState,1);
        auto* manage = new QPushButton(workbenchIcon("camera"),"摄像管理"); manage->setObjectName("customerCameraManage"); manage->setMinimumHeight(40);
        manage->setToolTip("进入摄像界面选择设备；不会自动连接或请求权限。");
        connect(manage,&QPushButton::clicked,this,[this] { tabs_->setCurrentIndex(3); });
        row->addWidget(manage); cameraLayout->addLayout(row);
    }
    cameraPreviews_.append(customerCamera->findChild<QLabel*>("customerCameraCardPreview"));
    customerLayout->addWidget(customerCamera, 0, 0);
    customerLayout->addWidget(makeChart("FV曲线", "采样点（频率未标定）", "FV（定义与单位待确认）", "customerFv"), 0, 1);
    customerLayout->addWidget(makeChart("FV曲线数据记录", "各会话经过时间 / s", "FV 峰值（单位待确认）", "customerHistoryCard"), 1, 0, 1, 2);
    customerLayout->setColumnStretch(0, 1); customerLayout->setColumnStretch(1, 1); customerLayout->setRowStretch(0, 1); customerLayout->setRowStretch(1, 1);
    tabs_->addTab(makeScrollArea(customer), QIcon(QStringLiteral(":/tabs/customer.svg")), "客户显示窗口");
    auto* debug = new QWidget; debug->setObjectName("debugPage"); auto* grid = new QGridLayout(debug); grid->setContentsMargins(0, 8, 0, 0); grid->setSpacing(12);
    grid->addWidget(makeChart("原始波形图", "窗口内时间 / s", "电压 / V", "rawCard", true), 0, 0);
    grid->addWidget(makeChart("拟合图", "采样点（频率未标定）", "拟合物理量待确认", "fitCard"), 0, 1);
    grid->addWidget(makeChart("FV曲线图", "采样点（频率未标定）", "FV（单位待确认）", "fvCard"), 1, 0);
    grid->addWidget(makeChart("FV峰值数据记录", "经过时间 / s", "FV 峰值（单位待确认）", "peakCard"), 1, 1);
    grid->setColumnStretch(0, 1); grid->setColumnStretch(1, 1); grid->setRowStretch(0, 1); grid->setRowStretch(1, 1);
    auto* simulationTools = new QWidget; simulationTools->setObjectName("debugSimulationTools"); simulationTools->setProperty("debugToolbar", true); auto* simulationLayout = new QHBoxLayout(simulationTools);
    simulationLayout->setContentsMargins(0, 0, 0, 0);
    auto* scenarioLabel = label("模拟验证场景（非测量）");
    simulationScenario_ = new QComboBox; simulationScenario_->setObjectName("simulationScenario");
    simulationScenario_->addItems({"正常电压", "打开失败", "3 块后读取失败", "每次读取延迟 1 秒"});
    simulationScenario_->setAccessibleName("模拟验证场景"); scenarioLabel->setBuddy(simulationScenario_);
    simulationScenario_->setToolTip("仅对下次手动启动生效，不访问硬件；复位恢复正常场景。延迟可被停止操作取消。");
    simulationLayout->addWidget(scenarioLabel); simulationLayout->addWidget(simulationScenario_); simulationLayout->addStretch();
    grid->addWidget(simulationTools, 2, 0, 1, 2);
    connect(simulationScenario_, &QComboBox::currentIndexChanged, this, [this] { updateControls(); });
    tabs_->addTab(makeScrollArea(debug), QIcon(QStringLiteral(":/tabs/debug.svg")), "调试界面");
    auto* peaks = new QWidget; peaks->setObjectName("peakDataPage"); auto* peaksLayout = new QVBoxLayout(peaks);
    peaksLayout->setContentsMargins(0, 4, 0, 0); peaksLayout->setSpacing(4);
    auto* historyChart = makeChart("峰值数据图", "时间 / s", "FV 峰值（单位待确认）", "historyCard");
    historyChart->setMinimumHeight(260); peaksLayout->addWidget(historyChart, 1);

    auto* records = new QFrame; records->setObjectName("peakRecordsPanel"); records->setProperty("peakPanel", true);
    auto* recordsLayout = new QVBoxLayout(records); recordsLayout->setContentsMargins(10, 4, 10, 8); recordsLayout->setSpacing(2);
    auto* recordsHeader = new QHBoxLayout; recordsHeader->setContentsMargins(0, 0, 0, 0);
    auto* toggleTable = new QPushButton(workbenchIcon("history"), "最近采集记录"); toggleTable->setObjectName("togglePeakTable"); toggleTable->setProperty("peakSectionTitle", true);
    toggleTable->setCheckable(true); toggleTable->setChecked(true); toggleTable->setAccessibleDescription("显示或收起与峰值曲线同源的数据表，不清除数据。");
    recordsHeader->addWidget(toggleTable); recordsHeader->addStretch();
    exportButton_ = new QPushButton(workbenchIcon("export"), "导出数据"); exportButton_->setObjectName("exportDataButton"); exportButton_->setEnabled(false);
    exportButton_->setAccessibleDescription("将当前一致性快照导出为峰值 CSV 和完整结果 JSON；导出失败不会标记为已保存。");
    connect(exportButton_, &QPushButton::clicked, this, &MainWindow::exportData); recordsHeader->addWidget(exportButton_); recordsLayout->addLayout(recordsHeader);
    peakTable_ = new QTableWidget(0, 6); peakTable_->setObjectName("peakTable");
    peakTable_->setHorizontalHeaderLabels({"序号", "采集时间", "FV峰值", "光强百分比（%）", "气体浓度", "状态"});
    peakTable_->setEditTriggers(QAbstractItemView::NoEditTriggers); peakTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    peakTable_->setFixedHeight(154); recordsLayout->addWidget(peakTable_, 1);
    peakTable_->setWordWrap(false); peakTable_->verticalHeader()->hide(); peakTable_->verticalHeader()->setDefaultSectionSize(24);
    peakTable_->horizontalHeader()->setFixedHeight(30);
    peakTable_->horizontalHeader()->setMinimumSectionSize(78); peakTable_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    peakTable_->setAccessibleName("完整峰值记录表");
    connect(toggleTable, &QPushButton::toggled, this, [this](bool visible) { peakTable_->setVisible(visible); });
    peaksLayout->addWidget(records);

    auto* statistics = new QFrame; statistics->setObjectName("peakStatisticsPanel"); statistics->setProperty("peakPanel", true);
    auto* statisticsLayout = new QVBoxLayout(statistics); statisticsLayout->setContentsMargins(10, 2, 10, 7); statisticsLayout->setSpacing(2);
    auto* statisticsHeader = new QWidget; statisticsHeader->setProperty("peakStatisticsHeader", true);
    auto* statisticsHeaderLayout = new QHBoxLayout(statisticsHeader); statisticsHeaderLayout->setContentsMargins(0, 0, 0, 0); statisticsHeaderLayout->setSpacing(8);
    auto* statisticsIcon = new QLabel; statisticsIcon->setProperty("workbenchIcon", "bars"); statisticsIcon->setFixedSize(24, 24);
    auto* statisticsTitle = new QLabel("峰值统计信息"); statisticsTitle->setProperty("peakStatisticsTitle", true);
    statisticsHeaderLayout->addWidget(statisticsIcon); statisticsHeaderLayout->addWidget(statisticsTitle); statisticsHeaderLayout->addStretch(); statisticsLayout->addWidget(statisticsHeader);
    auto* statisticRow = new QHBoxLayout; statisticRow->setContentsMargins(0, 0, 0, 0); statisticRow->setSpacing(10);
    const auto addStatistic = [&](const QString& caption, const QString& objectName, bool count) {
        auto* card = new QFrame; card->setProperty("peakStatCard", true); auto* cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(14, 4, 12, 4); cardLayout->setSpacing(0);
        auto* label = new QLabel(caption); label->setProperty("peakStatLabel", true); cardLayout->addWidget(label);
        auto* valueRow = new QHBoxLayout; valueRow->setContentsMargins(0, 0, 0, 0); valueRow->setSpacing(6);
        auto* value = new QLabel(count ? QStringLiteral("0") : QStringLiteral("—")); value->setObjectName(objectName); value->setProperty("peakStatValue", true);
        valueRow->addWidget(value); valueRow->addStretch();
        auto* unit = new QLabel(count ? QStringLiteral("次") : QStringLiteral("—")); unit->setObjectName(objectName + "Unit"); unit->setProperty("peakStatUnit", true); valueRow->addWidget(unit);
        cardLayout->addLayout(valueRow); statisticRow->addWidget(card, 1);
    };
    addStatistic("总采集次数", "peakStatCount", true); addStatistic("最大峰值", "peakStatMaximum", false);
    addStatistic("最小峰值", "peakStatMinimum", false); addStatistic("平均峰值", "peakStatAverage", false); addStatistic("峰值标准差", "peakStatDeviation", false);
    statisticsLayout->addLayout(statisticRow); peaksLayout->addWidget(statistics);
    tabs_->addTab(makeScrollArea(peaks), QIcon(QStringLiteral(":/tabs/peak.svg")), "峰值数据界面");
    auto* camera = new QWidget; camera->setObjectName("cameraPage");
    auto* cameraLayout = new QVBoxLayout(camera); cameraLayout->setContentsMargins(0, 12, 0, 0); cameraLayout->setSpacing(0);
    auto* cameraCard = new QFrame; cameraCard->setObjectName("cameraCard"); cameraCard->setProperty("chartCard", true); cameraCard->setProperty("cameraPanel", true);
    auto* cameraCardLayout = new QVBoxLayout(cameraCard); cameraCardLayout->setContentsMargins(12, 8, 12, 12); cameraCardLayout->setSpacing(0);
    cameraCardLayout->addWidget(cardHeading(QStringLiteral("摄像图像"), QStringLiteral("camera")));

    auto* preview = new CameraPreview; preview->setObjectName("cameraCardPreview"); preview->setProperty("cameraSurface", true);
    preview->setAlignment(Qt::AlignCenter); preview->setMinimumSize(QSize(600, 320)); preview->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    cameraPreviews_.append(preview); cameraCardLayout->addWidget(preview, 1);

    auto* previewLayout = new QVBoxLayout(preview); previewLayout->setContentsMargins(24, 24, 24, 24); previewLayout->setSpacing(0);
    previewLayout->addStretch(1);
    cameraDevicePanel_ = new QFrame(preview); cameraDevicePanel_->setObjectName("cameraDevicePanel"); cameraDevicePanel_->setProperty("cameraDevicePanel", true);
    cameraDevicePanel_->setMaximumWidth(760); cameraDevicePanel_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    auto* devicePanelLayout = new QVBoxLayout(cameraDevicePanel_); devicePanelLayout->setContentsMargins(18, 14, 18, 16); devicePanelLayout->setSpacing(10);
    auto* deviceTitle = new QLabel(QStringLiteral("摄像头未连接")); deviceTitle->setObjectName("cameraDevicePrompt"); deviceTitle->setProperty("cameraDevicePrompt", true); deviceTitle->setAlignment(Qt::AlignCenter);
    auto* deviceHint = new QLabel(QStringLiteral("刷新并选择 UVC 摄像设备后手动连接；页面不会自动请求摄像权限。")); deviceHint->setProperty("cameraDeviceHint", true); deviceHint->setAlignment(Qt::AlignCenter); deviceHint->setWordWrap(true);
    devicePanelLayout->addWidget(deviceTitle); devicePanelLayout->addWidget(deviceHint);
    auto* deviceBar = new QHBoxLayout; deviceBar->setContentsMargins(0, 0, 0, 0); deviceBar->setSpacing(8);
    cameraDevice_ = new QComboBox; cameraDevice_->setObjectName("cameraDeviceSelector"); cameraDevice_->setMinimumWidth(320);
    cameraDevice_->addItem(QStringLiteral("点击“刷新设备”查找 UVC 摄像头"), QByteArray());
    cameraRefresh_ = new QPushButton(workbenchIcon("refresh"), QStringLiteral("刷新设备")); cameraRefresh_->setObjectName("cameraRefreshButton");
    cameraConnect_ = new QPushButton(workbenchIcon("connect"), QStringLiteral("连接并预览")); cameraConnect_->setObjectName("cameraConnectButton");
    for (auto* button : {cameraRefresh_, cameraConnect_}) button->setMinimumHeight(40);
    deviceBar->addWidget(cameraDevice_, 1); deviceBar->addWidget(cameraRefresh_); deviceBar->addWidget(cameraConnect_);
    devicePanelLayout->addLayout(deviceBar); previewLayout->addWidget(cameraDevicePanel_, 0, Qt::AlignCenter); previewLayout->addStretch(1);

    auto* previewOverlay = new QFrame(preview); previewOverlay->setObjectName("cameraPreviewOverlay"); previewOverlay->setProperty("cameraPreviewOverlay", true);
    auto* previewOverlayLayout = new QHBoxLayout(previewOverlay); previewOverlayLayout->setContentsMargins(12, 5, 6, 5); previewOverlayLayout->setSpacing(8);
    cameraPreviewTime_ = new QLabel; cameraPreviewTime_->setObjectName("cameraPreviewTime");
    auto* fullscreen = new QPushButton(workbenchIcon("expand"), QString()); fullscreen->setObjectName("cameraFullscreenButton");
    fullscreen->setFixedSize(30, 30); fullscreen->setAccessibleName(QStringLiteral("全屏查看摄像预览")); fullscreen->setToolTip(QStringLiteral("全屏查看摄像预览，按 Esc 返回"));
    previewOverlayLayout->addWidget(cameraPreviewTime_); previewOverlayLayout->addWidget(fullscreen); previewOverlay->adjustSize();

    auto* cameraInfo = new QFrame; cameraInfo->setObjectName("cameraInfoBar"); cameraInfo->setProperty("cameraInfoBar", true);
    auto* cameraInfoLayout = new QHBoxLayout(cameraInfo); cameraInfoLayout->setContentsMargins(2, 6, 2, 6); cameraInfoLayout->setSpacing(10);
    auto* cameraStateColumn = new QWidget; auto* cameraStateLayout = new QVBoxLayout(cameraStateColumn); cameraStateLayout->setContentsMargins(0, 0, 0, 0); cameraStateLayout->setSpacing(1);
    auto* cameraStateLine = new QWidget; auto* cameraStateLineLayout = new QHBoxLayout(cameraStateLine); cameraStateLineLayout->setContentsMargins(0, 0, 0, 0); cameraStateLineLayout->setSpacing(8);
    auto* cameraStateIndicator = new QFrame; cameraStateIndicator->setObjectName("cameraStateIndicator"); cameraStateIndicator->setProperty("cameraStateKind", "idle"); cameraStateIndicator->setFixedSize(10, 10);
    cameraState_ = new QLabel(QStringLiteral("摄像头未连接 · 不会自动请求设备")); cameraState_->setObjectName("cameraCardState"); cameraState_->setProperty("cameraState", true);
    cameraStateLineLayout->addWidget(cameraStateIndicator); cameraStateLineLayout->addWidget(cameraState_);
    cameraSaveState_ = new QLabel(QStringLiteral("保存状态：暂无图片或录像")); cameraSaveState_->setObjectName("cameraSaveState"); cameraSaveState_->setProperty("cameraSaveState", true);
    cameraStateLayout->addWidget(cameraStateLine); cameraStateLayout->addWidget(cameraSaveState_); cameraInfoLayout->addWidget(cameraStateColumn);
    cameraDetails_ = new QLabel(QStringLiteral("分辨率：—    │    帧率：—    │    编码：—    │    设备：未选择")); cameraDetails_->setObjectName("cameraDetails");
    cameraDetails_->setProperty("cameraDetails", true); cameraDetails_->setAlignment(Qt::AlignVCenter); cameraInfoLayout->addWidget(cameraDetails_, 1);
    const auto makeCameraTool = [&](const QString& icon, const QString& text, const QString& name) {
        auto* button = new QPushButton(workbenchIcon(icon), text); button->setObjectName(name); button->setProperty("cameraTool", true);
        button->setMinimumHeight(40); button->setAccessibleName(QStringLiteral("摄像%1设置").arg(text)); return button;
    };
    auto* brightness = makeCameraTool(QStringLiteral("brightness"), QStringLiteral("亮度"), QStringLiteral("cameraBrightnessTool"));
    auto* contrast = makeCameraTool(QStringLiteral("contrast"), QStringLiteral("对比度"), QStringLiteral("cameraContrastTool"));
    auto* zoom = makeCameraTool(QStringLiteral("zoom"), QStringLiteral("变焦"), QStringLiteral("cameraZoomTool"));
    cameraSettingsButton_ = makeCameraTool(QStringLiteral("settings"), QStringLiteral("设置"), QStringLiteral("cameraAction4"));
    for (auto* button : {brightness, contrast, zoom, cameraSettingsButton_}) cameraInfoLayout->addWidget(button);
    cameraCardLayout->addWidget(cameraInfo);

    auto* cameraButtons = new QHBoxLayout; cameraButtons->setContentsMargins(16, 14, 16, 10); cameraButtons->setSpacing(22);
    cameraCapture_ = new QPushButton(workbenchIcon("camera"), QStringLiteral("拍摄")); cameraCapture_->setObjectName("cameraAction1"); cameraCapture_->setProperty("cameraRole", "primary");
    cameraRecord_ = new QPushButton(workbenchIcon("record"), QStringLiteral("录像")); cameraRecord_->setObjectName("cameraAction2"); cameraRecord_->setProperty("cameraRole", "record");
    cameraSaveLocation_ = new QPushButton(workbenchIcon("save"), QStringLiteral("数据保存")); cameraSaveLocation_->setObjectName("cameraAction3"); cameraSaveLocation_->setProperty("cameraRole", "primary");
    for (auto* button : {cameraCapture_, cameraRecord_, cameraSaveLocation_}) {
        button->setProperty("cameraAction", true); button->setIconSize(QSize(38, 38)); button->setMinimumHeight(82); cameraButtons->addWidget(button, 1);
    }
    cameraCapture_->setAccessibleDescription(QStringLiteral("将当前预览帧保存为 PNG 图片。"));
    cameraRecord_->setAccessibleDescription(QStringLiteral("开始或停止 MPEG-4 录像。"));
    cameraSaveLocation_->setAccessibleDescription(QStringLiteral("选择图片与录像保存目录。"));
    cameraSettingsButton_->setAccessibleDescription(QStringLiteral("设置分辨率、帧率、数字平移/缩放、画质调校及设备支持的对焦、曝光和白平衡。"));
    cameraCardLayout->addLayout(cameraButtons); cameraLayout->addWidget(cameraCard, 1);
    tabs_->addTab(makeScrollArea(camera), QIcon(QStringLiteral(":/tabs/camera.svg")), "摄像界面");
    connect(cameraRefresh_, &QPushButton::clicked, this, &MainWindow::refreshCameraDevices);
    connect(cameraConnect_, &QPushButton::clicked, this, &MainWindow::toggleCameraConnection);
    connect(cameraCapture_, &QPushButton::clicked, &cameraSession_, &CameraSession::captureStill);
    connect(cameraRecord_, &QPushButton::clicked, this, [this] {
        if (cameraSession_.state() == CameraState::Recording) cameraSession_.stopRecording();
        else cameraSession_.startRecording();
    });
    connect(cameraSaveLocation_, &QPushButton::clicked, this, [this] {
        const QString selected = QFileDialog::getExistingDirectory(this, QStringLiteral("选择摄像保存目录"), cameraSettings_.outputDirectory);
        if (selected.isEmpty()) return;
        cameraSettings_.outputDirectory = selected;
        QSettings settings(dataRoot_ + "/settings.ini", QSettings::IniFormat); settings.setValue("camera/outputDirectory", selected);
        cameraSaveState_->setText(QStringLiteral("保存目录：%1").arg(QDir::toNativeSeparators(selected)));
    });
    for (auto* button : {brightness, contrast, zoom}) connect(button, &QPushButton::clicked, this, &MainWindow::showCameraSettings);
    connect(cameraSettingsButton_, &QPushButton::clicked, this, &MainWindow::showCameraSettings);
    connect(fullscreen, &QPushButton::clicked, this, [this] {
        QDialog dialog(this); dialog.setWindowTitle(QStringLiteral("摄像预览")); dialog.setWindowState(Qt::WindowFullScreen);
        auto* layout = new QVBoxLayout(&dialog); layout->setContentsMargins(12, 12, 12, 12);
        auto* fullPreview = new CameraPreview; fullPreview->setAlignment(Qt::AlignCenter); fullPreview->setProperty("cameraSurface", true);
        fullPreview->setFrame(cameraSession_.latestFrame()); layout->addWidget(fullPreview, 1);
        connect(&cameraSession_, &CameraSession::frameReady, fullPreview, &CameraPreview::setFrame);
        dialog.exec();
    });
}
void MainWindow::refreshCameraDevices() {
    if (cameraSession_.active()) return;
    cameraRefresh_->setEnabled(false); cameraRefresh_->setText(QStringLiteral("正在刷新…"));
    QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    cameraDevices_ = CameraSession::discoverDevices();
    cameraDevice_->clear();
    if (cameraDevices_.isEmpty()) {
        cameraDevice_->addItem(QStringLiteral("未发现 UVC 摄像设备"), QByteArray());
        cameraSaveState_->setText(QStringLiteral("设备刷新完成：未发现可用摄像头，未请求开启设备。"));
    } else {
        for (const auto& device : cameraDevices_) {
            cameraDevice_->addItem(device.description + (device.isDefault ? QStringLiteral(" · 默认") : QString()), device.id);
        }
        const auto found = std::find_if(cameraDevices_.cbegin(), cameraDevices_.cend(), [](const auto& device) { return device.isDefault; });
        if (found != cameraDevices_.cend()) cameraDevice_->setCurrentIndex(int(std::distance(cameraDevices_.cbegin(), found)));
        cameraSaveState_->setText(QStringLiteral("设备刷新完成：发现 %1 个设备，尚未连接。").arg(cameraDevices_.size()));
    }
    cameraRefresh_->setText(QStringLiteral("刷新设备")); cameraRefresh_->setEnabled(true); updateCameraControls();
}
void MainWindow::toggleCameraConnection() {
    if (cameraSession_.active() || cameraSession_.state() == CameraState::Error) {
        cameraSession_.stop();
        return;
    }
    cameraSettings_.deviceId = cameraDevice_->currentData().toByteArray();
    const auto selected = std::find_if(cameraDevices_.cbegin(), cameraDevices_.cend(), [this](const auto& device) { return device.id == cameraSettings_.deviceId; });
    if (selected != cameraDevices_.cend() && !selected->formats.isEmpty()) {
        const bool requestedAvailable = std::any_of(selected->formats.cbegin(), selected->formats.cend(), [this](const auto& format) { return format.resolution == cameraSettings_.resolution; });
        if (!requestedAvailable) cameraSettings_.resolution = selected->formats.first().resolution;
    }
    if (!cameraSession_.start(cameraSettings_)) updateCameraControls();
}
void MainWindow::updateCameraControls() {
    if (!cameraDevice_) return;
    const auto state = cameraSession_.state();
    const bool active = cameraSession_.active();
    const bool hasDevice = !cameraDevice_->currentData().toByteArray().isEmpty();
    cameraDevice_->setEnabled(!active);
    cameraRefresh_->setEnabled(!active);
    cameraConnect_->setEnabled(active || hasDevice || state == CameraState::Error);
    cameraConnect_->setText(active || state == CameraState::Error ? QStringLiteral("断开并释放") : QStringLiteral("连接并预览"));
    const bool preview = state == CameraState::Previewing || state == CameraState::Recording;
    if (cameraDevicePanel_) cameraDevicePanel_->setVisible(!preview);
    if (auto* overlay = findChild<QWidget*>(QStringLiteral("cameraPreviewOverlay"))) overlay->setVisible(preview);
    if (auto* fullscreen = findChild<QPushButton*>(QStringLiteral("cameraFullscreenButton"))) fullscreen->setEnabled(preview);
    if (cameraPreviewTime_) cameraPreviewTime_->setVisible(preview);
    cameraCapture_->setEnabled(preview && !cameraSession_.latestFrame().isNull());
    cameraRecord_->setEnabled(preview);
    cameraRecord_->setText(state == CameraState::Recording ? QStringLiteral("停止录像") : QStringLiteral("录像"));
    cameraRecord_->setIcon(workbenchIcon(state == CameraState::Recording ? QStringLiteral("stop") : QStringLiteral("record")));
    cameraSaveLocation_->setEnabled(!active);
    cameraSettingsButton_->setEnabled(state != CameraState::Stopping);
    if (auto* indicator = findChild<QFrame*>(QStringLiteral("cameraStateIndicator"))) {
        const char* kind = state == CameraState::Error ? "error" : state == CameraState::Recording ? "recording" : preview ? "active" : "idle";
        if (indicator->property("cameraStateKind").toString() != QLatin1String(kind)) {
            indicator->setProperty("cameraStateKind", kind); indicator->style()->unpolish(indicator); indicator->style()->polish(indicator);
        }
    }
    const QString deviceName = hasDevice ? cameraDevice_->currentText().remove(QStringLiteral(" · 默认")) : QStringLiteral("未选择");
    if (cameraDetails_) {
        cameraDetails_->setText(preview
            ? QStringLiteral("%1 × %2    │    %3 FPS    │    %4    │    设备：%5")
                .arg(cameraSettings_.resolution.width()).arg(cameraSettings_.resolution.height())
                .arg(cameraSettings_.frameRate, 0, 'f', cameraSettings_.frameRate == std::floor(cameraSettings_.frameRate) ? 0 : 1)
                .arg(cameraSession_.recordingContainer(), deviceName)
            : QStringLiteral("分辨率：—    │    帧率：—    │    编码：—    │    设备：%1").arg(deviceName));
    }
    if (!cameraState_) return;
    switch (state) {
    case CameraState::Disconnected: cameraState_->setText(QStringLiteral("摄像头未连接 · 不会自动请求设备")); break;
    case CameraState::Starting: cameraState_->setText(QStringLiteral("摄像头正在连接…")); break;
    case CameraState::Previewing: cameraState_->setText(QStringLiteral("摄像头已连接")); break;
    case CameraState::Recording: cameraState_->setText(QStringLiteral("正在录像  %1 s").arg(cameraSession_.recordingDurationMs() / 1000.0, 0, 'f', 1)); break;
    case CameraState::Stopping: cameraState_->setText(QStringLiteral("正在完成录像封装并释放设备…")); break;
    case CameraState::Error: cameraState_->setText(QStringLiteral("摄像故障 · %1").arg(cameraSession_.errorString())); break;
    }
    if(auto* overview = findChild<QLabel*>("customerCameraCardState")) {
        overview->setText(cameraState_->text());
        overview->setToolTip(overview->text());
    }
}
void MainWindow::showCameraSettings() {
    QDialog dialog(this); dialog.setObjectName("cameraSettingsDialog"); dialog.setWindowTitle(QStringLiteral("摄像参数与画面调校")); dialog.resize(760, 820);
    auto* root = new QVBoxLayout(&dialog);
    auto* scroll = new QScrollArea; scroll->setWidgetResizable(true); scroll->setFrameShape(QFrame::NoFrame);
    auto* content = new QWidget; auto* layout = new QVBoxLayout(content); layout->setContentsMargins(8, 8, 8, 8); layout->setSpacing(10);
    scroll->setWidget(content); root->addWidget(scroll, 1);

    auto* basicGroup = new QGroupBox(QStringLiteral("设备格式（下次连接生效）")); auto* basic = new QFormLayout(basicGroup);
    auto* resolution = new QComboBox;
    const auto id = cameraDevice_->currentData().toByteArray();
    const auto selected = std::find_if(cameraDevices_.cbegin(), cameraDevices_.cend(), [&](const auto& device) { return device.id == id; });
    QList<QSize> sizes;
    if (selected != cameraDevices_.cend()) for (const auto& format : selected->formats) if (!sizes.contains(format.resolution)) sizes.append(format.resolution);
    if (sizes.isEmpty()) sizes = {QSize(640, 480), QSize(1280, 720), QSize(1920, 1080)};
    for (const auto& size : sizes) resolution->addItem(QStringLiteral("%1 × %2").arg(size.width()).arg(size.height()), size);
    int resolutionIndex = resolution->findData(cameraSettings_.resolution); if (resolutionIndex < 0) resolutionIndex = 0; resolution->setCurrentIndex(resolutionIndex);
    auto* frameRate = number(cameraSettings_.frameRate, 1.0, 240.0, QStringLiteral(" fps"));
    basic->addRow(QStringLiteral("分辨率"), resolution); basic->addRow(QStringLiteral("帧率"), frameRate); layout->addWidget(basicGroup);

    auto* viewportGroup = new QGroupBox(QStringLiteral("画面位置与变换（预览/拍照）")); auto* viewport = new QFormLayout(viewportGroup);
    auto* mirror = new QCheckBox(QStringLiteral("水平镜像")); mirror->setObjectName("cameraMirror"); mirror->setChecked(cameraSettings_.mirrorPreview);
    auto* rotation = new QComboBox; for (int degrees : {0, 90, 180, 270}) rotation->addItem(QStringLiteral("%1°").arg(degrees), degrees);
    rotation->setCurrentIndex(std::max(0, rotation->findData(cameraSettings_.rotationDegrees)));
    rotation->setObjectName("cameraRotation");
    auto* digitalZoom = number(cameraSettings_.digitalZoom, 1.0, 4.0, QStringLiteral("×")); digitalZoom->setSingleStep(0.1); digitalZoom->setObjectName("cameraDigitalZoom");
    auto* panX = number(cameraSettings_.digitalPanX, -1.0, 1.0, QString()); panX->setSingleStep(0.1); panX->setObjectName("cameraPanX");
    auto* panY = number(cameraSettings_.digitalPanY, -1.0, 1.0, QString()); panY->setSingleStep(0.1); panY->setObjectName("cameraPanY");
    auto* panPad = new QWidget; auto* panGrid = new QGridLayout(panPad); panGrid->setContentsMargins(0, 0, 0, 0); panGrid->setSpacing(4);
    auto makePan = [&](const QString& text, const QString& name) { auto* button = new QPushButton(text); button->setObjectName(name); button->setMinimumSize(52, 40); return button; };
    auto* panUp = makePan(QStringLiteral("↑ 上"), "cameraPanUp"); auto* panDown = makePan(QStringLiteral("↓ 下"), "cameraPanDown");
    auto* panLeft = makePan(QStringLiteral("← 左"), "cameraPanLeft"); auto* panRight = makePan(QStringLiteral("→ 右"), "cameraPanRight"); auto* panCenter = makePan(QStringLiteral("居中"), "cameraPanCenter");
    panGrid->addWidget(panUp, 0, 1); panGrid->addWidget(panLeft, 1, 0); panGrid->addWidget(panCenter, 1, 1); panGrid->addWidget(panRight, 1, 2); panGrid->addWidget(panDown, 2, 1);
    viewport->addRow(QStringLiteral("数字平移"), panPad); viewport->addRow(QStringLiteral("水平位置"), panX); viewport->addRow(QStringLiteral("垂直位置"), panY);
    viewport->addRow(QStringLiteral("数字缩放"), digitalZoom); viewport->addRow(QStringLiteral("镜像"), mirror); viewport->addRow(QStringLiteral("旋转"), rotation); layout->addWidget(viewportGroup);

    auto makeInteger = [&](int value, int low, int high, const QString& name) { auto* box = new QSpinBox; box->setRange(low, high); box->setValue(value); box->setObjectName(name); box->setMinimumHeight(36); return box; };
    auto* imageGroup = new QGroupBox(QStringLiteral("软件画质调校（预览/拍照）")); auto* imageForm = new QFormLayout(imageGroup);
    auto* brightness = makeInteger(cameraSettings_.brightness, -100, 100, "cameraBrightness");
    auto* contrast = makeInteger(cameraSettings_.contrast, 0, 200, "cameraContrast");
    auto* saturation = makeInteger(cameraSettings_.saturation, 0, 200, "cameraSaturation");
    auto* hue = makeInteger(cameraSettings_.hue, -180, 180, "cameraHue");
    auto* sharpness = makeInteger(cameraSettings_.sharpness, 0, 100, "cameraSharpness");
    auto* gamma = number(cameraSettings_.gamma, 0.1, 3.0, QString()); gamma->setSingleStep(0.1); gamma->setObjectName("cameraGamma");
    imageForm->addRow(QStringLiteral("亮度 (-100…100)"), brightness); imageForm->addRow(QStringLiteral("对比度 (100=原始)"), contrast);
    imageForm->addRow(QStringLiteral("饱和度 (100=原始)"), saturation); imageForm->addRow(QStringLiteral("色调 (-180°…180°)"), hue);
    imageForm->addRow(QStringLiteral("清晰度 (0…100)"), sharpness); imageForm->addRow(QStringLiteral("伽马 (0.1…3.0)"), gamma); layout->addWidget(imageGroup);

    const auto capabilities = cameraSession_.controlCapabilities();
    const QString unavailable = cameraSession_.active() ? QStringLiteral("当前设备/驱动未报告此 UVC 能力。") : QStringLiteral("连接摄像头后才能检测硬件能力。");
    auto* hardwareGroup = new QGroupBox(QStringLiteral("硬件 UVC 控制（按设备能力启用）")); auto* hardware = new QFormLayout(hardwareGroup);
    auto* hardwareZoom = number(cameraSettings_.hardwareZoom, capabilities.hardwareZoom ? capabilities.minimumHardwareZoom : 1.0,
        capabilities.hardwareZoom ? capabilities.maximumHardwareZoom : 1.0, QStringLiteral("×")); hardwareZoom->setSingleStep(0.1); hardwareZoom->setObjectName("cameraHardwareZoom"); hardwareZoom->setEnabled(capabilities.hardwareZoom); hardwareZoom->setToolTip(capabilities.hardwareZoom ? QString() : unavailable);
    auto* autoFocus = new QCheckBox(QStringLiteral("自动对焦")); autoFocus->setObjectName("cameraAutoFocus"); autoFocus->setChecked(cameraSettings_.autoFocus); autoFocus->setEnabled(capabilities.autoFocus || capabilities.manualFocus); autoFocus->setToolTip(autoFocus->isEnabled() ? QString() : unavailable);
    auto* focusDistance = number(cameraSettings_.focusDistance, 0.0, 1.0, QString()); focusDistance->setSingleStep(0.05); focusDistance->setObjectName("cameraFocusDistance"); focusDistance->setEnabled(capabilities.manualFocus && !autoFocus->isChecked()); focusDistance->setToolTip(capabilities.manualFocus ? QString() : unavailable);
    auto* autoExposure = new QCheckBox(QStringLiteral("自动曝光")); autoExposure->setObjectName("cameraAutoExposure"); autoExposure->setChecked(cameraSettings_.autoExposure); autoExposure->setEnabled(capabilities.autoExposure || capabilities.manualExposure);
    auto* exposureCompensation = number(cameraSettings_.exposureCompensation, -8.0, 8.0, QStringLiteral(" EV")); exposureCompensation->setSingleStep(0.1); exposureCompensation->setObjectName("cameraExposureCompensation"); exposureCompensation->setEnabled(capabilities.exposureCompensation);
    const double exposureLow = capabilities.minimumExposureSeconds > 0 ? capabilities.minimumExposureSeconds : 0.00001;
    const double exposureHigh = capabilities.maximumExposureSeconds > exposureLow ? capabilities.maximumExposureSeconds : 1.0;
    auto* exposureTime = number(cameraSettings_.manualExposureSeconds, exposureLow, exposureHigh, QStringLiteral(" s")); exposureTime->setDecimals(6); exposureTime->setObjectName("cameraExposureTime"); exposureTime->setEnabled(capabilities.manualExposure && !autoExposure->isChecked());
    auto* iso = makeInteger(cameraSettings_.manualIso, capabilities.minimumIso > 0 ? capabilities.minimumIso : 1, capabilities.maximumIso > 0 ? capabilities.maximumIso : 6400, "cameraIso"); iso->setEnabled(capabilities.isoSensitivity && !autoExposure->isChecked());
    auto* autoWhite = new QCheckBox(QStringLiteral("自动白平衡")); autoWhite->setObjectName("cameraAutoWhiteBalance"); autoWhite->setChecked(cameraSettings_.autoWhiteBalance); autoWhite->setEnabled(capabilities.autoWhiteBalance || capabilities.manualWhiteBalance);
    auto* temperature = makeInteger(cameraSettings_.colorTemperature, 2000, 10000, "cameraColorTemperature"); temperature->setSuffix(QStringLiteral(" K")); temperature->setEnabled(capabilities.manualWhiteBalance && !autoWhite->isChecked());
    hardware->addRow(QStringLiteral("镜头/硬件变焦"), hardwareZoom); hardware->addRow(autoFocus); hardware->addRow(QStringLiteral("对焦距离 (0…1)"), focusDistance);
    hardware->addRow(autoExposure); hardware->addRow(QStringLiteral("曝光补偿"), exposureCompensation); hardware->addRow(QStringLiteral("手动曝光时间"), exposureTime); hardware->addRow(QStringLiteral("手动 ISO/增益"), iso);
    hardware->addRow(autoWhite); hardware->addRow(QStringLiteral("手动色温"), temperature); layout->addWidget(hardwareGroup);
    auto* hint = label(QStringLiteral("说明：资料未声明物理云台，上下左右为数字画面平移。软件画质调校影响预览与 PNG 拍照；录像仍保存相机原始 UVC 画面。")); hint->setObjectName("cameraTuningScope"); layout->addWidget(hint);
    layout->addStretch();

    const CameraSettings original = cameraSettings_;
    auto readSettings = [&] {
        CameraSettings value = cameraSettings_;
        value.resolution = resolution->currentData().toSize(); value.frameRate = frameRate->value();
        value.mirrorPreview = mirror->isChecked(); value.rotationDegrees = rotation->currentData().toInt();
        value.digitalPanX = panX->value(); value.digitalPanY = panY->value(); value.digitalZoom = digitalZoom->value();
        value.brightness = brightness->value(); value.contrast = contrast->value(); value.saturation = saturation->value(); value.hue = hue->value(); value.sharpness = sharpness->value(); value.gamma = gamma->value();
        value.autoFocus = autoFocus->isChecked(); value.focusDistance = focusDistance->value(); value.hardwareZoom = hardwareZoom->value();
        value.autoExposure = autoExposure->isChecked(); value.exposureCompensation = exposureCompensation->value(); value.manualExposureSeconds = exposureTime->value(); value.manualIso = iso->value();
        value.autoWhiteBalance = autoWhite->isChecked(); value.colorTemperature = temperature->value(); return value;
    };
    auto applyLive = [&] { if (cameraSession_.active()) cameraSession_.updateSettings(readSettings()); };
    const auto connectDouble = [&](QDoubleSpinBox* box) { connect(box, &QDoubleSpinBox::valueChanged, &dialog, applyLive); };
    const auto connectInteger = [&](QSpinBox* box) { connect(box, &QSpinBox::valueChanged, &dialog, applyLive); };
    for (auto* box : {digitalZoom, panX, panY, gamma, hardwareZoom, focusDistance, exposureCompensation, exposureTime}) connectDouble(box);
    for (auto* box : {brightness, contrast, saturation, hue, sharpness, iso, temperature}) connectInteger(box);
    connect(mirror, &QCheckBox::toggled, &dialog, applyLive); connect(rotation, &QComboBox::currentIndexChanged, &dialog, applyLive);
    connect(autoFocus, &QCheckBox::toggled, &dialog, [&](bool automatic) { focusDistance->setEnabled(capabilities.manualFocus && !automatic); applyLive(); });
    connect(autoExposure, &QCheckBox::toggled, &dialog, [&](bool automatic) { exposureTime->setEnabled(capabilities.manualExposure && !automatic); iso->setEnabled(capabilities.isoSensitivity && !automatic); applyLive(); });
    connect(autoWhite, &QCheckBox::toggled, &dialog, [&](bool automatic) { temperature->setEnabled(capabilities.manualWhiteBalance && !automatic); applyLive(); });
    connect(panUp, &QPushButton::clicked, &dialog, [&] { panY->setValue(std::max(-1.0, panY->value() - 0.1)); });
    connect(panDown, &QPushButton::clicked, &dialog, [&] { panY->setValue(std::min(1.0, panY->value() + 0.1)); });
    connect(panLeft, &QPushButton::clicked, &dialog, [&] { panX->setValue(std::max(-1.0, panX->value() - 0.1)); });
    connect(panRight, &QPushButton::clicked, &dialog, [&] { panX->setValue(std::min(1.0, panX->value() + 0.1)); });
    connect(panCenter, &QPushButton::clicked, &dialog, [&] { panX->setValue(0.0); panY->setValue(0.0); });

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::RestoreDefaults | QDialogButtonBox::Ok | QDialogButtonBox::Cancel); root->addWidget(buttons);
    buttons->button(QDialogButtonBox::RestoreDefaults)->setText(QStringLiteral("恢复默认"));
    buttons->button(QDialogButtonBox::Ok)->setText(QStringLiteral("确定")); buttons->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
    connect(buttons->button(QDialogButtonBox::RestoreDefaults), &QPushButton::clicked, &dialog, [&] {
        panX->setValue(0); panY->setValue(0); digitalZoom->setValue(1); mirror->setChecked(false); rotation->setCurrentIndex(rotation->findData(0));
        brightness->setValue(0); contrast->setValue(100); saturation->setValue(100); hue->setValue(0); sharpness->setValue(0); gamma->setValue(1);
    });
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept); connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (dialog.exec() != QDialog::Accepted) { if (cameraSession_.active()) cameraSession_.updateSettings(original); return; }
    cameraSettings_ = readSettings(); if (cameraSession_.active()) cameraSession_.updateSettings(cameraSettings_);
    QSettings settings(dataRoot_ + "/settings.ini", QSettings::IniFormat);
    settings.setValue("camera/mirror", cameraSettings_.mirrorPreview); settings.setValue("camera/rotation", cameraSettings_.rotationDegrees);
    settings.setValue("camera/frameRate", cameraSettings_.frameRate);
    settings.setValue("camera/digitalPanX", cameraSettings_.digitalPanX); settings.setValue("camera/digitalPanY", cameraSettings_.digitalPanY); settings.setValue("camera/digitalZoom", cameraSettings_.digitalZoom);
    settings.setValue("camera/brightness", cameraSettings_.brightness); settings.setValue("camera/contrast", cameraSettings_.contrast); settings.setValue("camera/saturation", cameraSettings_.saturation);
    settings.setValue("camera/hue", cameraSettings_.hue); settings.setValue("camera/sharpness", cameraSettings_.sharpness); settings.setValue("camera/gamma", cameraSettings_.gamma);
    settings.setValue("camera/autoFocus", cameraSettings_.autoFocus); settings.setValue("camera/focusDistance", cameraSettings_.focusDistance); settings.setValue("camera/hardwareZoom", cameraSettings_.hardwareZoom);
    settings.setValue("camera/autoExposure", cameraSettings_.autoExposure); settings.setValue("camera/exposureCompensation", cameraSettings_.exposureCompensation); settings.setValue("camera/manualExposureSeconds", cameraSettings_.manualExposureSeconds); settings.setValue("camera/manualIso", cameraSettings_.manualIso);
    settings.setValue("camera/autoWhiteBalance", cameraSettings_.autoWhiteBalance); settings.setValue("camera/colorTemperature", cameraSettings_.colorTemperature);
}
void MainWindow::renderCameraFrame(const QImage& image) {
    for (auto* preview : cameraPreviews_) {
        if (!preview) continue;
        if (image.isNull()) {
            static_cast<CameraPreview*>(preview)->setFrame({});
            preview->setPixmap({});
            preview->setText(preview->objectName() == QStringLiteral("cameraCardPreview") ? QString() : QStringLiteral("摄像头未连接\n预览已停止并释放"));
        } else {
            preview->setText({}); static_cast<CameraPreview*>(preview)->setFrame(image);
        }
    }
    updateCameraControls();
}
AcquisitionConfig MainWindow::config() const {
    AcquisitionConfig c;
    const QString selected = device_->currentData().toString();
    c.deviceId = selected == QStringLiteral("simulation") ? "simulation" : selected.startsWith(QStringLiteral("daq:")) ? selected.mid(4).toStdString() : "";
    c.inputChannel = input_->currentData().toString().toStdString();
    c.outputChannel = output_->currentData().toString().toStdString();
    c.waveform = static_cast<Waveform>(waveform_->currentData().toInt());
    c.outputFrequencyHz = frequency_->value(); c.outputAmplitudeV = amplitude_->value(); c.outputOffsetV = offset_->value();
    c.sampleRateHz = sampleRate_->value(); c.displayWindowSamples = displaySamples_->value(); c.inputMinimumV = minimum_->value(); c.inputMaximumV = maximum_->value(); return c;
}
const DaqDeviceDescriptor* MainWindow::selectedDaqDevice() const {
    const QString selected = device_->currentData().toString();
    if (!selected.startsWith(QStringLiteral("daq:"))) return nullptr;
    const auto found = daqDevices_.constFind(selected.mid(4));
    return found == daqDevices_.cend() ? nullptr : &found.value();
}
void MainWindow::applySelectedDevice() {
    const bool simulation = device_->currentData().toString() == QStringLiteral("simulation");
    const auto* descriptor = selectedDaqDevice();
    input_->clear(); output_->clear(); waveform_->clear();
    if (simulation) {
        input_->addItem("模拟电压通道", "simulation/voltage");
        output_->addItem("无输出（AI-only）", ""); waveform_->addItem("未配置", static_cast<int>(Waveform::Sine));
        sampleRate_->setRange(1, 1000000); minimum_->setRange(-10, 10); maximum_->setRange(-10, 10);
    } else if (descriptor) {
        for (const auto& channel : descriptor->capabilities.inputChannels) input_->addItem(QString::fromStdString(channel), QString::fromStdString(channel));
        output_->addItem("无输出（AI-only）", "");
        for (const auto& channel : descriptor->capabilities.outputChannels) output_->addItem(QString::fromStdString(channel), QString::fromStdString(channel));
        waveform_->addItem("正弦波", static_cast<int>(Waveform::Sine)); waveform_->addItem("方波", static_cast<int>(Waveform::Square));
        waveform_->addItem("锯齿波", static_cast<int>(Waveform::Sawtooth)); waveform_->addItem("三角波", static_cast<int>(Waveform::Triangle));
        sampleRate_->setRange(descriptor->capabilities.minimumSampleRateHz, descriptor->capabilities.maximumSampleRateHz);
        minimum_->setRange(descriptor->capabilities.inputMinimumV, descriptor->capabilities.inputMaximumV);
        maximum_->setRange(descriptor->capabilities.inputMinimumV, descriptor->capabilities.inputMaximumV);
    } else {
        input_->addItem("未配置", ""); output_->addItem("无输出（AI-only）", "");
        waveform_->addItem("未配置", static_cast<int>(Waveform::Sine));
    }
    updateControls();
}
void MainWindow::refreshDaqDevices() {
    if (discoveryWorker_ || runtime_.active()) return;
    const QString previous = device_->currentData().toString();
    while (device_->count() > 2) device_->removeItem(2);
    daqDevices_.clear();
    discoveryNotice_.clear();
    refreshDevices_->setText("正在发现设备…");
    refreshDevices_->setEnabled(false);
    auto backend = daqBackend_;
    discoveryWorker_ = QThread::create([this, backend, previous] {
        auto result = backend->discover();
        QMetaObject::invokeMethod(this, [this, previous, result = std::move(result)]() mutable {
            for (auto& descriptor : result.devices) {
                const QString id = QString::fromStdString(descriptor.id);
                const QString title = descriptor.productType.empty() ? id : id + QStringLiteral(" · ") + QString::fromStdString(descriptor.productType);
                daqDevices_.insert(id, descriptor); device_->addItem(title, QStringLiteral("daq:") + id);
            }
            const int restore = device_->findData(previous); if (restore >= 0) device_->setCurrentIndex(restore);
            if (!result.error.empty()) discoveryNotice_ = QString::fromStdString(result.error);
            else discoveryNotice_ = QStringLiteral("设备发现完成：%1 台可用 NI DAQ 设备；未创建 AI/AO 任务。").arg(result.devices.size());
            applySelectedDevice();
        }, Qt::QueuedConnection);
    });
    updateControls();
    refreshDevices_->setEnabled(false);
    auto* worker = discoveryWorker_;
    connect(worker, &QThread::finished, this, [this, worker] {
        if (discoveryWorker_ == worker) discoveryWorker_ = nullptr;
        worker->deleteLater(); refreshDevices_->setText(niDaqmxCompiled() ? "刷新 NI 设备" : "检查 NI 设备支持"); updateControls();
        if (closePending_ && !runtime_.active() && !exportWorker_) { closePending_ = false; QTimer::singleShot(0, this, &QWidget::close); }
    });
    worker->start();
}
void MainWindow::updateControls() {
    if (!notice_) return;
    const bool active = runtime_.active(); const bool simulation = device_->currentData().toString() == QStringLiteral("simulation");
    const auto* descriptor = selectedDaqDevice();
    const auto invalid = simulation ? validate(config(), simulationCapabilities()) : descriptor ? validate(config(), descriptor->capabilities) : validate(config());
    parameters_->setEnabled(!active); start_->setEnabled(!active && !discoveryWorker_ && (simulation || descriptor) && invalid.empty());
    stop_->setEnabled(active && (runtime_.state() == RunState::Starting || runtime_.state() == RunState::Running)); reset_->setEnabled(!active);
    simulationScenario_->setEnabled(!active && simulation);
    refreshDevices_->setEnabled(!active && !discoveryWorker_);
    input_->setEnabled(!active && descriptor); output_->setEnabled(!active && descriptor && !descriptor->capabilities.outputChannels.empty());
    const bool ao = descriptor && !config().outputChannel.empty();
    waveform_->setEnabled(!active && ao); frequency_->setEnabled(!active && ao); amplitude_->setEnabled(!active && ao); offset_->setEnabled(!active && ao);
    start_->setToolTip(QString::fromStdString(invalid));
    start_->setAccessibleDescription(active ? "采集会话尚未结束" : QString::fromStdString(invalid));
    const bool invalidRange = maximum_->value() <= minimum_->value();
    const QString rangeError = invalidRange ? QStringLiteral("量程上限必须大于量程下限，请修改这两个字段。") : QString{};
    for (auto* field : {maximum_, minimum_}) {
        if (field->property("invalid").toBool() != invalidRange) {
            field->setProperty("invalid", invalidRange); field->style()->unpolish(field); field->style()->polish(field);
        }
        field->setAccessibleDescription(rangeError); field->setToolTip(rangeError);
    }
    parameterHint_->setText(invalidRange ? rangeError : (!invalid.empty() && (simulation || descriptor)) ? QString::fromStdString(invalid) : simulation ? QStringLiteral("模拟源仅提供电压，输出参数不可用。显示点数不改变采样率或处理批次。") : descriptor ?
        QStringLiteral("%1 · %2；留空输出通道即 AI-only，启动前再按当前能力快照校验。").arg(QString::fromStdString(descriptor->id), QString::fromStdString(descriptor->productType)) :
        QStringLiteral("请选择模拟源，或刷新并选择 NI 设备。"));
    if (!invalid.empty() && simulation && !active) notice_->setText(QString::fromStdString(invalid));
    else if (runtime_.error().isEmpty()) notice_->setText(discoveryNotice_.isEmpty() ? "旧版算法回放适配层已接入；FV / 频率 / 光强 / 浓度标定未确认，模拟数据不用于测量。" : discoveryNotice_);
    if (!runtime_.error().isEmpty()) notice_->setText(runtime_.error());
    else if (simulation && simulationScenario_->currentIndex() != 0)
        notice_->setText("模拟故障注入已选择：" + simulationScenario_->currentText() + "。仅影响模拟源，无硬件输出。");
    QString state;
    switch (runtime_.state()) {
    case RunState::Ready: state = simulation ? "模拟就绪 · 无硬件输出" : descriptor ? "NI 设备就绪 · 等待手动启动" : "未连接 · 请选择数据源"; break;
    case RunState::Starting: state = activeSessionIsSimulation_ ? "模拟 · 启动中" : "NI DAQ · 正在创建 AI/AO 任务"; break;
    case RunState::Running: state = activeSessionIsSimulation_ ? "模拟运行 · 非测量数据" : "NI DAQ 运行 · 原始电压"; break;
    case RunState::StopRequested: state = activeSessionIsSimulation_ ? "模拟 · 正在停止" : "NI DAQ · 正在停止并释放任务"; break;
    case RunState::Draining: state = activeSessionIsSimulation_ ? "模拟 · 正在排空处理队列" : "NI DAQ 已释放 · 正在排空后台"; break;
    case RunState::Stopped: state = activeSessionIsSimulation_ ? "模拟已停止 · 保留最后数据" : "NI DAQ 已停止 · 任务已清理"; break;
    case RunState::Error: state = active ? "错误 · 等待后台安全退出" : "错误 · 后台已停止"; break;
    }
    status_->setText(state);
}
void MainWindow::startAcquisition() {
    // Neither raw source has a confirmed scan/FV definition yet, so this step
    // deliberately validates transport and storage without invoking TDLAS.
    const bool simulation = device_->currentData().toString() == QStringLiteral("simulation");
    const auto* descriptor = selectedDaqDevice();
    if ((!simulation && !descriptor) || runtime_.active() || confirmationPending_) return;
    if (!(simulation ? validate(config(), simulationCapabilities()) : validate(config(), descriptor->capabilities)).empty()) { updateControls(); return; }
    QScopedValueRollback<bool> confirming(confirmationPending_, true);
    if (hasSessionData() && QMessageBox::question(this, "开始新会话", "当前会话已有数据。开始新会话将替换内存中的原始曲线和采集统计，保留已有批次历史；已完成的归档或导出文件不受影响。是否继续？", QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes) return;
    std::unique_ptr<ISampleSource> source;
    if (simulation) {
        SimulationOptions options;
        switch (simulationScenario_->currentIndex()) {
        case 1: options.failOnOpen = true; break;
        case 2: options.failAfterBlocks = 3; break;
        case 3: options.readDelayMs = 1000; break;
        default: break;
        }
        source = makeSimulationSource(options);
    } else {
        try { source = daqBackend_->makeSource(*descriptor); }
        catch (const std::exception& error) { notice_->setText(QStringLiteral("无法创建 DAQ 会话：") + QString::fromUtf8(error.what())); return; }
    }
    currentArchive_ = std::make_shared<AsyncRawArchive>(QDir(dataRoot_).filePath("raw-runs"), config().queueCapacitySamples);
    activeSessionIsSimulation_ = simulation;
    if (runtime_.startWithResults(config(), std::move(source), [](const SampleBlock&) { return std::vector<AcquisitionFrame>{}; }, currentArchive_)) {
        hasRendered_ = false; refreshSnapshot();
    } else currentArchive_.reset();
}
void MainWindow::refreshSnapshot() {
    refreshResults();
    const auto snapshot = runtime_.snapshot();
    const auto& stats = snapshot.stats;
    const auto shutdown = runtime_.shutdownSnapshot();
    QString closeStage = runtime_.active() && !shutdown.stopRequested ? QStringLiteral("运行中") : QStringLiteral("尚未开始");
    if (shutdown.stopRequested && !shutdown.sourceClosed) closeStage = QStringLiteral("等待数据源释放");
    if (shutdown.sourceClosed) closeStage = QStringLiteral("数据源已释放 → 正在关闭输入");
    if (shutdown.inputClosed) closeStage = QStringLiteral("数据源已释放 → 输入已关闭 → 正在排空");
    if (shutdown.processingDrained) closeStage = QStringLiteral("数据源已释放 → 输入已关闭 → 处理已排空");
    if (shutdown.stopTimedOut) closeStage += QStringLiteral("（已超时，继续等待）");
    const QString accounting = !stats.accountingConsistent() ? QStringLiteral("异常") :
        stats.terminalComplete() ? QStringLiteral("完整") : QStringLiteral("进行中且一致");
    diagnostics_->setText(QString("输入 %1 块 / %2 点 · 终态 %3 块 / %4 点\n"
                                  "排队 %5 块 / %6 点 · 正在处理 %7 块 / %8 点\n"
                                  "队列高水位 %9 块 / %10 点 · 拒收 %11 块 · 失败 %12 块\n"
                                  "终态核对：%13 · 关闭：%14")
        .arg(stats.acceptedBlocks).arg(stats.acceptedSamples).arg(stats.terminalBlocks()).arg(stats.terminalSamples())
        .arg(stats.queuedBlocks).arg(stats.queuedSamples).arg(stats.processingBlocks).arg(stats.processingSamples)
        .arg(stats.highWaterBlocks).arg(stats.highWaterSamples).arg(stats.rejectedBlocks).arg(stats.failedBlocks)
        .arg(accounting, closeStage));
    if (snapshot.samples.empty()) {
        hasRendered_ = false; rawSeries_->clear();
        for (auto* state : rawStates_) state->setText(runtime_.active() ? "模拟电压 · 等待首块数据" :
            runtime_.state() == RunState::Error ? "模拟故障 · 无有效电压数据" : "等待采集 · 仅模拟电压");
        return;
    }
    const auto time = QDateTime::fromMSecsSinceEpoch(snapshot.acquiredAtUtcMs, QTimeZone::UTC).toLocalTime().toString("yyyy-MM-dd HH:mm:ss.zzz t");
    for (auto* state : rawStates_) state->setText((runtime_.active() ? "模拟电压 · " : "已停止 · 最后有效 · ") + time);
    if (hasRendered_ && renderedRunId_ == snapshot.runId && renderedSequence_ == snapshot.lastSequence) return;
    renderedRunId_ = snapshot.runId; renderedSequence_ = snapshot.lastSequence; hasRendered_ = true;
    QList<QPointF> points;
    const auto budget = static_cast<std::size_t>(tokens_["chart"].toObject()["suggestedVisiblePointBudget"].toInt(5000));
    // Display-only min/max buckets preserve extrema; the algorithm sees every sample.
    const auto step = std::max<std::size_t>(1, (snapshot.samples.size() + budget / 2 - 1) / (budget / 2));
    for (std::size_t i = 0; i < snapshot.samples.size(); i += step) {
        const auto end = std::min(i + step, snapshot.samples.size());
        const auto range = std::minmax_element(snapshot.samples.begin() + i, snapshot.samples.begin() + end);
        auto a = static_cast<std::size_t>(range.first - snapshot.samples.begin()); auto b = static_cast<std::size_t>(range.second - snapshot.samples.begin());
        if (a > b) std::swap(a, b);
        points.append(QPointF(a / snapshot.sampleRateHz, snapshot.samples[a]));
        if (a != b) points.append(QPointF(b / snapshot.sampleRateHz, snapshot.samples[b]));
    }
    rawSeries_->replace(points);
    auto* chart = rawSeries_->chart();
    if (!chartView(chart) || chartView(chart)->autoFollow()) {
        qobject_cast<QValueAxis*>(chart->axes(Qt::Horizontal).first())->setRange(0, std::max(0.001, (snapshot.samples.size() - 1) / snapshot.sampleRateHz));
        const auto range = std::minmax_element(snapshot.samples.begin(), snapshot.samples.end());
        const double pad = std::max(0.1, (*range.second - *range.first) * 0.1);
        qobject_cast<QValueAxis*>(chart->axes(Qt::Vertical).first())->setRange(*range.first - pad, *range.second + pad);
    }
}
void MainWindow::renderCurves(QChart* chart, const std::vector<Curve>& curves, const QStringList& names) {
    chart->removeAllSeries();
    const auto colors = tokens_["themes"].toObject()[night_ ? "night" : "day"].toObject();
    bool any = false; double xmin = 0, xmax = 1, ymin = -1, ymax = 1;
    for (std::size_t ci = 0; ci < curves.size(); ++ci) {
        const auto& curve = curves[ci];
        if (curve.x.size() != curve.y.size() || (!curve.pointValid.empty() && curve.pointValid.size() != curve.y.size())) continue;
        const auto valid = [&](std::size_t i) { return std::isfinite(curve.x[i]) && std::isfinite(curve.y[i]) && (curve.pointValid.empty() || curve.pointValid[i]); };
        const auto step = std::max<std::size_t>(1, (curve.y.size() + 2499) / 2500);
        QList<QPointF> points;
        auto flush = [&] {
            if (points.empty()) return;
            auto* line = new QLineSeries; line->replace(points);
            const bool peakHistory = chart == resultCharts_.value("historyCard");
            line->setPointsVisible(peakHistory || points.size() == 1); line->setMarkerSize(peakHistory ? 6.0 : 5.0);
            const bool fit = names.size() > static_cast<qsizetype>(ci) && names[static_cast<qsizetype>(ci)] == "拟合";
            line->setProperty("fitted", fit);
            line->setName(names.size() > static_cast<qsizetype>(ci) ? names[static_cast<qsizetype>(ci)] : QString::fromStdString(curve.yQuantity));
            const auto colorKey = fit ? "seriesFit" : chart == resultCharts_["peakCard"] || chart == resultCharts_["historyCard"] ? "seriesPeak" : "seriesFv";
            line->setProperty("colorKey", colorKey);
            line->setPen(QPen(QColor(colors[colorKey].toString()), 2, fit ? Qt::DashLine : Qt::SolidLine));
            chart->addSeries(line); for (auto* axis : chart->axes()) line->attachAxis(axis); points.clear();
        };
        for (std::size_t i = 0; i < curve.y.size();) {
            if (!valid(i)) { flush(); ++i; continue; }
            auto end = i;
            while (end < curve.y.size() && end - i < step && valid(end)) ++end;
            auto lo = i, hi = i;
            for (auto j = i; j < end; ++j) {
                if (curve.y[j] < curve.y[lo]) lo = j;
                if (curve.y[j] > curve.y[hi]) hi = j;
                if (!any) { xmin = xmax = curve.x[j]; ymin = ymax = curve.y[j]; any = true; }
                xmin = std::min(xmin, curve.x[j]); xmax = std::max(xmax, curve.x[j]); ymin = std::min(ymin, curve.y[j]); ymax = std::max(ymax, curve.y[j]);
            }
            if (lo > hi) std::swap(lo, hi);
            points.append(QPointF(curve.x[lo], curve.y[lo]));
            if (hi != lo) points.append(QPointF(curve.x[hi], curve.y[hi]));
            i = end;
        }
        flush();
    }
    chart->legend()->setVisible(!names.empty());
    // A curve split at invalid points still has exactly one legend entry.
    QStringList legendNames;
    for (auto* marker : chart->legend()->markers()) {
        marker->setVisible(!legendNames.contains(marker->label())); legendNames.append(marker->label());
    }
    auto* x = qobject_cast<QValueAxis*>(chart->axes(Qt::Horizontal).first());
    auto* y = qobject_cast<QValueAxis*>(chart->axes(Qt::Vertical).first());
    if (!curves.empty()) {
        x->setTitleText(QString::fromStdString(curves.front().xQuantity + " / " + curves.front().xUnit));
        y->setTitleText(QString::fromStdString(curves.front().yQuantity + " / " + curves.front().yUnit));
    } else { x->setTitleText("坐标待确认"); y->setTitleText("物理量 / 单位待确认"); }
    if (!chartView(chart) || chartView(chart)->autoFollow()) {
        x->setRange(xmin, xmax > xmin ? xmax : xmin + 1);
        const auto pad = std::max(0.1, (ymax - ymin) * 0.1); y->setRange(ymin - pad, ymax + pad);
    }
}
void MainWindow::refreshResults() {
    const auto changed = runtime_.results().snapshotAfter(renderedResults_.revision);
    if (changed) {
        renderedResults_ = *changed;
        const auto& current = renderedResults_.latest;
        const auto fv = current && current->quality == Quality::Valid ? std::vector<Curve>{current->fv} : std::vector<Curve>{};
        renderCurves(resultCharts_["customerFv"], fv); renderCurves(resultCharts_["fvCard"], fv);
        std::vector<Curve> fit; QStringList fitNames;
        if (current && current->quality == Quality::Valid) {
            fit.push_back(current->observed); fitNames << "观测";
            if (current->fitStatus == "success" && current->observed.xQuantity == current->fitted.xQuantity && current->observed.xUnit == current->fitted.xUnit &&
                current->observed.yQuantity == current->fitted.yQuantity && current->observed.yUnit == current->fitted.yUnit) { fit.push_back(current->fitted); fitNames << "拟合"; }
        }
        renderCurves(resultCharts_["fitCard"], fit, fitNames);
        std::vector<Curve> history;
        // A chart must never put incompatible units on the same numerical axis.
        const auto segments = peakSegments(renderedResults_);
        const auto unit = segments.empty() ? std::string{} : segments.back().unit;
        for (const auto& segment : segments) if (segment.unit == unit) history.push_back(Curve{segment.elapsedSeconds, segment.values, "各会话经过时间", "s", "FV 峰值", unit});
        renderCurves(resultCharts_["peakCard"], history);
        QStringList historyLegend; for (std::size_t index = 0; index < history.size(); ++index) historyLegend << QStringLiteral("FV峰值");
        renderCurves(resultCharts_["historyCard"], history, historyLegend);
        renderCurves(resultCharts_["customerHistoryCard"], history);
        updateCustomerHistoryRange();
        if (auto* range = findChild<QComboBox*>("peakTimeRange")) {
            auto* chart = resultCharts_["historyCard"]; auto* view = chartView(chart);
            if (view && view->autoFollow()) {
                double low = 0.0, high = 0.0; bool any = false;
                for (auto* item : chart->series()) if (auto* line = qobject_cast<QLineSeries*>(item))
                    for (const auto& point : line->points()) { if (!any) { low = high = point.x(); any = true; } low = std::min(low, point.x()); high = std::max(high, point.x()); }
                const int seconds = range->currentData().toInt();
                if (seconds > 0 && any) low = std::max(low, high - seconds);
                if (auto* axis = qobject_cast<QValueAxis*>(chart->axes(Qt::Horizontal).value(0))) axis->setRange(low, high > low ? high : low + 1.0);
            }
        }
        if (history.empty()) for (const auto& key : {QStringLiteral("peakCard"), QStringLiteral("historyCard")})
            qobject_cast<QValueAxis*>(resultCharts_[key]->axes(Qt::Horizontal).first())->setTitleText("各会话经过时间 / s");
        const QString historyState = renderedResults_.entries.empty() ? "等待完整 FV 批次" : segments.empty() ?
            QString("共 %1 批次 · 暂无有效峰值 · 原因见表").arg(renderedResults_.entries.size()) :
            QString("共 %1 批次 · 显示最近单位 %2 · 全量见表").arg(renderedResults_.entries.size()).arg(QString::fromStdString(unit));
        resultStates_["peakCard"]->setText(historyState); resultStates_["historyCard"]->setText(historyState);
        resultStates_["historyCard"]->setVisible(segments.empty());
        resultStates_["customerHistoryCard"]->setText(historyState);
        peakTable_->setRowCount(static_cast<int>(renderedResults_.entries.size()));
        int row = 0;
        for (const auto& entry : renderedResults_.entries) {
            const auto& p = entry.peak;
            const auto metric = [](const std::optional<double>& value, int decimals) { return value ? QString::number(*value, 'f', decimals) : QStringLiteral("—"); };
            const Metrics metrics = entry.frame ? entry.frame->metrics : Metrics{};
            const QString status = p.peakValue ? QStringLiteral("●  有效") : QStringLiteral("●  无效：%1").arg(QString::fromStdString(p.qualityReason));
            const QStringList cells = {QString::number(row + 1),
                QDateTime::fromMSecsSinceEpoch(p.acquiredAtUtcMs, QTimeZone::UTC).toLocalTime().toString("yyyy-MM-dd  HH:mm:ss"),
                metric(p.peakValue, 3), metric(metrics.intensityPercent, 1), metric(metrics.concentration, 2), status};
            for (int column = 0; column < cells.size(); ++column) {
                auto* item = new QTableWidgetItem(cells[column]);
                item->setTextAlignment(column == 1 || column == 5 ? Qt::AlignCenter : Qt::AlignRight | Qt::AlignVCenter);
                item->setToolTip(column == 0 ? QStringLiteral("会话：%1\n批次：%2").arg(QString::fromStdString(p.runId), QString::fromStdString(p.acquisitionId)) :
                    column == 2 ? QStringLiteral("%1 %2").arg(cells[column], QString::fromStdString(p.fvUnit)) :
                    column == 5 ? (p.peakValue ? QStringLiteral("FV峰值有效") : QString::fromStdString(p.qualityReason)) : cells[column]);
                if (column == 5) {
                    const auto theme = tokens_["themes"].toObject()[night_ ? "night" : "day"].toObject();
                    item->setForeground(QColor(theme[p.peakValue ? "success" : "danger"].toString()));
                }
                peakTable_->setItem(row, column, item);
            }
            ++row;
        }
        if (auto* count = findChild<QLabel*>("peakStatCount")) count->setText(QString::number(renderedResults_.entries.size()));
        std::vector<double> peakValues;
        for (const auto& entry : renderedResults_.entries)
            if (entry.peak.peakValue && entry.peak.fvUnit == unit) peakValues.push_back(*entry.peak.peakValue);
        double minimum = 0.0, maximum = 0.0, mean = 0.0, deviation = 0.0;
        if (!peakValues.empty()) {
            const auto limits = std::minmax_element(peakValues.begin(), peakValues.end()); minimum = *limits.first; maximum = *limits.second;
            for (const double value : peakValues) mean += value; mean /= static_cast<double>(peakValues.size());
            for (const double value : peakValues) deviation += (value - mean) * (value - mean);
            deviation = std::sqrt(deviation / static_cast<double>(peakValues.size()));
        }
        const auto setStatistic = [this, &unit, &peakValues](const char* name, double value) {
            if (auto* label = findChild<QLabel*>(name)) label->setText(peakValues.empty() ? QStringLiteral("—") : QString::number(value, 'f', 3));
            if (auto* label = findChild<QLabel*>(QString::fromLatin1(name) + "Unit")) label->setText(unit.empty() ? QStringLiteral("—") : QString::fromStdString(unit));
        };
        setStatistic("peakStatMaximum", maximum); setStatistic("peakStatMinimum", minimum);
        setStatistic("peakStatAverage", mean); setStatistic("peakStatDeviation", deviation);
        exportButton_->setEnabled(!renderedResults_.entries.empty() && !exportWorker_);
        customerExport_->setEnabled(!renderedResults_.entries.empty() && !exportWorker_);
    }
    const auto& current = renderedResults_.latest;
    const auto number = [](const std::optional<double>& v) { return v ? QString::number(*v, 'g', 8) : QStringLiteral("—"); };
    const Metrics m = current ? current->metrics : Metrics{};
    QString state = "等待完整批次 · 算法/物理定义待接入";
    if (current) {
        state = (runtime_.active() ? QStringLiteral("当前批次 · ") : QStringLiteral("已停止 · ")) +
            QDateTime::fromMSecsSinceEpoch(current->acquiredAtUtcMs, QTimeZone::UTC).toLocalTime().toString("HH:mm:ss.zzz t");
        if (current->quality != Quality::Valid) state += " · " + QString::fromStdString(current->qualityReason);
    }
    for (const auto& key : {QStringLiteral("peakAbsorbance"), QStringLiteral("intensityPercent"), QStringLiteral("concentration")}) {
        const QString text = key == "peakAbsorbance" ? number(m.peakAbsorbance) : key == "intensityPercent" ? number(m.intensityPercent) :
            m.concentration ? number(m.concentration) + " " + QString::fromStdString(m.concentrationUnit) : QStringLiteral("未校准");
        for (auto* reading : findChildren<QLabel*>(key)) { reading->setText(text); reading->setToolTip(state); }
    }
    resultStates_["customerFv"]->setText(state); resultStates_["fvCard"]->setText(state);
    resultStates_["fitCard"]->setText(current ? state + " · 拟合 " + QString::fromStdString(current->fitStatus) : state);
}
void MainWindow::exportData() {
    if (exportWorker_) return;
    const auto snapshot = runtime_.results().snapshot();
    if (snapshot.entries.empty()) { notice_->setText("没有可导出的完整批次结果"); return; }
    const QString suggested = QDir(dataRoot_).filePath("exports/" + QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss"));
    const QString directory = QFileDialog::getExistingDirectory(this, "选择导出目录", QFileInfo(suggested).absolutePath());
    if (directory.isEmpty()) return;
    const QString target = QDir(directory).filePath(QFileInfo(suggested).fileName());
    auto result = std::make_shared<ExportResult>();
    exportButton_->setEnabled(false); exportButton_->setText("正在导出……"); notice_->setText("正在后台导出一致性结果快照……");
    customerExport_->setEnabled(false); customerExport_->setText("正在导出……");
    exportWorker_ = QThread::create([snapshot, target, result] { *result = exportResults(snapshot, target); });
    connect(exportWorker_, &QThread::finished, this, [this, result] {
        auto* worker = exportWorker_; exportWorker_ = nullptr; worker->deleteLater(); exportButton_->setText("导出 CSV + JSON");
        customerExport_->setText("导出数据");
        if (result->succeeded) notice_->setText("导出完成：" + result->directory);
        else notice_->setText("导出失败，未标记为已保存：" + result->error);
        exportButton_->setEnabled(!runtime_.results().snapshot().entries.empty());
        customerExport_->setEnabled(!runtime_.results().snapshot().entries.empty());
        if (closePending_ && !runtime_.active() && !discoveryWorker_) { closePending_ = false; QTimer::singleShot(0, this, &QWidget::close); }
    });
    exportWorker_->start();
}
bool MainWindow::hasSessionData() const {
    // Destructive-view decisions use accepted data even when the display timer
    // has not painted it. Persisted files are not deleted by either action.
    return runtime_.snapshot().stats.acceptedBlocks != 0 || !runtime_.results().snapshot().entries.empty();
}
void MainWindow::resetDisplay() {
    if (runtime_.active() || confirmationPending_) return;
    QScopedValueRollback<bool> confirming(confirmationPending_, true);
    if (hasSessionData() && QMessageBox::question(this, "清空当前会话", "当前会话已有数据。复位将清空内存中的全部曲线、峰值历史、指标和统计，保留参数以及已完成的归档或导出文件。是否继续？", QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes) return;
    if (!runtime_.reset()) return;
    simulationScenario_->setCurrentIndex(0); hasRendered_ = false; rawSeries_->clear();
    for (auto* chart : charts_) {
        chart->zoomReset(); if (auto* view = chartView(chart)) view->setAutoFollow(true);
        const auto* view = chartView(chart); const QString cardName = view && view->parentWidget() ? view->parentWidget()->objectName() : QString();
        for (auto* axis : chart->axes()) if (auto* value = qobject_cast<QValueAxis*>(axis)) {
            if (cardName == "rawCard") value->setRange(axis->orientation() == Qt::Horizontal ? 0 : -2, axis->orientation() == Qt::Horizontal ? 10 : 2);
            else if (cardName == "fitCard") value->setRange(axis->orientation() == Qt::Horizontal ? -3 : 0, axis->orientation() == Qt::Horizontal ? 3 : 1);
            else if (cardName == "fvCard") value->setRange(axis->orientation() == Qt::Horizontal ? -3.5 : -0.5, axis->orientation() == Qt::Horizontal ? 3.5 : 2.5);
            else if (cardName == "peakCard") value->setRange(axis->orientation() == Qt::Horizontal ? 0 : 0, axis->orientation() == Qt::Horizontal ? 600 : 1.6);
            else value->setRange(axis->orientation() == Qt::Horizontal ? 0 : -1, 1);
        }
    }
    for (auto* state : rawStates_) state->setText("等待采集 · 仅模拟电压"); refreshSnapshot();
}
void MainWindow::setNightTheme(bool night) {
    night_ = night; const auto colors = tokens_["themes"].toObject()[night ? "night" : "day"].toObject();
    QString style = R"(
        QWidget { background: @background; color: @text; font-family: 'Microsoft YaHei'; font-size: 14px; }
        QFrame#header { background: qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 @surface,stop:.55 @surfaceSubtle,stop:1 @surface); border-bottom: 1px solid @primary; }
        QLabel#brandIcon { color: @primary; font-size: 30px; font-weight: 700; }
        QLabel#brandTitle { color: @selectedTabText; font-size: 24px; font-weight: 700; }
        QLabel#brandPulse { color: @primary; font-size: 22px; }
        QLabel#brandSlogan { color: @primary; font-size: 13px; }
        QLabel#headerClock, QLabel#headerUser { color: @selectedTabText; font-weight: 600; }
        QLabel#acquisitionStatus { color: @success; font-weight: 600; padding: 5px 10px; border-radius: 12px; background: @surface; }
        QPushButton#settingsButton, QPushButton#themeButton { border: 0; background: transparent; color: @selectedTabText; padding: 4px 8px; }
        QGroupBox, QFrame[chartCard="true"], QFrame#metricCard { background: qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 @surface,stop:1 @background); }
        QGroupBox { border: 1px solid @borderSubtle; border-radius: 8px; margin-top: 12px; font-size: 17px; font-weight: 600; }
        QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 6px; color: @selectedTabText; }
        QLabel { background: transparent; }
        QLabel[heading="true"] { color: @selectedTabText; font-size: 18px; font-weight: 600; }
        QFrame[debugChartCard="true"] { background: qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 @surface,stop:1 @background); }
        QFrame[debugChartCard="true"] QFrame[cardHeader="true"] { background: transparent; border: 0; min-height: 46px; }
        QFrame[debugChartCard="true"] QLabel[heading="true"] { color: @selectedTabText; font-size: 20px; font-weight: 700; }
        QFrame[debugChartCard="true"] QLabel[chartToolLabel="true"] { color: @selectedTabText; font-size: 13px; font-weight: 600; margin-left: 4px; }
        QComboBox[chartTool="true"] { min-height: 32px; background: @surface; border: 1px solid @controlBorder; border-radius: 6px; padding: 0 18px 0 8px; }
        QPushButton[debugTool="true"] { min-height: 32px; color: @selectedTabText; border-color: @controlBorder; background: @surface; padding: 0 7px; }
        QPushButton[debugTool="true"]:checked { color: @selectedTabText; background: @selectedTabBackground; border-color: @primary; }
        QWidget[debugToolbar="true"] { min-height: 28px; }
        QFrame[cardHeader="true"] { background: @surfaceSubtle; border: 0; border-bottom: 1px solid @borderSubtle; border-top-left-radius: 7px; border-top-right-radius: 7px; }
        QLabel[chartState="true"] { color: @textSecondary; font-size: 12px; }
        QFrame#customerFv, QFrame#customerHistoryCard { background: @surface; }
        QFrame[peakHistoryCard="true"] { background: @surface; border: 1px solid @borderSubtle; border-radius: 8px; }
        QFrame[peakHistoryCard="true"] QFrame[cardHeader="true"] { min-height: 48px; background: qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 @surface,stop:1 @surfaceSubtle); }
        QFrame[peakHistoryCard="true"] QLabel[heading="true"] { color: @selectedTabText; font-size: 19px; font-weight: 700; }
        QFrame[peakHistoryCard="true"] QLabel[chartToolLabel="true"] { color: @selectedTabText; font-size: 13px; font-weight: 600; }
        QFrame[peakHistoryCard="true"] QComboBox[chartTool="true"] { min-width: 92px; min-height: 34px; background: @surface; border: 1px solid @borderSubtle; border-radius: 6px; }
        QPushButton[peakImageAction="true"] { min-height: 38px; color: @primary; background: @surfaceSubtle; border: 1px solid @borderSubtle; padding: 0 12px; font-weight: 600; }
        QFrame[peakPanel="true"] { background: @surface; border: 1px solid @borderSubtle; border-radius: 8px; }
        QPushButton[peakSectionTitle="true"] { min-height: 34px; color: @selectedTabText; background: transparent; border: 0; padding: 0 4px; font-size: 18px; font-weight: 700; text-align: left; }
        QPushButton[peakSectionTitle="true"]:hover, QPushButton[peakSectionTitle="true"]:checked { color: @selectedTabText; background: transparent; border: 0; }
        QWidget[peakStatisticsHeader="true"] { min-height: 26px; background: transparent; }
        QLabel[peakStatisticsTitle="true"] { color: @selectedTabText; font-size: 16px; font-weight: 700; }
        QFrame[peakStatCard="true"] { min-height: 54px; background: qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 @surface,stop:1 @background); border: 1px solid @borderSubtle; border-radius: 7px; }
        QLabel[peakStatLabel="true"] { color: @primary; font-size: 12px; }
        QLabel[peakStatValue="true"] { color: @primary; font-size: 23px; font-weight: 600; }
        QLabel[peakStatUnit="true"] { color: @primary; font-size: 12px; }
        QComboBox#customerHistoryRange { min-height: 38px; }
        QLabel[metricIcon="true"] { color: @primary; font-size: 25px; font-weight: 700; }
        QLabel[metricCaption="true"] { color: @selectedTabText; font-size: 14px; font-weight: 600; }
        QLabel[metricUnit="true"] { color: @textSecondary; font-size: 12px; }
        QLabel[metricValue="true"] { color: @primary; font-size: 25px; font-weight: 700; }
        QFrame[chartCard="true"], QFrame#metricCard, QFrame#sidebarMetric { border: 1px solid @borderSubtle; border-radius: 8px; }
        QFrame#sidebarMetric { background: @surface; }
        QComboBox, QSpinBox, QDoubleSpinBox { min-height: 34px; background: @surface; border: 1px solid @controlBorder; border-radius: 7px; padding: 0 9px; }
        QComboBox::drop-down { width: 24px; border: 0; border-left: 1px solid @borderSubtle; border-top-right-radius: 7px; border-bottom-right-radius: 7px; }
        QComboBox QAbstractItemView { background: @surface; color: @text; border: 1px solid @controlBorder; selection-background-color: @selectedTabBackground; selection-color: @selectedTabText; }
        QWidget[parameterControl="true"] { background: transparent; }
        QDoubleSpinBox[parameterEditor="true"], QSpinBox[parameterEditor="true"] { border-top-right-radius: 0; border-bottom-right-radius: 0; border-right: 0; padding-left: 10px; padding-right: 10px; }
        QLabel[unitSegment="true"] { min-height: 34px; color: @textSecondary; background: @surfaceSubtle; border: 1px solid @controlBorder; border-left: 1px solid @borderSubtle; border-top-right-radius: 7px; border-bottom-right-radius: 7px; padding: 0 8px; }
        QPushButton { min-height: 32px; background: @surface; border: 1px solid @controlBorder; border-radius: 6px; padding: 0 7px; }
        QLabel#parameterHint { color: @textSecondary; font-size: 12px; }
        QLabel[parameterNumber="true"] { color: @primary; }
        QLabel[parameterCaptionText="true"] { font-size: 13px; }
        QWidget[parameterCaption="true"], QWidget[metricLabels="true"] { background: transparent; }
        QPushButton#refreshDevicesButton { min-height: 22px; padding: 0 8px; }
        QPushButton[action="start"] { min-height: 54px; color: @onPrimary; border-color: @success; background: @success; font-weight: 700; }
        QPushButton[action="stop"] { min-height: 54px; color: @onPrimary; border-color: @danger; background: @danger; font-weight: 700; }
        QPushButton[action="reset"] { min-height: 54px; color: @selectedTabText; border-color: @primary; background: @selectedTabBackground; font-weight: 700; }
        QPushButton:hover { background: @selectedTabBackground; color: @selectedTabText; }
        QPushButton[action="start"]:hover { background: @success; color: @onPrimary; border: 2px solid @focus; }
        QPushButton[action="stop"]:hover { background: @danger; color: @onPrimary; border: 2px solid @focus; }
        QPushButton:pressed { background: @primaryHover; color: @onPrimary; }
        QPushButton:checked { background: @selectedTabBackground; color: @selectedTabText; border-color: @primary; }
        QPushButton:disabled, QComboBox:disabled, QSpinBox:disabled, QDoubleSpinBox:disabled { background: @disabledBackground; color: @disabledText; }
        QPushButton:focus, QComboBox:focus, QSpinBox:focus, QDoubleSpinBox:focus { border: 2px solid @focus; }
        QDoubleSpinBox[invalid="true"] { border: 2px solid @danger; }
        QTabBar::tab:focus { border: 2px solid @focus; }
        QTabWidget::pane { border: 0; top: -1px; }
        QTabBar { font-size: 17px; font-weight: 600; }
        QTableWidget { background: @surface; gridline-color: @borderSubtle; }
        QHeaderView::section { background: @surfaceSubtle; color: @text; padding: 8px; border: 1px solid @borderSubtle; }
        QTableWidget#peakTable { color: @selectedTabText; background: @surface; border: 1px solid @borderSubtle; selection-background-color: @selectedTabBackground; selection-color: @selectedTabText; }
        QTableWidget#peakTable::item { padding: 2px 8px; border-bottom: 1px solid @borderSubtle; }
        QTableWidget#peakTable QHeaderView::section { min-height: 28px; color: @selectedTabText; background: @surfaceSubtle; border: 0; border-right: 1px solid @borderSubtle; border-bottom: 1px solid @borderSubtle; font-weight: 600; }
        QFrame[cameraPanel="true"] { background: @surface; border: 1px solid @borderSubtle; border-radius: 8px; }
        QFrame[cameraPanel="true"] QFrame[cardHeader="true"] { min-height: 48px; background: @cameraStatusSurface; }
        QLabel[cameraSurface="true"] { color: @textSecondary; font-size: 17px; background: @cameraPreviewSurface; border: 1px solid @borderSubtle; border-radius: 5px; }
        QFrame[cameraDevicePanel="true"] { background: @surface; border: 1px solid @primary; border-radius: 8px; }
        QLabel[cameraDevicePrompt="true"] { color: @selectedTabText; font-size: 18px; font-weight: 700; }
        QLabel[cameraDeviceHint="true"] { color: @textSecondary; font-size: 13px; }
        QFrame[cameraPreviewOverlay="true"] { background: @cameraOverlayBackground; border: 0; border-radius: 5px; }
        QLabel#cameraPreviewTime { color: @cameraOverlayText; font-size: 13px; }
        QPushButton#cameraFullscreenButton { min-width: 30px; min-height: 30px; max-width: 30px; max-height: 30px; color: @cameraOverlayText; background: transparent; border: 0; padding: 0; }
        QFrame[cameraInfoBar="true"] { min-height: 58px; background: @cameraStatusSurface; border: 0; border-top: 1px solid @borderSubtle; border-bottom: 1px solid @borderSubtle; }
        QFrame#cameraStateIndicator { border: 0; border-radius: 5px; }
        QFrame#cameraStateIndicator[cameraStateKind="idle"] { background: @textSecondary; }
        QFrame#cameraStateIndicator[cameraStateKind="active"] { background: @success; }
        QFrame#cameraStateIndicator[cameraStateKind="recording"], QFrame#cameraStateIndicator[cameraStateKind="error"] { background: @danger; }
        QLabel[cameraState="true"] { color: @success; font-size: 14px; font-weight: 600; }
        QLabel[cameraDetails="true"] { color: @selectedTabText; font-size: 13px; }
        QLabel[cameraSaveState="true"] { color: @textSecondary; font-size: 11px; }
        QPushButton[cameraTool="true"] { min-height: 40px; color: @primary; background: transparent; border: 0; padding: 0 6px; font-size: 13px; font-weight: 600; }
        QPushButton[cameraTool="true"]:hover { color: @selectedTabText; background: @selectedTabBackground; border: 1px solid @borderSubtle; }
        QPushButton[cameraAction="true"] { min-height: 80px; border-radius: 6px; font-size: 21px; font-weight: 700; padding: 0 18px; }
        QPushButton[cameraAction="true"][cameraRole="primary"] { color: @primary; border: 1px solid @primary; background: qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 @cameraActionTop,stop:1 @cameraActionBottom); }
        QPushButton[cameraAction="true"][cameraRole="record"] { color: @danger; border: 1px solid @danger; background: qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 @cameraRecordTop,stop:1 @cameraRecordBottom); }
        QPushButton[cameraAction="true"]:hover { border: 2px solid @focus; }
        QPushButton[cameraAction="true"]:pressed { color: @onPrimary; background: @primaryHover; }
        QPushButton[cameraAction="true"]:disabled { color: @disabledText; border-color: @controlBorder; background: @disabledBackground; }
        QLabel#notice, QLabel#runtimeDiagnostics { color: @textSecondary; }
        QScrollArea { border: 0; }
    )";
    auto keys = colors.keys();
    std::sort(keys.begin(), keys.end(), [](const QString& a, const QString& b) { return a.size() > b.size(); });
    for (const auto& key : keys) style.replace("@" + key, colors[key].toString());
    qApp->setStyleSheet(style);
    for (auto* heading : findChildren<QLabel*>()) {
        if (!heading->property("chartTitle").toBool()) continue;
        heading->setMinimumWidth(heading->fontMetrics().horizontalAdvance(heading->text()) + 10);
    }
    QPalette palette; palette.setColor(QPalette::Window, QColor(colors["background"].toString())); palette.setColor(QPalette::WindowText, QColor(colors["text"].toString()));
    palette.setColor(QPalette::Base, QColor(colors["surface"].toString())); palette.setColor(QPalette::Text, QColor(colors["text"].toString()));
    palette.setColor(QPalette::Button, QColor(colors["surface"].toString())); palette.setColor(QPalette::ButtonText, QColor(colors["text"].toString())); qApp->setPalette(palette);
    palette.setColor(QPalette::Disabled,QPalette::ButtonText,QColor(colors["disabledText"].toString())); qApp->setPalette(palette);
    if (peakTable_) for (int row = 0; row < peakTable_->rowCount(); ++row)
        if (auto* item = peakTable_->item(row, 5)) item->setForeground(QColor(colors[item->text().contains(QStringLiteral("有效")) && !item->text().contains(QStringLiteral("无效")) ? "success" : "danger"].toString()));
    auto* sampleTabs = static_cast<SampleTabBar*>(tabs_->tabBar()); sampleTabs->colors=colors; sampleTabs->update();
    for(auto* icon : findChildren<QLabel*>()) if(icon->property("workbenchIcon").isValid())
        icon->setPixmap(workbenchIcon(icon->property("workbenchIcon").toString()).pixmap(icon->size(),devicePixelRatioF()));
    for (auto* chart : charts_) {
        chart->setBackgroundVisible(false); chart->setPlotAreaBackgroundBrush(QColor(colors["chartBackground"].toString())); chart->setPlotAreaBackgroundVisible(true);
        QFont chartFont("Microsoft YaHei"); chartFont.setPixelSize(12);
        for (auto* axis : chart->axes()) {
            axis->setLabelsFont(chartFont); axis->setTitleFont(chartFont); axis->setMinorGridLineVisible(false);
            axis->setLabelsColor(QColor(colors["chartAxis"].toString())); axis->setTitleBrush(QColor(colors["textSecondary"].toString()));
            axis->setLinePenColor(QColor(colors["chartAxis"].toString())); axis->setGridLineColor(QColor(colors["chartGrid"].toString()));
        }
        chart->legend()->setLabelColor(QColor(colors["text"].toString()));
        for (auto* series : chart->series()) if (auto* line = qobject_cast<QLineSeries*>(series)) {
            const QString key = line == rawSeries_ ? QStringLiteral("seriesRaw") : line->property("colorKey").toString();
            line->setPen(QPen(QColor(colors[key.isEmpty() ? QStringLiteral("seriesFv") : key].toString()), 2, line->property("fitted").toBool() ? Qt::DashLine : Qt::SolidLine));
        }
    }
    theme_->setText(night ? "切换日间" : "切换夜间");
    QDir().mkpath(dataRoot_); QSettings settings(dataRoot_ + "/settings.ini", QSettings::IniFormat); settings.setValue("ui/theme", night ? "night" : "day");
}
void MainWindow::closeEvent(QCloseEvent* event) {
    if ((cameraSession_.active() && cameraSession_.state() != CameraState::Stopping) || cameraSession_.state() == CameraState::Error) cameraSession_.stop();
    if (!runtime_.active() && !exportWorker_ && !discoveryWorker_ && !cameraSession_.active()) { event->accept(); return; }
    closePending_ = true;
    if (runtime_.active()) runtime_.requestStop();
    notice_->setText(cameraSession_.active() ? "正在安全关闭：完成录像封装并释放摄像设备……" : discoveryWorker_ ? "正在安全关闭：等待设备发现返回……" : exportWorker_ ? "正在安全关闭：等待后台导出完成……" : "正在安全关闭：停止数据源并排空后台队列与归档……"); event->ignore();
}
}
