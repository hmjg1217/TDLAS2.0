#include "daq/daq_backend.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <thread>

#ifndef TDLAS_HAS_DAQMX
#define TDLAS_HAS_DAQMX 0
#endif

#if TDLAS_HAS_DAQMX
#include <NIDAQmx.h>
#endif

namespace tdlas {
namespace {

#if TDLAS_HAS_DAQMX

std::string extendedError() {
    char buffer[4096] = {};
    DAQmxGetExtendedErrorInfo(buffer, static_cast<uInt32>(sizeof(buffer)));
    return buffer;
}

[[noreturn]] void throwDaqError(const char* operation, int32 code) {
    throw std::runtime_error(std::string(operation) + " 失败，DAQmx 错误码 " + std::to_string(code) + "：" + extendedError());
}

void checkDaq(const char* operation, int32 code) {
    if (code < 0) throwDaqError(operation, code);
}

template <typename Reader>
std::string readString(const char* operation, Reader reader) {
    std::vector<char> buffer(32768, '\0');
    const auto code = reader(buffer.data(), static_cast<uInt32>(buffer.size()));
    checkDaq(operation, code);
    return buffer.data();
}

template <typename Reader>
std::vector<double> readDoubleArray(const char* operation, Reader reader) {
    const int32 required = reader(nullptr, 0);
    if (required < 0) throwDaqError(operation, required);
    if (required == 0) return {};
    std::vector<double> values(static_cast<std::size_t>(required));
    checkDaq(operation, reader(values.data(), static_cast<uInt32>(values.size())));
    return values;
}

std::vector<std::string> splitNames(const std::string& text) {
    std::vector<std::string> result;
    std::size_t start = 0;
    while (start < text.size()) {
        const auto comma = text.find(',', start);
        const auto end = comma == std::string::npos ? text.size() : comma;
        auto first = text.find_first_not_of(" \t\r\n", start);
        if (first != std::string::npos && first < end) {
            auto last = text.find_last_not_of(" \t\r\n", end - 1);
            result.push_back(text.substr(first, last - first + 1));
        }
        if (comma == std::string::npos) break;
        start = comma + 1;
    }
    return result;
}

std::pair<double, double> outerRange(const std::vector<double>& ranges) {
    if (ranges.size() < 2 || (ranges.size() % 2) != 0) throw std::runtime_error("设备返回的电压量程数组无效");
    double low = ranges[0], high = ranges[1];
    for (std::size_t i = 0; i < ranges.size(); i += 2) {
        if (!std::isfinite(ranges[i]) || !std::isfinite(ranges[i + 1]) || ranges[i] >= ranges[i + 1])
            throw std::runtime_error("设备返回的电压量程包含无效值");
        low = std::min(low, ranges[i]);
        high = std::max(high, ranges[i + 1]);
    }
    return {low, high};
}

std::vector<double> makeWaveform(const AcquisitionConfig& config) {
    const auto count = std::max<std::size_t>(2, static_cast<std::size_t>(std::llround(config.sampleRateHz / config.outputFrequencyHz)));
    std::vector<double> values(count);
    for (std::size_t i = 0; i < count; ++i) {
        const double phase = static_cast<double>(i) / static_cast<double>(count);
        double normalized = 0;
        switch (config.waveform) {
        case Waveform::Square: normalized = phase < 0.5 ? 1.0 : -1.0; break;
        case Waveform::Sawtooth: normalized = 2.0 * phase - 1.0; break;
        case Waveform::Triangle: normalized = phase < 0.5 ? 4.0 * phase - 1.0 : 3.0 - 4.0 * phase; break;
        case Waveform::Sine: normalized = std::sin(6.283185307179586 * phase); break;
        }
        values[i] = config.outputOffsetV + config.outputAmplitudeV * normalized;
    }
    return values;
}

class NiDaqSource final : public ISampleSource {
public:
    explicit NiDaqSource(DaqDeviceDescriptor device) : device_(std::move(device)) {}
    ~NiDaqSource() override { close(); }

    void open(const AcquisitionConfig& config, const std::string& runId) override {
        if (opened_) throw std::logic_error("NI-DAQmx 会话已打开");
        if (runId.empty()) throw std::invalid_argument("NI-DAQmx 会话标识不能为空");
        if (const auto error = validate(config, device_.capabilities); !error.empty()) throw std::invalid_argument(error);
        config_ = config;
        runId_ = runId;
        sequence_ = 0;
        sampleIndex_ = 0;
        started_ = std::chrono::steady_clock::now();
        try {
            if (!config.outputChannel.empty()) configureAo();
            configureAi();
            if (aoTask_) checkDaq("DAQmxStartTask(AO)", DAQmxStartTask(aoTask_));
            checkDaq("DAQmxStartTask(AI)", DAQmxStartTask(aiTask_));
            opened_ = true;
        } catch (...) {
            close();
            throw;
        }
    }

