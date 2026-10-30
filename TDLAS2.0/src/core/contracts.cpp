#include "core/contracts.h"
#include <cmath>
#include <algorithm>
#include <limits>

namespace tdlas {
std::string validate(const AcquisitionConfig& c) {
    if (c.deviceId.empty()) return "请选择数据源";
    if (c.inputChannel.empty()) return "请选择采集通道";
    if (!std::isfinite(c.sampleRateHz) || c.sampleRateHz <= 0) return "采样率必须为有限正数";
    if (!std::isfinite(c.inputMinimumV) || !std::isfinite(c.inputMaximumV) || c.inputMinimumV >= c.inputMaximumV)
        return "量程上限必须大于有限的量程下限";
    if (!c.displayWindowSamples || !c.readBlockSamples || !c.processingWindowSamples ||
        c.queueCapacitySamples < c.readBlockSamples || (c.scanPeriodSamples && !*c.scanPeriodSamples))
        return "窗口和块大小必须大于零，队列容量必须能容纳完整读取块";
    if (c.displayWindowSamples > 1000000 || c.readBlockSamples > 1000000 || c.processingWindowSamples > 10000000 ||
        (c.scanPeriodSamples && *c.scanPeriodSamples > 10000000) || c.queueCapacitySamples > 10000000)
        return "缓冲配置超过基础版内存预算";
    if (!c.outputChannel.empty()) {
        if (c.waveform != Waveform::Sine && c.waveform != Waveform::Square && c.waveform != Waveform::Sawtooth && c.waveform != Waveform::Triangle)
            return "输出波形无效";
        if (!std::isfinite(c.outputFrequencyHz) || c.outputFrequencyHz <= 0) return "输出频率必须为有限正数";
        if (!std::isfinite(c.outputAmplitudeV) || c.outputAmplitudeV < 0 || !std::isfinite(c.outputOffsetV))
            return "峰值幅值必须非负且输出参数必须有限";
        if (c.outputOffsetV - c.outputAmplitudeV < -10 || c.outputOffsetV + c.outputAmplitudeV > 10)
            return "输出范围必须位于 −10 V 至 +10 V";
        if (c.sampleRateHz < 10 * c.outputFrequencyHz) return "采样率必须至少为输出频率的 10 倍";
    }
    return {};
}
std::string validate(const AcquisitionConfig& c, const DeviceCapabilities& d) {
    if (auto error = validate(c); !error.empty()) return error;
    const auto contains = [](const auto& values, const auto& value) { return std::find(values.begin(), values.end(), value) != values.end(); };
    if (c.deviceId != d.deviceId || !contains(d.inputChannels, c.inputChannel)) return "设备或采集通道不属于能力快照";
    if (!std::isfinite(d.minimumSampleRateHz) || !std::isfinite(d.maximumSampleRateHz) || d.minimumSampleRateHz <= 0 || d.minimumSampleRateHz > d.maximumSampleRateHz ||
        !std::isfinite(d.inputMinimumV) || !std::isfinite(d.inputMaximumV) || d.inputMinimumV >= d.inputMaximumV) return "设备输入能力未知或无效";
    if (c.sampleRateHz < d.minimumSampleRateHz || c.sampleRateHz > d.maximumSampleRateHz || c.inputMinimumV < d.inputMinimumV || c.inputMaximumV > d.inputMaximumV)
        return "采样率或输入范围超出设备能力";
    if (!c.outputChannel.empty()) {
        if (!contains(d.outputChannels, c.outputChannel) || !contains(d.waveforms, c.waveform)) return "设备不支持所选输出通道或波形";
        if (!d.outputMinimumV || !d.outputMaximumV || !d.maximumOutputFrequencyHz || !std::isfinite(*d.outputMinimumV) || !std::isfinite(*d.outputMaximumV) ||
            !std::isfinite(*d.maximumOutputFrequencyHz) || *d.outputMinimumV >= *d.outputMaximumV || *d.maximumOutputFrequencyHz <= 0) return "设备输出能力尚未确认";
        if (c.outputOffsetV - c.outputAmplitudeV < *d.outputMinimumV || c.outputOffsetV + c.outputAmplitudeV > *d.outputMaximumV || c.outputFrequencyHz > *d.maximumOutputFrequencyHz ||
            (d.maximumOutputSampleRateHz && (!std::isfinite(*d.maximumOutputSampleRateHz) || *d.maximumOutputSampleRateHz <= 0 || c.sampleRateHz > *d.maximumOutputSampleRateHz)))
            return "输出参数超出设备能力";
    }
    return {};
}
std::string validateIdentity(const AcquisitionFrame& f) {
    if (f.runId.empty() || f.acquisitionId.empty() || f.deviceId.empty() || f.channelId.empty() || !f.configRevision || f.processingVersion.empty())
        return "完整批次缺少身份、配置修订或处理版本";
    if (!std::isfinite(f.elapsedSeconds) || f.elapsedSeconds < 0) return "批次单调时间无效";
    if (!f.batch.sampleCount || f.batch.firstBlockSequence > f.batch.lastBlockSequence ||
        f.batch.firstSampleIndex > std::numeric_limits<std::uint64_t>::max() - f.batch.sampleCount) return "完整批次样本范围无效";
    return {};
}
AcquisitionFrame normalizeFrame(AcquisitionFrame f) {
    for (const auto* c : {&f.raw, &f.observed, &f.fitted, &f.fv}) {
        if (c->x.size() != c->y.size() || (!c->pointValid.empty() && c->pointValid.size() != c->y.size()) || (!c->y.empty() && (c->xQuantity.empty() || c->xUnit.empty() || c->yQuantity.empty() || c->yUnit.empty()))) {
            f.quality = Quality::Invalid; f.qualityReason = "曲线坐标长度不一致或物理量/单位缺失"; break;
        }
    }
    for (const auto& diagnostic : f.processingDiagnostics) {
        if (diagnostic.stage.empty() || !std::isfinite(diagnostic.durationMs) || diagnostic.durationMs < 0) {
            f.quality = Quality::Invalid;
            f.qualityReason = "处理阶段诊断缺少名称或耗时无效";
            break;
        }
    }
    if (f.quality != Quality::Valid && f.qualityReason.empty()) f.qualityReason = f.quality == Quality::Uncalibrated ? "物理定义或标定未确认" : "批次无效";
    auto finite = [](std::optional<double>& v) { if (v && !std::isfinite(*v)) v.reset(); };
    finite(f.metrics.peakAbsorbance); finite(f.metrics.intensityPercent); finite(f.metrics.concentration);
    if (f.quality != Quality::Valid || f.metrics.quality == Quality::Invalid) {
        f.metrics.peakAbsorbance.reset(); f.metrics.intensityPercent.reset(); f.metrics.concentration.reset();
    }
    if (f.metrics.quality != Quality::Valid || f.metrics.concentrationUnit.empty() || f.metrics.calibrationId.empty() || f.metrics.gasSpecies.empty()) f.metrics.concentration.reset();
    return f;
}
PeakRecord extractPeak(const AcquisitionFrame& f) {
    PeakRecord p;
    p.runId = f.runId; p.acquisitionId = f.acquisitionId; p.sequence = f.sequence;
    p.acquiredAtUtcMs = f.acquiredAtUtcMs; p.elapsedSeconds = f.elapsedSeconds; p.fvUnit = f.fv.yUnit;
    p.qualityReason = f.qualityReason;
    p.quality = f.quality;
    if (f.quality != Quality::Valid || f.fv.x.size() != f.fv.y.size() || (!f.fv.pointValid.empty() && f.fv.pointValid.size() != f.fv.y.size())) {
        if (f.fv.x.size() != f.fv.y.size() || (!f.fv.pointValid.empty() && f.fv.pointValid.size() != f.fv.y.size())) p.quality = Quality::Invalid;
        if (p.qualityReason.empty()) p.qualityReason = "批次质量无效或坐标长度不一致";
        return p;
    }
    for (std::size_t i = 0; i < f.fv.y.size(); ++i) {
        if ((f.fv.pointValid.empty() || f.fv.pointValid[i]) && std::isfinite(f.fv.x[i]) && std::isfinite(f.fv.y[i]) && (!p.peakValue || f.fv.y[i] > *p.peakValue)) {
            p.peakValue = f.fv.y[i]; p.peakX = f.fv.x[i];
        }
    }
    p.quality = p.peakValue ? Quality::Valid : Quality::Invalid;
    if (!p.peakValue) p.qualityReason = "空批次或无有限有效点";
    return p;
}
}
