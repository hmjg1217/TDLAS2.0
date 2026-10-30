#include "processing/tdlas_processor.h"
#include "runtime/acquisition_runtime.h"

#include <QtTest>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <limits>
#include <memory>

using namespace tdlas;

namespace {

LegacyTdlasConfig replayConfig() {
    LegacyTdlasConfig c;
    c.samplesPerCycle = 200;
    c.averagedCyclesPerSpectrum = 4;
    c.averagedSpectra = 1;
    c.windowSamples = 800;
    return c;
}

std::vector<double> replayWindow(const LegacyTdlasConfig& c = replayConfig()) {
    std::vector<double> values;
    values.reserve(c.windowSamples);
    for (std::size_t spectrum = 0; spectrum < c.averagedSpectra; ++spectrum) {
        for (std::size_t cycle = 0; cycle < c.averagedCyclesPerSpectrum; ++cycle) {
            for (std::size_t sample = 0; sample < c.samplesPerCycle; ++sample) {
                const double phase = static_cast<double>(sample) / static_cast<double>(c.samplesPerCycle - 1);
                const double dip = 0.4 * std::exp(-0.5 * std::pow((phase - 0.58) / 0.065, 2));
                const double alignmentMarker = sample == 0 ? 0.02 : 0.0;
                values.push_back(1.05 - dip + alignmentMarker + 0.0002 * static_cast<double>(cycle));
            }
        }
    }
    return values;
}

SampleBlock block(std::uint64_t sequence, std::vector<double> samples, std::size_t firstSample = 0) {
    SampleBlock b;
    b.runId = "replay-run"; b.sequence = sequence; b.deviceId = "replay"; b.channelId = "replay/ai0";
    b.configRevision = 7; b.sampleRateHz = 1000;
    b.acquiredAtUtcMs = 1793088000000 + static_cast<std::int64_t>(firstSample);
    b.elapsedNs = static_cast<std::int64_t>(firstSample) * 1000000;
    b.samples = std::move(samples);
    return b;
}

class ReplaySource final : public ISampleSource {
public:
    explicit ReplaySource(std::vector<double> samples) : samples_(std::move(samples)) {}
    void open(const AcquisitionConfig& config, const std::string& runId) override {
        config_ = config; runId_ = runId;
    }
    std::shared_ptr<const SampleBlock> read(const std::atomic_bool& stop) override {
        if (stop || offset_ == samples_.size()) return {};
        const auto count = std::min<std::size_t>(config_.readBlockSamples, samples_.size() - offset_);
        auto result = std::make_shared<SampleBlock>();
        result->runId = runId_; result->sequence = sequence_++; result->deviceId = config_.deviceId;
        result->channelId = config_.inputChannel; result->configRevision = config_.revision;
        result->sampleRateHz = config_.sampleRateHz;
        result->acquiredAtUtcMs = 1793088000000 + static_cast<std::int64_t>(offset_);
        result->elapsedNs = static_cast<std::int64_t>(offset_) * 1000000;
        result->samples.assign(samples_.begin() + static_cast<std::ptrdiff_t>(offset_),
                               samples_.begin() + static_cast<std::ptrdiff_t>(offset_ + count));
        offset_ += count;
        return result;
    }
    void close() noexcept override {}
private:
    AcquisitionConfig config_;
    std::string runId_;
    std::vector<double> samples_;
    std::size_t offset_ = 0;
    std::uint64_t sequence_ = 0;
};

} // namespace

class ProcessingTest : public QObject {
    Q_OBJECT
private slots:
    void configurationBoundary() {
        auto c = replayConfig(); QVERIFY(validate(c).empty());
        c.windowSamples = 799; QVERIFY(!validate(c).empty());
        c = replayConfig(); c.samplesPerCycle = 0; QVERIFY(!validate(c).empty());
        c = replayConfig(); c.voltageOffsetV = std::numeric_limits<double>::infinity(); QVERIFY(!validate(c).empty());
        c = replayConfig(); c.windowSamples = 10000001; QVERIFY(!validate(c).empty());
        QVERIFY_THROWS_EXCEPTION(std::invalid_argument, LegacyTdlasProcessor{c});
    }

    void reviewedLegacyReferenceReplay() {
        LegacyTdlasProcessor processor(replayConfig());
        const auto result = processor.processWindow(replayWindow());
        QVERIFY2(result.succeeded, result.message.c_str());
        QCOMPARE(result.absorbance.size(), std::size_t(122));
        QCOMPARE(result.fittedAbsorbance.size(), result.absorbance.size());
        QCOMPARE(result.legacyFrequencyOffset.size(), result.absorbance.size());
        QVERIFY(result.legacyFittedPeak); QVERIFY(result.legacyIntensityPercent);
        // Frozen by compiling the reviewed v1.6 source recorded in
        // test-data/legacy-v1.6-reference.json against this replay generator.
        const auto observedPeak = std::max_element(result.absorbance.begin(), result.absorbance.end());
        QVERIFY(std::abs(result.absorbance.front() - (-0.021487667374973665)) < 1e-12);
        QVERIFY(std::abs(result.absorbance[result.absorbance.size() / 2] - (-0.015674740603855326)) < 1e-12);
        QVERIFY(std::abs(result.absorbance.back() - 0.0051944692415708477) < 1e-12);
        QVERIFY(std::abs(*observedPeak - 0.017193038676769822) < 1e-12);
        QCOMPARE(std::distance(result.absorbance.begin(), observedPeak), std::ptrdiff_t(76));
        QCOMPARE(*result.legacyFittedPeak, 0.0);
        QVERIFY(std::abs(*result.legacyIntensityPercent - 1.3482829999516444) < 1e-12);
        QCOMPARE(result.diagnostics.front().stage, std::string("input"));
        QCOMPARE(result.diagnostics.back().stage, std::string("voigt_fit"));
        QCOMPARE(result.diagnostics.size(), std::size_t(6));
        for (const auto& stage : result.diagnostics) { QVERIFY(stage.succeeded); QVERIFY(stage.durationMs >= 0); }

        auto spectraConfig = replayConfig(); spectraConfig.averagedCyclesPerSpectrum = 1;
        spectraConfig.averagedSpectra = 4;
        const auto spectraResult = LegacyTdlasProcessor(spectraConfig).processWindow(replayWindow(spectraConfig));
        QVERIFY(spectraResult.succeeded); QCOMPARE(spectraResult.diagnostics.size(), std::size_t(6));
    }

