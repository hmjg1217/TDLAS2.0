#pragma once

#include "core/contracts.h"

#include <cstddef>
#include <cstdint>
#include <deque>
#include <optional>
#include <string>
#include <vector>

namespace tdlas {

// These values reproduce the reviewed v1.6 C++ path. They are algorithm
// assumptions, not confirmed physical calibration. The adapter never exposes
// them as a valid FV, intensity or concentration measurement.
struct LegacyTdlasConfig {
    std::size_t windowSamples = 10000;
    std::size_t samplesPerCycle = 500;
    std::size_t averagedCyclesPerSpectrum = 20;
    std::size_t averagedSpectra = 1;
    double voltageOffsetV = 4.2;
};

std::string validate(const LegacyTdlasConfig& config);

struct LegacyTdlasSpectrum {
    bool succeeded = false;
    std::string failureStage;
    std::string message;
    std::size_t sampleOffset = 0;
    std::size_t cycleSamples = 0;
    std::vector<double> legacyFrequencyOffset;
    std::vector<double> absorbance;
    std::vector<double> fittedAbsorbance;
    std::vector<double> voltage;
    std::vector<double> fittedVoltage;
    std::optional<double> legacyFittedPeak;
    std::optional<double> legacyIntensityPercent;
    std::vector<ProcessingStageDiagnostic> diagnostics;
};

class LegacyTdlasProcessor final {
public:
    explicit LegacyTdlasProcessor(LegacyTdlasConfig config = {});
    const LegacyTdlasConfig& config() const noexcept { return config_; }
    LegacyTdlasSpectrum processWindow(const std::vector<double>& samples) const;
private:
    LegacyTdlasConfig config_;
};

// Stateful bridge from arbitrary source blocks to complete, ordered frames.
// One instance belongs to exactly one processing thread and one run.
class LegacyTdlasAdapter final {
public:
    explicit LegacyTdlasAdapter(LegacyTdlasConfig config);
    std::vector<AcquisitionFrame> process(const SampleBlock& block);
    std::size_t bufferedSamples() const noexcept { return samples_.size(); }
private:
    struct BufferedPart {
        std::uint64_t blockSequence = 0;
        std::int64_t acquiredAtUtcMs = 0;
        std::int64_t elapsedNs = 0;
        std::size_t offset = 0;
        std::size_t remaining = 0;
    };
    AcquisitionFrame makeFrame(const SampleBlock& identity, const LegacyTdlasSpectrum& spectrum);
    void consume(std::size_t count);

    LegacyTdlasProcessor processor_;
    std::vector<double> samples_;
    std::deque<BufferedPart> parts_;
    std::string runId_, deviceId_, channelId_;
    std::uint64_t configRevision_ = 0;
    std::uint64_t nextBlockSequence_ = 0;
    std::uint64_t nextBatchSequence_ = 0;
    std::uint64_t firstSampleIndex_ = 0;
    double sampleRateHz_ = 0;
    bool initialized_ = false;
};

} // namespace tdlas
