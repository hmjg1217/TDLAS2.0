#include "daq/sample_source.h"
#include <QtTest>
#include <cmath>
#include <future>
#include <thread>

using namespace tdlas;
namespace {
AcquisitionConfig configuration() {
    AcquisitionConfig c; c.deviceId = "simulation"; c.inputChannel = "simulation/voltage";
    c.sampleRateHz = 10000; c.readBlockSamples = 25; return c;
}
}
class SimulationTest : public QObject {
    Q_OBJECT
private slots:
    void deterministicAcrossBlocksAndRuns() {
        auto c = configuration(); c.revision = 17;
        auto source = makeSimulationSource(); std::atomic_bool stop{false};
        source->open(c, "first");
        const auto a = source->read(stop), b = source->read(stop);
        QCOMPARE(a->runId, std::string("first")); QCOMPARE(a->sequence, std::uint64_t(0));
        QCOMPARE(b->sequence, std::uint64_t(1)); QCOMPARE(b->deviceId, c.deviceId);
        QCOMPARE(b->channelId, c.inputChannel); QCOMPARE(b->configRevision, c.revision);
        QCOMPARE(b->sampleRateHz, c.sampleRateHz); QCOMPARE(b->samples.size(), c.readBlockSamples);
        QVERIFY(a->acquiredAtUtcMs > 0); QVERIFY(a->elapsedNs >= 0); QVERIFY(b->elapsedNs >= a->elapsedNs);
        std::vector<double> combined = a->samples; combined.insert(combined.end(), b->samples.begin(), b->samples.end());
        for (std::size_t i = 0; i < combined.size(); ++i)
            QVERIFY(std::abs(combined[i] - 0.7 * std::sin(2 * 3.141592653589793 * 10 * i / c.sampleRateHz)) < 1e-12);
        source->close(); source->close();
        c.readBlockSamples = 50; c.displayWindowSamples = 10; c.processingWindowSamples = 123;
        source->open(c, "second"); const auto replay = source->read(stop);
        QCOMPARE(replay->samples, combined); QCOMPARE(replay->sequence, std::uint64_t(0));
        QCOMPARE(replay->runId, std::string("second")); source->close();
    }
    void capabilityAndLifecycleValidation() {
        auto source = makeSimulationSource(); auto c = configuration(); std::atomic_bool stop{false};
        QVERIFY_THROWS_EXCEPTION(std::logic_error, source->read(stop));
        QVERIFY(simulationCapabilities().outputChannels.empty());
        c.outputChannel = "simulation/ao";
        QVERIFY_THROWS_EXCEPTION(std::invalid_argument, source->open(c, "run"));
        c = configuration(); c.deviceId = "real-device";
        QVERIFY_THROWS_EXCEPTION(std::invalid_argument, source->open(c, "run"));
        c = configuration(); c.sampleRateHz = 1000001;
        QVERIFY_THROWS_EXCEPTION(std::invalid_argument, source->open(c, "run"));
        c = configuration(); QVERIFY_THROWS_EXCEPTION(std::invalid_argument, source->open(c, ""));
        source->open(c, "run"); QVERIFY_THROWS_EXCEPTION(std::logic_error, source->open(c, "other"));
        source->close(); QVERIFY_THROWS_EXCEPTION(std::logic_error, source->read(stop));
        SimulationOptions options; options.readDelayMs = -1;
        auto invalid = makeSimulationSource(options);
        QVERIFY_THROWS_EXCEPTION(std::invalid_argument, invalid->open(c, "run"));
    }
    void explicitFaultsAndReopen() {
        auto c = configuration(); std::atomic_bool stop{false}; SimulationOptions options; options.failOnOpen = true;
        auto source = makeSimulationSource(options);
        QVERIFY_THROWS_EXCEPTION(std::runtime_error, source->open(c, "run")); source->close();
        options.failOnOpen = false; options.failAfterBlocks = 3; source = makeSimulationSource(options);
        for (int run = 0; run < 2; ++run) {
            source->open(c, "run");
            for (std::uint64_t sequence = 0; sequence < 3; ++sequence) QCOMPARE(source->read(stop)->sequence, sequence);
            QVERIFY_THROWS_EXCEPTION(std::runtime_error, source->read(stop)); source->close();
        }
        options.failAfterBlocks = 0; source = makeSimulationSource(options); source->open(c, "run");
        QVERIFY_THROWS_EXCEPTION(std::runtime_error, source->read(stop)); source->close();
    }
    void pacingAndCancellation() {
        auto c = configuration(); c.sampleRateHz = 1000; c.readBlockSamples = 100;
        std::atomic_bool stop{false}; auto source = makeSimulationSource(); source->open(c, "run");
        QElapsedTimer elapsed; elapsed.start(); QVERIFY(source->read(stop)); QVERIFY(source->read(stop));
        QVERIFY(elapsed.elapsed() >= 90); source->close();
        // Both a low-rate scheduled read and the diagnostic delay must cancel.
        for (bool delayed : {false, true}) {
            SimulationOptions options; options.readDelayMs = delayed ? 10000 : 0;
            c.sampleRateHz = 1; source = makeSimulationSource(options); source->open(c, "run");
            stop = false; if (!delayed) QVERIFY(source->read(stop));
            std::atomic_bool entered{false};
            auto pending = std::async(std::launch::async, [&] { entered = true; return source->read(stop); });
            while (!entered) std::this_thread::yield();
            QTest::qWait(20); elapsed.restart(); stop = true;
            QVERIFY(!pending.get()); QVERIFY(elapsed.elapsed() < 500); source->close();
        }
    }
};
QTEST_GUILESS_MAIN(SimulationTest)
#include "simulation_test.moc"
