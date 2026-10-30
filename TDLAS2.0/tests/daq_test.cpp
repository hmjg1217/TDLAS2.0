#include "daq/daq_backend.h"
#include "runtime/acquisition_runtime.h"
#include <QtTest>
#include <QSignalSpy>

using namespace tdlas;

namespace {
AcquisitionConfig fakeConfig(bool ao = false) {
    AcquisitionConfig config;
    config.deviceId = "FakeDev1";
    config.inputChannel = "FakeDev1/ai0";
    config.outputChannel = ao ? "FakeDev1/ao0" : "";
    config.sampleRateHz = 10000;
    config.readBlockSamples = 100;
    config.queueCapacitySamples = 10000;
    return config;
}
}

class DaqTest final : public QObject {
    Q_OBJECT
private slots:
    void discoveryAndCompiledState() {
        auto observation = std::make_shared<FakeDaqObservation>();
        auto backend = makeFakeDaqBackend({}, observation);
        const auto result = backend->discover();
        QCOMPARE(observation->discoveryCalls.load(), std::uint64_t(1));
        QVERIFY(result.error.empty());
        QCOMPARE(result.devices.size(), std::size_t(1));
        QCOMPARE(result.devices.front().id, std::string("FakeDev1"));
        QVERIFY(validate(fakeConfig(), result.devices.front().capabilities).empty());
#if TDLAS_EXPECT_DAQMX
        QVERIFY(niDaqmxCompiled());
#else
        QVERIFY(!niDaqmxCompiled());
#endif
    }

    void aiOnlyDoesNotStartAoAndCleansAfterDisconnect() {
        FakeDaqOptions options;
        options.disconnectAfterReads = 2;
        options.readDelayMs = 2;
        auto observation = std::make_shared<FakeDaqObservation>();
        auto backend = makeFakeDaqBackend(options, observation);
        const auto device = backend->discover().devices.front();
        AcquisitionRuntime runtime;
        QSignalSpy stopped(&runtime, &AcquisitionRuntime::stopped);
        QVERIFY(runtime.start(fakeConfig(false), backend->makeSource(device), [](const SampleBlock&) {}));
        QTRY_COMPARE_WITH_TIMEOUT(stopped.size(), 1, 3000);
        QCOMPARE(runtime.state(), RunState::Error);
        QVERIFY(runtime.error().contains("设备断开"));
        QCOMPARE(observation->aiStarts.load(), std::uint64_t(1));
        QCOMPARE(observation->aoStarts.load(), std::uint64_t(0));
        QCOMPARE(observation->closeCalls.load(), std::uint64_t(1));
        QVERIFY(!observation->taskOpen.load());
        const auto shutdown = runtime.shutdownSnapshot();
        QVERIFY(shutdown.sourceClosed);
        QVERIFY(shutdown.inputClosed);
        QVERIFY(shutdown.processingDrained);
        QVERIFY(shutdown.sourceClosedOrder < shutdown.inputCloseOrder);
        QVERIFY(shutdown.inputCloseOrder < shutdown.processingDrainedOrder);
    }

    void aoSessionStopsAndCleans() {
        FakeDaqOptions options;
        options.readDelayMs = 5;
        auto observation = std::make_shared<FakeDaqObservation>();
        auto backend = makeFakeDaqBackend(options, observation);
        const auto device = backend->discover().devices.front();
        AcquisitionRuntime runtime;
        QSignalSpy stopped(&runtime, &AcquisitionRuntime::stopped);
        QVERIFY(runtime.start(fakeConfig(true), backend->makeSource(device), [](const SampleBlock&) {}));
        QTRY_VERIFY_WITH_TIMEOUT(observation->readCalls.load() >= 2, 2000);
        runtime.requestStop();
        QTRY_COMPARE_WITH_TIMEOUT(stopped.size(), 1, 3000);
        QCOMPARE(runtime.state(), RunState::Stopped);
        QCOMPARE(observation->aiStarts.load(), std::uint64_t(1));
        QCOMPARE(observation->aoStarts.load(), std::uint64_t(1));
        QCOMPARE(observation->closeCalls.load(), std::uint64_t(1));
        QVERIFY(!observation->taskOpen.load());
    }

    void startupFailureStillRunsCleanup() {
        FakeDaqOptions options;
        options.failOnOpen = true;
        auto observation = std::make_shared<FakeDaqObservation>();
        auto backend = makeFakeDaqBackend(options, observation);
        const auto device = backend->discover().devices.front();
        AcquisitionRuntime runtime;
        QSignalSpy stopped(&runtime, &AcquisitionRuntime::stopped);
        QVERIFY(runtime.start(fakeConfig(), backend->makeSource(device), [](const SampleBlock&) {}));
        QTRY_COMPARE_WITH_TIMEOUT(stopped.size(), 1, 3000);
        QCOMPARE(runtime.state(), RunState::Error);
        QCOMPARE(observation->openCalls.load(), std::uint64_t(1));
        QCOMPARE(observation->closeCalls.load(), std::uint64_t(1));
        QCOMPARE(observation->aiStarts.load(), std::uint64_t(0));
        QVERIFY(!observation->taskOpen.load());
    }

    void capabilityRejectsUnsafeAoBeforeTaskStart() {
        auto limited = defaultFakeDaqDevice();
        limited.capabilities.outputMinimumV = -2.0;
        limited.capabilities.outputMaximumV = 2.0;
        FakeDaqOptions options; options.devices = {limited};
        auto observation = std::make_shared<FakeDaqObservation>();
        auto backend = makeFakeDaqBackend(options, observation);
        const auto device = backend->discover().devices.front();
        auto config = fakeConfig(true);
        config.outputAmplitudeV = 3;
        AcquisitionRuntime runtime;
        QSignalSpy stopped(&runtime, &AcquisitionRuntime::stopped);
        QVERIFY(runtime.start(config, backend->makeSource(device), [](const SampleBlock&) {}));
        QTRY_COMPARE_WITH_TIMEOUT(stopped.size(), 1, 3000);
        QCOMPARE(runtime.state(), RunState::Error);
        QCOMPARE(observation->aiStarts.load(), std::uint64_t(0));
        QCOMPARE(observation->aoStarts.load(), std::uint64_t(0));
        QCOMPARE(observation->closeCalls.load(), std::uint64_t(1));
    }
};

QTEST_MAIN(DaqTest)
#include "daq_test.moc"