    std::shared_ptr<const SampleBlock> read(const std::atomic_bool& stop) override {
        if (!opened_) throw std::logic_error("NI-DAQmx 会话未打开");
        std::vector<double> samples(config_.readBlockSamples);
        int32 count = 0;
        while (!stop && count == 0) {
            const int32 code = DAQmxReadAnalogF64(aiTask_, static_cast<int32>(config_.readBlockSamples), 0.2,
                DAQmx_Val_GroupByChannel, samples.data(), static_cast<uInt32>(samples.size()), &count, nullptr);
            if (code == DAQmxErrorSamplesNotYetAvailable) continue;
            checkDaq("DAQmxReadAnalogF64", code);
        }
        if (stop) return {};
        if (count <= 0) return {};
        samples.resize(static_cast<std::size_t>(count));
        auto block = std::make_shared<SampleBlock>();
        block->runId = runId_;
        block->sequence = sequence_++;
        block->deviceId = config_.deviceId;
        block->channelId = config_.inputChannel;
        block->configRevision = config_.revision;
        block->sampleRateHz = config_.sampleRateHz;
        block->acquiredAtUtcMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        block->elapsedNs = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now() - started_).count();
        block->samples = std::move(samples);
        sampleIndex_ += block->samples.size();
        return block;
    }

    void close() noexcept override {
        if (aiTask_) {
            DAQmxStopTask(aiTask_);
            DAQmxClearTask(aiTask_);
            aiTask_ = nullptr;
        }
        if (aoTask_) {
            DAQmxStopTask(aoTask_);
            DAQmxClearTask(aoTask_);
            aoTask_ = nullptr;
        }
        opened_ = false;
    }

private:
    void configureAi() {
        checkDaq("DAQmxCreateTask(AI)", DAQmxCreateTask("", &aiTask_));
        checkDaq("DAQmxCreateAIVoltageChan", DAQmxCreateAIVoltageChan(aiTask_, config_.inputChannel.c_str(), "",
            DAQmx_Val_Cfg_Default, config_.inputMinimumV, config_.inputMaximumV, DAQmx_Val_Volts, nullptr));
        checkDaq("DAQmxCfgSampClkTiming(AI)", DAQmxCfgSampClkTiming(aiTask_, "", config_.sampleRateHz,
            DAQmx_Val_Rising, DAQmx_Val_ContSamps, static_cast<uInt64>(config_.readBlockSamples)));
    }

    void configureAo() {
        const auto waveform = makeWaveform(config_);
        checkDaq("DAQmxCreateTask(AO)", DAQmxCreateTask("", &aoTask_));
        checkDaq("DAQmxCreateAOVoltageChan", DAQmxCreateAOVoltageChan(aoTask_, config_.outputChannel.c_str(), "",
            *device_.capabilities.outputMinimumV, *device_.capabilities.outputMaximumV, DAQmx_Val_Volts, nullptr));
        checkDaq("DAQmxCfgSampClkTiming(AO)", DAQmxCfgSampClkTiming(aoTask_, "", config_.sampleRateHz,
            DAQmx_Val_Rising, DAQmx_Val_ContSamps, static_cast<uInt64>(waveform.size())));
        checkDaq("DAQmxSetWriteRegenMode", DAQmxSetWriteRegenMode(aoTask_, DAQmx_Val_AllowRegen));
        int32 written = 0;
        checkDaq("DAQmxWriteAnalogF64", DAQmxWriteAnalogF64(aoTask_, static_cast<int32>(waveform.size()), false, 2.0,
            DAQmx_Val_GroupByChannel, waveform.data(), &written, nullptr));
        if (written != static_cast<int32>(waveform.size())) throw std::runtime_error("NI-DAQmx AO 波形未完整写入");
    }

    DaqDeviceDescriptor device_;
    AcquisitionConfig config_;
    std::string runId_;
    TaskHandle aiTask_ = nullptr;
    TaskHandle aoTask_ = nullptr;
    bool opened_ = false;
    std::uint64_t sequence_ = 0;
    std::uint64_t sampleIndex_ = 0;
    std::chrono::steady_clock::time_point started_;
};

#endif // TDLAS_HAS_DAQMX

class NiDaqBackend final : public IDaqBackend {
public:
    std::string name() const override { return "NI-DAQmx"; }

