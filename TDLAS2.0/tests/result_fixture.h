#pragma once
#include "daq/sample_source.h"
#include <limits>

// Synthetic, dimensionless contract fixtures only. Never used by the application.
inline tdlas::AcquisitionFrame testFrame(std::uint64_t sequence, const std::string& run = "test-run") {
    tdlas::AcquisitionFrame f;
    f.runId = run; f.acquisitionId = "batch-" + std::to_string(sequence); f.sequence = sequence;
    f.deviceId = "test"; f.channelId = "test/input"; f.configRevision = 1;
    f.acquiredAtUtcMs = 1793088000000 + static_cast<std::int64_t>(sequence) * 1000;
    f.elapsedSeconds = static_cast<double>(sequence) * 0.25;
    f.batch = {sequence * 3, 3, sequence, sequence};
    f.quality = tdlas::Quality::Valid; f.processingVersion = "contract-test-only";
    f.fv = {{0, 1, 2}, {-3, -1, -2}, "sample index", "1", "test FV", "1"};
    f.observed = f.fv; f.fitted = f.fv; f.fitStatus = "success";
    f.metrics.peakAbsorbance = 0.25; f.metrics.intensityPercent = 120;
    return f;
}
class ResultTestSource final : public tdlas::ISampleSource {
public:
    explicit ResultTestSource(std::size_t count = 100) : count_(count) {}
    void open(const tdlas::AcquisitionConfig& config, const std::string& run) override { config_ = config; run_ = run; }
    std::shared_ptr<const tdlas::SampleBlock> read(const std::atomic_bool& stop) override {
        if (stop || next_ == count_) return {};
        auto b = std::make_shared<tdlas::SampleBlock>(); b->runId = run_; b->sequence = next_++;
        b->deviceId = config_.deviceId; b->channelId = config_.inputChannel; b->configRevision = config_.revision;
        b->sampleRateHz = config_.sampleRateHz; b->samples = {0, 1, 2}; return b;
    }
    void close() noexcept override {}
private:
    tdlas::AcquisitionConfig config_; std::string run_; std::size_t next_ = 0, count_;
};
inline tdlas::AcquisitionFrame testResult(const tdlas::SampleBlock& b) {
    auto f = testFrame(b.sequence, b.runId); f.deviceId = b.deviceId; f.channelId = b.channelId; f.configRevision = b.configRevision; return f;
}
