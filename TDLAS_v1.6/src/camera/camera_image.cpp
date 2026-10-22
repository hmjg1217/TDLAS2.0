#include "camera/camera_image.h"

#include <QFileInfo>
#include <QImageWriter>
#include <QPainter>
#include <QSaveFile>
#include <QTransform>
#include <algorithm>

namespace tdlas {

QSize cameraCanvasSize(const QSize &source, int rotationDegrees)
{
    if (source.isEmpty()) return {};
    QTransform rotation;
    rotation.rotate(rotationDegrees);
    const QRectF bounds = rotation.mapRect(QRectF(QPointF(), QSizeF(source)));
    // Round instead of ceil for right-angle floating point residue.
    return QSize(qCeil(bounds.width() - 1e-8), qCeil(bounds.height() - 1e-8));
}

void paintCameraImage(QPainter &painter, const QImage &source, const QRectF &viewport,
                      const QPoint &offsetPercent, int rotationDegrees)
{
    if (source.isNull() || viewport.isEmpty()) return;
    const QSize canvas = cameraCanvasSize(source.size(), rotationDegrees);
    const double scale = std::min(viewport.width() / canvas.width(),
                                  viewport.height() / canvas.height());
    const QRectF target(viewport.center().x() - canvas.width() * scale / 2,
                         viewport.center().y() - canvas.height() * scale / 2,
                         canvas.width() * scale, canvas.height() * scale);
    painter.save();
    painter.setClipRect(target);
    painter.fillRect(target, Qt::black);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.translate(target.center());
    painter.scale(scale, scale);
    painter.translate(canvas.width() * offsetPercent.x() / 100.0,
                      canvas.height() * offsetPercent.y() / 100.0);
    painter.rotate(rotationDegrees);
    painter.drawImage(QPointF(-source.width() / 2.0, -source.height() / 2.0), source);
    painter.restore();
}

QImage cameraPhotograph(const QImage &source, const QPoint &offsetPercent, int rotationDegrees)
{
    if (source.isNull()) return {};
    QImage result(cameraCanvasSize(source.size(), rotationDegrees), QImage::Format_RGB32);
    result.fill(Qt::black);
    QPainter painter(&result);
    paintCameraImage(painter, source, QRectF(result.rect()), offsetPercent, rotationDegrees);
    return result;
}

bool saveCameraPhotograph(const QImage &image, const QString &path, QString *error)
{
    if (image.isNull()) {
        if (error) *error = QStringLiteral("尚未拍摄照片");
        return false;
    }
    QByteArray format = QFileInfo(path).suffix().toLatin1().toLower();
    if (format == "jpg") format = "jpeg";
    if (format != "png" && format != "jpeg" && format != "bmp") {
        if (error) *error = QStringLiteral("请选择 PNG、JPG 或 BMP 图片格式");
        return false;
    }
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        if (error) *error = file.errorString();
        return false;
    }
    QImageWriter writer(&file, format);
    writer.setQuality(95);
    if (!writer.write(image)) {
        if (error) *error = writer.errorString();
        return false;
    }
    if (!file.commit()) {
        if (error) *error = file.errorString();
        return false;
    }
    return true;
}

} // namespace tdlas
