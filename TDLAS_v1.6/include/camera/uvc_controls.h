#pragma once

#include <QByteArray>
#include <QString>
#include <memory>

namespace tdlas {

enum class CameraControl {
    Brightness, Contrast, Hue, Sharpness, Saturation, Gamma, WhiteBalance, Focus, Zoom
};

struct CameraControlRange {
    bool supported = false;
    bool manualSupported = false;
    bool autoSupported = false;
    int minimum = 0;
    int maximum = 0;
    int step = 1;
    int defaultValue = 0;
    int value = 0;
    bool automatic = false;
};

// Standard UVC controls exposed by the Windows USB video class driver.
// All methods, including destruction, run on the owning GUI thread.
class UvcControls final {
public:
    UvcControls();
    ~UvcControls();
    bool open(const QByteArray &deviceId, const QString &description, QString *error);
    void close();
    CameraControlRange range(CameraControl control) const;
    bool set(CameraControl control, int value, bool automatic, QString *error);

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace tdlas
