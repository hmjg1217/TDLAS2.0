#pragma once

#include <QImage>
#include <QPoint>
#include <QRectF>

class QPainter;

namespace tdlas {

// Offsets are percentages of the rotated image canvas; positive = right/down.
// Preview and saved photographs share exactly the same transformation.
QSize cameraCanvasSize(const QSize &source, int rotationDegrees);
void paintCameraImage(QPainter &painter, const QImage &source, const QRectF &viewport,
                      const QPoint &offsetPercent, int rotationDegrees);
QImage cameraPhotograph(const QImage &source, const QPoint &offsetPercent, int rotationDegrees);
bool saveCameraPhotograph(const QImage &image, const QString &path, QString *error);

} // namespace tdlas
