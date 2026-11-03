#include "ChartWidget.h"

#include <QPainter>
#include <QPainterPath>

#include <cmath>

namespace clientui {

namespace {

constexpr double kPi = 3.14159265358979323846;

double gaussian(double x, double center, double width)
{
    const double z = (x - center) / width;
    return std::exp(-0.5 * z * z);
}

double seriesValue(ChartWidget::Mode mode, double x)
{
    if (mode == ChartWidget::Mode::FvCurve) {
        return 0.04 + 0.82 * gaussian(x, 0.52, 0.055) + 0.05 * gaussian(x, 0.43, 0.13);
    }
    return 0.49 + 0.014 * std::sin(x * kPi * 4.0) + 0.010 * std::sin(x * kPi * 71.0);
}

QString xTick(ChartWidget::Mode mode, int index)
{
    if (mode == ChartWidget::Mode::FvCurve) {
        return index == 0 ? QStringLiteral("0") : QStringLiteral("%1k").arg(index * 20);
    }
    return QString::number(index * 120);
}

QString yTick(ChartWidget::Mode mode, int index)
{
    if (mode == ChartWidget::Mode::FvCurve) {
        return QString::number(index * 0.03, 'f', 2);
    }
    return QString::number(0.095 + index * 0.0015, 'f', 3);
}

} // namespace

ChartWidget::ChartWidget(Mode mode, QWidget *parent)
    : QWidget(parent)
    , mode_(mode)
{
    setObjectName(QStringLiteral("chartWidget"));
    setMinimumSize(250, 150);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void ChartWidget::setAxisLabels(const QString &xLabel, const QString &yLabel)
{
    xLabel_ = xLabel;
    yLabel_ = yLabel;
    update();
}

void ChartWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), QColor(QStringLiteral("#ffffff")));

    const int left = qMin(68, qMax(46, width() / 10));
    const QRectF plot(left, 18, qMax(40, width() - left - 18), qMax(40, height() - 64));

    painter.setPen(QPen(QColor(QStringLiteral("#dbe8f5")), 1.0));
    for (int i = 0; i <= 5; ++i) {
        const qreal x = plot.left() + plot.width() * i / 5.0;
        painter.drawLine(QPointF(x, plot.top()), QPointF(x, plot.bottom()));
    }
    for (int i = 0; i <= 4; ++i) {
        const qreal y = plot.bottom() - plot.height() * i / 4.0;
        painter.drawLine(QPointF(plot.left(), y), QPointF(plot.right(), y));
    }

    QFont tickFont = font();
    tickFont.setPointSizeF(qMax(7.5, font().pointSizeF() - 1.0));
    painter.setFont(tickFont);
    painter.setPen(QColor(QStringLiteral("#48617f")));
    for (int i = 0; i <= 5; ++i) {
        const qreal x = plot.left() + plot.width() * i / 5.0;
        painter.drawText(QRectF(x - 30, plot.bottom() + 6, 60, 20), Qt::AlignHCenter | Qt::AlignTop, xTick(mode_, i));
    }
    for (int i = 0; i <= 4; ++i) {
        const qreal y = plot.bottom() - plot.height() * i / 4.0;
        painter.drawText(QRectF(0, y - 10, plot.left() - 8, 20), Qt::AlignRight | Qt::AlignVCenter, yTick(mode_, i));
    }

    painter.setPen(QPen(QColor(QStringLiteral("#8ca9cb")), 1.2));
    painter.drawLine(plot.bottomLeft(), plot.bottomRight());
    painter.drawLine(plot.bottomLeft(), plot.topLeft());

    QPainterPath path;
    constexpr int samples = 260;
    for (int i = 0; i < samples; ++i) {
        const double x = static_cast<double>(i) / (samples - 1);
        const double value = qBound(0.02, seriesValue(mode_, x), 0.98);
        const QPointF point(plot.left() + x * plot.width(), plot.bottom() - value * plot.height());
        i == 0 ? path.moveTo(point) : path.lineTo(point);
    }
    painter.setClipRect(plot.adjusted(-2, -2, 2, 2));
    painter.setPen(QPen(QColor(QStringLiteral("#0879ef")), 2.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawPath(path);
    painter.setClipping(false);

    painter.setPen(QColor(QStringLiteral("#36506f")));
    painter.drawText(QRectF(plot.left(), height() - 25, plot.width(), 20), Qt::AlignCenter, xLabel_);
    painter.save();
    painter.translate(14, plot.center().y());
    painter.rotate(-90.0);
    painter.drawText(QRectF(-plot.height() / 2.0, -9, plot.height(), 18), Qt::AlignCenter, yLabel_);
    painter.restore();

    painter.setPen(QColor(QStringLiteral("#6d86a5")));
    painter.drawText(QRectF(plot.right() - 120, plot.top() + 5, 114, 18), Qt::AlignRight, QStringLiteral("SIM · 模拟数据"));
}

} // namespace clientui
