#include "CameraPreviewWidget.h"

#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>

namespace clientui {

CameraPreviewWidget::CameraPreviewWidget(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("cameraPreview"));
    setMinimumSize(360, 220);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void CameraPreviewWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);

    const QRectF canvas = rect().adjusted(1, 1, -1, -1);
    QPainterPath clip;
    clip.addRoundedRect(canvas, 10, 10);
    painter.setClipPath(clip);

    QLinearGradient background(0, 0, 0, height());
    background.setColorAt(0.0, QColor(QStringLiteral("#263b4c")));
    background.setColorAt(0.58, QColor(QStringLiteral("#101b26")));
    background.setColorAt(1.0, QColor(QStringLiteral("#07111c")));
    painter.fillRect(rect(), background);

    const qreal midY = height() * 0.50;
    const qreal tubeLeft = width() * 0.20;
    const qreal tubeRight = width() * 0.82;
    const qreal tubeHeight = height() * 0.30;

    painter.setPen(Qt::NoPen);
    QLinearGradient metal(0, midY - tubeHeight, 0, midY + tubeHeight);
    metal.setColorAt(0.0, QColor(QStringLiteral("#d7e0e7")));
    metal.setColorAt(0.20, QColor(QStringLiteral("#607181")));
    metal.setColorAt(0.48, QColor(QStringLiteral("#f0f5f8")));
    metal.setColorAt(0.70, QColor(QStringLiteral("#394d5e")));
    metal.setColorAt(1.0, QColor(QStringLiteral("#ced9e0")));
    painter.setBrush(metal);
    painter.drawRoundedRect(QRectF(tubeLeft - 38, midY - tubeHeight * 0.62, 62, tubeHeight * 1.24), 10, 10);
    painter.drawRoundedRect(QRectF(tubeRight - 24, midY - tubeHeight * 0.62, 62, tubeHeight * 1.24), 10, 10);

    QLinearGradient glass(0, midY - tubeHeight / 2, 0, midY + tubeHeight / 2);
    glass.setColorAt(0.0, QColor(185, 226, 244, 130));
    glass.setColorAt(0.45, QColor(230, 246, 253, 85));
    glass.setColorAt(0.75, QColor(88, 136, 159, 110));
    glass.setColorAt(1.0, QColor(198, 231, 244, 120));
    painter.setBrush(glass);
    painter.setPen(QPen(QColor(214, 239, 250, 170), 2.0));
    painter.drawRoundedRect(QRectF(tubeLeft, midY - tubeHeight / 2, tubeRight - tubeLeft, tubeHeight), tubeHeight / 3, tubeHeight / 3);

    painter.setPen(QPen(QColor(255, 52, 68, 80), 11, Qt::SolidLine, Qt::RoundCap));
    painter.drawLine(QPointF(width() * 0.08, midY), QPointF(width() * 0.94, midY));
    painter.setPen(QPen(QColor(QStringLiteral("#ff596d")), 3.2, Qt::SolidLine, Qt::RoundCap));
    painter.drawLine(QPointF(width() * 0.08, midY), QPointF(width() * 0.94, midY));

    painter.setClipping(false);
    painter.fillRect(QRectF(0, height() - 42, width(), 42), QColor(5, 14, 23, 205));
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(QStringLiteral("#ff304f")));
    painter.drawEllipse(QPointF(22, height() - 21), 5, 5);
    painter.setPen(Qt::white);
    painter.drawText(QRectF(36, height() - 35, width() * 0.45, 28), Qt::AlignVCenter, QStringLiteral("模拟预览"));
    painter.drawText(QRectF(width() * 0.55, height() - 35, width() * 0.42, 28), Qt::AlignRight | Qt::AlignVCenter, QStringLiteral("1920 × 1080   30 FPS"));

    painter.setPen(QPen(QColor(255, 255, 255, 210), 2.0));
    const qreal x = width() - 32;
    const qreal y = 18;
    painter.drawLine(QPointF(x - 12, y), QPointF(x, y));
    painter.drawLine(QPointF(x, y), QPointF(x, y + 12));
    painter.drawLine(QPointF(x - 28, y), QPointF(x - 40, y));
    painter.drawLine(QPointF(x - 40, y), QPointF(x - 40, y + 12));
}

} // namespace clientui
