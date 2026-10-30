#include "daq/daq_backend.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <stdexcept>
#include <thread>

namespace tdlas {
namespace {

class FakeDaqSource final : public ISampleSource {
public:
    FakeDaqSource(DaqDeviceDescriptor device, FakeDaqOptions options,
                  std::shared_ptr<FakeDaqObservation> observation)
        : device_(std::move(device)), options_(std::move(options)), observation_(std::move(observation)) {}

    void open(const AcquisitionConfig& config, const std::string& runId) override {
        ++observation_->openCalls;
        if (opened_) throw std::logic_error("Fake DAQ 会话已打开");
        if (runId.empty()) throw std::invalid_argument("Fake DAQ 会话标识不能为空");
        if (const auto error = validate(config, device_.capabilities); !error.empty()) throw std::invalid_argument(error);
        if (options_.readDelayMs < 0 || options_.readDelayMs > 10000) throw std::invalid_argument("Fake DAQ 读取延迟必须在 0..10000 ms 内");
        if (options_.failOnOpen) throw std::runtime_error("Fake DAQ 故障注入：启动失败");
        config_ = config;
        runId_ = runId;
        sequence_ = 0;
        sampleIndex_ = 0;
        started_ = std::chrono::steady_clock::now();
        opened_ = true;
        observation_->taskOpen = true;
        ++observation_->aiStarts;
        if (!config.outputChannel.empty()) ++observation_->aoStarts;
    }

    std::shared_ptr<const SampleBlock> read(const std::atomic_bool& stop) override {
        if (!opened_) throw std::logic_error("Fake DAQ 会话未打开");
        const auto due = std::chrono::steady_clock::now() + std::chrono::milliseconds(options_.readDelayMs);
        while (!stop && std::chrono::steady_clock::now() < due) std::this_thread::sleep_for(std::chrono::milliseconds(2));
        if (stop) return {};
        if (options_.disconnectAfterReads && sequence_ >= *options_.disconnectAfterReads)
            throw std::runtime_error("Fake DAQ 故障注入：设备断开");
        ++observation_->readCalls;
        auto block = std::make_shared<SampleBlock>();
        block->runId = runId_;
        block->sequence = sequence_;
        block->deviceId = config_.deviceId;
        block->channelId = config_.inputChannel;
        block->configRevision = config_.revision;
        block->sampleRateHz = config_.sampleRateHz;
        block->acquiredAtUtcMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        block->elapsedNs = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now() - started_).count();
        block->samples.reserve(config_.readBlockSamples);
        for (std::size_t i = 0; i < config_.readBlockSamples; ++i) {
            if ((i % 4096) == 0 && stop) return {};
            const double t = static_cast<double>(sampleIndex_ + i) / config_.sampleRateHz;
            block->samples.push_back(0.25 * std::sin(6.283185307179586 * 7.0 * t));
        }
        ++sequence_;
        sampleIndex_ += config_.readBlockSamples;
        return block;
    }

    void close() noexcept override {
        if (closeCalled_) return;
        closeCalled_ = true;
        opened_ = false;
        observation_->taskOpen = false;
        ++observation_->closeCalls;
    }

private:
    DaqDeviceDescriptor device_;
    FakeDaqOptions options_;
    std::shared_ptr<FakeDaqObservation> observation_;
    AcquisitionConfig config_;
    std::string runId_;
    bool opened_ = false;
    bool closeCalled_ = false;
    std::uint64_t sequence_ = 0;
    std::uint64_t sampleIndex_ = 0;
    std::chrono::steady_clock::time_point started_;
};

class FakeDaqBackend final : public IDaqBackend {
public:
    FakeDaqBackend(FakeDaqOptions options, std::shared_ptr<FakeDaqObservation> observation)
        : options_(std::move(options)), observation_(std::move(observation)) {
        if (options_.devices.empty()) options_.devices.push_back(defaultFakeDaqDevice());
    }

    std::string name() const override { return "Fake DAQ"; }

    DaqDiscoveryResult discover() override {
        ++observation_->discoveryCalls;
        if (options_.discoveryDelayMs < 0 || options_.discoveryDelayMs > 10000)
            return {{}, "Fake DAQ 发现延迟配置无效"};
        std::this_thread::sleep_for(std::chrono::milliseconds(options_.discoveryDelayMs));
        if (options_.failDiscovery) return {{}, "Fake DAQ 故障注入：设备发现失败"};
        return {options_.devices, {}};
    }

    std::unique_ptr<ISampleSource> makeSource(const DaqDeviceDescriptor& descriptor) override {
        const auto found = std::find_if(options_.devices.begin(), options_.devices.end(), [&](const auto& candidate) {
            return candidate.id == descriptor.id;
        });
        if (found == options_.devices.end()) throw std::invalid_argument("Fake DAQ 设备不在当前发现快照中");
        return std::make_unique<FakeDaqSource>(*found, options_, observation_);
    }

private:
    FakeDaqOptions options_;
    std::shared_ptr<FakeDaqObservation> observation_;
};

} // namespace

DaqDeviceDescriptor defaultFakeDaqDevice() {
    DaqDeviceDescriptor device;
    device.id = "FakeDev1";
    device.productType = "Fake multifunction DAQ";
    device.capabilities.deviceId = device.id;
    device.capabilities.inputChannels = {"FakeDev1/ai0", "FakeDev1/ai1"};
    device.capabilities.outputChannels = {"FakeDev1/ao0"};
    device.capabilities.waveforms = {Waveform::Sine, Waveform::Square, Waveform::Sawtooth, Waveform::Triangle};
    device.capabilities.minimumSampleRateHz = 1.0;
    device.capabilities.maximumSampleRateHz = 100000.0;
    device.capabilities.inputMinimumV = -10.0;
    device.capabilities.inputMaximumV = 10.0;
    device.capabilities.outputMinimumV = -10.0;
    device.capabilities.outputMaximumV = 10.0;
    device.capabilities.maximumOutputFrequencyHz = 10000.0;
    device.capabilities.maximumOutputSampleRateHz = 100000.0;
    return device;
}

std::shared_ptr<IDaqBackend> makeFakeDaqBackend(FakeDaqOptions options,
                                                std::shared_ptr<FakeDaqObservation> observation) {
    if (!observation) observation = std::make_shared<FakeDaqObservation>();
    return std::make_shared<FakeDaqBackend>(std::move(options), std::move(observation));
}

} // namespace tdlas