    DaqDiscoveryResult discover() override {
#if !TDLAS_HAS_DAQMX
        return {{}, "当前构建未启用 NI-DAQmx；模拟数据源仍可使用"};
#else
        try {
            const auto names = splitNames(readString("DAQmxGetSysDevNames", [](char* data, uInt32 size) {
                return DAQmxGetSysDevNames(data, size);
            }));
            DaqDiscoveryResult result;
            for (const auto& id : names) {
                const auto product = readString("DAQmxGetDevProductType", [&](char* data, uInt32 size) {
                    return DAQmxGetDevProductType(id.c_str(), data, size);
                });
                const auto inputs = splitNames(readString("DAQmxGetDevAIPhysicalChans", [&](char* data, uInt32 size) {
                    return DAQmxGetDevAIPhysicalChans(id.c_str(), data, size);
                }));
                if (inputs.empty()) continue;
                double aiMinRate = 0, aiMaxRate = 0;
                checkDaq("DAQmxGetDevAIMinRate", DAQmxGetDevAIMinRate(id.c_str(), &aiMinRate));
                checkDaq("DAQmxGetDevAIMaxSingleChanRate", DAQmxGetDevAIMaxSingleChanRate(id.c_str(), &aiMaxRate));
                const auto inputRange = outerRange(readDoubleArray("DAQmxGetDevAIVoltageRngs", [&](double* data, uInt32 size) {
                    return DAQmxGetDevAIVoltageRngs(id.c_str(), data, size);
                }));
                DaqDeviceDescriptor device;
                device.id = id;
                device.productType = product;
                device.capabilities.deviceId = id;
                device.capabilities.inputChannels = inputs;
                device.capabilities.minimumSampleRateHz = aiMinRate;
                device.capabilities.maximumSampleRateHz = aiMaxRate;
                device.capabilities.inputMinimumV = inputRange.first;
                device.capabilities.inputMaximumV = inputRange.second;
                auto outputs = splitNames(readString("DAQmxGetDevAOPhysicalChans", [&](char* data, uInt32 size) {
                    return DAQmxGetDevAOPhysicalChans(id.c_str(), data, size);
                }));
                if (!outputs.empty()) {
                    bool32 sampleClock = false;
                    checkDaq("DAQmxGetDevAOSampClkSupported", DAQmxGetDevAOSampClkSupported(id.c_str(), &sampleClock));
                    if (sampleClock) {
                        double aoMaxRate = 0;
                        checkDaq("DAQmxGetDevAOMaxRate", DAQmxGetDevAOMaxRate(id.c_str(), &aoMaxRate));
                        const auto outputRange = outerRange(readDoubleArray("DAQmxGetDevAOVoltageRngs", [&](double* data, uInt32 size) {
                            return DAQmxGetDevAOVoltageRngs(id.c_str(), data, size);
                        }));
                        device.capabilities.outputChannels = std::move(outputs);
                        device.capabilities.waveforms = {Waveform::Sine, Waveform::Square, Waveform::Sawtooth, Waveform::Triangle};
                        device.capabilities.outputMinimumV = outputRange.first;
                        device.capabilities.outputMaximumV = outputRange.second;
                        device.capabilities.maximumOutputSampleRateHz = aoMaxRate;
                        device.capabilities.maximumOutputFrequencyHz = aoMaxRate / 10.0;
                    }
                }
                if (!std::isfinite(aiMinRate) || !std::isfinite(aiMaxRate) || aiMinRate <= 0 || aiMinRate > aiMaxRate)
                    throw std::runtime_error("设备 " + id + " 返回的 AI 采样率能力无效");
                result.devices.push_back(std::move(device));
            }
            if (result.devices.empty()) result.error = "NI-DAQmx 未返回具有可用 AI 通道的设备";
            return result;
        } catch (const std::exception& error) {
            return {{}, error.what()};
        }
#endif
    }

    std::unique_ptr<ISampleSource> makeSource(const DaqDeviceDescriptor& descriptor) override {
#if TDLAS_HAS_DAQMX
        return std::make_unique<NiDaqSource>(descriptor);
#else
        (void)descriptor;
        throw std::runtime_error("当前构建未启用 NI-DAQmx");
#endif
    }
};

} // namespace

bool niDaqmxCompiled() noexcept { return TDLAS_HAS_DAQMX != 0; }
std::shared_ptr<IDaqBackend> makeNiDaqBackend() { return std::make_shared<NiDaqBackend>(); }

} // namespace tdlas
