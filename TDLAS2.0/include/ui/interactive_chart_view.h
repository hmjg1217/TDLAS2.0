#pragma once

#include <QChartView>
#include <QPoint>

namespace tdlas {

// Common interaction policy for every product chart. User navigation disables
// automatic range updates until the toolbar reset/follow action is requested.
class InteractiveChartView final : public QChartView {
    Q_OBJECT
public:
    explicit InteractiveChartView(QChart* chart, QWidget* parent = nullptr);
    bool autoFollow() const noexcept { return autoFollow_; }
    void setAutoFollow(bool enabled);
signals:
    void autoFollowChanged(bool enabled);
protected:
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
private:
    bool hasPoints() const;
    void zoom(double factor);
    void showNearestPoint(const QPoint& position);
    bool panning_ = false;
    bool autoFollow_ = true;
    QPoint lastPosition_;
};

} // namespace tdlas
