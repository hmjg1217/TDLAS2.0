#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace tdlas {
enum class Waveform { Sine, Square, Sawtooth, Triangle };
struct AcquisitionConfig {
    std::string deviceId;
    std::string inputChannel;
    std::string outputChannel; // Empty means AI-only.
    Waveform waveform = Waveform::Sine;
    double sampleRateHz = 10000.0;
    double outputFrequencyHz = 10.0;
    double outputAmplitudeV = 1.0; // Peak, not peak-to-peak.
    double outputOffsetV = 0.0;
    double inputMinimumV = -10.0;
    double inputMaximumV = 10.0;
    std::size_t displayWindowSamples = 2000;
    std::size_t readBlockSamples = 1000;
    std::size_t processingWindowSamples = 100000;
    std::optional<std::size_t> scanPeriodSamples; // Unknown until configured.
    std::size_t queueCapacitySamples = 100000;
    std::uint64_t revision = 1;
};
// Structural checks only; device capability validation belongs to a backend.
std::string validate(const AcquisitionConfig& config);
struct DeviceCapabilities {
    std::string deviceId;
    std::vector<std::string> inputChannels, outputChannels;
    std::vector<Waveform> waveforms;
    double minimumSampleRateHz = 0, maximumSampleRateHz = 0;
    double inputMinimumV = 0, inputMaximumV = 0;
    // Absent means unknown, never assume an AO range from a device name.
    std::optional<double> outputMinimumV, outputMaximumV, maximumOutputFrequencyHz;
    std::optional<double> maximumOutputSampleRateHz;
};
std::string validate(const AcquisitionConfig&, const DeviceCapabilities&);
struct SampleBlock {
    std::string runId;
    std::uint64_t sequence = 0;
    std::int64_t acquiredAtUtcMs = 0;
    std::int64_t elapsedNs = 0;
    std::string deviceId;
    std::string channelId;
    std::uint64_t configRevision = 0;
    double sampleRateHz = 0;
    std::vector<double> samples;
};
enum class Quality { Valid, Invalid, Uncalibrated };
struct Curve {
    std::vector<double> x, y;
    std::string xQuantity, xUnit, yQuantity, yUnit;
    std::vector<bool> pointValid; // Empty: finite points eligible; otherwise one flag per point.
};
struct Metrics {
    std::optional<double> peakAbsorbance, intensityPercent, concentration;
    std::string concentrationUnit, calibrationId, gasSpecies;
    Quality quality = Quality::Uncalibrated;
};
// An adapter explicitly supplies the complete batch's original sample extent.
// Neither UI refresh nor displayWindowSamples determines this boundary.
struct BatchExtent {
    std::uint64_t firstSampleIndex = 0, sampleCount = 0;
    std::uint64_t firstBlockSequence = 0, lastBlockSequence = 0;
};
struct ProcessingStageDiagnostic {
    std::string stage;
    bool succeeded = false;
    std::string message;
    double durationMs = 0;
};
struct AcquisitionFrame {
    std::string runId, acquisitionId, deviceId, channelId;
    std::uint64_t sequence = 0, configRevision = 0;
    std::int64_t acquiredAtUtcMs = 0;
    double elapsedSeconds = 0;
    BatchExtent batch;
    Quality quality = Quality::Invalid;
    std::string qualityReason;
    Curve raw, observed, fitted, fv;
    std::string processingVersion, baselineMethod, polarityTransform, modelId, fitStatus;
    // Ordered, bounded stage summaries for one complete batch. They are not a
    // per-sample log and therefore do not grow after the frame is published.
    std::vector<ProcessingStageDiagnostic> processingDiagnostics;
    Metrics metrics;
};
// Invalid curve contents are retained as a null batch; invalid identity/time
// cannot be ordered reliably and are rejected at the result boundary.
std::string validateIdentity(const AcquisitionFrame&);
AcquisitionFrame normalizeFrame(AcquisitionFrame frame);
struct PeakRecord {
    std::string runId, acquisitionId, fvUnit, qualityReason;
    std::uint64_t sequence = 0;
    std::int64_t acquiredAtUtcMs = 0;
    double elapsedSeconds = 0;
    std::optional<double> peakValue, peakX;
    Quality quality = Quality::Invalid;
};
PeakRecord extractPeak(const AcquisitionFrame& frame);
} // namespace tdlas
