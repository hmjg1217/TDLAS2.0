#include "ui/interactive_chart_view.h"
#include <QKeyEvent>
#include <QLineSeries>
#include <QMouseEvent>
#include <QToolTip>
#include <QValueAxis>
#include <QWheelEvent>
#include <cmath>
#include <limits>

namespace tdlas {
InteractiveChartView::InteractiveChartView(QChart* chart, QWidget* parent) : QChartView(chart, parent) {
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setRenderHint(QPainter::Antialiasing);
    setAccessibleName(QStringLiteral("交互式曲线图"));
    setAccessibleDescription(QStringLiteral("滚轮或加减键缩放，左键拖拽或方向键平移，移动鼠标查看最近点，R 复位并恢复自动跟随"));
}
void InteractiveChartView::setAutoFollow(bool enabled) {
    if (autoFollow_ == enabled) return;
    autoFollow_ = enabled; emit autoFollowChanged(enabled);
}
bool InteractiveChartView::hasPoints() const {
    if (!chart()) return false;
    for (auto* series : chart()->series())
        if (auto* line = qobject_cast<QLineSeries*>(series); line && !line->points().isEmpty()) return true;
    return false;
}
void InteractiveChartView::zoom(double factor) {
    if (!chart() || factor <= 0) return;
    if (!hasPoints()) { setAutoFollow(false); return; }
    setAutoFollow(false); chart()->zoom(factor);
}
void InteractiveChartView::wheelEvent(QWheelEvent* event) {
    if (event->angleDelta().y() == 0) { QChartView::wheelEvent(event); return; }
    zoom(event->angleDelta().y() > 0 ? 1.25 : 0.8); event->accept();
}
void InteractiveChartView::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        panning_ = true; lastPosition_ = event->pos(); setCursor(Qt::ClosedHandCursor); setFocus(); event->accept(); return;
    }
    QChartView::mousePressEvent(event);
}
void InteractiveChartView::mouseMoveEvent(QMouseEvent* event) {
    if (panning_) {
        const QPoint delta = event->pos() - lastPosition_; lastPosition_ = event->pos();
        setAutoFollow(false); if (hasPoints()) chart()->scroll(-delta.x(), delta.y()); event->accept(); return;
    }
    showNearestPoint(event->pos()); QChartView::mouseMoveEvent(event);
}
void InteractiveChartView::mouseReleaseEvent(QMouseEvent* event) {
    if (panning_ && event->button() == Qt::LeftButton) {
        panning_ = false; unsetCursor(); event->accept(); return;
    }
    QChartView::mouseReleaseEvent(event);
}
void InteractiveChartView::leaveEvent(QEvent* event) { QToolTip::hideText(); QChartView::leaveEvent(event); }
void InteractiveChartView::keyPressEvent(QKeyEvent* event) {
    switch (event->key()) {
    case Qt::Key_Plus: case Qt::Key_Equal: zoom(1.25); break;
    case Qt::Key_Minus: zoom(0.8); break;
    case Qt::Key_Left: setAutoFollow(false); if (hasPoints()) chart()->scroll(-20, 0); break;
    case Qt::Key_Right: setAutoFollow(false); if (hasPoints()) chart()->scroll(20, 0); break;
    case Qt::Key_Up: setAutoFollow(false); if (hasPoints()) chart()->scroll(0, 20); break;
    case Qt::Key_Down: setAutoFollow(false); if (hasPoints()) chart()->scroll(0, -20); break;
    case Qt::Key_R: case Qt::Key_0: chart()->zoomReset(); setAutoFollow(true); break;
    case Qt::Key_F: setAutoFollow(!autoFollow_); break;
    default: QChartView::keyPressEvent(event); return;
    }
    event->accept();
}
void InteractiveChartView::showNearestPoint(const QPoint& position) {
    if (!chart() || !chart()->plotArea().contains(mapToScene(position))) { QToolTip::hideText(); return; }
    double bestDistance = std::numeric_limits<double>::max(); QPointF bestValue; QString bestName;
    for (auto* abstract : chart()->series()) {
        auto* line = qobject_cast<QLineSeries*>(abstract); if (!line || !line->isVisible()) continue;
        for (const auto& value : line->points()) {
            const QPointF pixel = chart()->mapToPosition(value, line);
            const double dx = pixel.x() - position.x(), dy = pixel.y() - position.y();
            const double distance = dx * dx + dy * dy;
            if (distance < bestDistance) { bestDistance = distance; bestValue = value; bestName = line->name(); }
        }
    }
    if (bestDistance <= 144.0) QToolTip::showText(mapToGlobal(position), QStringLiteral("%1\nx = %2\ny = %3").arg(bestName, QString::number(bestValue.x(), 'g', 10), QString::number(bestValue.y(), 'g', 10)), this);
    else QToolTip::hideText();
}
} // namespace tdlas
