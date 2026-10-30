#include "runtime/acquisition_runtime.h"
#include <QtTest>
#include <QFile>
#include <QJsonDocument>
#include <QTemporaryDir>
#include <chrono>
#include <thread>
#include "result_fixture.h"
using namespace tdlas;
namespace {
AcquisitionConfig configuration() { AcquisitionConfig c; c.deviceId = "simulation"; c.inputChannel = "simulation/voltage"; return c; }
struct AtomicReleaseGuard {
    std::atomic_bool& value;
    ~AtomicReleaseGuard() { value = true; }
};
class BurstSource : public ISampleSource {
public:
    explicit BurstSource(std::atomic_bool& closed, bool failOpen = false, int blocks = 100, std::size_t samples = 1000)
        : closed_(closed), fail_(failOpen), blocks_(blocks), samples_(samples) {}
    void open(const AcquisitionConfig& config, const std::string& runId) override {
        if (fail_) throw std::runtime_error("injected open failure");
        config_ = config; runId_ = runId;
    }
    std::shared_ptr<const SampleBlock> read(const std::atomic_bool& stop) override {
        if (stop || count_ == blocks_) return {};
        auto b = std::make_shared<SampleBlock>(); b->runId = runId_; b->sequence = count_++;
        b->deviceId = config_.deviceId; b->channelId = config_.inputChannel; b->configRevision = config_.revision;
        b->samples.resize(samples_, 0.5); b->sampleRateHz = config_.sampleRateHz; return b;
    }
    void close() noexcept override { closed_ = true; }
private:
    std::atomic_bool& closed_; bool fail_; int blocks_; std::size_t samples_; int count_ = 0;
    AcquisitionConfig config_; std::string runId_;
};
}
class RuntimeTest : public QObject {
    Q_OBJECT
private slots:
    void startingCancellationAndReentry() {
        AcquisitionRuntime runtime; QSignalSpy stopped(&runtime, &AcquisitionRuntime::stopped);
        QSignalSpy states(&runtime, &AcquisitionRuntime::stateChanged);
        bool rejectedStart = false, rejectedReset = false;
        connect(&runtime, &AcquisitionRuntime::stateChanged, this, [&](RunState state) {
            if (state != RunState::Starting) return;
            rejectedStart = !runtime.start(configuration(), makeSimulationSource(), [](const SampleBlock&) {});
            rejectedReset = !runtime.reset(); runtime.requestStop(); runtime.requestStop();
        });
        QVERIFY(runtime.start(configuration(), makeSimulationSource(), [](const SampleBlock&) {}));
        QTRY_VERIFY_WITH_TIMEOUT(!runtime.active(), 2000);
        QVERIFY(rejectedStart); QVERIFY(rejectedReset); QCOMPARE(stopped.count(), 1);
        QCOMPARE(runtime.state(), RunState::Stopped); QCOMPARE(runtime.snapshot().stats.acceptedBlocks, std::uint64_t(0));
        for (const auto& arguments : states) QVERIFY(qvariant_cast<RunState>(arguments[0]) != RunState::Running);
        runtime.requestStop(); QCOMPARE(stopped.count(), 1);
    }
    void simulationReadFaultAndRecovery() {
        AcquisitionRuntime runtime; SimulationOptions options; options.failAfterBlocks = 3;
        QSignalSpy faults(&runtime, &AcquisitionRuntime::faultOccurred);
        QVERIFY(runtime.start(configuration(), makeSimulationSource(options), [](const SampleBlock&) {}));
        QTRY_VERIFY_WITH_TIMEOUT(!runtime.active(), 2000);
        QCOMPARE(runtime.state(), RunState::Error); QCOMPARE(faults.count(), 1);
        QVERIFY(runtime.error().contains("读取失败"));
        const auto before = runtime.snapshot(); QCOMPARE(before.stats.acceptedBlocks, std::uint64_t(3));
        QCOMPARE(before.stats.processedBlocks, before.stats.acceptedBlocks);
        QVERIFY(!before.samples.empty()); QTest::qWait(60); QCOMPARE(runtime.snapshot().samples, before.samples);
        QVERIFY(runtime.start(configuration(), makeSimulationSource(), [](const SampleBlock&) {}));
        QTRY_VERIFY_WITH_TIMEOUT(runtime.snapshot().stats.processedBlocks > 0, 2000);
        runtime.requestStop(); QTRY_VERIFY_WITH_TIMEOUT(!runtime.active(), 2000);
        QCOMPARE(runtime.state(), RunState::Stopped); QVERIFY(runtime.error().isEmpty());
        QVERIFY(runtime.snapshot().runId != before.runId);
        QVERIFY(runtime.reset()); QCOMPARE(runtime.state(), RunState::Ready); QVERIFY(runtime.snapshot().runId.empty());
    }
    void invalidRestartPreservesData() {
        AcquisitionRuntime runtime; SimulationOptions options; options.failAfterBlocks = 1;
        QVERIFY(runtime.start(configuration(), makeSimulationSource(options), [](const SampleBlock&) {}));
        QTRY_VERIFY_WITH_TIMEOUT(!runtime.active(), 2000);
        const auto before = runtime.snapshot(); auto invalid = configuration(); invalid.sampleRateHz = 0;
        QVERIFY(!runtime.start(invalid, makeSimulationSource(), [](const SampleBlock&) {}));
        QCOMPARE(runtime.snapshot().samples, before.samples); QCOMPARE(runtime.snapshot().runId, before.runId);
        QVERIFY(!runtime.active()); QVERIFY(runtime.reset()); QVERIFY(runtime.error().isEmpty());
    }
    void completeResultsWithoutUiPolling() {
        AcquisitionRuntime runtime; auto c = configuration();
        QVERIFY(runtime.startWithResults(c, std::make_unique<ResultTestSource>(), [](const SampleBlock& b) { return std::vector<AcquisitionFrame>{testResult(b), testResult(b)}; }));
        QTRY_VERIFY_WITH_TIMEOUT(!runtime.active(), 3000);
        const auto results = runtime.results().snapshot(); QCOMPARE(results.entries.size(), std::size_t(100));
        QCOMPARE(runtime.snapshot().stats.processedBlocks, std::uint64_t(100)); QCOMPARE(runtime.state(), RunState::Stopped);
        QVERIFY(runtime.startWithResults(c, std::make_unique<ResultTestSource>(1), [](const SampleBlock& b) { return std::vector<AcquisitionFrame>{testResult(b)}; }));
        QTRY_VERIFY_WITH_TIMEOUT(!runtime.active(), 3000);
        QCOMPARE(runtime.results().snapshot().entries.size(), std::size_t(101)); QCOMPARE(peakSegments(runtime.results().snapshot()).size(), std::size_t(2));
        QVERIFY(runtime.reset()); QVERIFY(runtime.results().snapshot().entries.empty());
    }
    void archiveDrainsBeforeStoppedAndOverflowFails() {
        QTemporaryDir root; QVERIFY(root.isValid());
        AcquisitionRuntime runtime; auto c = configuration(); c.queueCapacitySamples = 3000;
        auto archive = std::make_shared<AsyncRawArchive>(root.path(), 3000);
        QVERIFY(runtime.start(c, std::make_unique<ResultTestSource>(2), [](const SampleBlock&) {}, archive));
        QTRY_VERIFY_WITH_TIMEOUT(!runtime.active(), 3000); QCOMPARE(runtime.state(), RunState::Stopped);
        QFile manifest(archive->runDirectory() + "/manifest.json"); QVERIFY(manifest.open(QIODevice::ReadOnly));
        const auto object = QJsonDocument::fromJson(manifest.readAll()).object(); QCOMPARE(object["state"].toString(), QString("complete"));
        QCOMPARE(object["blocks"].toInteger(), qint64(2)); QCOMPARE(object["samples"].toInteger(), qint64(6));
        QVERIFY(QFile::exists(archive->runDirectory() + "/raw.csv"));

        QVERIFY(runtime.reset());
        auto tooSmall = std::make_shared<AsyncRawArchive>(root.path(), 2);
        QVERIFY(runtime.start(c, std::make_unique<ResultTestSource>(1), [](const SampleBlock&) {}, tooSmall));
        QTRY_VERIFY_WITH_TIMEOUT(!runtime.active(), 3000); QCOMPARE(runtime.state(), RunState::Error);
        QVERIFY(runtime.error().contains("写入队列已满"));
        QFile failedManifest(tooSmall->runDirectory() + "/manifest.json"); QVERIFY(failedManifest.open(QIODevice::ReadOnly));
        QCOMPARE(QJsonDocument::fromJson(failedManifest.readAll()).object()["state"].toString(), QString("incomplete"));
    }
    void resultFailureStopsAndPreservesAcceptedResults() {
        AcquisitionRuntime runtime;
        QVERIFY(runtime.startWithResults(configuration(), std::make_unique<ResultTestSource>(2), [](const SampleBlock& b) {
            auto f = testResult(b); if (b.sequence) f.runId = "foreign"; return std::vector<AcquisitionFrame>{f};
        }));
        QTRY_VERIFY_WITH_TIMEOUT(!runtime.active(), 3000); QCOMPARE(runtime.state(), RunState::Error);
        QCOMPARE(runtime.results().snapshot().entries.size(), std::size_t(1));
        auto stats = runtime.snapshot().stats; QCOMPARE(stats.acceptedBlocks, stats.processedBlocks + stats.failedBlocks);
    }
    void resultCapacityFailsExplicitly() {
        AcquisitionRuntime runtime;
        QVERIFY(runtime.startWithResults(configuration(), std::make_unique<ResultTestSource>(1), [](const SampleBlock& b) {
            std::vector<AcquisitionFrame> frames;
            for (int i = 0; i < 2001; ++i) { auto f = testResult(b); f.sequence = i; f.acquisitionId = std::to_string(i); frames.push_back(std::move(f)); }
            return frames;
        }));
        QTRY_VERIFY_WITH_TIMEOUT(!runtime.active(), 3000); QCOMPARE(runtime.state(), RunState::Error);
        QVERIFY(runtime.error().contains("缓存已满")); QCOMPARE(runtime.results().snapshot().entries.size(), std::size_t(2000));
        QCOMPARE(runtime.snapshot().stats.failedBlocks, std::uint64_t(1));
    }
    void stopIsAsyncAndDrains() {
        AcquisitionRuntime runtime;
        std::atomic_int processed{0};
        QVERIFY(runtime.start(configuration(), makeSimulationSource(), [&](const SampleBlock&) { ++processed; std::this_thread::sleep_for(std::chrono::milliseconds(20)); }));
        QVERIFY(!runtime.start(configuration(), makeSimulationSource(), [](const SampleBlock&) {}));
        QTRY_VERIFY_WITH_TIMEOUT(processed.load() >= 3, 2000);
        QElapsedTimer elapsed; elapsed.start(); runtime.requestStop(); QVERIFY(elapsed.elapsed() < 100);
        QVERIFY(!runtime.reset()); QTRY_VERIFY_WITH_TIMEOUT(!runtime.active(), 3000);
        QCOMPARE(runtime.state(), RunState::Stopped);
        auto stats = runtime.snapshot().stats; QCOMPARE(stats.acceptedBlocks, stats.processedBlocks); QCOMPARE(stats.failedBlocks, std::uint64_t(0));
        QVERIFY(!runtime.snapshot().samples.empty()); QVERIFY(runtime.reset()); QVERIFY(runtime.snapshot().samples.empty());
        QVERIFY(runtime.start(configuration(), makeSimulationSource(), [](const SampleBlock&) {})); runtime.requestStop(); QTRY_VERIFY_WITH_TIMEOUT(!runtime.active(), 3000);
    }
    void slowProcessorPressureAndOrderedShutdown() {
        AcquisitionRuntime runtime; auto c = configuration(); c.queueCapacitySamples = 8000;
        std::atomic_bool closed{false}, entered{false}, release{false};
        AtomicReleaseGuard releaseOnExit{release};
        int ticks = 0; QTimer timer; timer.setInterval(1); connect(&timer, &QTimer::timeout, this, [&] { ++ticks; }); timer.start();
        QVERIFY(runtime.start(c, std::make_unique<BurstSource>(closed, false, 8), [&](const SampleBlock&) {
            entered = true;
            while (!release.load()) std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }));
        QTRY_VERIFY_WITH_TIMEOUT(entered.load(), 1000);
        QTRY_VERIFY_WITH_TIMEOUT(runtime.shutdownSnapshot().inputClosed, 1000);
        const auto during = runtime.snapshot().stats;
        QCOMPARE(during.acceptedBlocks, std::uint64_t(8)); QCOMPARE(during.acceptedSamples, std::uint64_t(8000));
        QCOMPARE(during.processingBlocks, std::size_t(1)); QCOMPARE(during.processingSamples, std::size_t(1000));
        QCOMPARE(during.queuedBlocks, std::size_t(7)); QCOMPARE(during.queuedSamples, std::size_t(7000));
        QVERIFY(during.accountingConsistent()); QVERIFY(!during.terminalComplete());
        const auto draining = runtime.shutdownSnapshot();
        QVERIFY(closed); QVERIFY(draining.sourceClosed); QVERIFY(draining.inputClosed); QVERIFY(!draining.processingDrained);
        QVERIFY(draining.sourceClosedOrder > 0); QVERIFY(draining.sourceClosedOrder < draining.inputCloseOrder);
        QTest::qWait(30); const bool responsive = ticks > 0; release = true;
        QTRY_VERIFY_WITH_TIMEOUT(!runtime.active(), 2000);
        const auto done = runtime.snapshot().stats; QVERIFY(done.terminalComplete());
        QCOMPARE(done.terminalBlocks(), done.acceptedBlocks); QCOMPARE(done.terminalSamples(), done.acceptedSamples);
        QVERIFY(done.highWaterBlocks >= 7 && done.highWaterBlocks <= 8);
        QCOMPARE(done.highWaterSamples, done.highWaterBlocks * std::size_t(1000));
        const auto stopped = runtime.shutdownSnapshot();
        QVERIFY(stopped.processingDrained); QVERIFY(stopped.inputCloseOrder < stopped.processingDrainedOrder);
        QVERIFY(responsive); QCOMPARE(runtime.state(), RunState::Stopped);
    }
    void overflowStopsAndKeepsUiResponsive() {
        AcquisitionRuntime runtime; auto c = configuration(); c.queueCapacitySamples = 1000;
        std::atomic_bool closed{false}; int ticks = 0; QTimer timer; timer.setInterval(1); connect(&timer, &QTimer::timeout, this, [&] { ++ticks; }); timer.start();
        QVERIFY(runtime.start(c, std::make_unique<BurstSource>(closed), [](const SampleBlock&) { std::this_thread::sleep_for(std::chrono::milliseconds(80)); }));
        QTRY_VERIFY_WITH_TIMEOUT(!runtime.active(), 3000);
        QVERIFY(closed); QVERIFY(ticks > 0); QCOMPARE(runtime.state(), RunState::Error);
        const auto stats = runtime.snapshot().stats; QCOMPARE(stats.rejectedBlocks, std::uint64_t(1)); QCOMPARE(stats.acceptedBlocks, stats.processedBlocks);
        QCOMPARE(stats.acceptedSamples, stats.processedSamples); QVERIFY(stats.terminalComplete());
        const auto shutdown = runtime.shutdownSnapshot(); QVERIFY(shutdown.sourceClosed); QVERIFY(shutdown.inputClosed); QVERIFY(shutdown.processingDrained);
        QVERIFY(shutdown.sourceClosedOrder < shutdown.inputCloseOrder); QVERIFY(shutdown.inputCloseOrder < shutdown.processingDrainedOrder);
        QVERIFY(!runtime.error().isEmpty());
    }
    void openFailureCleansUp() {
        AcquisitionRuntime runtime; std::atomic_bool closed{false};
        QVERIFY(runtime.start(configuration(), std::make_unique<BurstSource>(closed, true), [](const SampleBlock&) {}));
        QTRY_VERIFY_WITH_TIMEOUT(!runtime.active(), 2000); QVERIFY(closed); QCOMPARE(runtime.state(), RunState::Error);
    }
    void processorFailureAccountsForAcceptedBlocks() {
        AcquisitionRuntime runtime; std::atomic_bool closed{false};
        QVERIFY(runtime.start(configuration(), std::make_unique<BurstSource>(closed), [](const SampleBlock&) { throw std::runtime_error("injected processing failure"); }));
        QTRY_VERIFY_WITH_TIMEOUT(!runtime.active(), 3000); QVERIFY(closed); QCOMPARE(runtime.state(), RunState::Error);
        const auto stats = runtime.snapshot().stats; QCOMPARE(stats.acceptedBlocks, stats.processedBlocks + stats.failedBlocks); QVERIFY(stats.failedBlocks > 0);
        QCOMPARE(stats.acceptedSamples, stats.processedSamples + stats.failedSamples); QVERIFY(stats.terminalComplete());
    }
    void stopTimeoutDoesNotPretendStopped() {
        AcquisitionRuntime runtime; std::atomic_bool entered{false}; QSignalSpy faults(&runtime, &AcquisitionRuntime::faultOccurred);
        QVERIFY(runtime.start(configuration(), makeSimulationSource(), [&](const SampleBlock&) {
            entered = true; std::this_thread::sleep_for(std::chrono::milliseconds(5700));
        }));
        QTRY_VERIFY_WITH_TIMEOUT(entered.load(), 1000); runtime.requestStop();
        QTRY_COMPARE_WITH_TIMEOUT(runtime.state(), RunState::Error, 5600);
        QVERIFY(runtime.active()); QVERIFY(!runtime.reset()); QVERIFY(runtime.shutdownSnapshot().stopTimedOut);
        QCOMPARE(faults.count(), 1); runtime.requestStop(); runtime.requestStop(); QTest::qWait(20); QCOMPARE(faults.count(), 1);
        QTRY_VERIFY_WITH_TIMEOUT(!runtime.active(), 3000); QVERIFY(!runtime.error().isEmpty());
        const auto shutdown = runtime.shutdownSnapshot(); QVERIFY(shutdown.processingDrained);
        QVERIFY(shutdown.sourceClosedOrder < shutdown.inputCloseOrder); QVERIFY(shutdown.inputCloseOrder < shutdown.processingDrainedOrder);
        QVERIFY(runtime.snapshot().stats.terminalComplete());
    }
};
QTEST_GUILESS_MAIN(RuntimeTest)
#include "runtime_test.moc"
