#include "daq/sample_source.h"
#include <chrono>
#include <algorithm>
#include <cmath>
#include <thread>
#include <stdexcept>

namespace tdlas {
namespace {
class SimulationSource final : public ISampleSource {
    SimulationOptions options_;
    bool opened_ = false;
    AcquisitionConfig config_;
    std::string runId_;
    std::uint64_t sequence_ = 0, sample_ = 0;
    std::chrono::steady_clock::time_point started_, due_;
public:
    explicit SimulationSource(SimulationOptions options) : options_(options) {}
    void open(const AcquisitionConfig& config, const std::string& runId) override {
        if (opened_) throw std::logic_error("模拟源已打开");
        if (auto error = validate(config, simulationCapabilities()); !error.empty()) throw std::invalid_argument(error);
        if (runId.empty()) throw std::invalid_argument("模拟会话标识不能为空");
        if (options_.readDelayMs < 0 || options_.readDelayMs > 10000) throw std::invalid_argument("模拟读取延迟必须在 0..10000 ms 内");
        if (options_.failOnOpen) throw std::runtime_error("模拟故障注入：打开失败（未访问硬件）");
        config_ = config; runId_ = runId; sequence_ = sample_ = 0;
        started_ = due_ = std::chrono::steady_clock::now();
        opened_ = true;
    }
    std::shared_ptr<const SampleBlock> read(const std::atomic_bool& stop) override {
        if (!opened_) throw std::logic_error("模拟源尚未打开");
        const auto ready = std::max(due_, std::chrono::steady_clock::now() + std::chrono::milliseconds(options_.readDelayMs));
        while (!stop && std::chrono::steady_clock::now() < ready) std::this_thread::sleep_for(std::chrono::milliseconds(5));
        if (stop) return {};
        if (options_.failAfterBlocks && sequence_ >= *options_.failAfterBlocks)
            throw std::runtime_error("模拟故障注入：读取失败（未访问硬件）");
        auto b = std::make_shared<SampleBlock>();
        b->runId = runId_; b->sequence = sequence_; b->sampleRateHz = config_.sampleRateHz;
        b->deviceId = config_.deviceId; b->channelId = config_.inputChannel; b->configRevision = config_.revision;
        b->acquiredAtUtcMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        b->elapsedNs = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - started_).count();
        b->samples.reserve(config_.readBlockSamples);
        for (std::size_t i = 0; i < config_.readBlockSamples; ++i) {
            if (i % 4096 == 0 && stop) return {};
            const double time = static_cast<double>(sample_ + i) / config_.sampleRateHz;
            b->samples.push_back(0.7 * std::sin(2 * 3.141592653589793 * 10 * time));
        }
        ++sequence_; sample_ += config_.readBlockSamples;
        due_ += std::chrono::duration_cast<std::chrono::steady_clock::duration>(
            std::chrono::duration<double>(config_.readBlockSamples / config_.sampleRateHz));
        return b;
    }
    void close() noexcept override { opened_ = false; }
};
}
DeviceCapabilities simulationCapabilities() {
    DeviceCapabilities c; c.deviceId = "simulation"; c.inputChannels = {"simulation/voltage"};
    c.minimumSampleRateHz = 1; c.maximumSampleRateHz = 1000000;
    c.inputMinimumV = -10; c.inputMaximumV = 10;
    return c; // No AO channels or inferred hardware capability.
}
std::unique_ptr<ISampleSource> makeSimulationSource(SimulationOptions options) { return std::make_unique<SimulationSource>(options); }
}
