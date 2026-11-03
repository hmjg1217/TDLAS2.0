#pragma once

#include <QWidget>

namespace adminui {

class CameraPreviewWidget final : public QWidget
{
public:
    explicit CameraPreviewWidget(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
};

} // namespace adminui