    void stageFailuresAreDiagnostic() {
        auto input = replayWindow(); input[10] = std::numeric_limits<double>::quiet_NaN();
        auto failed = LegacyTdlasProcessor(replayConfig()).processWindow(input);
        QVERIFY(!failed.succeeded); QCOMPARE(failed.failureStage, std::string("low_pass"));
        QVERIFY(!failed.diagnostics.back().succeeded); QVERIFY(!failed.message.empty());

        LegacyTdlasConfig tiny; tiny.samplesPerCycle = 5; tiny.averagedCyclesPerSpectrum = 1;
        tiny.averagedSpectra = 1; tiny.windowSamples = 5;
        failed = LegacyTdlasProcessor(tiny).processWindow({1, 0.9, 0.8, 0.9, 1});
        QVERIFY(!failed.succeeded); QCOMPARE(failed.failureStage, std::string("baseline"));

        const auto c = replayConfig();
        failed = LegacyTdlasProcessor(c).processWindow(std::vector<double>(c.windowSamples, 0));
        QVERIFY(!failed.succeeded); QCOMPARE(failed.failureStage, std::string("voigt_fit"));
    }

    void adapterBatchesAcrossArbitraryBlocksAndStaysUncalibrated() {
        auto samples = replayWindow(); const auto second = samples;
        samples.insert(samples.end(), second.begin(), second.end());
        samples.insert(samples.end(), 37, 0.5);
        LegacyTdlasAdapter adapter(replayConfig());
        std::vector<AcquisitionFrame> frames;
        std::size_t offset = 0; std::uint64_t sequence = 0;
        while (offset < samples.size()) {
            const auto count = std::min<std::size_t>(300, samples.size() - offset);
            std::vector<double> part(samples.begin() + static_cast<std::ptrdiff_t>(offset),
                                     samples.begin() + static_cast<std::ptrdiff_t>(offset + count));
            auto produced = adapter.process(block(sequence++, std::move(part), offset));
            frames.insert(frames.end(), produced.begin(), produced.end()); offset += count;
        }
        QCOMPARE(frames.size(), std::size_t(2)); QCOMPARE(adapter.bufferedSamples(), std::size_t(37));
        QCOMPARE(frames[0].batch.firstSampleIndex, std::uint64_t(0));
        QCOMPARE(frames[0].batch.firstBlockSequence, std::uint64_t(0));
        QCOMPARE(frames[0].batch.lastBlockSequence, std::uint64_t(2));
        QCOMPARE(frames[1].batch.firstSampleIndex, std::uint64_t(800));
        QCOMPARE(frames[1].batch.firstBlockSequence, std::uint64_t(2));
        QCOMPARE(frames[1].batch.lastBlockSequence, std::uint64_t(5));
        for (const auto& frame : frames) {
            QCOMPARE(frame.quality, Quality::Uncalibrated); QCOMPARE(frame.metrics.quality, Quality::Uncalibrated);
            QVERIFY(frame.fv.x.empty()); QVERIFY(frame.fv.y.empty()); QVERIFY(!frame.observed.y.empty());
            QVERIFY(!frame.qualityReason.empty()); QCOMPARE(frame.fitStatus, std::string("success_uncalibrated"));
            QVERIFY(!frame.processingDiagnostics.empty()); QVERIFY(!frame.metrics.peakAbsorbance);
            QVERIFY(!frame.metrics.intensityPercent); QVERIFY(!frame.metrics.concentration);
        }
        auto discontinuous = block(sequence + 1, {1}, offset);
        QVERIFY_THROWS_EXCEPTION(std::invalid_argument, adapter.process(discontinuous));
    }

    void runtimeAcceptsReplayWithoutInventingMeasurements() {
        AcquisitionRuntime runtime;
        AcquisitionConfig acquisition; acquisition.deviceId = "replay"; acquisition.inputChannel = "replay/ai0";
        acquisition.sampleRateHz = 1000; acquisition.readBlockSamples = 300; acquisition.queueCapacitySamples = 2400;
        auto adapter = std::make_shared<LegacyTdlasAdapter>(replayConfig());
        QVERIFY(runtime.startWithResults(acquisition, std::make_unique<ReplaySource>(replayWindow()),
            [adapter](const SampleBlock& input) { return adapter->process(input); }));
        QTRY_VERIFY_WITH_TIMEOUT(!runtime.active(), 3000);
        QCOMPARE(runtime.state(), RunState::Stopped);
        const auto snapshot = runtime.results().snapshot(); QCOMPARE(snapshot.entries.size(), std::size_t(1));
        const auto& frame = *snapshot.entries.front().frame;
        QCOMPARE(frame.quality, Quality::Uncalibrated); QVERIFY(frame.fv.y.empty());
        QVERIFY(!snapshot.entries.front().peak.peakValue); QVERIFY(!frame.metrics.peakAbsorbance);
        QVERIFY(runtime.snapshot().stats.terminalComplete()); QVERIFY(runtime.error().isEmpty());
    }
};

QTEST_GUILESS_MAIN(ProcessingTest)
#include "processing_test.moc"
