#include "camera/uvc_controls.h"

#include <algorithm>
#ifdef Q_OS_WIN
#include <windows.h>
#include <dshow.h>
#include <wrl/client.h>
#endif

namespace tdlas {

struct UvcControls::Impl {
#ifdef Q_OS_WIN
    bool comOwned = false;
    Microsoft::WRL::ComPtr<IBaseFilter> filter;
    Microsoft::WRL::ComPtr<IAMVideoProcAmp> video;
    Microsoft::WRL::ComPtr<IAMCameraControl> camera;
#endif
};

#ifdef Q_OS_WIN
namespace {
using Microsoft::WRL::ComPtr;

bool cameraProperty(CameraControl control)
{
    return control == CameraControl::Focus || control == CameraControl::Zoom;
}

long propertyId(CameraControl control)
{
    switch (control) {
    case CameraControl::Brightness: return VideoProcAmp_Brightness;
    case CameraControl::Contrast: return VideoProcAmp_Contrast;
    case CameraControl::Hue: return VideoProcAmp_Hue;
    case CameraControl::Sharpness: return VideoProcAmp_Sharpness;
    case CameraControl::Saturation: return VideoProcAmp_Saturation;
    case CameraControl::Gamma: return VideoProcAmp_Gamma;
    case CameraControl::WhiteBalance: return VideoProcAmp_WhiteBalance;
    case CameraControl::Focus: return CameraControl_Focus;
    case CameraControl::Zoom: return CameraControl_Zoom;
    }
    return -1;
}

QString readProperty(IPropertyBag *bag, const wchar_t *name)
{
    VARIANT value;
    VariantInit(&value);
    QString result;
    if (SUCCEEDED(bag->Read(name, &value, nullptr)) && value.vt == VT_BSTR)
        result = QString::fromWCharArray(value.bstrVal);
    VariantClear(&value);
    return result;
}

QString normalizedId(QString value)
{
    value = value.toLower();
    value.remove(QStringLiteral("@device:pnp:"));
    // Media Foundation and DirectShow append different interface GUIDs to
    // the same physical USB instance path.
    const qsizetype interfaceGuid = value.indexOf(QStringLiteral("#{"));
    if (interfaceGuid >= 0) value.truncate(interfaceGuid);
    return value;
}
} // namespace
#endif

UvcControls::UvcControls() : m_impl(std::make_unique<Impl>())
{
#ifdef Q_OS_WIN
    const HRESULT result = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    m_impl->comOwned = SUCCEEDED(result);
#endif
}

UvcControls::~UvcControls()
{
    close();
#ifdef Q_OS_WIN
    if (m_impl->comOwned) CoUninitialize();
#endif
}

void UvcControls::close()
{
#ifdef Q_OS_WIN
    m_impl->camera.Reset();
    m_impl->video.Reset();
    m_impl->filter.Reset();
#endif
}

bool UvcControls::open(const QByteArray &deviceId, const QString &description, QString *error)
{
    close();
#ifdef Q_OS_WIN
    ComPtr<ICreateDevEnum> devices;
    HRESULT result = CoCreateInstance(CLSID_SystemDeviceEnum, nullptr, CLSCTX_INPROC_SERVER,
                                      IID_PPV_ARGS(devices.GetAddressOf()));
    if (FAILED(result)) {
        if (error) *error = QStringLiteral("无法访问 Windows 摄像头控制接口 (0x%1)")
                               .arg(quint32(result), 8, 16, QLatin1Char('0'));
        return false;
    }
    ComPtr<IEnumMoniker> enumerator;
    if (devices->CreateClassEnumerator(CLSID_VideoInputDeviceCategory,
                                       enumerator.GetAddressOf(), 0) != S_OK) {
        if (error) *error = QStringLiteral("未发现 UVC 控制接口");
        return false;
    }
    ComPtr<IMoniker> moniker;
    ComPtr<IMoniker> selected;
    ComPtr<IMoniker> nameMatch;
    int nameMatches = 0;
    const QString requested = normalizedId(QString::fromUtf8(deviceId));
    while (enumerator->Next(1, moniker.ReleaseAndGetAddressOf(), nullptr) == S_OK) {
        ComPtr<IPropertyBag> bag;
        if (FAILED(moniker->BindToStorage(nullptr, nullptr, IID_PPV_ARGS(bag.GetAddressOf()))))
            continue;
        const QString path = normalizedId(readProperty(bag.Get(), L"DevicePath"));
        if (!requested.isEmpty() && !path.isEmpty() && requested == path) {
            selected = moniker;
            break;
        }
        if (readProperty(bag.Get(), L"FriendlyName") == description) {
            nameMatch = moniker;
            ++nameMatches;
        }
    }
    // Never bind a different camera just because it has a similar name.
    if (!selected && nameMatches == 1) selected = nameMatch;
    if (!selected) {
        if (error) *error = QStringLiteral("未能唯一匹配此摄像头的 UVC 控制接口");
        return false;
    }
    result = selected->BindToObject(nullptr, nullptr, IID_PPV_ARGS(m_impl->filter.GetAddressOf()));
    if (FAILED(result)) {
        if (error) *error = QStringLiteral("摄像头控制接口打开失败 (0x%1)")
                               .arg(quint32(result), 8, 16, QLatin1Char('0'));
        return false;
    }
    m_impl->filter.As(&m_impl->video);
    m_impl->filter.As(&m_impl->camera);
    if (!m_impl->video && !m_impl->camera) {
        if (error) *error = QStringLiteral("设备未提供标准 UVC 参数控制");
        return false;
    }
    return true;
#else
    Q_UNUSED(deviceId)
    Q_UNUSED(description)
    if (error) *error = QStringLiteral("当前平台未实现 UVC 参数控制，仍可预览及拍照");
    return false;
#endif
}

CameraControlRange UvcControls::range(CameraControl control) const
{
    CameraControlRange range;
#ifdef Q_OS_WIN
    long minimum = 0, maximum = 0, step = 1, defaultValue = 0, capabilities = 0;
    long value = 0, flags = 0;
    HRESULT result = E_NOINTERFACE;
    HRESULT readResult = E_NOINTERFACE;
    if (cameraProperty(control) && m_impl->camera) {
        result = m_impl->camera->GetRange(propertyId(control), &minimum, &maximum, &step,
                                          &defaultValue, &capabilities);
        readResult = m_impl->camera->Get(propertyId(control), &value, &flags);
    } else if (!cameraProperty(control) && m_impl->video) {
        result = m_impl->video->GetRange(propertyId(control), &minimum, &maximum, &step,
                                         &defaultValue, &capabilities);
        readResult = m_impl->video->Get(propertyId(control), &value, &flags);
    }
    if (SUCCEEDED(result) && SUCCEEDED(readResult) && minimum <= maximum) {
        range.supported = true;
        // Both DirectShow interfaces use Auto=1 and Manual=2.
        range.autoSupported = (capabilities & 1) != 0;
        range.manualSupported = (capabilities & 2) != 0;
        range.minimum = minimum;
        range.maximum = maximum;
        range.step = std::max(1L, step);
        range.defaultValue = defaultValue;
        range.value = value;
        range.automatic = (flags & 1) != 0;
    }
#else
    Q_UNUSED(control)
#endif
    return range;
}

bool UvcControls::set(CameraControl control, int value, bool automatic, QString *error)
{
    const CameraControlRange limits = range(control);
    if (!limits.supported || (automatic && !limits.autoSupported)
        || (!automatic && !limits.manualSupported)) {
        if (error) *error = QStringLiteral("设备不支持该参数或调节模式");
        return false;
    }
#ifdef Q_OS_WIN
    const long bounded = std::clamp(value, limits.minimum, limits.maximum);
    const long aligned = limits.minimum + ((bounded - limits.minimum) / limits.step) * limits.step;
    const long flags = automatic ? 1 : 2;
    const HRESULT result = cameraProperty(control)
        ? m_impl->camera->Set(propertyId(control), aligned, flags)
        : m_impl->video->Set(propertyId(control), aligned, flags);
    if (SUCCEEDED(result)) return true;
    if (error) *error = QStringLiteral("摄像头拒绝参数设置 (0x%1)")
                           .arg(quint32(result), 8, 16, QLatin1Char('0'));
#else
    Q_UNUSED(value)
#endif
    return false;
}

} // namespace tdlas
