#pragma once

#include <QWidget>

namespace clientui {

class ChartWidget final : public QWidget
{
public:
    enum class Mode {
        FvCurve,
        PeakTrend
    };

    explicit ChartWidget(Mode mode, QWidget *parent = nullptr);

    void setAxisLabels(const QString &xLabel, const QString &yLabel);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    Mode mode_;
    QString xLabel_;
    QString yLabel_;
};

} // namespace clientui
